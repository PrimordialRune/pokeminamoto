"""Load then save with no edits gives byte-identical files, for all four formats; edits touch only what they should."""
import difflib
import glob
import hashlib
import os
import re
import shutil
import sys
import tempfile
import unittest

import helpers
from medit import arena, encounters, paths, species, trainers

REPO = helpers.REPO
FILES = [paths.TRAINERS, paths.ENCOUNTERS, paths.ARENA]


def digest(rel):
    with open(os.path.join(REPO, rel), "rb") as f:
        return hashlib.sha256(f.read()).hexdigest()


def diff_lines(a, b):
    return [l for l in difflib.unified_diff(a.split("\n"), b.split("\n"), lineterm="", n=0) if l[:1] in "+-" and l[:3] not in ("+++", "---")]


class SpeciesRoundTrip(unittest.TestCase):
    def test_every_record_is_byte_identical(self):
        ss = species.SpeciesStore(paths.Store())
        names = ss.names()
        self.assertGreater(len(names), 150)
        for n in names:
            self.assertEqual(ss.render(ss.load(n)), ss.read_text(n), n)

    def test_save_writes_only_to_out(self):
        before = {p: digest(os.path.relpath(p, REPO)) for p in glob.glob(os.path.join(REPO, paths.SPECIES_DIR, "*.json"))[:20]}
        with tempfile.TemporaryDirectory() as out:
            ss = species.SpeciesStore(paths.Store(out=out))
            rec = ss.load("pikachu")
            rec["levelUpMoves"][2][0] = 7
            p = ss.save("pikachu", rec)
            self.assertTrue(p.startswith(out))
            self.assertEqual(len(diff_lines(ss.read_text("pikachu"), open(os.path.join(REPO, paths.SPECIES_DIR, "pikachu.json")).read())), 2)
        for p, d in before.items():
            self.assertEqual(d, digest(os.path.relpath(p, REPO)))


class TrainersRoundTrip(unittest.TestCase):
    def setUp(self):
        self.text = open(os.path.join(REPO, paths.TRAINERS), encoding="utf-8", newline="").read()

    def test_unedited_is_byte_identical(self):
        p = trainers.Party(self.text)
        self.assertEqual(p.dumps(), self.text)
        self.assertGreater(len(p.trainers()), 50)

    def test_every_block_re_renders_identically(self):
        """The model is complete: rendering a trainer from it (not from the saved raw lines) gives the same lines."""
        for t in trainers.Party(self.text).trainers():
            self.assertTrue(t.lossless, t.id)

    def test_comments_and_markers_survive_an_edit(self):
        p = trainers.Party(self.text)
        t = p.get("TRAINER_YUJIROU")
        j = t.to_json()
        j["mons"][0]["fields"][0][1] = "9"
        j["mons"].append({"species": "Pidgey", "nick": None, "gender": None, "item": "Oran Berry", "head": "", "fields": [["Level", "8"]], "moves": ["Gust", "Sand Attack"]})
        t.set_from_json(j)
        out = p.dumps()
        markers = re.compile(r"^/\*\s*(BEGIN|END)\s+[\w-]+\s*\*/$", re.M)
        self.assertEqual(markers.findall(out), markers.findall(self.text))
        self.assertEqual(re.findall(r"/\*.*?\*/", out, re.S), re.findall(r"/\*.*?\*/", self.text, re.S))
        d = diff_lines(self.text, out)
        self.assertEqual([l for l in d if l.startswith("-")], ["-Level: 2"])
        self.assertIn("+Pidgey @ Oran Berry", d)
        self.assertIn("+- Gust", d)
        # re-parse: the edit is still inside the same BEGIN block, and nothing else moved
        q = trainers.Party(out)
        self.assertEqual(q.get("TRAINER_YUJIROU").owner, "tokiwa-spine")
        self.assertEqual([x.id for x in q.trainers()], [x.id for x in trainers.Party(self.text).trainers()])
        self.assertEqual(trainers.Party(out).dumps(), out)

    def test_a_trainer_that_is_not_edited_keeps_its_bytes(self):
        p = trainers.Party(self.text)
        t = p.get("TRAINER_DAISY")
        j = t.to_json()
        t.set_from_json(j)          # set to the same values: not dirty
        self.assertFalse(t.dirty)
        self.assertEqual(p.dumps(), self.text)


class EncountersRoundTrip(unittest.TestCase):
    def test_byte_identical(self):
        e = encounters.Encounters(paths.Store())
        self.assertEqual(e.render(), e.text)

    def test_edit_changes_one_line(self):
        e = encounters.Encounters(paths.Store())
        ent = e.get(0)["entry"]
        ent["land_mons"]["mons"][0]["max_level"] += 1
        e.replace(0, ent)
        d = diff_lines(e.text, e.render())
        self.assertEqual(len(d), 2, d)


class ArenaRoundTrip(unittest.TestCase):
    def test_byte_identical(self):
        a = arena.ArenaKnobs(paths.Store())
        self.assertEqual(a.render(), a.text)

    def test_knob_names_match_the_lab(self):
        a = arena.ArenaKnobs(paths.Store())
        sys.path.insert(0, paths.SIM_ROOT)
        from minasim import arena as lab
        self.assertEqual([k["name"] for k in a.knobs], lab.FIELDS)
        self.assertEqual(a.values()["ventBlockers"], sum(lab.BLOCK.values()))

    def test_edit_keeps_comments_and_spelling(self):
        a = arena.ArenaKnobs(paths.Store())
        a.set_values({"wetTurns": 7, "leekAlwaysCrits": 1, "ventBlockers": 3})
        d = diff_lines(a.text, a.render())
        self.assertEqual(len(d), 6, d)
        self.assertIn("+    .leekAlwaysCrits = TRUE,        // a high crit ratio, not a sure crit (answer 70)", d)
        self.assertTrue(any("ARENA_BLOCK_STRING_SHOT | ARENA_BLOCK_OBSTRUCT, // obstruction moves" in l for l in d))
        b = arena.ArenaKnobs.__new__(arena.ArenaKnobs)
        self.assertEqual(arena.evaluate("ARENA_BLOCK_STRING_SHOT | ARENA_BLOCK_OBSTRUCT"), 3)


class RepoUntouched(unittest.TestCase):
    def test_default_store_never_writes_the_repo(self):
        before = {f: digest(f) for f in FILES}
        with tempfile.TemporaryDirectory() as out:
            s = paths.Store(out=out)
            s.write(paths.TRAINERS, "x")
            self.assertEqual(open(os.path.join(out, paths.TRAINERS)).read(), "x")
            self.assertEqual(s.read(paths.TRAINERS), "x")       # a saved copy wins on reload
        self.assertEqual(before, {f: digest(f) for f in FILES})

    def test_in_place_writes_a_temp_copy(self):
        with tempfile.TemporaryDirectory() as root:
            os.makedirs(os.path.join(root, "src/data"))
            shutil.copy(os.path.join(REPO, paths.TRAINERS), os.path.join(root, paths.TRAINERS))
            s = paths.Store(root=root, in_place=True)
            p = trainers.Party(s.read(paths.TRAINERS))
            s.write(paths.TRAINERS, p.dumps())
            self.assertEqual(open(os.path.join(root, paths.TRAINERS), newline="").read(), open(os.path.join(REPO, paths.TRAINERS), newline="").read())


if __name__ == "__main__":
    unittest.main()
