"""Tests for the studio lab's slice additions: data-driven journeys, the money model, the status-move policy and the gate.

    python3 tools/balance_sim/tests/test_slice_lab.py

Needs the host library (`make balance-lib`). The parity test replays the journey as it ran before the lab revision.
"""
import json
import os
import sys
import tempfile
import time
import unittest

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.dirname(HERE))

from minasim import economy as ECON, gate as GATE, journey as J, policies, sigdata as SIG   # noqa: E402
from minasim.gamedata import Game                                                          # noqa: E402

SCENARIO = os.path.join(os.path.dirname(HERE), "scenarios", "masara_nibi.yaml")


def sim_args(**over):
    class A:
        model = None
        wild = "json"
        signature = "off"
        arena = None
        no_arena = False
        arena_use = 0.5
        scenario = SCENARIO
    a = A()
    for k, v in over.items():
        setattr(a, k, v)
    return a


class Economy(unittest.TestCase):
    def test_prizes_from_the_class_table(self):
        self.assertEqual(ECON.prize("Bird Keeper", 9), 4 * 9 * 8)          # gTrainerMoneyTable: BIRD_KEEPER 8
        self.assertEqual(ECON.prize("Rival", 5), 4 * 5 * 15)
        self.assertEqual(ECON.prize("Leader", 14), 4 * 14 * 25)
        self.assertEqual(ECON.prize("Unlisted Class", 10), 4 * 10 * 5)       # {0xFF, 5}

    def test_prices_and_start_money_from_the_sources(self):
        self.assertEqual(ECON.item_price("POKE_BALL"), 200)
        self.assertEqual(ECON.item_price("POTION"), 300)
        self.assertEqual(ECON.item_price("ANTIDOTE"), 100)
        self.assertEqual(ECON.start_money(), 3000)

    def test_ball_throws_stop_at_the_supply(self):
        import random
        used, ok = ECON.balls_to_catch(random.Random(1), 0)
        self.assertEqual((used, ok), (0, False))
        for seed in range(50):
            used, ok = ECON.balls_to_catch(random.Random(seed), 3)
            self.assertTrue(1 <= used <= 3 if ok else used == 3)


class Data(unittest.TestCase):
    def test_signatures_follow_the_ifdefs(self):
        sigs = SIG.signatures(set())
        self.assertEqual(sigs.get("CHARMANDER"), "FLAME_CLAWS")
        self.assertEqual(sigs.get("FARFETCHD"), "LEEK_SLAP")
        self.assertNotIn("BULBASAUR", sigs)                                    # Bond fallback: no move yet

    def test_wild_rows_come_from_the_json(self):
        g = Game.get()
        rows = J.wild_rows_json(g, "MAP_ROUTE1")
        self.assertAlmostEqual(sum(r[3] for r in rows), 100.0)
        self.assertTrue({"PIDGEY", "RATTATA"} <= {r[0] for r in rows})
        self.assertIsNone(J.wild_rows_json(g, "MAP_NOT_A_MAP"))

    def test_stub_parties_fall_back_and_real_ones_win(self):
        g = Game.get()
        saved = g.party_trainers
        try:
            st = {"fight": "TRAINER_X", "_party": [{"species": "PIDGEY", "level": 7, "moves": None, "item": None}], "trainer_class": "Youngster"}
            g.party_trainers = {"TRAINER_X": {"id": "TRAINER_X", "class": "Bird Keeper", "ai": [],
                                              "party": [{"species": "RATTATA", "level": 2, "moves": None, "item": None}]}}
            self.assertEqual(J.resolve_trainer(g, st)["BULBASAUR"]["source"], "yaml")
            g.party_trainers["TRAINER_X"]["party"] = [{"species": "SPEAROW", "level": 7, "moves": ["PECK"], "item": None}]
            info = J.resolve_trainer(g, st)["SQUIRTLE"]
            self.assertEqual((info["source"], info["party"][0]["species"], info["class"]), ("party", "SPEAROW", "Bird Keeper"))
            st["fight"], st["trainer"] = "TRAINER_X", ["TRAINER_X"] * 3
            self.assertEqual(len(J.resolve_trainer(g, st)), 3)
            g.party_trainers = {}
            del st["_party"]
            with self.assertRaises(ValueError):
                J.resolve_trainer(g, st)
        finally:
            g.party_trainers = saved

    def test_scenario_names_trainer_constants(self):
        sc = J.load(SCENARIO)
        ids = [st["fight"] for st in sc["steps"] if "fight" in st]
        self.assertTrue(all(i.startswith("TRAINER_") for i in ids))
        self.assertIn("TRAINER_YUJIROU", ids)


