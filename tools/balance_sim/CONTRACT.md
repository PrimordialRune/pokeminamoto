# minasim contract (engine ⇄ analysis ⇄ GUI)

Everything under `tools/balance_sim/`. The engine, the analysis and the GUI are built by
different people in parallel; this file is the interface between them. Change it only
together with every side that uses it.

## Layout

```
tools/balance_sim/
  host/            C harness + generators that host-compile the real battle sources (engine team)
  minasim/         Python package (run as `python3 -m minasim ...` from tools/balance_sim)
    engine.py      ctypes wrapper around build/host/libminasim.so (engine team)
    runner.py      multiprocess batch runner (engine team)
    gamedata.py    species / moves / items / trainers / maps read from the repo sources (analysis team)
    stages.py      game stages derived from map + trainer + encounter data (analysis team)
    teams.py       plausible player teams per stage and band (analysis team)
    policies.py    player and foe policies: random, greedy, average, skilled (analysis team)
    signals/       one module per signal family (analysis team)
    report.py      writes build/balance/runs/<run_id>/report.json (analysis team)
    gui/           local web GUI, `python3 -m minasim gui` (UI team)
  tests/
```

Outputs go to `build/balance/` (gitignored with the rest of `build/`).

## Engine API (Python)

```python
from minasim.engine import Engine, Mon
eng = Engine()                     # builds/loads build/host/libminasim.so (make balance-lib)
r = eng.battle(
    seed=1,
    player=[Mon("CHARMANDER", 14, moves=["SCRATCH", "EMBER"], item=None, ivs=None, tp_spent=0)],
    foe=[Mon("PIDGEY", 12)],       # or trainer="TRAINER_BROCK" to load the trainers.party team
    player_policy="greedy",        # name from policies.py, or a callable(state) -> action
    foe_policy="rom",              # "rom" = the ROM's own AI scripts
    log=False)
# r = {"winner": 0|1|2(draw/turn cap), "turns": int,
#      "player": [{"species","level","hp","max_hp","tp","tp_max","status"}...], "foe": [...],
#      "log": [{"turn","side","slot","action":"move|switch|item|talk|special",
#               "move","damage","crit","miss","effectiveness","ko"}] }   # only when log=True
```

`Mon(species, level, moves=None, item=None, ivs=None, nature=None, tp_spent=0)`: species and
moves are constant names without the `SPECIES_` / `MOVE_` prefix; moves None = the last four
level-up moves, like a wild mon. `action` objects given to a callable policy:
`{"type":"move","slot":0-3}`, `{"type":"switch","slot":n}`, `{"type":"item","item":"POTION","target":n}`,
`{"type":"talk","kind":"provoke|unnerve"}`, `{"type":"special"}`.

`minasim.runner.run_batch(specs, jobs=os.cpu_count())` runs a list of battle() kwargs in a
process pool and returns results in order.

## report.json (analysis → GUI)

