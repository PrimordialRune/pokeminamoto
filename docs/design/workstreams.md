# Design Workstreams: Where Each One Stands

Status of each design thread as last recorded, what it is waiting on, and where its documents live. Update this file when a workstream moves.

Source: imported memory from the original claude.ai project (last updated before 2026-10-11), the four files in `sources/`, and the Notion workspace (read 2026-10-15).

| Workstream | Latest document | State | Waiting on |
|---|---|---|---|
| Game design (stats, TP, ranks, signatures, trainer states, battle flow) | `sources/gdd-v1.md`; Notion "GAME DESIGN DOCUMENT" (edited 2026-03-18) | Specified on paper; nothing implemented in the ROM | Decisions in `open-questions.md` §A (move rank slots, TP numbers, stats storage) |
| Story | `sources/story-v1.1.txt` (Minamoto_Story_v1.1) | Acts and characters written | Gym/leader reconciliation with the arena doc |
| Arena Ecology (gym battles, move taxonomy) | `sources/arena-ecology-v2.1.txt`; Excel "Move Tag Index" workbook (not in repo) | 7 gyms designed; tag taxonomy at v1.1 review | The 8 open tag issues below; remaining bosses |
| City design | `sources/city-design-framework-v2.1.md` | Framework v2.1 settled. Tokiwa City v5.1 approved; Nibi City v3.0 done | Tokiwa and Nibi docs are **not in the repo or project files** (ask Omar) |
| Data (species, moves, trainers, maps) | `data/*.csv` (exported from Notion 2026-10-15) | Rows exist; many numeric fields empty | Move costs, learnsets, trainer rosters |
| UI | `ui.md` | See that file | Asset export from `MyRes/` |

## Arena Ecology: tag taxonomy v1.1

The move taxonomy behind arena rules uses five orthogonal axes:

- **VECTOR**: CONTACT, PROJECTILE, BEAM, WAVE, POWDER
- **THERMAL**: HOT, COLD
- **PHASE**: LIQUID, GASEOUS
- **EFFECT**: CONDUCTIVE, TOXIC, DRAIN, MENTAL, SPECTRAL
- **ENVIRONMENT**: WIND, IMPACT, AQUATIC, SUBTERRANEAN

Added in v1.1: ACOUSTIC/SONIC, RESTRAIN, EXPLOSIVE. A universal **Pseudo-Focus Sash** rule applies to every Gym Leader Pokémon (survives a one-hit KO at 1 HP).

Note: `sources/arena-ecology-v2.1.txt` still lists the older flat 22-category list (WIND, GROUND-IMPACT, ... SUPPORT, including SUPPRESSION). The five-axis v1.1 taxonomy lives in the Excel workbook, which supersedes that list (inferred from dates and from SUPPRESSION being dropped).

### Eight open issues before the taxonomy is stable

1. MENTAL is overloaded (Hurricane is wrongly tagged MENTAL).
2. HAZE needs a RESET tag so it doesn't misfire Erika's SPIRIT PRESSURE rule.
3. EXPLOSIVE has no declared axis.
4. Rock Burst is tagged PROJECTILE + EXPLOSIVE, which contradicts itself.
5. Bye-Bye Punch wrongly carries EXPLOSIVE.
6. EXPLOSION + HOT together need an explicit precedence rule for COMBUSTION.
7. DIG / TUNNELING are missing CONTACT vector tags.
8. The workbook's Compact View is out of sync with the Index sheet.

Next after that: design the remaining boss fights (Giovanni, Silph Chief, Agatha, Sabrina, Lance, Bruno, Lorelei).

## City design

- **Tokiwa City** v5.1, approved.
- **Nibi City** v3.0: built on Maebashi research; the museum is the main worldbuilding hub; Silph is the implied financier of the reconstruction; Hida Makoto's Graveler request is the first researcher quest and quietly implicates the player in Silph's acquisition network.
- Next: cross-check Nibi v3 against v2 for anything worth keeping; continue the remaining cities with the v2.1 framework.
- Open narrative question: do the scientist NPCs running trade-evolution quests know what Silph does with the specimens?

## Implementation (ROM)

See `CLAUDE.md` "Where things stand": the Gen-1 data pass is done on `develop`, but the game is still Emerald's Hoenn map, story and battle engine. No Minamoto mechanic is in the code yet.