class Rules(unittest.TestCase):
    def test_bands_are_the_plan_bands(self):
        per = {"BULBASAUR": 90.0, "CHARMANDER": 90.0, "SQUIRTLE": 90.0}
        self.assertEqual(GATE.judge("regular", per, 90.0, 3)[0], "PASS")
        self.assertEqual(GATE.judge("regular", per, 98.0, 3)[0], "FAIL")                       # above 97
        self.assertEqual(GATE.judge("regular", dict(per, BULBASAUR=79.0), 90.0, 3)[0], "FAIL")  # each >= 80
        self.assertEqual(GATE.judge("ken", {"BULBASAUR": 55, "CHARMANDER": 75, "SQUIRTLE": 75}, 68.3, 3)[0], "PASS")
        self.assertEqual(GATE.judge("ken", {"BULBASAUR": 45, "CHARMANDER": 75, "SQUIRTLE": 75}, 65.0, 3)[0], "FAIL")
        self.assertEqual(GATE.judge("gym", {"BULBASAUR": 40, "CHARMANDER": 70, "SQUIRTLE": 70}, 60.0, 3)[0], "PASS")
        self.assertEqual(GATE.judge("gym", {"BULBASAUR": 73, "CHARMANDER": 36, "SQUIRTLE": 96}, 68.3, 3)[0], "FAIL")   # Takeshi on Grunge Rock
        self.assertEqual(GATE.judge("lab", {"BULBASAUR": 90, "CHARMANDER": 10, "SQUIRTLE": 50}, 50.0, 3)[0], "REPORT")
        self.assertEqual(GATE.judge("r22", {"BULBASAUR": 43, "CHARMANDER": 21, "SQUIRTLE": 9}, 24.0, 3)[0], "FLAG")
        self.assertEqual(GATE.judge("r22", {"BULBASAUR": 43, "CHARMANDER": 21, "SQUIRTLE": 10}, 24.0, 3)[0], "REPORT")

    def test_every_slice_fight_has_a_rule(self):
        sc = J.load(SCENARIO)
        for st in sc["steps"]:
            if "fight" in st:
                self.assertIn(st["fight"], GATE.FIGHT_RULE)


class StatusPolicy(unittest.TestCase):
    def state(self, foe_hp=40, status=None):
        g = Game.get()
        return {"side": 0, "turn": 1, "seed": 5, "kind": "trainer",
                "me": {"species": "BULBASAUR", "level": 7, "hp": 20, "max_hp": 22, "tp": 90, "tp_max": 100, "status": None, "stages": {},
                       "moves": ["TACKLE", "LEECH_SEED"], "tp_costs": [3, 8], "special": None},
                "foe": {"species": "RATTATA", "level": 7, "hp": foe_hp, "max_hp": 40, "status": status, "stages": {}},
                "party": [], "items": {},
                "legal": [{"type": "move", "slot": 0}, {"type": "move", "slot": 1}]}

    def test_average_player_uses_status_at_the_rate(self):
        p = dict(policies.policy_params("average"), mistake_rate=0.0, status_use_rate=1.0)
        self.assertEqual(policies.average_policy(self.state(), p)["slot"], 1)               # Leech Seed
        self.assertEqual(policies.average_policy(self.state(foe_hp=15), p)["slot"], 0)      # foe not healthy: attack
        p0 = dict(p, status_use_rate=0.0)
        self.assertEqual(policies.average_policy(self.state(), p0)["slot"], 0)              # the old policy: never

    def test_rate_is_a_measured_share_not_zero(self):
        p = dict(policies.policy_params("average"), mistake_rate=0.0, status_use_rate=0.3)
        used = sum(policies.average_policy(dict(self.state(), seed=s), p)["slot"] == 1 for s in range(400))
        self.assertTrue(60 < used < 180, used)

    def test_no_potions_in_wild_battles(self):
        p = dict(policies.policy_params("average"), mistake_rate=0.0, status_use_rate=0.0)
        s = self.state()
        s["me"]["hp"] = 3
        s["items"] = {"POTION": 1}
        s["legal"].append({"type": "item", "item": "POTION"})
        self.assertEqual(policies.average_policy(s, p)["type"], "item")                      # trainer: heals
        s["kind"] = "wild"
        self.assertEqual(policies.average_policy(s, p)["type"], "move")                      # wild: keeps the Potion


