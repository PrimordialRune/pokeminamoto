# Balance model

A Monte Carlo battle simulator for tuning Minamoto's early game. It rebuilds Omar's MATH MODELS
simulator (Notion page "MATH MODELS", summarised in `docs/design/research-and-sources.md` section 4)
on top of the ROM's own data, and adds the Minamoto rules: TP, crit stages, the damage formula without
the flat +2, type badge boosts and the gym leader Focus Sash.

Nothing is copied by hand. `gamedata.py` parses the decomp sources each run, so a change to
`src/data/battle_moves.h` (a TP cost), a learnset or a base stat shows up in the model immediately.

Needs Python 3 and PyYAML (`python3 -m pip install pyyaml`). Runs in a couple of seconds.

## Commands

```bash
cd tools/balance

# One battle, many times. Teams are comma lists of SPECIES:LEVEL[MOVE/MOVE]@ITEM.
python3 sim.py duel CHARMANDER:14 "ISITSUBUTE:11,IWARK:14" --boss --badges 1

# Win rate by player level, to find where a boss "opens up".
python3 sim.py duel SQUIRTLE:L "KAMONEGI:11@LEEK" --boss --levels 8-16

# Damage ranges and hits to KO for each of the attacker's moves.
python3 sim.py ttk STARMIE:21 IVYSAUR:21

# TP economy: regen per turn and how many uses in a row each cost allows.
python3 sim.py tp CHARMANDER SQUIRTLE BULBASAUR PIDGE --moves ember water_gun vine_whip

# Whole early game (Masara to Hanada), N playthroughs. --all lists every trainer.
python3 sim.py run scenarios/early_game.yaml --starter BULBASAUR --n 300
python3 sim.py run scenarios/early_game.yaml --team --grind      # catch Pidgey/Nidoran, grind to boss targets
python3 sim.py run scenarios/early_game.yaml --battles gamma    # MathModels' battles-per-area draw, gamma(2,5)

# Any rule can be overridden on any command (put --set before the command):
python3 sim.py --set flat_plus2=true --set tp_regen_base=2 duel ...
python3 sim.py --set "tp_formula=15 if status else max(10, round5(power**2/80))" run scenarios/early_game.yaml
```

Species take English constants (`SPEAROW`), English names or Minamoto names (`ONISUZUME`, `KAMONEGI`,
`IWARK`), from `docs/design/data/species.csv`. A mon written without moves knows the last four moves
of its level-up learnset, like a wild or trainer mon in Emerald.

## Rules (engine.Rules), defaults = develop + PR #9 (with the +2 restored)

| Rule | Default | Where it lives in the ROM |
|---|---|---|
| `tp_max` | 100 | `TP_MAX` |
| `tp_regen_base`, `tp_int_div` | 5, 8 | `TP_REGEN_BASE`, `TP_REGEN_INT_DIVISOR`; INT = base Sp. Def; `tp_int_div=0` drops INT |
| `cplus`, `cplus_single_stage`, `cplus_wild_rank1` | off, 20/35, true | C+ move ranks: family slots, rank cap by evolution stage (FINDINGS section 9) |
| `tp_persist` | false | TP carries over between battles and refills only on a heal (not in the ROM yet) |
| `tp_formula` | ROM `tpCost` | any Python expression of `power`, `acc`, `status`, `rank`, `pp` (the move's PP); `round5()` available |
| `rank_costs`, `rank_status_costs` | off | cost by move rank, e.g. `5,15,40` and `10,20,30` (rank: Notion stars for family moves, else a power tier) |
| `crit_table` | 16,8,4,2,1 | `sCriticalHitChance[]` (1 in N per stage) |
| `flat_plus2` | true | `CalculateBaseDamage` (PR #9 removed it, Omar restored it) |
| `badge_boost`, `badge_types` | 1.25; Flying, Ground, Water, Electric, Ghost, Poison, Fire | `sBadgeTypes[]` |
| `badge3_speed` | false | Emerald's x1.1 speed for badge 3, removed in PR #9 |
| `gym_sash` | true | arena_ecology rule 0.1, **not in the ROM yet** |
| `stab`, `crit_mult`, `exp_trainer_mult` | 1.5, 2, 1.5 | as Emerald |

Damage follows the engine's integer order: base damage (stat stages, burn, badge boost, +2),
then crit x2, STAB, each defending type, then the 85-100% roll. `run --segments` (automatic with
`tp_persist`) reports each stretch between rests: battles, attacks, when TP first ran short. TP is paid when the move is used
(misses too), not when the mon can't move. Regen happens at end of turn for each mon on the field.

## What it models and what it doesn't

Modelled: stats from base stats and a random IV (no EVs, neutral nature), physical/special by type
(Gen 3), accuracy and evasion stages, stat-stage moves, sleep, poison, burn, paralysis, freeze,
confusion, flinch, Leech Seed, multi-hit, recoil, drain, fixed-damage moves, two-turn moves, Struggle,
Blaze/Torrent/Overgrow/Swarm, Intimidate, Leek on Farfetch'd, EXP (yield x level / 7, split among
participants, x1.5 for trainers), level-up moves, level evolutions, Potions between battles.

Not modelled yet: arena ecology states (AIRFLOW, TERRAIN, TIDE...), items used during battle,
switching by choice, held items other than Leek, abilities other than those above, trainer states,
Talk, Charisma, signature moves, move ranks. Moves whose effect isn't modelled are treated as plain
hits or no-ops; `Battle.stats.unmodelled` lists them.

Move choice (`--foe-ai random|trainer|smart|rank`): the player picks the strongest affordable hit, or the cheapest move that KOs. Wild mons
pick at random among affordable moves. Trainers pick the strongest hit 75% of the time and any
useful move (status moves included) otherwise, a rough stand-in for Emerald's trainer AI.

## Scenario files

`scenarios/early_game.yaml` is the Act One route (Masara to Hanada) from the Notion map table and
Omar's Pocket Monsters Green notes. Every number nobody has designed yet is marked `PLACEHOLDER`
(mostly Gen 1 Green/RB trainer teams and the number of wild battles per area). Steps are `area`
(wild battles), `trainer`, `boss` (with `badge`, `sash`, `target` arrival levels), `catch` and `heal`.
Gym leaders are retried after a loss, as a player would, and the report counts the tries.
