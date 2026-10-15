# Open Questions, Gaps and Contradictions

Order: biggest blockers for engine work first.

## A. Questions only Omar can answer (design decisions)

1. **Roster size.** Notion PKMN INDEX has 180 rows (ALPHA 30, ALPHA v2 10, BETA 140) including placeholders like `[NYAKO]`, `[TORA1-3]`. The brief says "the 151". Which rows ship? Are TORA/NYAKO beta-only lines kept, and do they need unique base stats and sprites?
2. **Gym count and leaders.** Arena doc defines 7 gyms (Jiro, Brock, Misty, Surge, Erika, Koga, Coleman) and excludes Sabrina "pending character confirmation". Story doc says Katsura holds Cinnabar; Blaine/Giovanni are not gym leaders. Where do Sabrina, Katsura and the 8th gym fit, and what is the badge count and order? Which type does each Badge represent for the Badge x1.25 rule (7 listed; 8th?)?
3. **Move Rank mechanics.** When a move ranks up (TACKLE -> LUNGE), does the old move stay selectable (both usable, as the GDD example says) or is it replaced? Does the player still have 4 slots? This decides whether a save/RAM cost exists.
4. **Signature move slot.** Exempt from the 4-move limit: is it a fifth menu entry always present? One per species or per evolution line (Notion shows relations with multiple monsters)? Can a Pokemon have several?
5. **TP numbers.** GDD: 100 TP at battle start, recovery per turn = 5 + INT/8 (net of cost), cost 5-100. Notion Moves DB Cost column is empty for sampled rows. Who sets costs and when? Does TP persist across battles or reset (GDD says start of battle = 100)? Does Struggle exist and cost TP?
6. **Stats storage.** Keep EVs/IVs or move to Gen-1 DVs (GDD uses DV)? Is Nature kept (formula has NATURE)? Single Special means no Sp.Atk/Sp.Def split; confirm. How is INT assigned for Gen-1 species (is there a table already in MathModels pokemon.yaml)?
7. **Charisma details.** Hidden stat or shown? Exact gain/loss rules (Talk, victories, subquests, informal transactions)? Where are the Take-room threshold values? Is it capped at 100 (calibration table 0-100)?
8. **Talk action.** What does Talk do against wild Pokemon? Does it cost a turn (GDD: it does not end the turn)? How many talk lines per trainer; who writes them? How do Trainer States get activated "via a move" vs dialogue?
9. **Evolution by trade.** Lore explains trade evolutions as Silph experiments; is the trade mechanic kept in play (yes presumably, via the Link Cable?) or replaced by a Silph facility? Not specified.
10. **"Wild Nature" checkbox** in PKMN INDEX and "Release" tiers (ALPHA / ALPHA v2 / BETA): what are the exact meanings and the release plan (does ALPHA = playable up to Pewter)?
11. **Dynamic Market prices and PMC rank progression**: rank tiers, requirements, rewards are mentioned but not defined.
12. **Illusory Pokemon (Pokemon Illusori)**: which ones (Mirage habitat = Articuno, Zapdos, Moltres, Mew, Mewtwo?), and how Charisma gates them.
13. **Time-of-day / RTC**: events are time-window based; is the RTC (Emerald battery) a hard requirement or is a fake clock used?

## B. Contradictions between documents

- **Gym leader naming**: Notion TRAINERS lists Takeshi (Brock), Kasumi (Misty), Shizuka (Erika), Osugi (Koga), Natsume (Sabrina), Yujirou (Jiro), Philip Surge; arena doc uses Jiro/Brock/Misty/Surge/Erika/Koga/Coleman; Story doc uses Katsura (Cinnabar), Lt. Surge. Coleman (Fire gym, arena doc) is not in Notion TRAINERS at all.
- **Fire gym**: Story doc says Katsura holds Cinnabar (the Fire gym in vanilla) and Blaine is not a gym leader; arena doc has Coleman as the Fire/Double Battle gym. Katsura vs Coleman unresolved.
- **Ghost gym**: arena doc gym 5 = Erika, Ghost type (in vanilla Erika is Grass; Sabrina Psychic). Notion BOSS BATTLES describes Erika as a sacred tree / Grass-themed (water every 3 turns, no fire). Notion types are Kodama/Erika as Grass; arena v2.1 says Ghost.
- **Boss battle designs**: Notion BOSS BATTLES (older, simpler) contradict v2.1 for every gym (e.g. Jiro: "Farfetch'd attack ALWAYS crits" vs v2.1 requires +4 stages via FULL AIRFLOW first-mover; Koga: "non-Poison KO after 3 turns" vs DENSITY-based per-turn poison rates; Misty: tide "doubles speed" vs +50% power and ACC changes; Surge: coil per player vs one shared coil). Also Coleman's Notion entry appears to be an English AI-generated draft inconsistent with the Italian ones. Treat v2.1 as newer (inferred); confirm.
- **Sandstorm damage**: Jiro doc says 1/8 per turn (1/16 with humidity); Notion BOSS BATTLES says 1/16; also Brock's Sandstorm 1/8 non-Mountain/non-Ground.
- **Ground/Aerial sandstorm immunity**: arena doc 0.3 reference to "see 0.3" for sandstorm immunity is wrong (0.3 is the crit table); a cross-reference error in v2.1.
- **Power range**: GDD text says Power 0-250; Notion Power slider max 150.
- **Exp curve typo**: FLUCTUATING first line in GDD has unbalanced parentheses and stops at L<98 (no case for L>=98); ERRATIC middle bounds overlap (L=68 appears in two cases). Medium Slow constant "-140" vs standard Gen-3 "-140" ok.
- **Run rules**: GDD says Run applies to wild battles, but the diagram routes trainer-battle Run back to the action menu.
- **Charisma**: project description calls it a "hidden player stat"; GDD describes visible effects; city framework gives numeric calibration (0-100).
- **Ken/Shigeru**: GDD "nipoti" (nephews/grandchildren) of Okido vs Story "grandchildren".
- **Name spelling**: Silph vs "Sylph" (Notion trainer classes SYLPH CHIEF/WORKER); Pokemon Index vs PokeMon Index.
- **Town naming**: Notion uses Japanese names (Tokiwa, Nibi, Hanada, Kuchiba, Shion, Tamamushi, Yamabuki, Sekichiku, Guren) while Story/Arena docs use English (Lavender, Vermilion, Celadon, Saffron, Cerulean, Cinnabar). Pick one scheme for in-game text.

