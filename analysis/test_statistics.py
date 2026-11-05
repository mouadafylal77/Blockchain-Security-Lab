import math
import unittest

import aggregate as a

class StatisticsTests(unittest.TestCase):
    def test_known_distribution_and_missing(self):
        result=a.describe([1,2,3,None],"known_distribution")
        self.assertEqual((result["n"],result["missing"]),(3,1))
        self.assertEqual((result["mean"],result["median"],result["sd"]),(2,2,1))
        self.assertEqual((result["min"],result["max"]),(1,3))
        self.assertEqual((result["mean_ci_low"],result["mean_ci_high"]),(1,3))
        self.assertEqual(a.describe([None,None],"missing")["n"],0)
        self.assertIsNone(a.describe([5],"singleton")["mean_ci_low"])

    def test_paired_uncertainty(self):
        left=[10,20,30];right=[11,21,31]
        result=a.describe([y-x for x,y in zip(left,right)],"paired")
        self.assertEqual((result["mean"],result["sd"],result["mean_ci_low"],result["mean_ci_high"]),(1,0,1,1))

    def test_confusion_micro_not_macro(self):
        result=a.micro_detection([{"tp":1,"fp":1,"tn":2,"fn":0},{"tp":0,"fp":0,"tn":3,"fn":1}],"micro_test")
        self.assertEqual((result["tp"],result["fp"],result["tn"],result["fn"]),(1,1,5,1))
        self.assertEqual((result["micro_precision"],result["micro_recall"],result["micro_f1"]),(.5,.5,.5))

    def test_binary_boundary_intervals(self):
        result=a.describe([1]*30,"ones")
        self.assertEqual((result["mean_ci_low"],result["mean_ci_high"]),(1,1))
        low,high=a.wilson(30,30)
        self.assertAlmostEqual(low,.8864866068,places=8)
        self.assertAlmostEqual(high,1,places=10)
        low,high=a.wilson(0,30)
        self.assertAlmostEqual(low,0,places=10)
        self.assertGreater(high,.11)

    def test_reproducible_bootstrap_and_invalid_values(self):
        self.assertEqual(a.describe(list(range(30)),"repro"),a.describe(list(range(30)),"repro"))
        with self.assertRaises(ValueError):a.describe([float("nan")],"invalid")

if __name__=="__main__":unittest.main()
