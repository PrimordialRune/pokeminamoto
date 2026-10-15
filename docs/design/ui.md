# UI Design

Every UI screen and mockup in Omar's Notion, plus the UI rules scattered across the GDD and the brainstorm page, each marked for how ready it is to build on the pokeemerald engine.

Source: Notion UI gallery database "🈵 User Intrface" ([current GDD copy](https://app.notion.com/p/31b3d985050a81b081d2dd8b144bd3b5); the legacy GDD embeds a second view, [31b3d985050a8176bde5d0d3596432c9](https://app.notion.com/p/31b3d985050a8176bde5d0d3596432c9), of the same data source). Single entry [Battle UI](https://app.notion.com/p/31b3d985050a812b8d3df453a579947e), created/last edited 2026-03-06. Also: [GAME DESIGN DOCUMENT](https://app.notion.com/p/31b3d985050a81d6bc43e31f3caf6d35) "Input e Funzionalità" and "Battaglia" (last edited 2026-03-18), and [BRAINSTORM & IDEAS](https://app.notion.com/p/31b3d985050a8139901ce8e12d981039) (2026-03-06). Read 2026-10-15.

**Readiness labels.**

- **Ready**: the layout or rule is fully shown or stated. It can be built now with existing engine data or placeholder values.
- **Needs mechanic**: the UI is clear, but it displays data that doesn't exist in the engine yet (TP, Talk, Charisma).
- **Underspecified**: the intent is known but not the details.

The engine notes (file names, functions) are **(inferred)** from the pokeemerald source in this repo. They are not in Omar's documents.

---

## 1. The UI gallery (Notion)

The gallery holds **one entry**. Its schema has Name, Tags (only option: BATTLE), Version and Created; the view is "UI GALLERY", a gallery of cards.

| Name | Tags | Version | Content |
|---|---|---|---|
| Battle UI | BATTLE | v0.1 | Main image `UItest2.gif` (5 frames). Inside a toggle "OLD VERSIONS": `BattleUI_v1_big.gif` (6 frames), with caption: *"UI di Prova, Da aggiungere i TP che al momento risultano mancanti."* ("Test UI; TP still to be added, currently missing.") |

No written spec is attached; the GIFs are the spec. The images can't be committed from Notion. Local copies live in `E:\...\PKMN_Minamoto\MyRes\UI\` (`UItest2.ase/gif`, `BattleUI_v1.gif`, `BattleUI_v1-1/2/3.png`) and `MyRes/Scraps/` (see `art-direction.md`). Both GIFs are 960x640, which is the GBA's 240x160 at 4x, so they are drawn at native resolution (inferred from the exact 4x ratio). The descriptions below come from looking at the GIF frames.

### 1a. Battle UI v0.1: `UItest2.gif` (current)

Placeholder content: the enemy is Charizard, the player's mon is a purple, spiky back sprite (Nidoking/Nidorino-like, inferred), and the name strings are "BULBASAUR" / "BULBASAURB". These are art placeholders, not design data.

**Screen layout** (positions in GBA pixels, from the 4x image ÷ 4, approximate):

- **Background**: light-blue horizontal sky stripes at the top, white middle, a green grass strip across the screen at about y=45-55 (dithered edge), and an oval green-outlined grass platform under the enemy. The player's mon has no visible platform.
- **Enemy healthbox, top-left** (about x 4-108, y 6-44), with a dark navy outline, white fill and a speech-bubble tail pointing right toward the enemy:
  - Name in caps, "Lv100" right-aligned on line 2 (small "Lv" glyph + large digits).
  - **HP bar** (green), labelled "HP" in a navy tab.
  - **TP bar** (orange), labelled "TP", directly below the HP bar. **New vs Emerald.**
  - A **vertical column of six party indicators** on the box's left edge: two red/white "ball" pips and four empty grey dots. This shows the opponent's party count permanently.
- **Player healthbox, middle-right** (about x 132-235, y 74-116), same style, tail pointing left:
  - Name; a **status tag "⟨PSN⟩"** in purple with angle brackets, left of "Lv100".
  - HP bar (green) and TP bar (orange), each labelled.
  - A **blue EXP bar** along the bottom edge.
  - A vertical column of six party pips on the box's **right** edge (three red/white, three empty in the frame).
  - No numeric HP. Emerald shows "HP cur/max" for the player; the mockup has bars only (inferred design choice; confirm).
- **R / Talk badge, top-right corner** (about x 222-240, y 0-22): a navy tab with a white **"R"** above a **speech-bubble "…" icon**. It is the on-screen hint that R = Talk (GDD rule, section 2).
- **Bottom panel**, a red/coral "device" frame (Pokédex-like, inferred) across the bottom ~48 px:
  - A small top-left tab with a dark round lens and three dots, decoration like the original Pokédex.
  - **Left: message box** (about x 0-120), olive-green LCD-style fill. In action select it reads "BULBASAURB do?" (two lines in other frames: name, then "do?").
  - **Right: action menu**, a 2x2 grid of blue buttons: **FIGHT | PKMN** / **ITEM | RUN**. The selected button is drawn dark navy with white text (frame 1: FIGHT selected).
  - **Far right: SELECT hint**, an orange round "i" (info) icon with the label "SEL." under it. It suggests SELECT opens info during action selection (inferred; no written rule says what SEL does here. The GDD says SELECT chooses the active Pokémon in battle).
- **Move-selection frame**: the left box grows into a **wide navy move panel** (about x 0-123) over the message box, holding a 2x2 move grid: "FRAGRANCE TRAP" (wraps to two lines), "TACKLE", "EARTHQUAKE", "CLARVOYANCE" (sic). The selected move is a navy block (TACKLE). Its corners carry a small red cross icon (bottom-left) and three red lines (bottom-right), as decoration.
  - **Right: move info panel**: a tab with the move **type ("NORMAL")**, and below it **"TP 5"**, the move's TP cost, which replaces Emerald's "PP x/y". A **"=" icon** sits at the right. Its meaning is not stated; it might be the move category or rank (inferred; ask Omar).
- **Battle-message frame**: the green message box spans the **full width** for battle text ("HOUNDOUR used EMBER! / It was super effective!"). The action menu hides while messages print, as in Emerald.
- The party-pip columns, R badge and SEL hint stay visible throughout.

### 1b. Battle UI "v1" (older): `BattleUI_v1_big.gif`

The same scene and bottom panel, with these differences:

- **No TP bars** (hence the caption "TP still to be added").
- Enemy box top-left with an HP bar only; **party pips as a horizontal row of six dots below the box** (four grey, two red). The player box has the same row below it, on the right.
- Player box: name, ⟨PSN⟩, Lv100, HP bar, blue EXP bar.
- Top-right badge shows only the speech-bubble "…" icon, with no "R" letter.
- Action menu 2x2 **FIGHT | PKMN / ITEM | RUN**; the frames cycle the selection cursor through all four. No SEL. hint.
- The player back sprite is absent in all frames.

**v1 → v0.1 changes**: TP bars added; party pips moved from horizontal rows to vertical columns on the box edge; "R" added to the Talk badge; SEL/info hint added; move menu with TP cost shown.

---

## 2. UI rules from the GDD and brainstorm

| # | Rule (source) | Readiness | Engine notes (inferred) |
|---|---|---|---|
| R1 | **D-pad**: move in four directions; navigate menus (GDD Input). | Ready (vanilla). | Nothing to do. |
| R2 | **A**: interact/confirm (GDD). | Ready (vanilla). | Nothing to do. |
| R3 | **B**: cancel; **also used to run** (move faster) in the overworld (GDD). | Ready (vanilla). | Emerald's Running Shoes are hold-B after a flag. Making running available from the start is a one-flag change (`FLAG_SYS_B_DASH`). |
| R4 | **START**: opens the game menu (Pokémon status, items...); also starts the game (GDD). | Ready (vanilla). | Start menu contents may change later (Trainer Card, Index). |
| R5 | **SELECT**: extra functions: **show the region map**; **select the active Pokémon during battles**; cancel choices in the game menu (GDD). | Underspecified. Overworld "SELECT = region map" is **ready**. The battle meaning conflicts with the mockup's "SEL. / i" info icon. | Overworld: Emerald uses SELECT for the registered item (`src/field_control_avatar.c`). Rebind it to open the region map (`src/region_map.c`; reuse the Town Map item callback). In battle, the move menu already uses SELECT to swap moves (`battle_controller_player.c`). Decide which wins. |
| R6 | **L**: "additional functions" (GDD, unspecified). Brainstorm: **L = quick switch, PLA-style**. | Underspecified (idea). | A battle action that opens the party screen directly or cycles the next mon would hook `HandleInputChooseAction`. L is free in battle when the L=A option is off. |
| R7 | **R**: "additional functions" (GDD Input). In Battle: **pressing R during action selection starts Talk** with the opposing trainer. It gives hints about their strategy, can raise or lower Charisma, can trigger Trainer States, and **does not end the turn**. | Needs mechanic (Talk dialogue, Charisma, Trainer States). The **input hook and on-screen R badge are ready**. | `HandleInputChooseAction` in `src/battle_controller_player.c`. Show a message and return to action select without consuming the turn. Per-trainer talk lines would be new ROM text tables. Wild battles: undefined (open-questions A.8). |
| R8 | **Battle actions**: GDD lists **Fight, Switch, Talk, Bag, Run** (mermaid flow). The mockup labels them **FIGHT / PKMN / ITEM / RUN**, with Talk on R rather than a menu button. | **Ready** (relabel and reorder the 2x2 menu). | Emerald's grid is FIGHT/BAG over POKéMON/RUN. The mockup order is FIGHT/PKMN over ITEM/RUN. Strings live in `src/battle_message.c` (action menu text) and cursor positions in `battle_controller_player.c`. |
| R9 | **Run** only escapes wild battles; in trainer battles it returns to the action menu (GDD flow, dotted line). | Ready (vanilla behaviour). | — |
| R10 | **Fight → Move selector** with up to 4 moves (+ Signature, exempt from the 4-move cap); a move is usable only with enough TP (GDD). | The move panel layout is **ready**. TP cost display **needs the TP mechanic**. The fifth Signature slot is **underspecified** (the mockup shows only 4 moves). | `MoveSelectionDisplayPpString` / `MoveSelectionDisplayPpNumber` / `MoveSelectionDisplayMoveType` in `battle_controller_player.c`. Replace "PP x/y" with "TP n" from a per-move cost field. With placeholder costs this is a small change. |
| R11 | **Switch → PKMN selection**, with a **PKMN Info** branch that returns to selection (GDD flow). | Ready (vanilla party menu has Summary). | — |
| R12 | **Bag → Item → PKMN selection**; a **PokéDoll** ends the battle (GDD flow). | Ready (vanilla Poké Doll behaviour). | — |
| R13 | **TP display**: each battler shows a TP bar under HP (mockup v0.1). TP starts at 100 each battle (GDD). | Needs the TP mechanic. The **healthbox art and bar drawing are ready to prototype**. | Healthboxes are sprites built in `src/battle_interface.c` from `graphics/battle_interface/healthbox_singles_*.png` and `hpbar.png`. A second bar needs new healthbox art (the boxes get taller) plus a bar-fill routine copied from the HP bar. The EXP bar (`expbar.png`) already shows how a second bar is drawn on the player box. |
| R14 | **Persistent party indicators** (six pips per side on the healthbox edge). | **Ready**. | Emerald shows the party ball bar (`ball_status_bar.png`, `ball_display.png`) only at battle start, then hides it (`Task_HidePartyStatusSummary_*` in `battle_interface.c`). Keeping it visible and moving it vertical is a contained change. |
| R15 | **Status tag** "⟨PSN⟩" left of the level on the healthbox. | **Ready**. | Emerald already draws status icons (`status*.png`) in that spot; this is restyling. |
| R16 | **No numeric HP on the player box** (mockup). | Ready, but confirm intent (inferred). | `UpdateHpTextInHealthbox` in `battle_interface.c`. |
| R17 | **Bottom panel restyle**: red Pokédex-device frame, olive LCD message box, blue 2x2 buttons. | **Ready** (art exists in MyRes). | Battle textbox tiles/tilemap: `graphics/battle_interface/textbox.png`, `textbox_map.bin`, `textbox_0/1.pal`; window templates in `src/battle_bg.c`. GBA BG palettes limit the colour count per 8x8 tile, so the art needs a check against 16-colour palettes. |
| R18 | **Talk badge (R + speech bubble)** fixed at top-right, and **SEL./info** icon in the bottom-right panel. | The badge art is **ready**. What SEL. does is **underspecified**. | A small static sprite or BG tiles. |
| R19 | **Move info panel shows TYPE and TP cost**, plus an unexplained "=" icon. | Type: ready. TP: needs mechanic. "=" icon: underspecified. | See R10. |
| R20 | Mockup move names include "FRAGRANCE TRAP" (two-line wrap) and "CLARVOYANCE". **Move names longer than Emerald's 12-character limit** are expected to wrap. | Underspecified. | `MOVE_NAME_LENGTH` is 12 in Emerald. Longer names mean a data-size change and a wrapped text window (inferred). |

## 3. UI implied by mechanics (no mockup yet)

All of these are **underspecified** (no layout exists). Listed so they are not forgotten; details live in `mechanics.md` and `region-and-maps.md`.

- Summary screen: INT stat, single SPECIAL, COST (P$), move TP cost and rank stars (★), Signature move entry.
- Charisma indicator: hidden or shown is undecided (open-questions A.7).
- Pokémon Market menu with [Pokemon] / [Cure] / [Attrezzatura] sections; prices given by the operator, not a panel.
- Trainer Card. The Green-run notes observe that the original Green's trainer card shows **gym leader mugshots** (`research-and-sources.md`). That is reference material, not a stated requirement.
- Magazine items readable from the bag; CVS food items.
- Region map on SELECT (R5): Emerald's region map is Hoenn; it needs Kanto map art.

---

## 4. Implementation order suggestion (inferred, not Omar's)

1. **Action menu relabel/reorder** (R8) and **SELECT → region map** in the overworld (R5): text and input only, no art.
2. **Move panel "TP n"** (R10/R19) backed by a placeholder per-move cost table, so the TP mechanic can be developed against it.
3. **Persistent party pips** (R14) and **status tag restyle** (R15).
4. **Healthbox redesign with TP bar** (R13, R16), needing the MyRes art converted to GBA palettes.
5. **Bottom-panel restyle** (R17) and **R/Talk badge** (R18).
6. **R = Talk** input stub (R7): message plus return to the menu, before the full Talk/Charisma system.

RAM note (inferred): items 1-6 are almost entirely ROM (graphics, strings, code). Only TP itself (one byte per battler) needs RAM. See `mechanics.md` §2 for the budget warning (EWRAM ~95%).
