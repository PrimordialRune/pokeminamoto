"""Talk and Special end to end through the native engine (policy action -> ROM code -> outcome)."""
import os
import statistics
import sys
import unittest

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
try:
    from minasim.engine import Engine, Mon
    ENG = Engine(build=False)
except Exception:                                   # no native library: nothing to test here
    ENG = None


def first(st, typ, **kw):
    for a in st["legal"]:
        if a["type"] == typ and all(a.get(k) == v for k, v in kw.items()):
            return a
    return None


def best_move(st):
    return next(a for a in st["legal"] if a["type"] == "move")


@unittest.skipIf(ENG is None, "native engine not built")
class TalkSpecial(unittest.TestCase):
    def test_forced_special_changes_outcome(self):
        def with_special(st):
            return first(st, "special") or best_move(st)

        def without(st):
            return best_move(st)
        n_sp = dmg_sp = dmg_no = 0
        for s in range(1, 40):
            kw = dict(seed=s, player=[Mon("CHARMANDER", 16, moves=["SCRATCH"], awakened=True)], foe=[Mon("ONIX", 20)])
            a = ENG.battle(player_policy=with_special, log=True, **kw)
            b = ENG.battle(player_policy=without, log=True, **kw)
            n_sp += a["special_used"]
            self.assertEqual(b["special_used"], 0)
            self.assertIn("FLAME_CLAWS", a["moves_used"] if a["special_used"] else {"FLAME_CLAWS": 1})
            dmg_sp += sum(e["damage"] for e in a["log"] if e["side"] == 0 and e.get("move") == "FLAME_CLAWS")
            dmg_no += sum(e["damage"] for e in b["log"] if e["side"] == 0 and e.get("move"))
        self.assertGreater(n_sp, 0)
        self.assertGreater(dmg_sp, 0)
        self.assertNotEqual(dmg_sp, dmg_no)

    def test_forced_unnerve_changes_trainer_behaviour(self):
        def unnerve(st):
            return (first(st, "talk", kind="unnerve") if st["turn"] == 0 else None) or best_move(st)

        def never(st):
            return best_move(st)
        talked, hp_u, hp_n, fm_u, fm_n = 0, [], [], 0, 0
        for s in range(1, 120):
            kw = dict(seed=s, player=[Mon("CHARMANDER", 26, moves=["EMBER", "SLAM"])], foe=[Mon("RATICATE", 22), Mon("PIDGEOTTO", 22)], is_trainer=True,
                      trainer_class="YOUNGSTER", charisma=100)
            a = ENG.battle(player_policy=unnerve, log=True, **kw)
            b = ENG.battle(player_policy=never, log=True, **kw)
            talked += a["talk_used"]
            self.assertEqual(b["talk_used"], 0)
            self.assertTrue(any(e["action"] == "talk" and e["kind"] == "unnerve" for e in a["log"]))
            hp_u.append(sum(m["hp"] for m in a["player"]))
            hp_n.append(sum(m["hp"] for m in b["player"]))
        self.assertGreater(talked, 100)
        self.assertGreater(statistics.mean(hp_u), statistics.mean(hp_n))     # Panic (random moves) must help the player

    def test_talk_boss_malus(self):
        def prov(st):
            return (first(st, "talk", kind="provoke") if st["turn"] == 0 else None) or best_move(st)
        kw = dict(player=[Mon("CHARMANDER", 22)], foe=[Mon("SLOWPOKE", 20)], is_trainer=True, charisma=0, player_policy=prov, log=True)
        r1 = [ENG.battle(seed=s, trainer_class="YOUNGSTER", **kw) for s in range(1, 60)]
        r2 = [ENG.battle(seed=s, trainer_class="RIVAL", **kw) for s in range(1, 60)]
        self.assertNotEqual([(r["turns"], sum(m["hp"] for m in r["foe"])) for r in r1], [(r["turns"], sum(m["hp"] for m in r["foe"])) for r in r2])


if __name__ == "__main__":
    unittest.main()
