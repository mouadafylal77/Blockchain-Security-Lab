import copy
import json
from pathlib import Path
import tempfile
import unittest

import benchmark as b

class BenchmarkTests(unittest.TestCase):
    def test_preregistered_design(self):
        p=b.read(b.PROTOCOL)
        self.assertEqual(len(set(p["evaluation_seeds"])),30)
        self.assertFalse({7,42}&set(p["evaluation_seeds"]))
        self.assertEqual(sum(j[0]=="evaluation" for j in b.jobs(p,"evaluation")),720)
        self.assertEqual(sum(j[0]=="pilot" for j in b.jobs(p,"pilot")),3)

    def test_only_authorized_configuration_changes(self):
        source=b.ROOT/"configs/m4/sybil_adaptive_protected.json"
        original=source.read_bytes()
        c,_=b.expected_config(b.DEFAULT_OUTPUT,"sybil","adaptive","protected",1001,"evaluation")
        self.assertEqual(c["seed"],1001)
        self.assertNotEqual(c["security"]["reference_file"],b.read(source)["security"]["reference_file"])
        self.assertEqual(source.read_bytes(),original)
        bad=copy.deepcopy(c);bad["security"]["window_ticks"]+=1
        with self.assertRaises(ValueError):b.subset(c,bad)

    def test_strict_field_checks(self):
        with self.assertRaises(ValueError):b.numeric(True,"count")
        with self.assertRaises(ValueError):b.numeric(float("nan"),"metric")
        with self.assertRaises(ValueError):b.subset({"enabled":True},{"enabled":1})
        with self.assertRaises(ValueError):b.subset({"seed":1001},{"seed":1002})

    def test_exported_trace_payload_key(self):
        delayed=[{"type":"probation_delay","block":"h1"},{"type":"probation_delay","block":"h2"}]
        blocks={"h1":{"miner":"alice"},"h2":{"miner":"bob"}}
        self.assertEqual(b.honest_payload_delays(delayed,blocks,"alice"),1)

    def test_checkpoint_and_torn_tail(self):
        with tempfile.TemporaryDirectory(dir=b.ROOT/"build") as directory:
            out=Path(directory)
            record={"job":"synthetic_checkpoint","status":"valid"}
            b.append_manifest(out,record)
            with (out/"manifest.jsonl").open("ab") as f:f.write(b'{"job":"unfinished')
            self.assertEqual(b.latest_records(out)["synthetic_checkpoint"],record)
            b.repair_manifest(out)
            self.assertEqual(len(list(out.glob("torn_manifest_*.bin"))),1)
            b.append_manifest(out,{"job":"next","status":"error"})
            self.assertEqual(len(b.latest_records(out)),2)

if __name__=="__main__":unittest.main()
