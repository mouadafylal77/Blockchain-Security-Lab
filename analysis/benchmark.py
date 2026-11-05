"""Sequential M5 runner. No engine imports/edits; one JSON in memory per run."""
from __future__ import annotations

import argparse
import copy
import ctypes
import hashlib
import json
import math
import os
from pathlib import Path
import platform
import shutil
import subprocess
import sys
import time
from contextlib import contextmanager
from datetime import datetime, timezone

import psutil

ROOT = Path(__file__).resolve().parents[1]
PROTOCOL = ROOT / "analysis/m5_protocol.json"
REFERENCE = ROOT / "analysis/m5_reference.json"
BINARY = ROOT / "build/ucrt64/blockchain_sim.exe"
DEFAULT_OUTPUT = ROOT / "results/raw/m5_30seeds"
FAMILIES = {"attack_51":"chain_instability", "selfish_mining":"publication_anomaly", "block_withholding":"pool_deficit", "sybil":"connectivity_anomaly"}

def now():
    return datetime.now(timezone.utc).isoformat()

def digest(path):
    h = hashlib.sha256()
    with Path(path).open("rb") as f:
        for chunk in iter(lambda:f.read(1024*1024), b""):
            h.update(chunk)
    return h.hexdigest()

def read(path):
    with Path(path).open(encoding="utf-8-sig") as f:
        return json.load(f)

def write(path, data):
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_suffix(path.suffix + ".tmp")
    with temporary.open("w",encoding="utf-8",newline="\n") as f:
        json.dump(data,f,indent=2,ensure_ascii=False,allow_nan=False)
        f.write("\n");f.flush();os.fsync(f.fileno())
    os.replace(temporary,path)

def relative(path):
    return Path(path).resolve().relative_to(ROOT).as_posix()

def protected_paths():
    paths=[]
    for directory in ("src","configs","tests","third_party"):
        paths.extend(p for p in (ROOT/directory).rglob("*") if p.is_file())
    paths.extend(ROOT/p for p in ("AGENTS.md","PLAN.md","projet.md","README.md","docs/SIMULATION_MODEL.md","exo4.cpp","CMakeLists.txt","CMakePresets.json",".gitignore","scripts/test-ucrt64.ps1","scripts/run-m4-examples.ps1"))
    return sorted(set(paths))

def freeze():
    if REFERENCE.exists():
        verify_reference();return
    files={relative(p):digest(p) for p in protected_paths()}
    binaries={relative(BINARY):digest(BINARY)}
    for name in ("libcrypto-3-x64.dll","libstdc++-6.dll","libgcc_s_seh-1.dll","libwinpthread-1.dll"):
        p=Path("C:/msys64/ucrt64/bin")/name
        if p.exists():binaries[str(p)]=digest(p)
    log=(ROOT/"docs/DEVELOPMENT_LOG.md").read_bytes()
    commit=subprocess.check_output(["git","rev-parse","HEAD"],cwd=ROOT,text=True).strip()
    status=subprocess.check_output(["git","status","--porcelain=v1","--untracked-files=all"],cwd=ROOT,text=True)
    write(REFERENCE,{"created_utc":now(),"git_commit":commit,"git_status_at_freeze":status,
        "files":files,"binaries":binaries,"protocol_sha256":digest(PROTOCOL),
        "development_log_prefix":{"bytes":len(log),"sha256":hashlib.sha256(log).hexdigest()},
        "environment":{"python":sys.version,"executable":sys.executable,"platform":platform.platform(),"psutil":psutil.__version__},
        "initial_disk_free_bytes":shutil.disk_usage(ROOT).free})
    print(f"Frozen {len(files)} input files and {len(binaries)} binaries",flush=True)

def verify_reference():
    ref=read(REFERENCE)
    if digest(PROTOCOL)!=ref["protocol_sha256"]:raise RuntimeError("Preregistered protocol changed")
    if set(ref["files"])!={relative(p) for p in protected_paths()}:raise RuntimeError("M4 input file inventory changed")
    for section in ("files","binaries"):
        for name,sha in ref[section].items():
            p=Path(name) if Path(name).is_absolute() else ROOT/name
            if not p.is_file() or digest(p)!=sha:raise RuntimeError(f"Frozen M4 file changed: {name}")
    prefix=ref["development_log_prefix"]
    data=(ROOT/"docs/DEVELOPMENT_LOG.md").read_bytes()
    if hashlib.sha256(data[:prefix["bytes"]]).hexdigest()!=prefix["sha256"]:raise RuntimeError("M1-M4 development history changed")
    return ref