```jsonc
{
  "meta": {"run_id": "2026-10-27T1200-a55a8324", "commit": "a55a8324", "branch": "develop",
           "created": "ISO time", "battles": 6500000, "seconds": 812, "jobs": 8,
           "bands": ["average", "skilled"], "baseline_run": "run_id or null"},
  "stages": [{"id": "s03", "index": 3, "name": "Before Brock (Nibi)", "badges": 0,
              "level_range": [8, 14], "maps": ["MAP_ROUTE2", "..."]}],
  "flags": [{
    "id": "fight:TRAINER_BROCK:average",
    "kind": "species|move|fight|item|tp|level_curve|special|talk|rng|dominance|dead_content",
    "severity": "red|amber|info",
    "score": 0.0,                         // ranking: severity x confidence x exposure
    "stage": "s03", "band": "average|skilled|both",
    "subject": {"species": "ONIX", "move": null, "item": null, "trainer": "TRAINER_BROCK"},
    "title": "Brock is too hard for an average player",
    "detail": "First-try win rate 31% (target 55-75%). Onix's Bind + Rock Tomb...",
    "metric": {"name": "win_rate", "value": 0.31, "ci": [0.29, 0.33], "target": [0.55, 0.75]},
    "change": {"vs": "baseline run_id", "delta": -0.12, "status": "new|worse|better|same|resolved"},
    "suggestion": "Try Onix L12 or drop Rock Tomb (win rate 58% in a what-if run)",
    "repro": {"seed": 1234, "cmd": "python3 -m minasim duel --seed 1234 ..."}
  }],
  "species": [{"species": "PIKACHU", "stage": "s03", "band": "average",
               "rating": 1520, "win_rate": 0.64, "ci": [0.6, 0.68], "z": 2.3, "n": 450,
               "matchups": {"best": [{"species": "GEODUDE", "win_rate": 0.91}],
                            "worst": [{"species": "SANDSHREW", "win_rate": 0.22}]}}],
  "moves":   [{"move": "EMBER", "species": "CHARMANDER", "learn_level": 7, "stage": "s02",
               "tp_cost": 6, "damage_per_tp": 3.1, "usage": 0.71, "delta_win": 0.18,
               "ci": [0.15, 0.21], "verdict": "dominant|strong|ok|weak|dead"}],
  "fights":  [{"trainer": "TRAINER_BROCK", "name": "Brock", "class": "gym|rival|trainer|wild",
               "stage": "s03", "map": "MAP_PEWTER_CITY_GYM",
               "party": [{"species": "GEODUDE", "level": 12}, {"species": "ONIX", "level": 14}],
               "by_band": {"average": {"win_rate": 0.31, "ci": [0.29, 0.33], "hp_cost": 0.8,
                                       "tp_cost": 0.55, "items_used": 1.4, "blackout": 0.69,
                                       "turns": 9.2},
                           "skilled": {"...": "same keys"}},
               "by_starter": {"BULBASAUR": 0.82, "CHARMANDER": 0.22, "SQUIRTLE": 0.79},
               "target": [0.55, 0.75]}],
  "items":   [{"item": "POTION", "stage": "s02", "delta_win": 0.06, "ci": [0.04, 0.08],
               "price": 300, "delta_per_1000": 0.2, "verdict": "ok"}],
  "tp":      [{"stage": "s03", "band": "average", "battles_per_rest": 14.0, "target": [8, 15],
               "tp_left_at_gym": 0.4}],
  "level_curve": [{"stage": "s03", "band": "average", "player_level": 11.5,
                   "foe_level": 12.4, "exp_level": 11.0}]
}
```

Every list may be empty; the GUI shows an empty state. Unknown extra keys are allowed.
`build/balance/runs/index.json` lists runs: `[{"run_id", "created", "commit", "battles", "flags_red", "flags_amber"}]`.

## GUI server endpoints (`python3 -m minasim gui`, stdlib http.server, localhost:8765)

| Endpoint | Returns |
|---|---|
| `GET /` | the single-page app (`minasim/gui/static/`) |
| `GET /api/runs` | `runs/index.json` |
| `GET /api/report/<run_id>` | that run's report.json |
| `GET /api/data` | species (name, Minamoto name, types, base stats, learnset, evolutions), moves, items, trainers, stages |
| `POST /api/duel` | body `{player:[Mon], foe:[Mon] or trainer, n, player_policy, foe_policy}` → `{wins, n, ci, turns, sample_log}` |
| `POST /api/run` | starts a report run in the background → `{run_id}`; `GET /api/run/<run_id>/progress` → `{done, total, phase}` |
| `GET /sprites/pokemon/<species>/<front|back|icon>.png` | transparent PNG converted on the fly from `graphics/pokemon/` (palette index 0 = transparent), cached in `build/balance/sprites/` |
| `GET /sprites/item/<item>.png`, `/sprites/trainer/<pic>.png`, `/sprites/type/<type>.png` | same, from `graphics/items/icons`, `graphics/trainers/front_pics`, `graphics/types` |

Sprites are always read from the repo at request time, so new art shows up without a copy step.
When the engine library is missing, `/api/duel` and `/api/run` answer `{"error": "engine not built", "hint": "make balance-lib"}`
and the GUI still shows saved reports.
