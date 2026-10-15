# Research and Sources

Reference material from Omar's Notion workspace: the WEB RESOURCES link list, the Prof. Nishinomori lore quotes, his notes from a playthrough of the original Pocket Monsters Green, and the MATH MODELS balance-simulator code.

Source:
- [WEB RESOURCES](https://app.notion.com/p/31b3d985050a81029537fc1ac50cd273), last edited 2026-03-10
- [PROF. Nishinomori Quotes](https://app.notion.com/p/31b3d985050a81eba812e5e6b07f4db7), last edited 2026-03-06
- [Documentazione run PKMN Verde](https://app.notion.com/p/31b3d985050a81999825c0884aa358a6), last edited 2026-03-06, with its inline database [PKMN_Green_LP_Thresholds](https://app.notion.com/p/31b3d985050a81e0ab30cb51f2cb0c9c)
- [MATH MODELS](https://app.notion.com/p/31b3d985050a818a941ee686fe73fe29), last edited 2026-03-06

All read 2026-10-15.

---

## 1. Web resources

The page is a list of Notion bookmark blocks, most followed by a one-line caption. **The Notion API returns only each bookmark's title, not its URL**, so the links below are titles. Open the Notion page to follow them. Captions are Omar's (mostly English already); groups follow the page order.

| # | Bookmark title | Purpose (Omar's caption, condensed) |
|---|---|---|
| 1 | Pocket Monsters Carddass Trading Cards | Carddass card designs from 1996/97, with official Sugimori artwork of Pokémon using (signature?) moves. |
| 2 | Hi-res Pokémon Art | Pokémon "Poker Cards" トラカルタ (1996, published in CoroCoro): Pokémon in everyday situations, good for characterisation and interaction with the real world. |
| 3 | Moves not in the Pokémon games/In the TCG/Original Series | Kanto-era TCG cards: moves never implemented in the games; Red & Green's design phase was contemporaneous with the first sets. |
| 4 | RG Proto Move Analysis, Part 1: Mirror Move | Team Helix Chamber's analysis of Red & Green beta moves (shared caption with #5): insights into what early moves could have been. |
| 5 | RG Proto Move Analysis, Part 2: Reversal | (as #4) |
| 6 | What Dreams May Come: Prototype Data for Pokémon Red and Green | Team Helix Chamber's general analysis of the prototype; "great reference". |
| 7 | Pokémon world in relation to the real world | How Pokémon towns map to real Japanese cities; "really useful for the scenario creation". The city framework names this page as its primary reference (see `region-and-maps.md`). |
| 8 | List of moves | Index of Pokémon moves, for planning the movepool. |
| 9 | ポケットモンスター赤緑ベータ版　没トレーナー考察　まとめ（未完成） ("Pocket Monsters Red/Green beta: study of cut trainers, summary (unfinished)") | Shared caption for #9-13: Japanese blogs about the Red & Green prototype, with lots of information and theories. |
| 10 | 伝説のスターブロブ２のポケモン攻略 ("Legendary Star Blob 2's Pokémon guide") | (as #9) |
| 11 | Garbled title (mojibake, apparently Shift-JIS decoded wrongly) | (as #9). Probably another Legendary Star Blob 2 page (inferred). |
| 12 | 伝説のスターブロブ２のポケモン攻略 (same title as #10, a different bookmark) | (as #9) |
| 13 | ポケモンらしさ-3_デザイン史〜初代開発史編 ("What makes Pokémon Pokémon, 3: design history, first-generation development") | (as #9) |
| 14 | ポケモンWiki | Japanese Pokémon wiki. |
| 15 | Development:Pokémon Red and Blue | "Beta resources wiki that contains all the relevant data". Probably The Cutting Room Floor (inferred from the title format). |
| 16 | (untitled bookmark, no caption) | Added in the 2026-03-10 edit (inferred from the page's edit date). Its target is unknown. |
| 17 | 裏設定の宝庫『ポケットモンスター図鑑』 ("A treasure trove of hidden lore: the *Pocket Monster Encyclopedia*") | Blogspot article about the 1996 Pokémon Encyclopedia book. |
| 18 | Pokemon Illustrated Book Of Pocket Monsters ... ポケットモンスター図鑑 (Internet Archive) | Raw scan of the 1996 Pocket Monster Encyclopedia. A PDF copy is in `E:\...\PKMN_Minamoto\` (see `README.md`). |
| 19 | Embed: https://pkmncards.com/set/base-set/ | "Site for Pokémon cards with advanced filter research". This is the only real URL the API returned. |

Three uncaptioned images close the page (all named `Untitled.png`); their content is not described in text.

`principles-and-workflow.md` lists further research sources (Helix Chamber, TCRF, Bulbapedia, pret/pokered, hakuda2 / Legendary Star Blob 2, Glitch City Laboratories).

## 2. Prof. Nishinomori quotes

The quotes are English in the source. They are copied verbatim, including the source's typos ("freanch", "researche", "hot", "breaktrhough", "lyfecycle", "ad"). Each is attributed "~ PokeDex, 1997."

> It is said that Pocket Monsters (Colloquially shortened to Pokemon) first appeared some 2 million years ago. The first systematic study of Pokemon was undertaken in the late 18th century by a freanch author, Baron Tajirin, at the time, only 30 species had been discovered.
> Nevertheless the detailed data and sketches left by Baron Tajirin became the foundations for study thereafter. From France, researche into Pokemon spread throughout western europe to include England, Germany, Spain and Italy.
> ~ PokeDex, 1997.

> In line with the incredible progress of transportation Technology, the academic movement found a home in Japan at the end of 19th century. Professor Nishinomori is said to be the father of Pokemon studies in our country. In 1899, he published the paper 'An observation on the Evolution of Pikachu.', it described hot two Pokemon - which had been thought to be 2 entirely different creatures - were actually the same. One was simply the evolved form. As a result of this breaktrhough, Pokemon research entered a new phase of discovery, earning Prof. Nishinomori worldwide recognition.
> ~ PokeDex, 1997.

Omar's note (verbatim): "Professor Nishinomori comes from a comination of Koji Nishino (Snorlax) and Ken Sugimori, The english translation would be Prof. Westwood."

> Because of his paper, Japan came to be known as a leader in the study of Pokemon. Incidentally, 80 Pokemon species had been discovered by then. As of 1996, thanks to the investigative and research efforts of Prof. Oak, 150 species of Pokemon have been discovered.
> We are gradually gaining insight into their behaviour, lyfecycle, evolution, nature and so on. This guidebook examines the presently discovered 150 species of Pokemon by their habitats such ad Grasslands, Mountains, and near bodies of water.
> ~ PokeDex, 1997.

> In general, Pokémon evolve through growth. However, recent studies have concluded that Machamp and Golem have not evolved from growing.
> ~ PokeDex, 1997.

**Design hooks (inferred):**

- The habitat framing (Grasslands, Mountains, water) matches the PKMN INDEX Habitat taxonomy (`roster-and-encounters.md`).
- The Machamp/Golem line supports the Story doc's framing of trade evolutions as Silph experiments (`story-and-setting.md`).
- Baron Tajirin (Tajiri) and Prof. Nishinomori are candidate lore names for NPC text, magazines or the Index.

## 3. "Documentazione run PKMN Verde" (notes from a playthrough of Pocket Monsters Green)

Omar replayed the original Japanese **Pocket Monsters Midori (Green)**, SGB-enhanced, and recorded observations as reference for pacing and restored details. Translated from Italian.

**Screenshots** (image captions translated; images not copyable):

- `Pocket_Monsters_-_Midori_(Japan)_(SGB_Enhanced)-0.png`: Prof. Oak shows **Nidorino** as the example Pokémon, full-screen.
- `...-1.png`: on Mom's table there is a **teapot with a teacup**.
- `...-2.png`: the Pokémon Center has a **bell to call Joy** for assistance.
- `...-3.png`: the Market has **SALE signs** highlighting discounted goods. (Ties to the brainstorm "items on sale" idea.)
- `...-4.png`: the **trainer card shows gym leader mugshots**.

**Notes:**

- Route 1 Pokémon are Lv. 2-3.
- Bulbasaur learns **Leech Seed** (Parassiseme) earlier.
- In the patch of grass next to Viridian: Nidoran M/F, and Spearow (very rare), Lv. 2-4.
- First fight with Blue: Pidgey Lv. 9 and Charmander Lv. 8? (the question mark is in the source)
- Viridian (Forest, inferred) has Pikachu as a rare Pokémon, and Kakuna/Metapod Lv. 3-5.
- Brock has Geodude Lv. 11 and Onix Lv. 14.
- Other trainers use Pokémon like Brock's Onix, but you are always made to feel their Onix (Lv. 10) are weaker than the gym leader's (Lv. 14).
- Pokémon are introduced through trainers before they are available, to feed curiosity. E.g. Sandshrew shown by a grunt in Mt. Moon, then encounterable on Route 4.
- The dojo masters who teach Mega Kick and Mega Punch are absent.
- Misty: Starmie Lv. 21 and Staryu Lv. 18. Omar's own Pokémon were Lv. 18-21 on arrival.

**PKMN_Green_LP_Thresholds** (inline database, all 9 rows; "LP" presumably means level progression, inferred). Columns: Levels, Pokémon, Battles, Trainers. Empty cells are empty in the source.

| Name | Levels | Pokémon | Battles | Trainers |
|---|---|---|---|---|
| Route 1 | 3 | Pidgey, Rattata | | |
| Route 2 | 5 | Pidgey, Rattata, Caterpie | | |
| Viridian Forest | 6 | Caterpie, Weedle, Metapod, Kakuna, Pikachu | 9 | 3 |
| Route 12-1 | 4 | Rattata, Nidoran M/F, Spearow | 9 | 1 |
| Pewter City | | | 14 | 2 |
| Route 3 | 8 | Pidgey, Spearow, Jigglypuff | 14 | 9 |
| Mt. Moon | 10 | Paras, Zubat, Geodude, Clefairy | 16 | 12 |
| Route 4 | 12 | Rattata, Spearow, Sandshrew | | |
| Cerulean City | | | 21 | 3 |

Column meaning is not documented. Levels is probably the player's party level on reaching the area; Battles is probably the cumulative battle count; Trainers is probably the trainers fought there (all inferred). "Route 12-1" sits between Viridian Forest and Pewter in the data; it probably means the Viridian-side grass patch from the notes above, not vanilla Route 12 (inferred). These are pacing targets for the early game (Masara to Hanada).

## 4. MATH MODELS (balance simulator code)

The page contains four Python code blocks, the Notion snapshot of the simulators whose files live in `E:\...\PKMN_Minamoto\MathModels\` (see `art-direction.md`). Summary:

1. **PokeAPI scraper.** It fetches `farfetchd`, `spearow`, `nidoran-m` and `nidoran-f` from `https://pokeapi.co/api/v2/pokemon/`. For each it records name, HP/attack/defense/special/speed, types, and every move (name, type, power, accuracy, `level_min`), and looks up the evolution chain. It writes `pokemon.yaml`.
   - Bug (inferred): it reads stats by index (`stats[3]` as "special", `stats[4]` as "speed"). PokeAPI's order is hp, attack, defense, special-attack, special-defense, speed, so "special" gets Sp. Atk and "speed" gets Sp. Def.
   - The four species match Jiro's gym 1 roster (Farfetch'd) and the early routes (inferred).
2. **Move-weight update formula** (verbatim):
   ```python
   weight_factor = 1.0 / (1.0 + 0.1 * iteration)
   new_weight = (weight_factor * random.uniform(0.8, 1.2) * (damage_dealt / 255) * type_effectiveness) + ((1.0 - weight_factor) * prev_weight)
   ```
   This is a decaying-learning-rate average of a move's usefulness across simulated battles.
3. **OOP battle simulator** (`Pokemon` and `Battle` classes plus an `argparse` main):
   - Stats: `((2*base + randint(1,31)/4) * level)/100 + 5`. HP is the same with `+ level + 10`. This is the GDD formula with DV drawn from 1-31.
   - Damage: `(((2*L/5)+2) * (atk/def) * power)/50 * triangular(0.85, 1.0, 0.93) * effectiveness * (1.5 if STAB)`. This matches the GDD damage formula, minus the Badges and Critical terms. The roll is triangular rather than uniform.
   - Hit if `accuracy >= randint(1,100)`. The player picks moves by learned weights (`move['power'] + move['accuracy']) / 200 * effectiveness`, with weights multiplied on hit/miss); the enemy picks at random.
   - The main loads `data/type_effectiveness.yaml`, `pokemon.yaml`, `encounters.yaml` and `bosses.yaml`. It runs N battles of a random Lv. 3-6 mon against an encounter from the first area, adds `randint(30,120)*enemy_level/7` exp per win, then fights `bosses[0]`. It writes `simulation_results.yaml` (player mon, level, move weights, boss result).
   - Incomplete (inferred): there is no TP, no INT and no speed ties; `level_up` and `load_yaml` are called but not defined in the snippet; `pokemon_data` is a global.
4. **Older weight-only simulation** (in a "plain text" block). It runs 100 random pairings from `pokemon.yaml`, updates move weights with the formula in (2) using `move['power']` as damage, and declares the winner by summed weights. It is a toy model, not a damage simulation.

**Relevance (inferred):** the TP/INT system is not modelled yet, so these scripts can't balance TP costs. `newMM/` in the local MathModels folder is a newer rewrite that hasn't been read. `bosses.yaml` and `encounters.yaml` may hold boss teams and levels that Notion lacks (open-questions C).

## 5. Legacy GDD vs current GDD

Compared: [🗄️ GAME DESIGN FUNDATIONS (v1 — Legacy)](https://app.notion.com/p/31b3d985050a8106ba03f5a5384f344c) (last edited 2026-03-10) and [📋 GAME DESIGN DOCUMENT](https://app.notion.com/p/31b3d985050a81d6bc43e31f3caf6d35) (last edited 2026-03-18).

- **The prose is identical**: Abstract, Story outline, Core Loop, Input, Engagement techniques, Maps/Events, Stats, all formulas, Moves, Trainers, Charisma, Trainer States, Battle structure, the mermaid diagram and Aesthetics. Nothing in the legacy page is missing from the current GDD.
- **Embedded databases are separate copies.** Each page embeds its own instances of MAPS/EVENTS, PKMN INDEX, NORMALS, ELEMENTALS, SIGNATURE, LEARNABLE, TRAINERS, UI and Plot Chapters, each with a different database URL. The two UI embeds were checked: they share one data source, so the gallery content is identical. The other database pairs were not compared row by row.
- **BOSS BATTLES link**: the legacy page links the legacy BOSS BATTLES copy and the current page links the current copy. They are textually identical (see `boss-battles.md`), but the legacy copy's Jiro/Brock move links are dead.
- **Practical rule**: use the current GDD and its databases; the legacy page adds nothing.
