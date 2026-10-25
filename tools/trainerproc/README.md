# trainerproc: trainers as text

Every trainer and party lives in `src/data/trainers.party`, written in the
Showdown-style format of pokeemerald-expansion's trainerproc. `make` (through
`make generated`, see `generated_data_rules.mk`) builds the tool from
`tools/trainerproc/main.c` and turns the file into `src/data/trainers.h`,
which `src/data.c` wraps into `gTrainers[TRAINERS_COUNT]`. The output is
gitignored; edit the `.party` file, never `trainers.h`. Errors point at the
`.party` line, both from trainerproc and from the C compiler (`#line`).

```
src/data/trainers.party            the trainers (edit this)
include/constants/opponents.h      the TRAINER_* ids, TRAINERS_COUNT
include/data.h                     struct Trainer, struct TrainerMon
tools/trainerproc/main.c           the tool (C, built with the host cc)
tools/trainerproc/partydump.py     builds every party in the emulator, for before/after checks
```

The tool is expansion's (`tools/trainerproc/main.c` as of expansion commit
44bc3e04), with its output changed for this engine's `struct Trainer` and its
trainer keys cut to the ones this engine has. Every Pokémon field of
expansion's `struct TrainerMon` is kept.

## Format

```
=== TRAINER_ROXANNE_1 ===
Name: ROXANNE
Class: Leader
Pic: Leader Roxanne
Gender: Female
Music: Female
Items: Potion / Potion
Double Battle: No
AI: Check Bad Move / Try To Faint / Check Viability

Geodude @ Oran Berry
Level: 15
IVs: 12 HP / 12 Atk / 12 Def / 12 SpA / 12 SpD / 12 Spe
- Tackle
- Rock Throw
```

A trainer starts with `=== TRAINER_X ===` and runs to the next `===`. A blank
line separates the trainer's keys from the first Pokémon and each Pokémon
from the next. Comments are C comment blocks (the file goes through `cpp`
first); `//` is not a comment. Names are written either as words (`Oran
Berry`, `Check Bad Move`) or as the constant (`ITEM_ORAN_BERRY`): words become
the constant by upper-casing and turning spaces and dashes into `_`.

### Trainer keys

| Key | Required | Becomes |
|---|---|---|
| `Name` | yes | `trainerName` (up to 10 characters; may be empty, as for `TRAINER_NONE`) |
| `Pic` | yes | `trainerPic` = `TRAINER_PIC_<X>` |
| `Class` | | `trainerClass` = `TRAINER_CLASS_<X>` |
| `Gender` | | `Male` / `Female`: the top bit of `encounterMusic_gender` (`F_TRAINER_FEMALE`); a female trainer's Pokémon lean female |
| `Music` | | `TRAINER_ENCOUNTER_MUSIC_<X>`, the low bits of `encounterMusic_gender` |
| `Items` | | up to 4, `/`-separated, `ITEM_<X>` |
| `Double Battle` / `Battle Type` | | `Yes`/`No` or `Singles`/`Doubles`: `doubleBattle`. The design has no double battles; the two Route 103 double trainers still have it set |
| `AI` | | `/`-separated `AI_SCRIPT_<X>` flags (`include/constants/battle_ai.h`) |

Expansion's other trainer keys (`Mugshot`, `Starting Status`, `Difficulty`,
`Party Size` and the party pool keys, `Copy Pool`, `Macro`, `Multi Party`)
are refused: the engine has nothing that reads them.

### Pokémon

The first line is `Species`, optionally with a nickname, a gender and an item:
`Rocky (Geodude) (F) @ Oran Berry`. Then any of these lines, then up to four
`- Move` lines. At most 6 Pokémon per trainer.

| Line | Field | In the game |
|---|---|---|
| `Level: 15` | `lvl` | default 100 |
| `IVs: 12 HP / 12 Atk / ...` | `iv` (`TRAINER_PARTY_IVS`) | default 31 each. Equal IVs go through `CreateMon` exactly as Emerald's single IV did |
| `EVs: 252 HP / 4 Def` | `ev` (`TRAINER_PARTY_EVS`) | default none; missing stats are 0 |
| `- Move` (x1-4) | `moves` | none = the moves it would know by level-up, as in Emerald |
| `@ Item` | `heldItem` | |
| `(M)` / `(F)` | `gender` | picks a personality with that gender (ignored for one-gender species) |
| `Timid Nature` or `Nature: Timid` | `nature` | picks a personality with that nature; without one the nature comes from Emerald's personality formula |
| `Ability: Sturdy` | `ability` | the species' slot holding that ability; another ability is ignored |
| `Ball: Great` | `ball` (`ITEM_GREAT_BALL`) | default Poké Ball |
| `Happiness: 200` | `friendship` | default the species' base friendship (0 means default) |
| `Shiny: Yes` | `isShiny` | |
| `Nickname (Species)` | `nickname` | up to 10 characters |
| `Tera Type`, `Dynamax Level`, `Gigantamax`, `Tags` | `teraType`, `dynamaxLevel`, `gigantamaxFactor`, `shouldUseDynamax`, `tags` | stored only: the engine has no such mechanics. Tags are the `MON_POOL_TAG_*` names in `include/constants/trainers.h` |

`#pragma trainerproc level 50` and `#pragma trainerproc ivs ...` at the top of
the file change the defaults (`explicit` makes the line required). The
personality otherwise follows Emerald: a hash of the trainer's and the
species' names, so the same party always gets the same natures and genders.

## Adding a trainer

1. Add `#define TRAINER_X n` to `include/constants/opponents.h` with the next
   id and raise `TRAINERS_COUNT`. Ids must stay below `MAX_TRAINERS_COUNT`
   (864): each one has a defeated flag in the save.
2. Add its block to `src/data/trainers.party`.
3. Battle it from a map script, e.g. `trainerbattle_single TRAINER_X, Intro_Text, Defeat_Text`.
4. A trainer that comes back stronger gets a row in `gRematchTable`
   (`src/battle_setup.c`) listing its stages.

Deleted Hoenn trainers are defined as `TRAINER_NONE` in
`include/constants/removed_trainers.h` for the dead systems that still name
them. Do not use those names; delete them as the systems go.

## Minamoto trainer data: extension point

The design (`docs/design/mechanics.md` 5 and 7) has Trainer States, the
Talk action and TP, but none of them is per-trainer data yet: Trainer States
are a battle-time state set by Talk or by moves, and how many Talk lines a
trainer has and who writes them is an open question
(`docs/design/open-questions.md` 8). So the format has no keys for them.
When one is defined (say a trainer's Talk lines or a starting state):

1. add the field at the marked end of `struct Trainer` in `include/data.h`
   (ROM only: `gTrainers` is const, so it costs no RAM);
2. parse the key in `parse_trainer` and print it in `fprint_trainers`
   (`tools/trainerproc/main.c`), following `Items` or `AI`;
3. read it where the battle needs it.

## Checking a change

`partydump.py` creates each trainer's party in the emulator and saves the
bytes, so two builds can be compared:

```
python3 tools/trainerproc/partydump.py OLD_TREE /tmp/old
python3 tools/trainerproc/partydump.py NEW_TREE /tmp/new
diff -r -x gdb.log /tmp/old /tmp/new
```

The switch from Emerald's four party structs to this format was checked this
way: all 32 parties came out byte-identical (encrypted data, stats and all),
and every trainer's name, class, pic, music, items, AI flags and party size
matched.
