"""Validation parity: the editor reports the same problems as tools/check_data.py on a seeded bad file.

A "farm" is a temp repo root whose big directories are symlinks to the real repo and whose three data files
(species records, trainers.party, wild_encounters.json) are private copies, so a copy of check_data.py run inside it
reads the seeded bad files exactly as it would in the repo. The check functions run there (check_data.CHECKS, the
same code `make check` runs); the editor validates the same text through medit.validate. Messages must be equal
as sets, after taking away what the unedited farm already reports (the repo's known problems).
"""
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile
import unittest

import helpers
from medit import paths, species, trainers, validate

REPO = helpers.REPO
RUNNER = r'''
import json, sys
sys.path.insert(0, "tools")
import check_data
ctx = check_data.Ctx()
for name in sys.argv[1:]:
    getattr(check_data, name)(ctx)
print(json.dumps(ctx.errors))
'''


def make_farm(root):
    for d in ("include", "data", "docs", "sound", "graphics", "build"):
        if os.path.exists(os.path.join(REPO, d)):
            os.symlink(os.path.join(REPO, d), os.path.join(root, d))
    os.makedirs(os.path.join(root, "tools"))
    for n in os.listdir(os.path.join(REPO, "tools")):
        if n not in ("check_data.py", "check_data_known.json", "textlang.py"):
            os.symlink(os.path.join(REPO, "tools", n), os.path.join(root, "tools", n))
    for n in ("check_data.py", "textlang.py"):
        shutil.copy(os.path.join(REPO, "tools", n), os.path.join(root, "tools", n))
    json.dump({}, open(os.path.join(root, "tools", "check_data_known.json"), "w"))
    os.makedirs(os.path.join(root, "src", "data"))
    for n in os.listdir(os.path.join(REPO, "src")):
        if n != "data":
            os.symlink(os.path.join(REPO, "src", n), os.path.join(root, "src", n))
    for n in os.listdir(os.path.join(REPO, "src", "data")):
        if n not in ("species", "trainers.party", "wild_encounters.json"):
            os.symlink(os.path.join(REPO, "src", "data", n), os.path.join(root, "src", "data", n))
    shutil.copytree(os.path.join(REPO, "src/data/species"), os.path.join(root, "src/data/species"))
    for n in ("trainers.party", "wild_encounters.json"):
        shutil.copy(os.path.join(REPO, "src/data", n), os.path.join(root, "src/data", n))


def farm_errors(root, *checks):
    r = subprocess.run([sys.executable, "-c", RUNNER] + list(checks), cwd=root, capture_output=True, text=True)
    assert r.returncode == 0, r.stderr
    return {(n, m) for n, m in json.loads(r.stdout)}