@contextmanager
def campaign_lock(output):
    import msvcrt
    output.mkdir(parents=True,exist_ok=True)
    with (output/"campaign.lock").open("a+b") as f:
        if f.tell()==0:f.write(b"0");f.flush()
        f.seek(0)
        try:msvcrt.locking(f.fileno(),msvcrt.LK_NBLCK,1)
        except OSError as exc:raise RuntimeError("Another runner owns this campaign") from exc
        try:yield
        finally:f.seek(0);msvcrt.locking(f.fileno(),msvcrt.LK_UNLCK,1)

def append_manifest(output,record):
    with (output/"manifest.jsonl").open("a",encoding="utf-8") as f:
        f.write(json.dumps(record,ensure_ascii=False,allow_nan=False)+"\n");f.flush();os.fsync(f.fileno())

def latest_records(output):
    result={}
    p=output/"manifest.jsonl"
    if p.exists():
        lines=p.read_text(encoding="utf-8").splitlines()
        for i,line in enumerate(lines):
            try:r=json.loads(line)
            except json.JSONDecodeError:
                if i==len(lines)-1:continue
                raise
            result[r["job"]]=r
    return result

def expected_config(output,attack,detector,mode,seed,phase,calibration_path=None):
    source=ROOT/"configs/m4"/(f"{attack}_calibration.json" if phase=="calibration" else f"{attack}_{detector}_{mode}.json")
    c=read(source);c["seed"]=seed
    if detector=="adaptive" and phase!="calibration":
        c["security"]["reference_file"]=calibration_path or relative(output/"calibration"/attack/"attempt_1/calibration.json")
    # Only authorized fields differ from M4 templates.
    normalized=copy.deepcopy(c);original=read(source)
    normalized["seed"]=original["seed"]
    if "reference_file" in original["security"]:normalized["security"]["reference_file"]=original["security"]["reference_file"]
    else:normalized["security"].pop("reference_file",None)
    if normalized!=original:raise RuntimeError("Unauthorized configuration mutation")
    return c,source

def numeric(x,name,minimum=0):
    if isinstance(x,bool) or not isinstance(x,(int,float)) or not math.isfinite(x) or x<minimum:raise ValueError(f"Invalid numeric field: {name}")
    return x

def subset(expected,actual,path="config"):
    if isinstance(expected,dict):
        if not isinstance(actual,dict):raise ValueError(f"Wrong object at {path}")
        for k,v in expected.items():
            if k not in actual:raise ValueError(f"Missing {path}.{k}")
            subset(v,actual[k],path+"."+k)
    elif isinstance(expected,list):
        if not isinstance(actual,list) or len(expected)!=len(actual):raise ValueError(f"Wrong array {path}")
        for i,(a,b) in enumerate(zip(expected,actual)):subset(a,b,f"{path}[{i}]")
    elif type(expected) is bool:
        if type(actual) is not bool or actual!=expected:raise ValueError(f"Boolean mismatch {path}")
    elif expected!=actual:raise ValueError(f"Mismatch {path}: {expected!r} != {actual!r}")