## C. Gaps (not defined anywhere I could read)

- **Plot Chapters** Notion database: contains one empty untitled page; no chapter structure beyond the Story doc's acts.
- **MAPS/EVENTS**: 41 map names exist, but the sampled pages have no body content; no per-map event lists, NPC lists or trigger tables. Localized Name empty.
- **TRAINERS** and **BOSS BATTLES**: trainer pages are stubs (name/class only); no rosters (species/levels) in Notion; the gym trainers' teams and Boss teams are described only qualitatively. (MathModels `data/bosses.yaml`, `encounters.yaml` may hold levels - not read.)
- **Gym 8, Four Heavenly Kings, Champion/League** design: only mentioned. Siba, Kikuko, Kanna, Wataru appear in Notion TRAINERS (Bruno, Agatha, Lorelei, Lance) so the League is probably Elite Four-based; the "Four Heavenly Kings" membership is not stated.
- **Cinnabar / Blaine / Giovanni** battles: no mechanics.
- **Sabrina's arena** (Esper): only an unanswered design question in Notion ("AI as clairvoyance?").
- **Item list** (TM equivalents "Move Machines", TP restoratives, food items, magazines): only described.
- **COST (P$)** formula ("evaluation of all stats") undefined.
- **Natures / abilities**: the GDD formula includes NATURE but abilities are not mentioned; unknown whether Emerald abilities are removed.
- **Weather, held items, physical/special typing**: no spec.
- **Learnable moves**: 48 Move Machine rows in Notion; level-up learnsets and Move Rank tables not read.
- **UI gallery** Notion page not fetched.

## D. Things that live elsewhere and should be exported or pasted into the repo

**Update 2026-10-15:** the Notion databases are now exported to `data/` (species 180, overworld 42, trainers 47, moves 262, learnsets 50 rows for 4 species only, ui 1, tasks 6, plot chapters 1 empty). TP Cost is filled on one move only (POUND = 5); `evolves_into` is empty on every species row; boss team databases on the BOSS BATTLES pages return no rows; DEVELOPMENT ISSUES could not be read through the connector. Still missing from the repo and not in Notion, so Omar needs to upload or paste them:

- Tokiwa City design doc v5.1 and Nibi City design doc v3.0 (and Nibi v2 for the cross-check).
- The Excel Move Tag Index workbook (Move Tag Index, Tag Reference, Compact View sheets), which holds the v1.1 five-axis taxonomy.
- The confirmed 261-move CSV used for research.
- `MathModels/data/*.yaml` (bosses, encounters, pokemon stats) from the PC.
- Species dex descriptions (on each Notion species page body, not exported) if they should live in the repo.

Earlier list (kept for reference):

- Notion: PKMN INDEX (180 rows, with Habitat/Type/Encounters/Evolves Into/Signature Move), Moves (24 Normal, 39 Elemental, 48 Move Machine, 150 Signature), MAPS/EVENTS (41 maps with encounter relations), TRAINERS (45), BOSS BATTLES page, UI gallery, Plot Chapters. Suggest a CSV/markdown export per database, then commit under `docs/design/data/` or convert to C data tables.
- `E:\...\MathModels\data\*.yaml` and `newMM/` simulators: likely contain the current numeric stats and boss teams; document their relationship to Notion (which is source of truth).
- `MyRes/` art (maps, tilesets, UI, logo, trainer sprites): the repo should say where the final ROM-ready assets will be.
- Old chats/claude.ai project threads: any decisions made there (e.g. v2.1 arena choices, Sabrina exclusion) not captured in docs.
- The `Red Green Blue Prototype Assets/` and `PRET_Disasm/` folders: referenced as evidence base for "restored content"; summarise if intended as design input.
- `C:\Users\Prim\Downloads\Minamoto_Story_v1.1.docx`: confirm this is the latest version; it is the only copy mentioned outside claude.ai.

## E. Engine-risk flags to raise early (inferred)

- TP, INT, Talk and arena state add battle-time variables. EWRAM ~95% and IWRAM ~93-94% full: audit before adding fields to `gBattleMons`, `gBattleStruct` or `gBattleResources`.
- Changing `struct BoxPokemon` (PP bytes -> TP/rank, SpAtk/SpDef merge, INT) is a save-format change; decide on a version bump/clean-save policy.
- A fifth (Signature) move per Pokemon is a UI and AI change as well as a data change.
- 22-category per-move ecology tags and 150 signature moves are ROM-size heavy but RAM-free.
