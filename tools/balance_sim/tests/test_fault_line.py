"""Fault Line (Takeshi's arena): the Python reference model against the C source.

    python3 tools/balance_sim/tests/test_fault_line.py

No native library needed: it parses src/arena_ecology.c and include/arena_ecology.h.
"""
import os
import re
import sys
import unittest

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(os.path.dirname(HERE)))
sys.path.insert(0, os.path.dirname(HERE))

from minasim import arena as A   # noqa: E402


def read(rel):
    with open(os.path.join(ROOT, rel)) as f:
        return f.read()


class Parity(unittest.TestCase):
    def test_defaults_match_the_c_initializer(self):
        src = read("src/arena_ecology.c")
        body = src[src.index("gArenaTuning ="):]
        body = body[:body.index("};")]
        for knob, want in A.FAULT_LINE_DEFAULTS.items():
            m = re.search(r"\.%s\s*=\s*(\w+)," % knob, body)
            self.assertIsNotNone(m, knob)
            val = {"TRUE": 1, "FALSE": 0}.get(m.group(1), None)
            self.assertEqual(int(m.group(1)) if val is None else val, want, knob)

    def test_field_order_matches_the_c_struct(self):
        hdr = read("include/arena_ecology.h")
        body = hdr[hdr.index("struct ArenaTuning"):]
        body = body[:body.index("};")]
        names = re.findall(r"^\s*u16 (\w+);", body, re.M)
        self.assertEqual(names, A.FIELDS)

    def test_arena_ids_match_the_enum(self):
        hdr = read("include/arena_ecology.h")
        body = hdr[hdr.index("enum"):hdr.index("ARENA_COUNT")]
        ids = re.findall(r"^\s*ARENA_(\w+),", body, re.M)
        self.assertEqual({n: i for i, n in enumerate(ids)}, {k: v for k, v in A.ARENAS.items()})

    def test_terrain_constants(self):
        hdr = read("include/arena_ecology.h")
        for name, val in (("INTACT", 0), ("CRACKED", 1), ("FRACTURED", 2)):
            self.assertRegex(hdr, r"#define ARENA_TERRAIN_%s\s+%d\b" % (name, val))


class Model(unittest.TestCase):
    def test_terrain_only_gets_worse_and_messages_fire_once(self):
        fl = A.FaultLine()
        for _ in range(40):
            fl.after_move("TACKLE", 40, "NORMAL", True)
            fl.end_turn()
        self.assertEqual(fl.terrain, A.TERRAIN_FRACTURED)
        self.assertEqual(fl.queue, ["GROUND_CRACKS", "FLOOR_FRACTURED"])

    def test_mud_stops_cracks_and_wind_dries_it(self):
        fl = A.FaultLine()
        fl.after_move("WATER_GUN", 40, "WATER", True)
        self.assertEqual((fl.mud, fl.blows), (4, 1))
        for _ in range(5):
            fl.after_move("TACKLE", 40, "NORMAL", True)
        self.assertEqual(fl.blows, 1)                      # mud doesn't crack
        fl.after_move("GUST", 40, "FLYING", True)
        self.assertEqual(fl.mud, 0)                        # dried, no dust yet
        fl.after_move("GUST", 40, "FLYING", True)
        self.assertEqual(fl.storm, 3)
        self.assertEqual(fl.queue, ["FLOOR_MUD", "DUST_STORM"])

    def test_mountain_defense_and_speed(self):
        fl = A.FaultLine()
        self.assertEqual(fl.defense_pct(True), 200)         # q72
        self.assertEqual(fl.defense_pct(False), 100)
        self.assertEqual(fl.speed_pct(False), 67)
        self.assertEqual(fl.speed_pct(True), 100)


if __name__ == "__main__":
    unittest.main()