def validate_result(result,config,phase):
    if result.get("schema_version")!=1 or result.get("software_version")!="0.4.0":raise ValueError("Unexpected engine schema/version")
    subset(config,result["config"])
    metrics=result["metrics"];execution=result["execution"]
    for k in ("blocks_produced","events_processed","messages_sent","messages_delivered","reorganizations","fork_observations"):
        numeric(metrics[k],k)
    if metrics["blocks_produced"]>config["max_blocks"] or metrics["events_processed"]>config["max_events"]:raise ValueError("Budget exceeded")
    if execution["stop_reason"] not in ("max_blocks_drained","max_events","duration_ticks","queue_empty"):raise ValueError("Unknown termination")
    if len(result["nodes"])!=len(config["nodes"]) or len(result["blocks"])!=metrics["blocks_produced"]:raise ValueError("Inconsistent block/node count")
    for node in result["nodes"]:
        for k in ("height","known_blocks_excluding_genesis","stale_blocks","unknown_parent_blocks"):numeric(node[k],k)
        if node["stale_blocks"]>node["known_blocks_excluding_genesis"]:raise ValueError("Invalid stale count")
    if phase=="calibration":
        if config["attack"]["enabled"] or result["security"]["mode"]!="calibration":raise ValueError("Calibration must be honest")
        return
    sec=result["security"];e=sec["evaluation"];alerts=sec["alerts"]
    if type(result["attack"]["success"]) is not bool:raise ValueError("Missing binary attack outcome")
    truth=FAMILIES[config["scenario"].split("_fixed_")[0].split("_adaptive_")[0]] if config["attack"]["enabled"] else ""
    predicted={a["behavior"] for a in alerts}
    known=set(FAMILIES.values())
    if not predicted<=known:raise ValueError("Unknown alert family")
    actual={truth} if truth else set()
    counts={"tp":len(predicted&actual),"fp":len(predicted-actual),"fn":len(actual-predicted),"tn":len(known-predicted-actual)}
    if e["ground_truth_family"]!=truth:raise ValueError("Ground truth mismatch")
    for k,v in counts.items():
        if e[k]!=v:raise ValueError(f"Confusion count mismatch {k}")
    p=counts["tp"]/(counts["tp"]+counts["fp"]) if counts["tp"]+counts["fp"] else 0
    r=counts["tp"]/(counts["tp"]+counts["fn"]) if counts["tp"]+counts["fn"] else 0
    for k,v in (("precision",p),("recall",r),("f1",2*p*r/(p+r) if p+r else 0)):
        if not math.isclose(e[k],v,abs_tol=1e-12):raise ValueError(f"Detection score mismatch {k}")
    matched=[a["time"] for a in alerts if a["behavior"]==truth]
    if e["latency_ticks"]!=(min(matched) if matched else None):raise ValueError("Latency mismatch")
    for a in alerts:
        if a["time"]>execution["last_event_tick"] or not a["value"]>a["threshold"]:raise ValueError("Invalid alert evidence")
    if config["security"]["detector"]=="adaptive":
        ref=read(ROOT/config["security"]["reference_file"])
        if sec["initial_reference"]!=ref["metrics"]:raise ValueError("Run did not use the frozen honest reference")
    if "pool" in result["attack"]:
        pool=result["attack"]["pool"]
        if not math.isclose(sum(x["payout_units"] for x in pool["payouts"]),pool["canonical_reward_units_at_target"],abs_tol=1e-8):raise ValueError("Pool payout conservation failed")
        if sum(pool["observed_shares_by_member"])-pool["shares_total"]!=pool["shares_uncredited"]:raise ValueError("Pool credit conservation failed")

