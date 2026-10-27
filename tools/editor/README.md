# Minamoto data editor (MVP)

A local web editor for the slice's data, in the Balance Lab's look (it serves the Lab's stylesheet). Four editors,
each with a "simulate" button that runs the Lab's real battle code and shows win rates against the wanted bands:

| Tab | Edits | File | Simulate |
|---|---|---|---|
| Learnsets | level-up moves, evolutions, TM/HM list of one species | `src/data/species/<name>.json` | the Masara-to-Nibi journey with and without your change |
| Trainers | keys and party (species, level, item, nickname, 4 moves, IVs/EVs lines) | `src/data/trainers.party` | the fight in the chapter (when the scenario has a step with the trainer's name) and a stand-alone draw, per starter and player band |
| Encounters | species, levels and rate of a map's wild tables | `src/data/wild_encounters.json` | wild win rates per slot, weighted, plus walk-backs to an inn over 12 battles |
| Arenas | the 21 `gArenaTuning` knobs | `src/arena_ecology.c` | the arena fights (Yujirou, Takeshi) with your knobs and with the ROM defaults |

## Run

From the repo root (no packages to install; Python 3 standard library only):

    python3 tools/editor/run.py            # http://127.0.0.1:8766, opens a browser
    python3 tools/editor/run.py --no-browser --port 8766

The simulate buttons need the Lab engine: `make balance-lib` once (and after C changes). Without it the editors work
and the simulate buttons say so.

**Saves never touch the repo by default.** They go to `build/editor/out/<repo path>` (gitignored); reload shows your
saved copy, the "Saved edits" tab shows the diff against the repo and can discard a copy. To keep an edit, copy the
file over the repo one, or start with `--in-place` (then Save writes the repo files directly). `--out DIR` changes
the save directory. Everything else reads the repo only.

## What each editor guarantees

- **Round trip.** Load then save with no edit gives the same bytes: species records through speciesgen's own
  `canonical()`, `wild_encounters.json` as `json.dumps(indent=2)`, and `trainers.party` and `arena_ecology.c` by
  keeping every line the editor does not own. Trainers: the header comment, `/* BEGIN owner */` / `/* END owner */`
  markers and all other comments are never touched; a trainer you did not edit is written back from its original
  lines; an edited one is re-rendered in the file's own layout.
- **Validation = `tools/check_data.py`.** The editor calls check_data's own check functions (`species_learnsets`,
  `species_evolutions`, `species_base_data`, `species_contiguous`, `trainer_parties`, `wild_encounters`) on the edited
  text, with the file read swapped for the edit, so messages read exactly like `make check` prints them. Species
  also run through speciesgen's generator (unknown evolution targets and the like). Problems the repo already has
  (`tools/check_data_known.json`) are shown grey as "known"; anything new is red and blocks Save (the server's
  `force` flag overrides it). Arena knobs have no check_data rule; the editor only checks they fit a u16 (limits are
  PLACEHOLDER).
- **Simulation** runs `python3 -m medit.simrun` in a subprocess that uses minasim as a library (nothing in
  `tools/balance_sim` is changed). Edits are applied to that process's data only: nothing is written, nothing is
  rebuilt. Learnset edits change the moves the Lab gives player mons and the foes of edited species (the engine's own
  ROM learnset table is compiled in and not re-read).

## Layout

    run.py                  starts the server
    medit/paths.py          repo root, Store (read repo, write to the out directory)
    medit/species.py        species records        medit/trainers.py   lossless trainers.party parser
    medit/encounters.py     wild_encounters.json   medit/arena.py      gArenaTuning knobs
    medit/validate.py       check_data reuse       medit/simrun.py     minasim runner (subprocess)
    medit/server.py         HTTP API + static      static/             the pages (plain JS, no CDN scripts)
    tests/                  see below

## Tests

    python3 -W ignore -m unittest discover -s tools/editor/tests -p 'test_*.py'

- `test_roundtrip.py`: byte-identical load/save of all 153 species records, `trainers.party` (every block also
  re-renders from the model), `wild_encounters.json`, the arena knobs; an edit changes only the lines it should;
  comments and markers survive; nothing is written to the repo.
- `test_validation_parity.py`: seeds bad trainers, encounters and a bad species into a private copy of the repo
  layout, runs the real `check_data.py` there and requires the editor to report exactly the same new messages.
- `test_server.py`: the API end to end on a temp save directory (and a short real simulation).
- `tests/tour.py OUT_DIR [--video]`: screenshot tour with Playwright (`pip install playwright && playwright install chromium`):
  edits Yujirou's party, simulates, screenshots the band, saves, walks the other editors.

## Not in the MVP

Adding or deleting trainers, species or encounter maps (a trainer id must exist in `opponents.h` anyway), tutor and
egg-move editing, editing the number of wild slots, Signature/TP/move data, undo beyond Revert, multi-user use.
