# Region and Maps

Sources: Notion MAPS/EVENTS ("Overworld" data source), city_design_framework_v2.1, GDD v1 "Mappe ed Eventi", Story v1.1.

## The region

Kanto, reimagined as a post-war setting (the Pokemon War ended 10-20 years earlier). Each city is "a slice of 1980s-90s Japan" first and a game location second. Cities should feel like they existed before the player arrived and will continue after.

## Map list (Notion MAPS/EVENTS, ordered by Order ID, with release tag)

Total 41 entries. Names are as in Notion (towns in capitals use the Minamoto/Japanese names; routes use vanilla numbering). Order ID is the intended progression order; gaps in numbering (13, 18, 19) are in the database as-is.

| Order | Map | Release |
|---|---|---|
| 1 | MASARA TOWN | ALPHA |
| 2 | Route 1 | ALPHA |
| 3 | TOKIWA CITY | ALPHA |
| 4 | Route 22 | ALPHA |
| 5 | Route 2 | ALPHA |
| 5.5 | TOKIWA FOREST | ALPHA v2 |
| 6 | NIBI CITY | ALPHA v2 |
| 7 | Route 3 | ALPHA v2 |
| 8 | Mt. Moon | ALPHA v2 |
| 9 | Route 4 | ALPHA v2 |
| 10 | HANADA CITY | ALPHA v2 |
| 11 | Route 24 | ALPHA v2 |
| 11.5 | Route 25 | ALPHA v2 |
| 12 | Route 5 | ALPHA v2 |
| 14 | Route 6 | ALPHA v2 |
| 15 | KUCHIBA CITY | BETA |
| 16 | Digda Cave | BETA |
| 17 | Route 11 | BETA |
| 20 | Route 9 | BETA |
| 21 | Rock Tunnel | BETA |
| 22 | Route 10 | BETA |
| 23 | SHION TOWN | BETA |
| 24 | Route 12 | BETA |
| 25 | Route 8 | BETA |
| 26 | Route 7 | BETA |
| 27 | TAMAMUSHI CITY | BETA |
| 28 | YAMABUKI CITY | BETA |
| 29 | Route 13 | BETA |
| 30 | Route 14 | BETA |
| 31 | Route 15 | BETA |
| 32 | Route 16 | BETA |
| 33 | Cycling Road | BETA |
| 34 | Route 18 | BETA |
| 35 | SEKICHIKU CITY | BETA |
| 36 | Route 19 | BETA |
| 37 | Twin Islands | BETA |
| 38 | Route 20 | BETA |
| 39 | GUREN TOWN | BETA |
| 40 | Route 21 | BETA |
| 41 | Route 23 | BETA |
| 42 | VICTORY ROAD | BETA v2 |
| 43 | SEKIEI PLATEAU | BETA v2 |

Notion schema (Overworld): Name, Localized Name, Order ID, Release (ALPHA / ALPHA v2 / BETA / BETA v2), Encounters (relation to PKMN INDEX), Water Encounters (relation), Gifts/Trades/Events (relation to PKMN INDEX), rollups N Encounters / N Water Enc. "Localized Name" is empty in the sampled rows. Most map pages sampled have no body content.

Name correspondences between the Minamoto names and vanilla towns (partly from the Story doc, rest inferred and unconfirmed):
- Masara Town = Pallet Town (local art folder has `Pallet_Town_v1-v3.png`).
- (inferred, from the original Japanese town names and route order) Tokiwa = Viridian, Nibi = Pewter, Hanada = Cerulean, Kuchiba = Vermilion, Shion = Lavender, Tamamushi = Celadon, Yamabuki = Saffron, Sekichiku = Fuchsia, Guren = Cinnabar, Sekiei Plateau = Indigo Plateau. The Story and arena docs use English names (Lavender, Vermilion, Celadon, Saffron, Cerulean) for the same places, so the doc set mixes naming conventions (see open-questions).

## City design framework v2.1 - principles