def summarize(result,config):
    a=result["attack"];m=result["metrics"];sec=result["security"];e=sec["evaluation"];o=sec["overhead"]
    ac=config["attack"];adversary=ac["adversary"];target=ac["target"]
    target_node=result["nodes"][target];trace=result["trace"];blocks={b["hash"]:b for b in result["blocks"]}
    share=a["active_block_share_at_target"];hp=config["nodes"][adversary]["hash_power"]/sum(n["hash_power"] for n in config["nodes"])
    reorg=[x["detached_depth"] for x in trace if x["type"]=="reorganization" and x["node"]==config["nodes"][target]["id"]]
    row={"attack_success":int(a["success"]),**{k:e[k] for k in ("tp","fp","tn","fn","precision","recall","f1","latency_ticks")},
        "payment_loss":sec["economic"]["payment_loss_coin_units"],"payments_reversed":int(config["attack"]["type"]=="51" and a["success"]),
        "merchant_accepted":int(a["merchant_accepted"]),"accepted_at":a["merchant_accepted_at"] if a["merchant_accepted"] else None,
        "configured_confirmations":ac["confirmations"],"confirmations_at_acceptance":None,
        "max_reorg_depth":max(reorg,default=0),"target_reorganizations":len(reorg),
        "adversary_active_share":share,"hash_share":hp,"active_share_minus_hash_share":share-hp,
        "target_stale_ratio":target_node["stale_blocks"]/target_node["known_blocks_excluding_genesis"] if target_node["known_blocks_excluding_genesis"] else 0,
        "adversary_active_blocks":a["adversary_active_blocks_at_target"],"filtered_messages":a["filtered_messages"],
        "victim_height":target_node["height"],"victim_known_blocks":target_node["known_blocks_excluding_genesis"],
        "pool_reward":sec["economic"]["pool_reward_units"],"pool_full_withheld":None,"shares_observed":None,
        "adversary_payout":None,"honest_payout":None,"honest_shares_uncredited":None,"recovery_after_reconnect_ticks":None,
        "messages":m["messages_sent"],"messages_delivered":m["messages_delivered"],"events":m["events_processed"],
        "blocks_produced":m["blocks_produced"],"stop_reason":result["execution"]["stop_reason"],
        "last_event_tick":result["execution"]["last_event_tick"],"pending_events":result["execution"]["pending_events"],
        "feature_evaluations":o["feature_evaluations"],"deferred_messages":o["deferred_messages"],"deferred_ticks":o["deferred_ticks"],
        "reconnections":o["reconnections"],"sync_messages":o["sync_messages"],"shares_uncredited":o["shares_uncredited"],
        "peak_observation_samples":o["peak_observation_samples"],"uncalibrated_features_skipped":o["uncalibrated_features_skipped"],
        "payment_deferrals":o["payment_deferrals"]}
    if "pool" in a:
        pool=a["pool"];payouts=pool["payouts"]
        row.update(pool_full_withheld=pool["full_withheld"],shares_observed=sum(pool["observed_shares_by_member"]),
            adversary_payout=payouts[adversary]["payout_units"],honest_payout=sum(p["payout_units"] for i,p in enumerate(payouts) if i!=adversary),
            honest_shares_uncredited=sum(pool["observed_shares_by_member"][i]-p["shares"] for i,p in enumerate(payouts) if i!=adversary))
    target_id=config["nodes"][target]["id"];adversary_id=config["nodes"][adversary]["id"]
    reconnect=[x["time"] for x in trace if x["type"]=="peer_reconnection" and x["node"]==target_id]
    recovery=[x["time"] for x in trace if x["type"] in ("tip_extension","reorganization") and x["node"]==target_id and reconnect and x["time"]>=min(reconnect)]
    if recovery:row["recovery_after_reconnect_ticks"]=min(recovery)-min(reconnect)
    delayed=[x for x in trace if x["type"]=="probation_delay"]
    row["honest_recipient_delays"]=sum(x["node"]!=adversary_id for x in delayed)
    row["honest_producer_payload_delays"]=honest_payload_delays(delayed,blocks,adversary_id)
    row["process_signature"]=hashlib.sha256(json.dumps([(b["hash"],b["miner"],b["discovered_at"]) for b in result["blocks"]],sort_keys=True).encode()).hexdigest()
    return row

def honest_payload_delays(delayed,blocks,adversary_id):
    # M4 trace schema names the payload "block"; block inventory names it "hash".
    return sum(blocks[x["block"]]["miner"]!=adversary_id for x in delayed)

def child_cpu(process):
    if os.name!="nt":return None
    from ctypes import wintypes
    creation,exit_time,kernel,user=(wintypes.FILETIME() for _ in range(4))
    function=ctypes.windll.kernel32.GetProcessTimes
    function.argtypes=[wintypes.HANDLE]+[ctypes.POINTER(wintypes.FILETIME)]*4
    if not function(int(process._handle),ctypes.byref(creation),ctypes.byref(exit_time),ctypes.byref(kernel),ctypes.byref(user)):return None
    value=lambda f:(f.dwHighDateTime<<32)+f.dwLowDateTime
    return (value(kernel)+value(user))/1e7

def execute(config_path,directory,timeout):
    env=os.environ.copy();env["PATH"]="C:/msys64/ucrt64/bin;"+env.get("PATH","")
    begin=time.perf_counter();peak=0;samples=0
    with (directory/"process.log").open("wb") as log:
        process=subprocess.Popen([str(BINARY),"--config",str(config_path),"--output",str(directory)],cwd=ROOT,env=env,stdout=log,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW if os.name=="nt" else 0)
        observed=psutil.Process(process.pid)
        try:
            while process.poll() is None:
                try:peak=max(peak,observed.memory_info().rss);samples+=1
                except (psutil.NoSuchProcess,psutil.AccessDenied):pass
                if time.perf_counter()-begin>timeout:
                    process.kill();process.wait();raise TimeoutError("External wall timeout; no model budget changed")
                time.sleep(0.01)
            cpu=child_cpu(process)
        except BaseException:
            if process.poll() is None:process.terminate();process.wait()
            raise
    return {"exit_code":process.returncode,"wall_seconds":time.perf_counter()-begin,"child_cpu_seconds":cpu,
        "child_sampled_peak_rss_bytes":peak if samples else None,"rss_samples":samples}

