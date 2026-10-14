# Pocket Monsters: Minamoto - Design Docs (index)

English working summaries of the owner's design documents (mostly written in Italian), structured for use while building the ROM hack on pokeemerald. These are **summaries**: the originals are the source of truth. Anything marked "(inferred)" is not stated in a source document.

## Files

- `story-and-setting.md` - premise, tone, characters, factions, the Silph research program, act-by-act plot, sequel hooks.
- `region-and-maps.md` - Kanto as reimagined: map/route list from Notion, city design framework, building types (INN, PMC, CVS, Market), NPC/event rules, Charisma calibration.
- `roster-and-encounters.md` - the Pocket Monster index (Notion), habitat/characteristic taxonomy, Liminal species, gym "Arena Ecology" designs, trainers, boss battles.
- `mechanics.md` - every mechanic change vs vanilla Emerald, with formulas verbatim and an "Engine impact" line per mechanic.
- `art-direction.md` - aesthetic, UI notes, local art/asset folders.
- `open-questions.md` - gaps, contradictions, things only Omar can answer, things that live elsewhere and should be exported.

## Source documents and where the originals live

| Source | Original location |
|---|---|
| GDD v1 (game design doc: stats, formulas, TP, moves, trainer states, Charisma, battle flow) | claude.ai project knowledge file "GDD v1.md"; Notion workspace (page "GAME DESIGN FUNDATIONS (v1 - Legacy)") |
| Minamoto_Story_v1.1 | claude.ai project knowledge; `C:\Users\Prim\Downloads\Minamoto_Story_v1.1.docx` |
| arena_ecology_v2_1 (gym/arena designs, move taxonomy, habitats) | claude.ai project knowledge |
| city_design_framework_v2.1 | claude.ai project knowledge |
| Project description (English pillar summary) | claude.ai project description |
| Notion databases (PKMN INDEX, MAPS/EVENTS, Moves [NORMALS/ELEMENTALS/SIGNATURE/LEARNABLE], TRAINERS, BOSS BATTLES, Plot Chapters, UI) | Notion: Second Brain / Projects / POCKET MONSTERS: Minamoto / GAME DESIGN FUNDATIONS (v1 - Legacy). Read-only access worked when these docs were written. |
| Art, tilesets, UI mockups, logo, trainer sprites | `E:\Omar\Professional\GameDesign\Games\Pokemon\PKMN_Minamoto\MyRes\` |
| Python balance simulators and YAML data | `E:\Omar\Professional\GameDesign\Games\Pokemon\PKMN_Minamoto\MathModels\` |

The same `E:\...\PKMN_Minamoto\` folder also holds `PRET_Disasm/`, `Red Green Blue Prototype Assets/`, `Team-Aquas-Asset-Repo/`, a Pokemon Illustrated Book PDF, and `Pocket Monsters - Midori.gb` (reference material; not summarised here).

**The originals are the source of truth.** If these summaries disagree with a source, the source wins; please flag it in `open-questions.md`.

## Conventions

- Names are kept as the documents spell them (Satoshi, Masara Town, Dr. Okido, Katsura, Jiro...).
- Notion "Pocket Monster" names (e.g. TRANSEL, RATTA) are the Minamoto/Japanese-style names; "Localized Name" is the familiar name (Metapod, Raticate).
- Several documents contradict each other (e.g. which leader runs which gym); see `open-questions.md`.
