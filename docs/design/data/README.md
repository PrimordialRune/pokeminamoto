# Design data (Notion export)

CSV snapshots of the Notion databases under *Second Brain / Projects / POCKET MONSTERS: Minamoto*. Exported read-only on 2026-10-15. Notion is still the source of truth; re-export instead of editing these by hand if they drift.

All files are UTF-8 with a header row. Each row carries `notion_url`, the link to its Notion page. In relation columns, linked rows appear by title (the Notion title column, e.g. `FUSHIGIDANE`, not the localized name), joined with `; `. Multi-selects are joined with `; ` too, and checkboxes are `yes`/`no`. Species and trainer names use the Japanese-style romanised names as the title; the English name is in `localized_name`.

| File | Notion source | Rows | Columns | Empty or stubbed |
|------|---------------|-----:|---------|------------------|
| `species.csv` | Index, `collection://31b3d985-050a-8114-9b2a-000b39173741` (on the [DATABASE](https://app.notion.com/p/31b3d985050a81369cbdc50c64d2a9ac) page) | 180 | index, pocket_monster, localized_name, type, habitat, release, wild_nature, evolves_into, signature_move, encounters | The dex is a regional order (1 = FLAMIE/Growlithe), not national. 17 rows are placeholders in brackets (`[TORA1]`, `[CACTUS]`, `[DRAKE1]` and others), and 24 have no localized name. `evolves_into` is empty on every row. One row (`[ELEPHANT]`) has no signature move. `wild_nature` is `yes` on 8 rows. `encounters` is filled on 69 rows. Release: 30 ALPHA, 10 ALPHA v2, 140 BETA. An unnamed formula column could not be read and is left out. |
| `overworld.csv` | Overworld, `collection://31b3d985-050a-8129-a11c-000be644d119` | 42 | order_id, name, localized_name, release, encounters, water_encounters, gifts_trades_events, n_encounters, n_water_encounters | Kanto locations in story order (`order_id` includes 5.5 and 11.5). 10 locations have no encounters: Rock Tunnel and every town or city from Pewter onward, Indigo Plateau included. Pallet Town and Viridian City do have encounters. `n_encounters` and `n_water_encounters` are Notion rollups, recomputed here as counts of the relation columns. The page bodies are empty (one sample was checked). |
| `trainers.csv` | Trainers, `collection://31b3d985-050a-81f5-836a-000b47ff8a80` | 47 | name, localized_name, release | Trainer classes and named characters only: no parties, levels or locations. `release` is set on only 6 rows (all ALPHA). Page bodies are empty. |
| `moves.csv` | Moves, `collection://31b3d985-050a-8110-a96b-000b172b8134` | 262 | name, type, category, subcategory, rank, power, accuracy, cost, effects, signature_monster | Category counts: 24 NORMAL (8 body-part subcategories × ranks ★/★★/★★★), 39 ELEMENTAL (13 types × 3 ranks), 150 SIGNATURE, 48 MOVE MACHINE. One row (STRUGGLE) has no category. `power` is filled on 179 rows, `accuracy` on 247 and `cost` (TP) on 1 row only (POUND = 5). 55 signature moves have no power (mostly status moves). Signature moves are all rank ★, apart from Hitmonlee's and Hitmonchan's ★★/★★★ lines and HEAT TACKLE, which has no rank. The page bodies are empty (samples were checked). |
| `learnsets.csv` | Learnsets, `collection://31b3d985-050a-81e4-8644-000bcada36f2` | 50 | pocket_monster, species_index, level, move, move_category, move_power, move_rank | Covers only 4 species: FUSHIGIDANE 15, FUSHIGISOU 15, FLAMIE 12, HITOKAGE 8. `level` is text and is filled on 24 rows. It can be a range (`20-23`, `50-75`), and `00` appears to mean "known at start". FUSHIGISOU and FLAMIE rows are almost all blank. The species_index and move_* columns are Notion rollups, recomputed here from species.csv and moves.csv. The `ID` formula column could not be read and is left out. |
| `ui.csv` | User Intrface, `collection://31b3d985-050a-8171-8025-000b458b2102` (gallery on the GDD page *Estetica*, [UI](https://app.notion.com/p/31b3d985050a81b081d2dd8b144bd3b5)) | 1 | name, tags, version, created, notes | Has one row, "Battle UI" v0.1. Its page is only two GIF mockups (`UItest2.gif` current, `BattleUI_v1_big.gif` older); the older one's caption says the TP display is still missing. There is no written spec. The GIFs were not downloaded. |
| `tasks.csv` | PKMN_TASKS, `collection://31b3d985-050a-8181-a7de-000b68938996` | 6 | name, status, date_start, date_end, assign | 2022–2023 cleanup tasks: Debug Environment and Clean up Trainer Hill are Done; Dex and Maps are In progress; Moves and Battle Frontier are Not started. All are assigned to the project owner. The page bodies are empty. |
| `plot-chapters.csv` | Plot Chapters, `collection://31b3d985-050a-81d9-905f-000b84f626e9` (gallery on the GDD page) | 1 | name, tags | Has one untitled, empty row, so it is a stub. |

Not exported:

- **DEVELOPMENT ISSUES** ([link](https://app.notion.com/p/31b3d985050a812eab61d2b76eaf02b1)): Notion returns the database but no data source or rows, so it is empty or unreadable through the API. No CSV was written.

## Page bodies

- **Species pages** (3 were sampled: TRANSEL, RATTA, GOLDY): each page has a sprite image, a "Description" callout with dex-style flavour text (for example "Caterpie's evolution. While they are encased in a hard shell…") and an inline per-species "Learnset" view. These descriptions are not in `species.csv`; the other 177 pages were not fetched.
- The Moves, Trainers, Overworld, Tasks and Learnsets rows that were sampled have no body content.

## Relations

- `species.encounters` ↔ `overworld.encounters` and `species.signature_move` ↔ `moves.signature_monster` are the two sides of the same Notion relations. They were cross-checked and match exactly.
- `overworld.water_encounters` and `overworld.gifts_trades_events` link to species but have no reverse column in `species.csv`.