def jobs(protocol,phase):
    for attack in protocol["attacks"]:yield ("calibration",attack,"adaptive","calibration",protocol["calibration_seed"],"")
    if phase=="pilot":
        yield ("pilot","sybil","fixed","baseline",9001,"")
        yield ("pilot","block_withholding","adaptive","protected",9002,"")
        yield ("pilot","sybil","fixed","baseline",9001,"repeat")
    elif phase=="evaluation":
        for attack in protocol["attacks"]:
            for detector in protocol["detectors"]:
                for mode in protocol["modes"]:
                    for seed in protocol["evaluation_seeds"]:yield ("evaluation",attack,detector,mode,seed,"")

def run(output,phase,retry_errors=False):
    protocol=read(PROTOCOL);reference=verify_reference();output=output.resolve()
    records=latest_records(output);count=0;skipped=0;errors=0
    for job_phase,attack,detector,mode,seed,suffix in jobs(protocol,phase):
        job="/".join(map(str,(job_phase,attack,detector,mode,seed)))+("/"+suffix if suffix else "")
        calibration_job="/".join(map(str,("calibration",attack,"adaptive","calibration",protocol["calibration_seed"])))
        config,source=expected_config(output,attack,detector,mode,seed,job_phase,records.get(calibration_job,{}).get("calibration_path"))
        prior=records.get(job)
        if prior and prior["status"]=="valid":
            result_path=ROOT/prior["result_path"]
            if digest(result_path)!=prior["result_sha256"] or digest(ROOT/prior["config_path"])!=prior["config_sha256"]:raise RuntimeError("Validated artifact modified; campaign stopped")
            if digest(ROOT/prior["summary_path"])!=prior["summary_sha256"]:raise RuntimeError("Validated summary modified")
            subset(config,read(ROOT/prior["config_path"]))
            data=read(result_path);validate_result(data,config,job_phase);del data
            if job_phase=="calibration" and digest(ROOT/prior["calibration_path"])!=prior["calibration_sha256"]:raise RuntimeError("Calibration artifact changed")
            skipped+=1;continue
        if prior and prior["status"]=="error" and not retry_errors:errors+=1;continue
        if job_phase!="calibration":
            if records[calibration_job]["status"]!="valid":raise RuntimeError("Honest calibration unavailable; campaign family stopped")
        if shutil.disk_usage(output).free<protocol["execution"]["minimum_free_disk_bytes"]:raise RuntimeError("Disk below safety reserve; no data removed")
        attempt=(prior.get("attempt",0)+1) if prior else 1
        directory=output/"calibration"/attack/f"attempt_{attempt}" if job_phase=="calibration" else output/job/f"attempt_{attempt}"
        directory.mkdir(parents=True,exist_ok=True);config_path=directory/"config.json";write(config_path,config)
        record={"job":job,"phase":job_phase,"attack":attack,"detector":detector,"mode":mode,"seed":seed,"attempt":attempt,
            "started_utc":now(),"status":"running","source_config":relative(source),"source_sha256":digest(source),
            "config_path":relative(config_path),"config_sha256":digest(config_path),"binary_sha256":reference["binaries"][relative(BINARY)],
            "runner_sha256":digest(Path(__file__)),"protocol_sha256":digest(PROTOCOL),"reference_manifest_sha256":digest(REFERENCE)}
        append_manifest(output,record)
        try:
            resources=execute(config_path,directory,protocol["execution"]["wall_timeout_seconds"])
            record.update(resources)
            if resources["exit_code"]!=0:raise RuntimeError(f"Simulator exit code {resources['exit_code']}")
            result_path=directory/"results.json";data=read(result_path);validate_result(data,config,job_phase)
            if job_phase=="calibration":
                path=directory/"calibration.json";calibration=read(path)
                if calibration["window_ticks"]!=config["security"]["window_ticks"] or not calibration["metrics"]:raise ValueError("Invalid honest reference")
                subset(config,calibration["source_config"])
                record.update(calibration_path=relative(path),calibration_sha256=digest(path))
                row={}
            else:
                row=summarize(data,config)
                for key,value in row.items():
                    if isinstance(value,(int,float)) and not math.isfinite(value):raise ValueError(f"Nonfinite summary {key}")
                if detector=="adaptive":record["calibration_sha256"]=digest(ROOT/config["security"]["reference_file"])
            row.update({k:record[k] for k in ("job","phase","attack","detector","mode","seed","wall_seconds","child_cpu_seconds","child_sampled_peak_rss_bytes")})
            row["orchestrator_observed_rss_bytes"]=psutil.Process().memory_info().rss
            summary_path=directory/"summary.json";write(summary_path,row)
            record.update(status="valid",result_path=relative(result_path),result_sha256=digest(result_path),summary_path=relative(summary_path),summary_sha256=digest(summary_path))
            del data
        except KeyboardInterrupt:
            record.update(status="interrupted",error="KeyboardInterrupt",finished_utc=now());append_manifest(output,record);raise
        except Exception as exc:
            record.update(status="error",error=f"{type(exc).__name__}: {exc}");errors+=1
        record["finished_utc"]=now();append_manifest(output,record);records[job]=record;count+=1
        if count%20==0 or job_phase!="evaluation" or record["status"]!="valid":print(f"{count} launched; {job}: {record['status']} {record.get('error','')}",flush=True)
        # Detect accidental changes between every sequential run.
        if digest(BINARY)!=reference["binaries"][relative(BINARY)]:raise RuntimeError("Engine executable changed")
    verify_reference()
    if phase=="pilot":
        a=records["pilot/sybil/fixed/baseline/9001"];b=records["pilot/sybil/fixed/baseline/9001/repeat"]
        if a["status"]!="valid" or b["status"]!="valid" or a["result_sha256"]!=b["result_sha256"]:raise RuntimeError("Pilot reproduction failed")
    print(json.dumps({"launched_this_call":count,"validated_skipped":skipped,"technical_errors":errors}),flush=True)
    return 1 if errors else 0

