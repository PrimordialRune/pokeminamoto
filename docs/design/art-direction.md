# Art Direction and UI

Sources: GDD v1 "Estetica" / "Interfaccia Utente", Story v1.1 overview, local asset folders. Very little prescriptive art direction is written down; most of it lives in the art files themselves.

## Stated aesthetic (GDD)

- Pixel art with bright, saturated colours to evoke the retro look of the first Pokemon games, while making the world livelier and more colourful.
- Rationale in the GDD: immersion as a key factor for engagement and for learning the mechanics (cognitive-psychology argument).
- Story doc: "deliberately retro. Pixel art and bold colors evoke the first generation", but the narrative register is darker.
- Cities are grounded in 1980s-90s Japan (architecture, shop signage, CVS, INN, PMC); Silph logo appears on many surfaces as environmental storytelling (packaging, forms, TM cases).

## Input mapping (GDD, for UI work)

- D-pad: move / navigate menus. A: interact / confirm. B: cancel / run (hold). START: open menu / start game. SELECT: extra features (region map, choose active Pokemon in battle, cancel in menu). L and R: "additional functions" (R = Talk in battle).

## UI

- GDD links a Notion "UI" gallery of "UIs currently developed". Not read from Notion here (the page was not in the fetched set); the local mockups exist in `MyRes/UI/`.
- New UI elements implied by mechanics: TP bar/readout instead of PP, INT stat on the summary screen, Talk entry in the action menu, a fifth (Signature) move entry, COST (P$) stat, Charisma indicator (if shown), Market with [Pokemon]/[Cure]/[Attrezzatura] sections, magazine items, Trainer Card.

## Local asset folders (not moved or copied)

`E:\Omar\Professional\GameDesign\Games\Pokemon\PKMN_Minamoto\MyRes\` (top two levels):
- `Inspirations/` - 108 reference images/gifs/html
- `Maps/` - `Pallet_Town_v1.png`, `v2`, `v3`
- `Scraps/` - UI tests (`UItest*.ase/png`)
- `Tilesets/` - `001.pal/png`, `002.pal/png`, `nature.png`, `kek.png`, `kek2.png/bin`
- `Trainers/` - 26 trainer images (including several Brock files)
- `UI/` - `BattleUI_v1.gif`, `BattleUI_v1-1/2/3.png`, `UItest2.ase/gif`, `b_frlg_034.png`
- `fonts/` - `font_rend*.png/webp`
- `logo/` - `Minamoto_logo.png`, `logo_minamoto*.png`, `Untitled-1.psd`, `bg/`
- `notion_icons/` - `FIGHT.png`, type icons (`electric`, `fire`, `grass`, `normal`, `psychic`, `water`) incl. `.aseprite`
- Loose files: `016.gif`, `Sprite-0001.aseprite/png`, `Sprite-0001v2.png`, `Sprite-0003.ase`, `Sprite-0003v2.ase`, `Sprite-0003v3.png`, `icons.zip`, `waterecut.png`, a `gude-2023-07-12.log`.

`E:\...\PKMN_Minamoto\MathModels\` (related, not art): `Battle.py`, `Pokemon.py`, `pokemon_simulations.py`, `utils.py`, `simulation_results.yaml`, `data/` (`bosses.yaml`, `encounters.yaml`, `pokemon.yaml`, `pokemon_list.yaml`, `pokemon_old.yaml`, `type_effectiveness.yaml`), `newMM/` (`config.py/yaml`, `data_loader.py`, `main.py`, `pokemon.py`, `data/`, `entities/`, `implementations/`, `repos/`), `old/` (4 older scripts). The project description says Python battle simulators exist for balance testing.

Other related folders at `E:\...\PKMN_Minamoto\`: `PRET_Disasm/`, `Red Green Blue Prototype Assets/`, `Team-Aquas-Asset-Repo/`, `pokeminamoto@` (a repo-like folder, inferred), `Pocket Monsters - Midori.gb`, and a reference PDF (Illustrated Book of Pocket Monsters). Not summarised.