class Parity(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.tmp = tempfile.mkdtemp(prefix="medit-farm-")
        make_farm(cls.tmp)

    @classmethod
    def tearDownClass(cls):
        shutil.rmtree(cls.tmp, ignore_errors=True)

    def put(self, rel, text):
        with open(os.path.join(self.tmp, rel), "w", encoding="utf-8", newline="") as f:
            f.write(text)

    def restore(self, rel):
        shutil.copy(os.path.join(REPO, rel), os.path.join(self.tmp, rel))

    def compare(self, kind, checks, editor_errors, rel_restore):
        base = farm_errors(self.tmp, *checks) if False else None
        return base

    def test_trainers(self):
        text = open(os.path.join(REPO, paths.TRAINERS), encoding="utf-8", newline="").read()
        base = farm_errors(self.tmp, "trainer_parties")
        bad = text.replace("Level: 2\n", "Level: 140\n", 1)
        bad = bad.replace("\nRattata\n", "\nChikorita\n", 1)                  # a Gen 2 species
        bad = bad.replace("=== TRAINER_DAISY ===", "=== TRAINER_NOT_IN_OPPONENTS ===\nName: X\nClass: Youngster\nPic: Hiker\n\nPidgey\nLevel: 5\n- Sketch\n\n=== TRAINER_DAISY ===", 1)
        self.assertNotEqual(bad, text)
        self.put(paths.TRAINERS, bad)
        try:
            cli = farm_errors(self.tmp, "trainer_parties") - base
        finally:
            self.restore(paths.TRAINERS)
        ed = {(e["check"], e["message"]) for e in validate.validate_trainers(bad) if e["new"]}
        self.assertGreaterEqual(len(cli), 4, cli)
        self.assertEqual(ed, cli)

    def test_encounters(self):
        base = farm_errors(self.tmp, "wild_encounters")
        d = json.load(open(os.path.join(REPO, paths.ENCOUNTERS)))
        e = d["wild_encounter_groups"][0]["encounters"][0]
        e["land_mons"]["mons"][0]["species"] = "SPECIES_CHIKORITA"
        e["land_mons"]["mons"][1].update(min_level=9, max_level=3)
        e["land_mons"]["encounter_rate"] = 150
        e["map"] = "MAP_NOT_A_REAL_MAP"
        bad = json.dumps(d, indent=2) + "\n"
        self.put(paths.ENCOUNTERS, bad)
        try:
            cli = farm_errors(self.tmp, "wild_encounters") - base
        finally:
            self.restore(paths.ENCOUNTERS)
        ed = {(x["check"], x["message"]) for x in validate.validate_encounters(bad) if x["new"]}
        self.assertEqual(len(cli), 4, cli)
        self.assertEqual(ed, cli)

    def test_species(self):
        checks = ("species_learnsets", "species_evolutions", "species_contiguous")
        base = farm_errors(self.tmp, *checks)
        ss = species.SpeciesStore(paths.Store())
        rec = ss.load("pikachu")
        rec["levelUpMoves"] = [[1, "THUNDER_SHOCK"], [9, "NOT_A_MOVE"], [5, "GROWL"], [101, "TACKLE"]]
        rec["evolutions"] = [{"method": "LEVEL", "param": 1, "into": "PIDGEY"}]
        text = ss.render(rec)
        self.put("src/data/species/pikachu.json", text)
        try:
            cli = farm_errors(self.tmp, *checks) - base
        finally:
            self.restore("src/data/species/pikachu.json")
        ed = {(e["check"], e["message"]) for e in validate.validate_species("PIKACHU", rec) if e["new"] and e["check"] in checks}
        self.assertGreaterEqual(len(cli), 4, cli)
        self.assertEqual(ed, cli)

    def test_clean_files_report_nothing_new(self):
        for name in ("PIKACHU", "BULBASAUR", "MEW"):
            self.assertEqual([e for e in validate.validate_species(name, species.SpeciesStore(paths.Store()).load(name.lower())) if e["new"]], [])
        t = open(os.path.join(REPO, paths.TRAINERS), encoding="utf-8").read()
        self.assertEqual([e for e in validate.validate_trainers(t) if e["new"]], [])
        self.assertEqual([e for e in validate.validate_encounters(open(os.path.join(REPO, paths.ENCOUNTERS)).read()) if e["new"]], [])

    def test_cli_agrees_on_exit_status(self):
        """The real CLI fails on the seeded bad trainers file (exit 1) and passes on the pristine one."""
        def run():
            return subprocess.run([sys.executable, "tools/check_data.py", "--only", "trainer_parties"], cwd=self.tmp, capture_output=True, text=True)
        self.assertEqual(run().returncode, 0)
        text = open(os.path.join(REPO, paths.TRAINERS), encoding="utf-8").read().replace("Level: 2\n", "Level: 140\n", 1)
        self.put(paths.TRAINERS, text)
        try:
            r = run()
        finally:
            self.restore(paths.TRAINERS)
        self.assertEqual(r.returncode, 1)
        self.assertIn("level 140", r.stdout)
        self.assertIn("level 140", " ".join(e["message"] for e in validate.validate_trainers(text)))

    def test_arena_rules(self):
        errs = validate.validate_arena({"wetTurns": 70000, "ventBlockers": 17, "rockDefFlyers": 3, "rockSpeedPct": 5000})
        self.assertEqual(len(errs), 4)


if __name__ == "__main__":
    unittest.main()
