#!/usr/bin/env python3
"""Check that gamedata.py still parses the current decomp sources.

    python3 tools/balance/test_gamedata.py        (also runs in CI)

gamedata.py reads C headers with regexes, so a reformatted or restructured table makes it
silently return fewer rows. These checks pin the Gen 1 data that must always be there; they
do not depend on the Gen 2/3 leftovers, which are being deleted.
"""
import os
import sys
import unittest

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import gamedata  # noqa: E402

GEN1_COUNT = 151


class GameDataParses(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.d = gamedata.GameData()

    def test_all_gen1_species_present(self):
        names = list(self.d.species)
        self.assertEqual(names[0], "BULBASAUR")
        self.assertIn("MEW", names)
        gen1 = names[:names.index("MEW") + 1]
        self.assertEqual(len(gen1), GEN1_COUNT)
        for n in gen1:
            s = self.d.species[n]
            self.assertTrue(all(s[k] > 0 for k in ("hp", "atk", "def", "spe", "spa", "spd")), n)
            self.assertTrue(1 <= len(s["types"]) <= 2, n)
            self.assertIn(s["growth"], gamedata.GROWTH, n)
            self.assertGreater(s["exp_yield"], 0, n)

    def test_known_species_values(self):
        b = self.d.species["BULBASAUR"]
        self.assertEqual((b["hp"], b["atk"], b["def"], b["spe"], b["spa"], b["spd"]), (45, 49, 49, 45, 65, 65))
        self.assertEqual(b["types"], ("GRASS", "POISON"))
        self.assertEqual(self.d.species["MEWTWO"]["spa"], 154)

    def test_moves(self):
        m = self.d.moves
        self.assertGreaterEqual(len(m), 165)
        self.assertEqual(m["TACKLE"]["power"], 35)
        self.assertEqual(m["TACKLE"]["type"], "NORMAL")
        self.assertEqual(m["EMBER"]["type"], "FIRE")
        for name in ("TACKLE", "EMBER", "WATER_GUN", "THUNDERBOLT"):
            self.assertGreater(m[name]["tp"], 0, name)  # tpCost parsed

    def test_learnsets_and_evolutions(self):
        ls = self.d.learnsets
        for sp in ("BULBASAUR", "CHARMANDER", "SQUIRTLE", "MEW"):
            self.assertTrue(ls.get(sp), sp)
        self.assertIn((1, "SCRATCH"), ls["CHARMANDER"])
        self.assertEqual(self.d.evolutions["CHARMANDER"], (16, "CHARMELEON"))
        self.assertEqual(len(self.d.evolutions), 52)  # the Gen 1 species that evolve by level

    def test_type_chart(self):
        c = self.d.type_chart
        self.assertGreaterEqual(len(c), 90)
        self.assertEqual(c[("FIRE", "GRASS")], 20)
        self.assertEqual(c[("WATER", "FIRE")], 20)
        self.assertEqual(c[("NORMAL", "GHOST")], 0)
        self.assertEqual(c[("FIRE", "WATER")], 5)

    def test_helpers(self):
        self.assertEqual(self.d.resolve("bulbasaur"), "BULBASAUR")
        self.assertEqual(self.d.move("water gun")["type"], "WATER")
        self.assertEqual(self.d.evo_stage("VENUSAUR"), (3, True))
        self.assertEqual(gamedata.exp_for_level("MEDIUM_FAST", 10), 1000)


if __name__ == "__main__":
    unittest.main()
