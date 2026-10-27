"""Analysis tests. Run: python3 -m unittest discover -s tests (from tools/balance_sim). Uses the fake engine."""
import json
import os
import random
import sys
import unittest

import tempfile
os.environ["MINASIM_FAKE"] = "1"
os.environ["MINASIM_RUNS"] = tempfile.mkdtemp(prefix="minasim-runs-")
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from minasim import stats, policies  # noqa: E402
from minasim.fights import Catalog  # noqa: E402
from minasim.stages import Stages  # noqa: E402
from minasim.teams import TeamBuilder  # noqa: E402


class StatsTest(unittest.TestCase):
    def test_wilson(self):
        lo, hi = stats.wilson(50, 100)
        self.assertAlmostEqual(lo, 0.404, 2)
        self.assertAlmostEqual(hi, 0.596, 2)
        self.assertEqual(stats.wilson(0, 0), (0.0, 1.0))

    def test_bh(self):
        self.assertEqual(stats.bh([0.001, 0.5, 0.9, 0.004]), [True, False, False, True])

    def test_robust_z(self):
        z = stats.robust_z([1, 2, 3, 2, 2, 50])
        self.assertGreater(z[-1], 5)

    def test_paired(self):
        d, ci, z = stats.paired_diff([1, 1, 1, 0] * 50, [0, 1, 0, 0] * 50)
        self.assertGreater(d, 0.4)
        self.assertGreater(ci[0], 0.3)


class StagesTest(unittest.TestCase):
    def test_stage_shape(self):
        s = Stages()
        self.assertEqual(len(s.stages), 18)
        self.assertEqual(s.of_map("MAP_PEWTER_CITY_GYM"), s.of_map("MAP_PEWTER_CITY"))   # interiors inherit
        self.assertLess(s.of_map("MAP_ROUTE1"), s.of_map("MAP_ROUTE24"))
        self.assertEqual(s.of_map("MAP_ROUTE101"), 0)


class CatalogTest(unittest.TestCase):
    def test_catalog(self):
        c = Catalog()
        gyms = {f["gym_no"] for f in c.fights if f["class"] in ("gym", "boss")}
        self.assertEqual(gyms, set(range(1, 9)))
        self.assertTrue(all(f["source"] in ("rom", "design", "placeholder") for f in c.fights))
        self.assertTrue(all(0 <= f["stage"] < 18 for f in c.fights))
        rival = [f for f in c.fights if f["class"] == "rival"][0]
        party = c.party_for(rival, "CHARMANDER")
        self.assertEqual(party[-1]["species"] if rival["id"] != "rival:route103" else party[0]["species"], "SQUIRTLE" if rival["id"] == "rival:route103" else party[-1]["species"])

    def test_team_levels(self):
        c = Catalog()
        t = TeamBuilder(c)
        f = [x for x in c.fights if x["id"] == "gym:2"][0]
        a = t.player_team(f, "average", "CHARMANDER", random.Random(3))
        s = t.player_team(f, "skilled", "CHARMANDER", random.Random(3))
        self.assertLessEqual(a["mons"][0]["level"], 14)
        self.assertGreaterEqual(s["mons"][0]["level"], 13)
        self.assertEqual(a["mons"][0]["species"], "CHARMANDER")


class PolicyTest(unittest.TestCase):
    def state(self, hp=30):
        return {"side": 0, "turn": 1, "seed": 1, "kind": "wild",
                "me": {"species": "CHARMANDER", "level": 14, "hp": hp, "max_hp": 40, "tp": 90, "tp_max": 100, "status": None,
                       "stages": {}, "moves": ["SCRATCH", "EMBER", "WATER_GUN"], "tp_costs": [5, 5, 5], "special": None},
                "foe": {"species": "BULBASAUR", "level": 14, "hp": 40, "max_hp": 40, "status": None, "stages": {}},
                "party": [], "items": {"POTION": 2},
                "legal": [{"type": "move", "slot": 0}, {"type": "move", "slot": 1}, {"type": "move", "slot": 2},
                          {"type": "item", "item": "POTION", "target": 0}]}

    def test_super_effective(self):
        for name in ("greedy", "skilled"):
            self.assertEqual(policies.get_policy(name)(self.state())["slot"], 1, name)
        picks = [policies.average_policy(dict(self.state(), seed=i)).get("slot") for i in range(200)]
        self.assertGreater(picks.count(1) / 200, 0.8)           # ~10% mistakes

    def test_average_heals_low(self):
        picks = [policies.average_policy(dict(self.state(hp=5), seed=i)) for i in range(100)]
        self.assertGreater(sum(p["type"] == "item" for p in picks), 70)


class ReportTest(unittest.TestCase):
    def test_tiny_report(self):
        from minasim.report import run_report
        rep = run_report(run_id="test-tiny", options={"scale": 0.02, "jobs": 1, "signals": ["difficulty", "dominance"], "bands": ["average"]})
        for k in ("meta", "stages", "flags", "species", "moves", "fights", "items", "tp", "level_curve"):
            self.assertIn(k, rep)
        self.assertTrue(rep["fights"])
        f = rep["fights"][0]
        for k in ("trainer", "name", "class", "stage", "party", "by_band", "by_starter", "target", "source"):
            self.assertIn(k, f)
        for fl in rep["flags"]:
            for k in ("id", "kind", "severity", "score", "title", "detail", "metric", "change", "repro"):
                self.assertIn(k, fl)
        json.dumps(rep)

    def test_deterministic(self):
        from minasim.report import run_report
        o = {"scale": 0.02, "jobs": 1, "signals": ["difficulty"], "bands": ["average"]}
        a = run_report(run_id="test-det-a", options=o)
        b = run_report(run_id="test-det-b", options=o)
        self.assertEqual(a["fights"], b["fights"])


if __name__ == "__main__":
    unittest.main()