1. **Real-city grounding.** Each city is based on a real Japanese city; primary reference is the Bulbapedia page "Pokemon world in relation to the real world". Research 1980s-90s specifics (architecture, events, economy, landmarks) as a reserve of details; do not force connections that do not fit (Fuchsia/Cerulean may yield thin material, which is fine).
2. **Three identity layers**: (1) what Red/Green already established, (2) what betas/leaks reveal (Spaceworld 1997, 2020 source leak, prototype maps), (3) real-city research. Final identity is a critical synthesis, each choice articulable.
3. **Two audiences**: the *explorer* (reads everything) and the *lazy player* (mashes A). Both must have a valid game. The design goal for the lazy player is "gradual ludic shock": less info, access and support from the world, never arbitrary punishment.
4. **Charisma as the elegant shock mechanism** (see `mechanics.md`). Low Charisma means: NPCs withhold info, Take and Matsu INN rooms locked, subquests/characters inaccessible, possibly worse prices / harder challenges.
5. **Building composition is not a template.** Only near-constant is the INN. Every building must justify itself with specific content. Same-type buildings (INN, Market) vary in "character" (operator, physical condition, a unique detail).
6. **Environmental interactions** in three depths: pure flavour, informative flavour, narrative interaction. Rough ratio for 10 interactables: 7 / 2 / at most 1.
7. **Blocked progression and backtracking**: locked areas need a diegetic reason, a specific unlock trigger (badge, event, item, Charisma, time) and worthwhile content. Backtracking is incentivised, never mandatory.
8. **NPC design**: document *conversation themes*, not final text. Types: colour, informative, narrative, subquest (can overlap). For significant NPCs answer: first visit, narrative progression, Charisma threshold, time of day, repeat visit; state explicitly when there is no variation.
9. **Deliberate omission** (three kinds): ignorance, omission by choice, true misdirection.
10. **Environmental storytelling** used sparingly: infrastructure in varying maintenance, repurposed buildings, Silph material culture (the Silph logo accumulates on surfaces; each city adds at least one new surface), "the detail that doesn't fit" (one per city).
11. **Temporal events**: city lives independently of the player. Time windows: 07-09 morning operational; 09-12 late morning; 13-17 afternoon; 17-20 evening; 20-23 late evening (more honest dialogue); 23+ night. At least one night-only event per city.
12. **Narrative threads**: closed, semi-open, open. Effective thread = hook, withholding with a reason, payoff that retroactively reconfigures. Most important threads implicate the player (design these first).
13. **Casual trainers** reflect the city as a tendency; each has a distinguishing detail; the most unexpected trainer in a city must be genuinely difficult (the PMC certification does not measure everything).
14. **Tone rules**: show don't explain; no NPC has all the pieces; Silph is everywhere without being the protagonist; time is real; the player is a passing traveller; not everything can be deep.

## Recurring buildings

| Building | Function | Presence |
|---|---|---|
| INN | Rest + narrative hub | Near-universal |
| PMC | Challenge + rank progression; Silph circuit structure | Cities on the Silph circuit |
| Pokemon Market | Commerce | Where narratively sensible; not early |
| CVS | Consumables + temporal texture, open 24h | Urban and semi-urban cities |
| Residential / specialised | Worldbuilding | Variable |

Details:
- **INN**: Ume / Take / Matsu rooms (Japanese ranking: plum / bamboo / pine).
  - Ume: default, free with Trainer Card; rest restores HP + Status + TP.
  - Take: requires Charisma >= threshold; as Ume + after X steps a healing item appears in the room.
  - Matsu: unlocked by a major narrative event (automatic); as Take + resting increases friendship with Pokemon; consumables orderable for money.
  - (Document notes room progression and bonuses are to be revised.) Each INN has a named operator with age and history, and an internal notice board for commission subquests. Take/Matsu doors are closed until unlock, with no scene or announcement.
- **PMC**: Silph-circuit structure (clean, institutional, geometrically uniform). Does not necessarily replace anything; may reuse a pool/dojo/theatre/shrine or demolish something. Town's relation to the PMC must be visible (resentment, aspiration, indifference, commercial dependence); at least one residential NPC has a specific opinion. Not every PMC has a reception.
- **Trainer Card**: formal licence to the Silph circuit, issued by PMCs or Silph offices; grants perks including access to Ume rooms in affiliated INNs.
- **CVS**: never sells Pokeballs or Potions (Market domain). Sells food that replaces Gen-3 berries (effects vary by species, like flavour preferences) and **magazines** that act as in-game guides (Researcher, Trainer, Worldbuilding categories; each city introduces at least one unique magazine). Interactable-but-not-purchasable items (cigarettes, alcohol, adult magazines) as ambient texture and quest-item framework. Each CVS needs at least three distinct time moments, at least one at night.
- **Pokemon Market**: prices obtained by talking to the operator, not on a panel; menu sections [Pokemon], [Cure], [Attrezzatura/Equipment]; prices react to narrative events (shortages, Rocket activity). Market notice board for capture/sale/research subquests.
- **Subquest sources**: Market board, INN board, direct NPC dialogue. Informal economy: transactions with NPCs, mostly light, some with negative consequences (lost money or Charisma).

## Events and maps (GDD)

- A **map** is an explorable area (city rich in NPCs, route rich in Pokemon, or a transition area). An **event** is any set of actions started by a trigger: physical (dialogue start, stepping on a tile) and/or logical (set of conditions).
- Detailed per-map event lists live in the Notion MAPS/EVENTS database; they were not populated in the sampled pages (see `open-questions.md`).

## Local map assets

`MyRes/Maps/` holds `Pallet_Town_v1.png`, `v2`, `v3`. `MyRes/Tilesets/` holds `001.pal/png`, `002.pal/png`, `nature.png`, `kek*.png/bin`.
