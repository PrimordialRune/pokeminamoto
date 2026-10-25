# Mechanics (changes vs vanilla Pokemon Emerald)

Sources: GDD v1 (formulas quoted exactly, in LaTeX, Italian text translated), arena_ecology_v2_1, city_design_framework_v2.1, project description.

"Engine impact" lines are **(inferred)** by the summariser from general knowledge of pret/pokeemerald; they are not in the source documents. Budget notes: EWRAM is ~95% full and IWRAM ~93-94% in this project, so every new persistent or battle-time variable needs a RAM plan.

Quick map of pokeemerald areas referred to: `src/battle_main.c`, `src/battle_script_commands.c`, `src/battle_util.c`, `src/battle_ai_*.c`, `src/battle_controller_player.c`, `src/pokemon.c`, `include/pokemon.h` (struct `BoxPokemon`/`Pokemon`/`BattlePokemon`), `include/global.h` (SaveBlock1/2), `include/constants/battle*.h`, `src/data/pokemon/*` (base stats, learnsets, evolutions), `src/data/moves_info.h` (the move table), `src/battle_interface.c`, `src/pokemon_summary_screen.c`, `src/party_menu.c`, `src/data/text/*`.

## 1. Stats

### Permanent stats (GDD table)

| Stat | Abbr | Range | Description |
|---|---|---|---|
| LEVEL | LV | 1-100 | Drives other stats, move learning, evolution |
| EXPERIENCE | EXP | 1-6 | Growth curve: Erratic, Fast, Medium Fast, Medium Slow, Slow, Fluctuating |
| HEALTH POINTS | HP | 0-255 | Dynamic; 0 = KO |
| ATTACK | ATK | 1-255 | Interacts with DEF for damage |
| DEFENSE | DEF | 1-255 | Interacts with ATK for damage |
| INTELLIGENCE | INT | 1-255 | Modifies TP growth each turn |
| SPEED | SPD | 1-255 | Determines turn order |
| SPECIAL | SPL | 1-255 | Interacts with itself for special damage |
| COST | P$ | 1-255 | Pokemon's cost; evaluation of all stats |

- Differences vs Emerald: **INT** is new; there is a **single Special** (Gen-1 style, no SpAtk/SpDef split); **COST** (P$) is a derived/rating stat for markets.
- Battle-only stats: ACCURACY (ACC), EVASION (EVN), CRITICAL CHANCE (CRIT); start at 1 (GDD wording), change in battle, reset afterwards.
- Base stats range 1-255. Per GDD, current-stat and HP formulas use "DV" (Gen-1 term) and EV is also mentioned (HP "subject to permanent increases from Effort Values").
- **Engine impact (inferred):** `include/pokemon.h` `struct BoxPokemon` substructs (stat storage, EVs/IVs), `struct BattlePokemon` (add `int`, drop spAttack/spDefense or alias), `src/pokemon.c` `CalculateMonStats`, base-stat tables `src/data/pokemon/base_stats.h` (new INT column; merge SpAtk/SpDef), summary screen stat page, nature handling. EVs/IVs: Emerald stores 6 EVs and 6 IVs (spAtk and spDef fields); removing/merging can free bits, adding INT EV/IV may fit by re-using the SpDef slot. **Save-data risk:** changing `BoxPokemon` layout breaks save compatibility; any added field would need to reuse existing substructure bits (the 80-byte box mon is tight). COST stat can be computed at runtime (no storage). Base stat table grows by one byte per species (ROM only).

### Experience curves (GDD formulas, exactly)

ERRATIC
$$
\mathsf{EXP=\begin{cases}
\frac{L^3 \times (100-L)}{50}, &\text{se } L < 50 \\
\frac{L^3 \times (150-L)}{100}, &\text{se } 50 \leq L \leq 68 \\
\frac{L^3 \times [\frac{(1911-(10 \times L))}{3}]}{100}, &\text{se } 68 \leq L \leq 98 \\
\frac{L^3 \times (160-L)}{100}, &\text{se } L > 98
\end{cases}}
$$

FAST: $\mathsf{EXP= \frac{4 \times L^3}{5}}$

MEDIUM FAST: $\mathsf{EXP=L^3}$

MEDIUM SLOW: $\mathsf{EXP=\frac{6}{5} \times L^3 - 15 \times L^2 + 100 \times L - 140}$

SLOW: $\mathsf{EXP=\frac{5 \times L^3}{4}}$