def main():
    parser=argparse.ArgumentParser();parser.add_argument("action",choices=("freeze","verify","pilot","run","repair"));parser.add_argument("--output",type=Path,default=DEFAULT_OUTPUT);parser.add_argument("--retry-errors",action="store_true")
    args=parser.parse_args()
    if args.action=="freeze":freeze();return 0
    if args.action=="verify":verify_reference();print("Frozen M4 unchanged");return 0
    with campaign_lock(args.output):
        repair_manifest(args.output)
        if args.action=="repair":return repair_summaries(args.output)
        return run(args.output,"pilot" if args.action=="pilot" else "evaluation",args.retry_errors)

def repair_manifest(output):
    """Preserve a torn final journal fragment before any resumed append."""
    path=output/"manifest.jsonl"
    if not path.exists():return
    data=path.read_bytes()
    if not data or data.endswith(b"\n"):return
    boundary=data.rfind(b"\n")+1;tail=data[boundary:]
    try:json.loads(tail)
    except (json.JSONDecodeError,UnicodeDecodeError):
        archive=output/("torn_manifest_"+hashlib.sha256(tail).hexdigest()+".bin")
        archive.write_bytes(tail)
        with path.open("r+b") as f:f.truncate(boundary);f.flush();os.fsync(f.fileno())
        print(f"Archived incomplete journal fragment: {archive}",flush=True)
    else:
        with path.open("ab") as f:f.write(b"\n");f.flush();os.fsync(f.fileno())

def repair_summaries(output):
    """Revalidate successful C++ outputs after a Python extraction error; no subprocess."""
    verify_reference();records=latest_records(output);repaired=0
    for job,r in records.items():
        if r["status"]!="error" or r.get("exit_code")!=0 or r["phase"]=="calibration":continue
        config=read(ROOT/r["config_path"])
        if digest(ROOT/r["config_path"])!=r["config_sha256"]:raise RuntimeError("Input changed before repair")
        result_path=(ROOT/r["config_path"]).parent/"results.json"
        before=digest(result_path);data=read(result_path);validate_result(data,config,r["phase"])
        row=summarize(data,config)
        row.update({k:r[k] for k in ("job","phase","attack","detector","mode","seed","wall_seconds","child_cpu_seconds","child_sampled_peak_rss_bytes")})
        row["orchestrator_observed_rss_bytes"]=psutil.Process().memory_info().rss
        path=result_path.parent/"summary.json";write(path,row)
        if digest(result_path)!=before:raise RuntimeError("Raw output changed during repair")
        record=dict(r);record.update(status="valid",repair_of_error=r["error"],reprocessed_without_simulation=True,
            summary_builder_sha256=digest(Path(__file__)),result_path=relative(result_path),result_sha256=before,
            summary_path=relative(path),summary_sha256=digest(path),repaired_utc=now())
        append_manifest(output,record);repaired+=1;del data
    verify_reference();print(f"Repaired {repaired} summaries without launching a simulation",flush=True)
    return 0

if __name__=="__main__":
    sys.dont_write_bytecode=True
    raise SystemExit(main())
