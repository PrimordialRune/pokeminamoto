# Pocket Monsters: Minamoto - Design Knowledge Base

Start here. This folder is the repo copy of Omar's design work (Notion workspace, claude.ai project files, and notes from earlier sessions), arranged so an agent can find the one page a task needs. Most originals are in Italian; the topic pages are English summaries.

**The originals are the source of truth.** If a summary disagrees with `sources/` or Notion, the source wins; record the mismatch in `open-questions.md`. Anything marked "(inferred)" is not stated in a source.

## Read first

1. `principles-and-workflow.md` - the design rules every proposal is judged against, and how Omar wants to collaborate (harsh critique, evidence-grounded proposals, ask before commit/push).
2. `workstreams.md` - what each design thread's status is, its latest document, and what it waits on.
3. `open-questions.md` - unresolved decisions and contradictions. Check here before assuming an answer.

## Find the page for your task

| If you are working on... | Read | Data |
|---|---|---|
| Battle engine: TP instead of PP, stats, damage, accuracy, move ranks, signature moves, EXP curves | `mechanics.md` (formulas verbatim) | `data/moves.csv` |
| Trainer AI states, Talk action, Charisma | `mechanics.md` §5-7, `brainstorm.md` §1 | `data/trainers.csv` |
| Gym battles, arena rules, move tags | `roster-and-encounters.md`, `workstreams.md` (tag taxonomy v1.1 + 8 open issues), `boss-battles.md` (older Notion version) | `sources/arena-ecology-v2.1.txt` |
| Species, roster, habitats, encounters | `roster-and-encounters.md` | `data/species.csv`, `data/overworld.csv`, `data/learnsets.csv` |
| Maps, cities, buildings, NPCs, events | `region-and-maps.md` | `sources/city-design-framework-v2.1.md`, `data/overworld.csv` |
| Story, characters, factions, Silph | `story-and-setting.md` | `sources/story-v1.1.txt` |
| UI screens (battle UI, menus, input mapping) | `ui.md` (readiness per screen + engine files), `art-direction.md` | `data/ui.csv` |
| Art, palettes, local asset folders | `art-direction.md` | - |
| Loose ideas not yet in the GDD | `brainstorm.md` (each idea rated idea / partly specified / specified) | - |
| Research links, Prof. Nishinomori quotes, Green playthrough notes, balance simulator code | `research-and-sources.md` | - |

Engine work: also read `CLAUDE.md` (build, RAM limits: EWRAM ~95%, IWRAM ~93-94%) and `GEN1_ONLY_CHANGES.md`. Anything that adds per-mon or per-battle state needs a RAM/save plan first (`open-questions.md` §E).

## Folder layout

- `*.md` (this folder) - English topic summaries.
- `sources/` - verbatim copies of the four claude.ai project knowledge files: `gdd-v1.md`, `story-v1.1.txt`, `arena-ecology-v2.1.txt`, `city-design-framework-v2.1.md`. Do not edit these; replace them with a new version file when Omar revises one.
- `data/` - CSV exports of the Notion databases (species, overworld, trainers, moves, learnsets, ui, tasks, plot chapters), exported 2026-10-15. See `data/README.md` for columns and what is empty. Relation columns are resolved to names.

## Where the originals live

| Source | Location |
|---|---|
| Notion root | "POCKET MONSTERS: Minamoto" in Omar's Second Brain / Projects (`https://app.notion.com/p/31b3d985050a80fcbc1ad9fa1bcfd36b`). Contains the current GAME DESIGN DOCUMENT, the DATABASE page (Index, Overworld, Trainers, Moves, Learnsets, UI), PKMN_TASKS, DEVELOPMENT ISSUES, MATH MODELS, BRAINSTORM & IDEAS, PROF. Nishinomori Quotes, Documentazione run PKMN Verde, WEB RESOURCES, and an Archive (GAME DESIGN FUNDATIONS v1 Legacy, deprecated Regex Reference). |
| claude.ai project knowledge | GDD v1, Minamoto_Story_v1.1, arena_ecology_v2_1, city_design_framework_v2.1 (copied into `sources/`) |
| Art, tilesets, UI mockups, logo, trainer sprites | `E:\Omar\Professional\GameDesign\Games\Pokemon\PKMN_Minamoto\MyRes\` |
| Python balance simulators and YAML data | `E:\...\PKMN_Minamoto\MathModels\` |
| Excel Move Tag Index workbook, Tokiwa v5.1, Nibi v3.0 city docs | Not in the repo or project files yet (see `open-questions.md` §D) |

## Refreshing from Notion

Notion is read-only from agents. To refresh `data/`, fetch each database's data source and read its saved views (the SQL query tool has a usage cap on this plan; view mode does not). Keep the same CSV names and columns so diffs stay readable, and update the export date here and in `data/README.md`.

## Conventions

- Names are kept as the documents spell them (Satoshi, Masara Town, Dr. Okido, Katsura, Jiro...).
- Notion "Pocket Monster" names (TRANSEL, RATTA) are the Minamoto/Japanese-style names; "Localized Name" is the familiar one (Metapod, Raticate). Notion Index order is a regional dex, not National Dex order (1 = FLAMIE/Growlithe).
- Several documents contradict each other (e.g. which leader runs which gym); see `open-questions.md` §B.
