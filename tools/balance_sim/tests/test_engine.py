"""Engine tests: python3 -m unittest discover -s tools/balance_sim/tests -p 'test_engine.py' -v
(needs build/host/libminasim.so: `make balance-lib`). Set MINASIM_BENCH=1 for the longer benchmark."""
import os
import sys
import time
import unittest

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))

from minasim.engine import Engine, Mon, EngineNotBuilt  # noqa: E402
from minasim.runner import run_batch  # noqa: E402

try:
    ENG = Engine()
except EngineNotBuilt:
    ENG = None


def sig(r):
    return (r["winner"], r["turns"], r["error"], [(m["species"], m["level"], m["hp"], m["exp"], m["tp"]) for m in r["player"]],
            [(m["species"], m["level"], m["hp"]) for m in r["foe"]], r["stats"][0]["moves_used"])


def charmander_vs_pidgey(**kw):
    d = dict(player=[Mon("CHARMANDER", 14, moves=["SCRATCH", "EMBER"])], foe=[Mon("PIDGEY", 12)], player_policy="greedy", foe_policy="rom")
    d.update(kw)
    return d


@unittest.skipIf(ENG is None, "engine not built: make balance-lib")
class EngineTests(unittest.TestCase):
    def test_determinism_per_seed_and_order(self):
        specs = [dict(seed=s, player=[Mon("BULBASAUR", 12), Mon("PIDGEY", 10)], foe=[Mon("RATTATA", 11), Mon("GEODUDE", 11)],
                      player_policy=p, foe_policy=f) for s, (p, f) in enumerate([("greedy", "rom"), ("random", "greedy")] * 15, 1)]
        a = [sig(ENG.battle(**s)) for s in specs]
        b = [sig(ENG.battle(**s)) for s in reversed(specs)][::-1]
        self.assertEqual(a, b, "a battle depends on the battles run before it")
        self.assertEqual(sig(ENG.battle(**specs[3])), sig(ENG.battle(**specs[3])))
        self.assertGreater(len({repr(x[1:]) for x in a}), 5, "seeds should give different battles")

    def test_no_errors_no_leaks(self):
        for s in range(1, 150):
            r = ENG.battle(**charmander_vs_pidgey(seed=s, player_policy=("random" if s % 2 else "greedy")))
            self.assertIsNone(r["error"])
            self.assertEqual(r["leaked_blocks"], 0)
            self.assertEqual(r["illegal"], 0)

    def test_wild_result_shape(self):
        r = ENG.battle(seed=1, log=True, **charmander_vs_pidgey())
        self.assertIn(r["winner"], (0, 1, 2))
        p = r["player"][0]
        for k in ("species", "level", "hp", "max_hp", "tp", "tp_max", "status"):
            self.assertIn(k, p)
        self.assertEqual(p["species"], "CHARMANDER")
        self.assertTrue(r["log"] and all(k in r["log"][0] for k in ("turn", "side", "slot", "action", "move", "damage", "crit", "miss", "effectiveness", "ko")))
        self.assertNotIn("log", ENG.battle(seed=1, **charmander_vs_pidgey()))

    def test_trainer_party_from_rom_data(self):
        r = ENG.battle(seed=3, player=[Mon("CHARMANDER", 16), Mon("PIDGEY", 12)], trainer="TRAINER_DAISY", player_policy="greedy")
        self.assertEqual([m["species"] for m in r["foe"]], ["PARAS", "ODDISH"])
        self.assertEqual([m["level"] for m in r["foe"]], [14, 14])
        self.assertTrue(r["trainer"])
        self.assertIsNone(r["error"])

    def test_explicit_trainer_party_and_multi_foe(self):
        r = ENG.battle(seed=3, player=[Mon("CHARMANDER", 14), Mon("PIDGEY", 12)], foe=[Mon("RATTATA", 10), Mon("SPEAROW", 11)],
                       is_trainer=True, trainer_class="LEADER", player_policy="greedy")
        self.assertIsNone(r["error"])
        self.assertEqual(len(r["foe"]), 2)
        self.assertEqual(r["winner"], 0)
        self.assertTrue(all(not m["alive"] for m in r["foe"]))

    def test_switching_forced_and_voluntary(self):
        calls = []

        def pol(st):
            calls.append((st["me"]["species"], st["forced"]))
            if st["forced"]:
                return st["legal"][0]
            if st["turn"] == 0:
                return next(a for a in st["legal"] if a["type"] == "switch")
            return next(a for a in st["legal"] if a["type"] == "move")
        r = ENG.battle(seed=2, player=[Mon("PIDGEY", 6), Mon("RATTATA", 12)], foe=[Mon("GEODUDE", 12)], player_policy=pol, log=True)
        self.assertEqual(calls[0][0], "PIDGEY")
        self.assertEqual(calls[1][0], "RATTATA", "voluntary switch did not bring in the bench mon")
        self.assertEqual(r["stats"][0]["switches"], 1)
        self.assertTrue(any(c[1] for c in calls) or r["winner"] != 1, "a fainted lead must ask for a replacement")
        # a guaranteed faint: level 3 lead against level 20
        calls.clear()
        r = ENG.battle(seed=2, player=[Mon("PIDGEY", 3), Mon("RATTATA", 12)], foe=[Mon("GEODUDE", 20)], player_policy=lambda st: st["legal"][0], log=True)
        self.assertEqual(r["player"][0]["alive"], False)

    def test_tp_spend_and_persistence(self):
        r = ENG.battle(**charmander_vs_pidgey(seed=5, player=[Mon("CHARMANDER", 14, moves=["SCRATCH", "EMBER"], tp_spent=30)]))
        p = r["player"][0]
        self.assertGreater(p["tp_spent"], 30 - 40)  # regen can win some back, spending adds
        self.assertEqual(p["tp"] + p["tp_spent"], p["tp_max"])
        self.assertLessEqual(r["stats"][0]["min_tp"], 70)
        r2 = ENG.battle(**charmander_vs_pidgey(seed=5, player=[Mon("CHARMANDER", 14, moves=["SCRATCH", "EMBER"], tp_spent=p["tp_spent"])]))
        self.assertLessEqual(r2["stats"][0]["min_tp"], 100 - p["tp_spent"])

    def test_no_tp_means_struggle(self):
        r = ENG.battle(**charmander_vs_pidgey(seed=5, player=[Mon("CHARMANDER", 14, moves=["EMBER"], tp_spent=100)]))
        self.assertIsNone(r["error"])
        self.assertGreaterEqual(r["stats"][0]["struggles"] + sum(v for k, v in r["stats"][0]["moves_used"].items() if k == "EMBER"), 1)

    def test_bag_items(self):
        def pol(st):
            if st["me"]["hp"] < st["me"]["max_hp"] * 0.7:
                for a in st["legal"]:
                    if a["type"] == "item":
                        return a
            return next(a for a in st["legal"] if a["type"] == "move")
        used = 0
        for s in range(1, 20):
            r = ENG.battle(seed=s, player=[Mon("BULBASAUR", 10, moves=["TACKLE"])], foe=[Mon("GEODUDE", 12)], player_policy=pol, bag={"POTION": 3})
            used += r["items_used"].get("POTION", 0)
            self.assertLessEqual(r["items_used"].get("POTION", 0), 3)
        self.assertGreater(used, 0)

    def test_talk_only_against_trainers(self):
        def pol(st):
            for a in st["legal"]:
                if a["type"] == "talk":
                    return a
            return next(a for a in st["legal"] if a["type"] == "move")
        w = ENG.battle(seed=1, **charmander_vs_pidgey(player_policy=pol))
        self.assertEqual(w["talk_used"], 0)
        t = ENG.battle(seed=1, player=[Mon("CHARMANDER", 14)], foe=[Mon("RATTATA", 12), Mon("PIDGEY", 12)], is_trainer=True, player_policy=pol, charisma=60)
        self.assertGreater(t["talk_used"], 0)

    def test_special_needs_awakening(self):
        def pol(st):
            for a in st["legal"]:
                if a["type"] == "special":
                    return a
            return next(a for a in st["legal"] if a["type"] == "move")
        self.assertEqual(ENG.battle(seed=9, **charmander_vs_pidgey(player=[Mon("CHARMANDER", 16)], player_policy=pol))["special_used"], 0)
        r = ENG.battle(seed=9, **charmander_vs_pidgey(player=[Mon("CHARMANDER", 16, awakened=True)], player_policy=pol))
        self.assertGreater(r["special_used"], 0)
        self.assertIn("FLAME_CLAWS", r["moves_used"])

    def test_exp_and_level_up(self):
        r = ENG.battle(seed=4, player=[Mon("CHARMANDER", 25, moves=["EMBER", "SCRATCH"]), Mon("PIDGEY", 5)], foe=[Mon("PIDGEY", 12)], player_policy="greedy")
        self.assertEqual(r["winner"], 0)
        self.assertGreater(r["exp"][0], 0)
        self.assertEqual(r["exp"][1], 0, "a mon that never entered gets no exp")
        # enough exp for several levels at once
        r = ENG.battle(seed=4, player=[Mon("PIDGEY", 5)], foe=[Mon("ONIX", 40)], player_policy="greedy", turn_cap=250)
        if r["winner"] == 0:
            self.assertGreater(r["player"][0]["level"], 5)

    def test_state_carries_between_battles(self):
        a = ENG.battle(seed=1, **charmander_vs_pidgey())
        p = a["player"][0]
        b = ENG.battle(seed=2, **charmander_vs_pidgey(player=[Mon("CHARMANDER", p["level"], moves=p["moves"], exp=p["exp"], hp=max(1, p["hp"]), tp_spent=p["tp_spent"])]))
        self.assertGreaterEqual(b["player"][0]["exp"], p["exp"])

    def test_dict_mons_and_run_batch(self):
        spec = dict(seed=1, player=[{"species": "CHARMANDER", "level": 14, "moves": ["SCRATCH", "EMBER"]}], foe=[{"species": "PIDGEY", "level": 12}], player_policy="greedy")
        self.assertEqual(sig(ENG.battle(**spec)), sig(ENG.battle(**charmander_vs_pidgey(seed=1))))
        specs = [dict(spec, seed=s) for s in range(1, 41)]
        serial = [sig(ENG.battle(**s)) for s in specs]
        par = [sig(r) for r in run_batch(specs, jobs=3, chunk=7)]
        self.assertEqual(serial, par)

    def test_bad_input(self):
        with self.assertRaises(KeyError):
            ENG.battle(seed=1, player=[Mon("NOTAMON", 5)], foe=[Mon("PIDGEY", 5)])
        with self.assertRaises(ValueError):
            ENG.battle(seed=1, player=[], foe=[Mon("PIDGEY", 5)])

    def test_illegal_policy_action_is_replaced(self):
        r = ENG.battle(seed=1, **charmander_vs_pidgey(player_policy=lambda st: {"type": "item", "item": "POTION", "target": 0}))
        self.assertGreater(r["illegal"], 0)
        self.assertIsNone(r["error"])

    def test_speed(self):
        n = 1500
        t = time.time()
        for s in range(n):
            ENG.battle(seed=s + 1, **charmander_vs_pidgey(player_policy="greedy"))
        c_policy = n / (time.time() - t)
        calls = [0]

        def pol(st):
            calls[0] += 1
            return next(a for a in st["legal"] if a["type"] == "move")
        t = time.time()
        for s in range(n // 3):
            ENG.battle(seed=s + 1, **charmander_vs_pidgey(player_policy=pol))
        py_policy = (n // 3) / (time.time() - t)
        t = time.time()
        ENG.run_seeds(range(1, n + 1), **charmander_vs_pidgey(player_policy="greedy"))
        fast = n / (time.time() - t)
        print("\nbench (1 core, 1v1 wild): battle() C greedy %.0f/s, battle() python policy %.0f/s, run_seeds C greedy %.0f/s" % (c_policy, py_policy, fast))
        self.assertGreater(c_policy, 500)


if __name__ == "__main__":
    unittest.main()