FLUCTUATING
$$
\mathsf{EXP=\begin{cases}
\frac{L^3 \times ([\frac{L+1}{3}]+24}{50}, &\text{se } L < 15 \\[2ex]
\frac{L^3 \times (L+14)}{50}, &\text{se } 15 \leq L < 36 \\[2ex]
\frac{L^3 \times ([\frac{L}{2}]+32)}{50}, &\text{se } 36 \leq L< 98
\end{cases}}
$$

(`se` = "if". Transcribed as in the GDD, including a visibly unbalanced parenthesis in the FLUCTUATING first line and the Fluctuating range stopping at L<98; flagged in open-questions.) These match the standard Gen-3 curves, so **Engine impact (inferred): none** (pokeemerald's `gExperienceTables` already encodes them, though the Medium Slow constant may differ).

### HP formula (GDD)
$\mathsf{HP=\left[\frac{\left( 2 \times BASE + \frac{DV}{4}\right)\times LEVEL}{100}\right] + LEVEL+10}$

### General stat formula (GDD)
$\mathsf{CurrentStat=\left[\left[\frac{\left( 2 \times BASE + \frac{DV}{4}\right)\times LEVEL}{100}\right] + 5\right]\times NATURE}$

(Applies to HP, ATK, DEF, INT, SPD, SPL.) Note the GDD uses DV/4 where Emerald uses IV + EV/4; and a NATURE multiplier (so Natures stay). **Engine impact (inferred):** `CalculateMonStats` in `src/pokemon.c`; decide whether to keep IV/EV or switch to Gen-1 DVs (affects storage, see above).

## 2. Move properties

Each move has TYPE, POWER (0-250 per GDD text; Notion slider max 150), ACCURACY (1-100), COST (TP, 5-100), RANK. Max 4 moves known (Signature moves exempt).

### Damage (GDD)
$\mathsf{Damage=\Bigg[\frac{\left ( \frac{2\times Level}{5}+2 \right )\: \times\:  Power \: \times\:  \frac{Attack}{Defense}}{50}  \Bigg]\times Modifier}$

(Level 1-100; Power 0-250; Attack and Defense < 255.) Compared to Emerald there is no "+2" outside the bracket and the Modifier set differs.

**In the game (2026-10-22):** Emerald's flat +2 is kept on purpose (Omar's call): without it, Lv 5 fights took about twice as long in the early-game balance model (`/mnt/project-files/balance-model/FINDINGS.md` §2).

$\mathsf{ Modifier = Badges \times Critical \times RNG \times STAB \times Type \times Other }$

- **Badges**: a move used by a player's Pokemon that has the same type as a defeated badge: x1.25, else x1. (See Badge boost below.)
- **Critical**: x2 if crit else x1.
- **RNG**: random between 0.85 and 1.
- **STAB**: x1.5 if the move type equals the user's type else x1.
- **Type**: 0, 0.25, 0.5, 1, 2, 4.
- **Other**: status/move-effect modifiers.
- Variation is "Roll"; fixed-damage and always-hit moves exist.
- **Engine impact (inferred):** `CalculateBaseDamage` in `src/pokemon.c` / `battle_util.c`. Emerald already has random 85-100 and STAB 1.5; the Badges factor needs a per-badge-type lookup of the player's flags (flags live in SaveBlock1 `flags` already; no new save data, but a badge-to-type map in ROM). No physical/special split: damage uses the type-based or move-category distinction chosen (see Special/Signature).

### Accuracy (GDD)
$\mathsf{ Accuracy= \left ( MoveAccuracy \times Accuracy \times Evasion \right ) \geq RNG }$

MoveAccuracy 1-100; Accuracy/Evasion are the stage boosts; RNG 1-100; some moves are "infallible". **Engine impact (inferred):** matches Emerald's `AccuracyCheck` in `battle_script_commands.c`; minimal.

### Critical hits (arena_ecology 0.3, global)
Stage table: +0 6.25%, +1 12.5%, +2 25%, +3 50%, +4 100% (stages +3/+4 raised from standard). Pathways: high-crit moves +1; Focus Energy / Leek / arena first-mover bonus +2; etc. **Engine impact (inferred):** `gCriticalHitChance` table `sCriticalHitChance[]` in `battle_script_commands.c`; ROM-only change.
**In the game (2026-10-22):** `sCriticalHitChance[]` is `{16, 8, 4, 2, 1}`.

### Cost / Technical Points (TP) - replaces PP
- Each Pokemon starts a battle with **100 TP** (a dynamic battle property). Moves cost TP (5-100). A move can only be used if enough TP remain. A part of TP is recovered at the end of each turn depending on INT.
- Formula (GDD):
$\mathsf{ TP= \left ( TP \: Left-Cost \right ) +5 + \frac{INT}{8}}$
 (TP Left = current TP; Cost = used move's cost; INT = attacker's Intelligence.)
- TP restoratives exist as Bag items. Arena modifiers can scale TP cost/recovery (Erika's SPIRIT PRESSURE: cost x1.0-2.5, recovery x1.0-0.25). Struggle is the fallback when no move is affordable (implied by the Erika doc).
- **Engine impact (inferred):** the largest change. Replaces `pp[4]` / `ppBonuses` in `struct BoxPokemon` moves substruct (`include/pokemon.h`), the `BattlePokemon.pp[]` field, `src/battle_*` PP deduction (`PressurePPLose`, `MOVE_...` checks, `battle_controller_player.c` move menu, `battle_interface.c` PP display, summary screen move page, PP Up/PP Max/Ether/Elixir items, Pressure ability, Struggle logic in `battle_main.c`). TP is battle-only, so it needs per-battler RAM (`gBattleMons[x].tp` 1 byte x 4 battlers, plus per-turn recovery) but NO save data if kept transient and PP bytes in the save are repurposed or ignored. **RAM:** a few bytes in battle structs (EWRAM `gBattleMons` already exists; a new u8 may fit in padding). Per-move cost table is ROM (`gBattleMoves` new field). Moves in the party menu / overworld won't show PP (UI changes). Flag: if PP bytes are repurposed there is no extra save space.

## 3. Move categories

Four GDD categories (plus the separate 22-category *ecology* taxonomy in arena_ecology, which is orthogonal):

- **Natural / "Normals"** - learned in normal growth; use body parts to deal damage; subcategory BODY, ARMS, FISTS, KICKS, TEETH, CLAWS, HORNS, TAIL. Have a **move rank**.
- **Elemental / "Elementals"** - elemental-type moves, strongly governed by type effectiveness; can be learned by Pokemon not sharing the type if "logical requirements" are met. Have a **move rank**.
- **Signature / Special Moves** - exclusive to one species or evolutionary line; **not overwritable; exempt from the 4-move limit**; always usable if the TP cost can be paid; scale on SPECIAL via Special Factor.
- **Learnable** - everything else; learnable via level-up or Move Machine (artificial disc, i.e. TM equivalent); shared across species.

Notion Moves DB counts: NORMAL 24, ELEMENTAL 39, MOVE MACHINE 48, SIGNATURE 150, plus one uncategorised row. (Counts may not equal final design; the "Learnable" doc category maps to Notion "MOVE MACHINE".) The Cost column in the sampled rows is empty, so TP costs are not populated yet.

### Move Rank (GDD)
A move identifier inside Natural and Elemental moves. Levelling can "upgrade" a ranked move: this is equivalent to learning a new move with different properties which can be used instead of the original. Example from the GDD: FUSHIGIDANE learns TACKLE at level 1; at level 20 the natural move ranks up and LUNGE becomes usable in addition to TACKLE (different Power, Cost, effects). Notion data: TACKLE (NORMAL, BODY, Power 40, Acc 100, Rank one star); LUNGE (NORMAL, BODY, Power 70, Acc 90, Rank two stars). Rank is stored as text of stars.
- **Engine impact (inferred):** learnset format (`src/data/pokemon/level_up_learnsets`) can express both as separate moves; a rank-up that keeps both moves selectable means "a move slot unlocks an alternative" - needs either extra move slots (more moves than 4 known per mon, RAM/save) or a rank table that replaces the known move at level thresholds. **Save impact if both stay in the same slot with a rank toggle:** extra bits per move; **if the original is replaced by the upgrade**, no save cost. Needs a design decision (see open-questions).

### Signature moves and Special Factor
$\mathsf{Damage=\Bigg(\frac{\left ( \frac{2\times Level}{5}+2 \right )\: \times\:  Power \: \times\:  SF}{50}  \Bigg)\times Modifier}$

$\mathsf{SF=\left(\frac{Attack}{Defense}\right)\times\left(\frac{SpecialAtt}{SpecialDef}\right)\times 1,15}$

SpecialAtt = attacker's Special; SpecialDef = defender's Special (single stat used on both sides). **Engine impact (inferred):** new damage branch in `CalculateBaseDamage`; Signature move storage per species (Notion: 150 signature moves, effectively one+ per Pokemon); exemption from the 4-move cap means a 5th move slot, derived at runtime from species (no extra save data if stored by species lookup rather than per-mon, **but** ROM species->signature table needed). The summary screen / battle menu needs a 5th entry (UI work, `battle_controller_player.c` move selection, `battle_interface.c`).

## 4. Level-up and leveling
Level 1-100. Move upgrades by rank at thresholds (see above). Evolution by level and (lore) trading remains for the trade-evolution lineage.

## 5. Trainer states (Trainer AI states) - GDD table

An activatable modification of opposing trainer AI via moves or in-battle dialogue; duration and intensity scale with the player's Charisma; may grant special items.

| State | Effect |
|---|---|
| Berserk | Trainer attacks with high-risk, high-power moves |
| Analysis Paralysis | Trainer's Pokemon attacks last, but recovers double TP |
| Euphoria | Trainer only uses super-effective moves and switches Pokemon if it can use a super-effective move |
| Panic | Trainer chooses moves at random, ignoring strategy |
| Focus | Trainer uses moves with higher accuracy, even if less powerful |
| Fear | Trainer surrenders if only one Pokemon is left |
| Resolute | Trainer's Pokemon attacks first but consumes double TP |
| Overconfident | Trainer uses stat-boosting moves and items even when risky |
| Despair | Trainer only uses special moves |
| Hesitant | Trainer switches Pokemon frequently to find a defensive advantage |
| Inspired | Trainer always uses moves with chances of additional effects |
| Envy | Trainer tries to lower the opponent's stats |
| Careful | Trainer constantly uses healing items |

- **Engine impact (inferred):** `src/battle_ai_script_commands.c` / `battle_ai_switch_items.c`; a per-battle "trainer state" variable (1 byte) in battle RAM, and per-trainer-state AI flag changes in `include/constants/battle_ai.h`. Priority overrides (Analysis Paralysis / Resolute) hook turn-order calc in `battle_main.c` (`GetWhoStrikesFirst`). TP interplay (double TP recovery/consumption) depends on the TP system. No save data if transient. Small EWRAM use.

## 6. Charisma (player stat)

- Dynamic player property representing popularity/reputation; raised/lowered by player choices and battle performance (and Talk outcomes). Affects: availability of rare items, ability to challenge certain trainers, info from NPCs, access to special events, encounter/capture of **illusory Pokemon**, intensity/duration of Trainer States. A parallel challenge to the main story, important for completion.
- Calibration (city framework): 0-15 start/unknown; 15-30 early cities; 30-50 mid-early; ~50 equal to a Gym Leader's renown; 50-75 mid-late, regional figure; 75-100 endgame. Each city doc gives thresholds as low/medium/high plus an indicative number. Early-game content may have high-threshold dialogue for backtracking (e.g. Tokiwa NPC at Charisma 60-70).
- Uses in design: INN Take room needs Charisma >= threshold; NPC info; subquest access; prices; informal transactions can reduce Charisma.
- Charisma is described as hidden in the project description and as visible-to-NPC-effects in GDD; no UI for it documented (open-question).
- **Engine impact (inferred):** one persistent byte (0-100) in SaveBlock (`gSaveBlock1Ptr` free vars area, or a `VAR_` script variable - `gSaveBlock1Ptr->vars` slots are 16-bit and cost no new space if an unused VAR is used). Script commands for `addvar` thresholds; no battle RAM. Recommend implementing as an overworld `VAR_CHARISMA`, **no new save fields needed** (inferred).
- **In the game (2026-10-22):** `VAR_CHARISMA` (was unused var 0x404E), capped at `CHARISMA_MAX` 100. Scripts change it with `addcharisma(N)` (negative subtracts, clamps 0-100, clobbers `VAR_0x8004`) and read it with `var(VAR_CHARISMA)`; C uses `GetCharisma()` / `ChangeCharisma()` in `src/field_specials.c`. Nothing reads it yet.

## 7. Talk action

- Pressing **R** during the action-selection phase in battle opens a dialogue with the opposing trainer: gives hints about their behaviour/strategy, and can raise/lower Charisma; can activate Trainer States. Talk does not end the turn ("after an action, excluding conversation, the player's turn ends").
- Battle menu: Fight, Switch, Talk, Bag, Run (mermaid diagram in GDD: Battle Start -> Call to Action -> Fight/Talk/Run/Bag/Switch).
- **Engine impact (inferred):** new action in the battle action menu (`battle_controller_player.c` `HandleInputChooseAction`, `battle_interface.c`, action-selection window layout in `src/battle_bg.c`/`strings`), R-button handler (Emerald uses L/R for summary shortcuts), script/dialogue text for per-trainer talk lines (`src/data/trainers.h` + text), a new battle-script path (`BattleScript_...`). Text data is ROM. Wild-Pokemon Talk is not defined.

## 8. Other battle structure (GDD)

- **Fight**: pick one of up to 4 moves (plus Signature); Rampage moves auto-repeat 2-3 times without selection; Multi-hit; moves with per-turn damage that block switching.
- **Switch**: once per turn, highest priority; consumes the whole turn; incoming mon cannot attack the turn it enters; status kept; residual effects may hit the replacement.
- **Bag**: uses a whole turn; limited carried items; TP restoratives exist.
- **Run**: only against wild Pokemon; trainer-battle Run returns to the action menu (diagram shows dotted line).
- Battle ends when all of one side's Pokemon are KO'd or a Pokemon escapes.
- **Escape formula (GDD):**
$\mathsf{Escape \: Probability = \frac{UserSpeed \times 32}{(EnemySpeed \div 4) \mod 256} + 30 \times Attempts}$
 Where UserSpeed = speed of the Pokemon trying to flee, EnemySpeed = wild Pokemon speed, Attempts = consecutive attempts. This is the Gen-3 escape formula. **Engine impact (inferred):** matches `TryRunFromBattle` in `src/battle_main.c`; none.
- Battle types: standard, boss battles (gym leaders, Four Heavenly Kings, illusory Pokemon) with special conditions, and one Double Battle gym (Coleman, GYM 7). Battles can occur in gyms, wild zones, or a "battle tower".

## 9. Badge boost (GDD)
A move used by a player-owned Pokemon with the same type as a defeated gym badge gets x1.25 power (Modifier term "Badges"). **Engine impact (inferred):** reuses existing badge flags; ROM table of badge-to-type; the Gym types here are Flying, Ground, Water, Electric, Ghost, Poison, Fire (plus Gym 8 not detailed). No save data.
**In the game (2026-10-22):** `sBadgeTypes[]` in `src/pokemon.c` maps badges 1-7 to Flying, Ground, Water, Electric, Ghost, Poison, Fire (arena doc gym order); badge 8 is `TYPE_MYSTERY` until gym 8 is designed. Emerald's x1.1 stat badge boosts (Atk, Def, Sp. Atk, Sp. Def and Speed) are removed. Off in link and Frontier battles.

## 10. Arena Ecology (per-gym environment mechanics)
Each gym has its own state variables (AIRFLOW, TERRAIN/MOISTURE, TIDE/CONDITION, COIL/CHARGE, SPIRIT PRESSURE, MIASMA DENSITY, COMBUSTION/STEAM) driven by the move-ecology categories (see `roster-and-encounters.md`). **Engine impact (inferred):** large: a per-gym battle script hook plus arena state vars in battle RAM (a handful of bytes, some with turn timers), a move->ecology-category table in ROM (22 categories, multi-tag per move, likely 3 bytes per move), special-case speed/accuracy/evasion modifiers, status changes, Focus Sash globally for gym/Boss mons (`gBattleStruct` flag per mon), AI logic per leader. The Double Battle at Coleman uses Emerald's double-battle engine (already present). Persistent save data: none required (all transient).

## 11. Other systems mentioned
- **Escape/Run, Bag limits, Learnable via Move Machine, Pokemon Markets** (buy/sell Pokemon, prices dynamic), **Trainer Card**, **PMC rank progression**, **INN room tiers**, **CVS food replacing berries** (species-dependent effects) and **magazines** (inventory items rereadable). **Engine impact (inferred):** new item types and pocket/menu behaviour (`src/data/items.h`, `item_menu.c`), Market UI; dynamic prices need event-flag-driven price tables (flags/vars in SaveBlock1).
- **Time-of-day events** use the RTC (Emerald has one; hardware/emulator RTC reliability matters, inferred).