class Parity(unittest.TestCase):
    def test_legacy_model_matches_the_pre_lab_journey(self):
        """Before the lab revision the journey ran from the YAML teams, the YAML wild tables and a Potion restock at every inn."""
        with open(os.path.join(HERE, "baseline_legacy60.json")) as f:
            base = json.load(f)
        g = Game.get()
        saved = g.party_trainers
        g.party_trainers = {}                                                                # force the YAML fallback teams
        try:
            sc = J.prepare(sim_args(model="legacy", wild="yaml"))
            s = J.summarise(sc, J.simulate(sc, base["runs"]))
        finally:
            g.party_trainers = saved
        worst = 0.0
        for f in s["fights"]:
            for k, v in base["fights"][f["id"]].items():
                worst = max(worst, abs(f["by"][k]["win"] - v) * 100)
        self.assertLessEqual(worst, 2.0, "legacy journey drifted %.1f points from the baseline" % worst)


class Journey(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.sc = J.prepare(sim_args())
        cls.recs = J.simulate(cls.sc, 8, bands=("average",))

    def test_money_model_in_every_run(self):
        for r in self.recs:
            self.assertEqual(set(r["money_at"]), {"TRAINER_YUJIROU", "TRAINER_TAKESHI"})
            self.assertGreaterEqual(r["money_end"], 0)
            self.assertIsNotNone(r["balls_at_nibi"])
            # nothing sells Potions before Nibi, an inn restocks none: use can never exceed Kit + vendor + quests + the Nibi shop
            self.assertLessEqual(sum(r["potions_used"].get(s, 0) for s in ("Route 1", "Route 22", "Tokiwa arena", "Route 2 and forest")), 4)

    def test_ball_supply_before_nibi(self):
        for r in self.recs:
            self.assertLessEqual(r["balls_at_nibi"], ECON.OKIDO_BALLS + ECON.KIT_BALLS + ECON.FOREST_ITEM_BALL)

    def test_legacy_replays_the_old_inn_restock(self):
        sc = J.prepare(sim_args(model="legacy"))
        self.assertFalse(any(st.get("new") for st in sc["steps"]))
        self.assertNotIn("TRAINER_KEN_FOREST", [st.get("fight") for st in sc["steps"]])


class GateRun(unittest.TestCase):
    def test_gate_finishes_in_under_a_minute_and_writes_gates_md(self):
        with tempfile.TemporaryDirectory() as d:
            out = os.path.join(d, "gates.md")
            t0 = time.time()
            rc = GATE.main(["--runs", "60", "--out", out])
            took = time.time() - t0
            self.assertLess(took, 60.0)
            self.assertIn(rc, (0, 1))
            md = open(out, encoding="utf-8").read()
            for head in ("## 1. Fights against the bands", "## 3. Signature check", "## 4. Report-only metrics", "## 5. Status-move policy"):
                self.assertIn(head, md)
            self.assertIn("TP rests before Takeshi", md)
            self.assertIn("Poke Balls at Nibi", md)


if __name__ == "__main__":
    unittest.main()
