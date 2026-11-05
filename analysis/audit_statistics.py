"""Independent stdlib audit of cohort counts and descriptive/paired statistics."""
import csv
import math
import statistics

import benchmark as b

def close(actual,expected):
    if not math.isclose(float(actual),expected,rel_tol=1e-10,abs_tol=1e-10):raise AssertionError((actual,expected))

def main():
    reference=b.verify_reference()
    records=b.latest_records(b.DEFAULT_OUTPUT)
    verified_inputs=0
    for record in records.values():
        if record["status"]!="valid":raise AssertionError("An invalid execution remains")
        assert record["source_sha256"]==reference["files"][record["source_config"]]
        assert record["binary_sha256"]==reference["binaries"][b.relative(b.BINARY)]
        assert record["protocol_sha256"]==reference["protocol_sha256"]
        calibration_job=f"calibration/{record['attack']}/adaptive/calibration/7"
        expected,_=b.expected_config(b.DEFAULT_OUTPUT,record["attack"],record["detector"],record["mode"],record["seed"],record["phase"],records[calibration_job]["calibration_path"])
        assert b.read(b.ROOT/record["config_path"])==expected,"Unexpected M4 parameter change"
        assert b.digest(b.ROOT/record["config_path"])==record["config_sha256"]
        verified_inputs+=1
    tables=b.ROOT/"analysis/tables"
    def read_csv(name):
        with (tables/name).open(newline="",encoding="utf-8") as f:return list(csv.DictReader(f))
    runs=read_csv("m5_runs.csv");groups=read_csv("m5_groups.csv");stats=read_csv("m5_statistics.csv");pairs=read_csv("m5_paired.csv")
    assert len(runs)==720 and len(groups)==24
    grouped={}
    for row in runs:
        key=row["attack"],row["detector"],row["mode"]
        grouped.setdefault(key,{})[int(row["seed"])]=row
    seeds=b.read(b.PROTOCOL)["evaluation_seeds"]
    for g in groups:
        rows=grouped[g["attack"],g["detector"],g["mode"]]
        assert sorted(rows)==seeds and len(rows)==30
        for k in ("tp","fp","tn","fn"):close(g[k],sum(int(r[k]) for r in rows.values()))
        tp,fp,fn=(float(g[k]) for k in ("tp","fp","fn"))
        p=tp/(tp+fp) if tp+fp else 0;r=tp/(tp+fn) if tp+fn else 0
        close(g["micro_precision"],p);close(g["micro_recall"],r);close(g["micro_f1"],2*p*r/(p+r) if p+r else 0)
    def audit(row,values):
        assert int(row["n"])==len(values) and int(row["missing"])==30-len(values)
        if not values:
            assert row["mean"]=="" and row["mean_ci_low"]=="";return
        for name,value in (("mean",statistics.mean(values)),("median",statistics.median(values)),("min",min(values)),("max",max(values))):close(row[name],value)
        if len(values)>1:close(row["sd"],statistics.stdev(values))
        for prefix in ("mean","median"):
            if row[prefix+"_ci_low"]:
                low=float(row[prefix+"_ci_low"]);high=float(row[prefix+"_ci_high"])
                assert min(values)-1e-9<=low<=high<=max(values)+1e-9
    for row in stats:
        source=grouped[row["attack"],row["detector"],row["mode"]]
        values=[float(r[row["metric"]]) for r in source.values() if r[row["metric"]]!=""]
        audit(row,values)
    for row in pairs:
        comparison=row["comparison"]
        if row["detector"]=="adaptive_vs_fixed":
            mode=comparison.removeprefix("adaptive_minus_fixed_")
            left=grouped[row["attack"],"fixed",mode];right=grouped[row["attack"],"adaptive",mode]
        else:
            a,bb=("attacked","protected") if comparison=="protected_minus_attacked" else ("baseline","attacked")
            left=grouped[row["attack"],row["detector"],a];right=grouped[row["attack"],row["detector"],bb]
        metric=row["metric"]
        values=[float(right[s][metric])-float(left[s][metric]) for s in seeds if left[s][metric]!="" and right[s][metric]!=""]
        audit(row,values)
    result={"evaluations":720,"cohorts":24,"descriptive_rows_verified":len(stats),"paired_rows_verified":len(pairs),"original_inputs_verified":verified_inputs,"m4_unchanged":True}
    b.write(b.ROOT/"analysis/m5_statistics_audit.json",result)
    print(result)

if __name__=="__main__":main()
