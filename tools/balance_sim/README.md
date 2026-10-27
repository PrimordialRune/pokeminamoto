# balance_sim (minasim)

Balance lab for Minamoto: runs thousands of battles through the game's real battle code (no graphics) and
flags species, moves, fights and items that are too strong or too weak at each point of the game.

## Quick start (WSL or a cloud session, from the repo root)

    make balance-lib                                   # build the engine from the current sources
    cd tools/balance_sim
    python3 -m minasim run --quick                     # ~1 minute; a full run (no --quick) takes ~5 min on 4 cores
    python3 -m minasim gui                             # opens the Balance Lab at http://127.0.0.1:8765

The GUI pages: Red flags (ranked problems with "what if" fixes), Journey (difficulty across the game),
Fights, Species dex, Moves & items, Duel lab (run any matchup now), Compare (two runs). Press "?" for the guide.

Other commands: `python3 -m minasim duel --fight gym:1 --band average --starter BULBASAUR --seed 1` replays a
flag, `python3 -m minasim lint` lists maps, trainers and names the stage builder could not place.
Target win-rate bands and the average/skilled player settings live in `targets.yaml`. Stages come from the
map data and `docs/design/data/overworld.csv`; fights not in `src/data/trainers.party` yet come from the design
docs (`source: design`) or are generated (`source: placeholder`, never above "info" severity).
Interfaces between engine, analysis and GUI: `CONTRACT.md`. GUI design spec: `minasim/gui/DESIGN.md`.

## Engine (host build of the real battle code)

`minasim/engine.py` drives the game's own battle code (battle scripts, damage, AI, TP, Talk, Signature, EXP)
compiled for x86-64 as `build/host/libminasim.so`. Nothing is re-implemented in Python; the interface is in
`CONTRACT.md` ("Engine API").

Build (from the repo root; rebuilds when any source, header, data table or battle script changes):

    make balance-lib          # make_tools + make generated + tools/balance_sim/host/Makefile
    HB_DEBUG=1 make -f tools/balance_sim/host/Makefile   # -g -O0 for gdb (do a normal build before check)

How: each source goes cpp -> preproc -> host gcc (like the ROM, with shadow headers in `host/hostinc/`, no
tracked file is edited). The ARM battle scripts are assembled and their 32-bit pointers remapped (`hb_ptr.c`).
The battle controller is the real opponent controller with its commands overridden (`hb_ctrl_tail.c`): data
commands stay real, graphics/sound complete at once, decisions go to the policy. `hb_api.c` is the C API,
`gen.py` the generators. Missing platform symbols are auto-stubbed (return 0 / 64 KB zero data).

Use:

    from minasim.engine import Engine
    eng = Engine()
    r = eng.battle(seed=1, player=[...], foe=[...], policy="greedy")   # see CONTRACT.md
    from minasim.runner import ...                                    # process pool, one library per worker

Test and bench: `python3 tools/balance_sim/tests/test_engine.py` (determinism per seed, trainer load, switching,
TP spend, leak check, speed print). On one core: about 6400 battles/s with the C greedy policy, 3700/s with a
Python policy callback (1v1 wild); `run_seeds` with C greedy about 9800/s. Scale with the runner (one process per
core: the game code is full of globals).

Guards:

- `host/stubs_allowlist.txt` pins the set of auto-stubbed symbols. `make check` runs `host/check_stubs.py`
  (via `make balance-check`) and fails when a new stub appears, e.g. after a source starts calling something
  platform-specific. If the new stub is harmless, run `python3 tools/balance_sim/host/check_stubs.py --update`
  and review the diff; otherwise implement it in `hb_platform.c`.
- `host/oracle.py` runs the ROM's ELF in Unicorn (`pip install unicorn`) and compares function by function with
  the host: RNG (50 draws), `CreateMon` (the full 100-byte struct) and `CalculateBaseDamage` (random pairs, moves,
  held items, stat stages, burn, screens) must be bit-identical. Needs a ROM build:
  `make release TEST=1 -j4`, then `python3 tools/balance_sim/host/oracle.py --n 1000`. Run it after touching
  `src/random.c`, `src/pokemon.c`, `hb_platform.c` or the host build rules.
  Whole battles are not compared: the ROM stirs the RNG every VBlank and its animations draw RNG, so a host seed
  never reproduces a ROM battle.

Notes: RNG is seeded directly with the 32-bit seed. State is reset per battle without reloading (leak 0, tested).
The turn cap maxes at 250 (the game's `battleTurnCounter` is a u8).

Not supported: doubles and multi battles, link, Safari, running/catching/fleeing by the player, Focus Sash (the
item is dropped in the game), learning a move over four known moves (declined), chains of battles other than by
carrying `Mon` state (hp, TP, EXP) between calls.
