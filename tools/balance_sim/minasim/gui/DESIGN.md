# Minasim GUI: design spec

Design canvas (8 artboards, 1440 wide, interactive mockups with sample data):
https://claude.ai/artifact/46keJS8CYE5tAQ8DEFF9yD  (private; the owner shares it from the Share menu)

Audience: Omar, the game designer, daily. Goal: "is anything too strong or too weak at this point of the game?" at a glance. Plain words, sprites everywhere, no jargon. The GUI is part of the game's world: Gen 1 sprites, GBC colours, the battle UI's coral frame and olive-green LCD box.

The mockup uses sprite atlases only because the design canvas cannot call the server. The real GUI uses the endpoints in `CONTRACT.md` (`/sprites/pokemon/<species>/<front|icon>.png`, `/sprites/item/`, `/sprites/trainer/`, `/sprites/type/`). Gym leader art in the mock is placeholder (Kanto leaders have no sprites yet): map trainer id to a `front_pics` name in one table, fall back to a "?" tile.

## Words

| Never say | Say |
|---|---|
| CI, confidence interval | "± 3 pts" (margin of error). Tooltip: "run it again and the number stays within this much" |
| band | "player level": Average / Skilled |
| delta_win | "fights won thanks to it" (bar), "+18 pts" |
| z-score, rating | "strength" score (1500 = typical) |
| baseline run | "before" / "previous run" |
| severity red/amber/info | "Red flag" / "Amber" / "Good to know" |

Percent points are written "pts". Numbers use tabular figures.

## Tokens

All in `static/minasim.css` (the mockup's `minasim.css` is the starting point: copy it, swap the sprite atlas rules for `<img>`/`background-image` on the sprite endpoints).

| Token | Light | Dark | Use |
|---|---|---|---|
| `--bg` | #EFEBDD | #181328 | page (GSC paper / night navy) |
| `--panel` | #FBF9F0 | #271F3B | cards |
| `--panel2` | #E7E2D0 | #342B4E | wells, table headers, track backgrounds |
| `--ink` | #271F3B | #E7E6DB | text, 3px borders, hard shadows |
| `--ink2` | #5A5370 | #B7B2C9 | secondary text |
| `--line` | #CFC8B0 | #463C66 | hairlines |
| `--coral` | #E15F69 | same | card headers, active tab, "worse" (battle UI frame colour; text on it is always #271F3B) |
| `--lcd` | #A0C16C | same | "good" header, what-if boxes (the DMG green) |
| `--cyan` | #31A2F2 | same | buttons, average line, TP pips |
| `--grass` / `--water` | #6DC155 / #6B63FF | same | win bars / accents |
| `--band` | #A0C16C | #4E6B2A | target-range shading |
| red chip | text #A82820 on #F6D3CB | #FF9A8E on #4A2227 | `--red-t/-bg` |
| amber chip | #7A4F00 on #F7E3A4 | #F2C660 on #46381A | `--amb-t/-bg` |
| info chip | #24558F on #D3E5F6 | #8CC4F5 on #1E3350 | `--info-t/-bg` |
| ok chip | #3F5C20 on #DCEBC3 | #BFE08A on #2E4020 | `--ok-t/-bg` |

Dark mode: `prefers-color-scheme` plus `[data-theme]` override, as in the CSS. Strength tiles on the dex use fixed pastel fills with fixed dark text in both themes: #B9B4FF much too weak, #D6D3FF weak, #DCEBC3 fair, #F7E3A4 strong, #F5A7A3 much too strong. Never rely on colour alone: every tile prints its number, every chip prints a word, out-of-range gauge markers are also thicker.

Fonts (Google Fonts): headings and big numbers `Pixelify Sans` 500/700; micro labels and kbd `Press Start 2P` at 9-10px, uppercase, never for sentences; body `Nunito Sans` 400/700/800, 15px. Heading sizes 34 / 24 / 18.

Spacing: 4 / 8 / 12 / 16 / 24 / 32 px. Page gutter 32px (16px under 700px), max width 1440. Cards: 3px ink border, 8px radius, hard 4px offset shadow (`0 4px 0 var(--shadow)`), header strip in coral (default), lcd (good/summary) or cyan (data/charts). Buttons: 44px tall, 3px border, 3px press-down on :active. Touch targets at least 44px (chart dots are 44px hit areas around 14px dots).

Sprites: `image-rendering: pixelated`, integer scales only (icons 32 or 64, fronts 64 or 128, items 24 or 48, type chips 32x16 or 64x32). Palette index 0 transparent (the server already does this).

## Components

- **TopBar**: logo, 7 tabs (Red flags, Journey, Fights, Species, Moves & items, Duel lab, Compare), run picker (date, short commit, opens a list of `/api/runs`), "Start new run" (POST /api/run, then a progress bar in the bar). Wraps at narrow widths.
- **Card** with strip header; **LcdBox** (olive green, Pixelify text) for summaries and "What if" suggestions.
- **SeverityChip / VerdictChip / ChangeChip**: pill, 2px border, icon-free word chips. Verdicts: Too strong (red), Strong (amber), Fine (ok), Weak (info), Never used (grey). Change: worse (red, triangle down), better (ok, up), new (info), same, resolved (ok, check).
- **Gauge**: track, green target band, thin line for the margin, marker. Marker turns red/amber when outside the band. Always paired with a text value "31% ± 2 pts, wanted 55 to 75%". Aria: full sentence.
- **DeltaBar**: centred on zero, right (green) stronger, left (coral) weaker. Text "+18 pts ± 2" under it.
- **TPPips**: one pip per TP, max 10 then "+n".
- **SpriteTile** (dex): icon x2, name, strength number, filled by strength colour, dashed hatch when not yet available, dim 28% when filtered out.
- **TypeChip**: type icon from `/sprites/type/`. **TypeFilter**: row of 44px icon buttons, toggles.
- **StageSlider**: native range 0..N-1 with stage names under it, label of the current stage in Pixelify.
- **Segmented control**: Average / Skilled everywhere; the choice is remembered (localStorage, try/catch) and shared across screens.
- **LineChart** (journey, species): SVG, non-scaling strokes, target band polygon behind, 44px button hit areas for points. Average line cyan, skilled line yellow (#F2C660), target band green.
- **LogReplay**: battle frame (coral/olive stage with both sprites and HP bars), LCD message line, prev/next, clickable turn list with damage tags.
- **DuelPanel**: starter picker (sprite buttons), level, number of fights, skill, Run; result block: stacked win/lose/draw bar, win rate ± margin, turns; progress bar while running.
- **Dumbbell** (compare): two circles (before white, after blue) on a gauge with the target band.
- **Table**: sticky header in Press Start 2P, sortable headers are buttons, row hover tint.
- **Empty / error states**: LCD box with plain text and the fix. Engine missing: "The simulator is not built yet. Run `make balance-lib`." (disable Run buttons, keep reports readable). No report yet: "No runs yet. Start your first run." with a big Start button.

## Screens

1. **Home / Red flags** (Main). Top: Latest run card (red count, amber count, fights played, fixed/new since previous) and a strip of 12 stage columns whose height/colour is the flag count (click: Journey). Filter row: Both/Average/Skilled, kind chips (All, Fights, Species, Moves, Items, TP), stage select. Then cards in a responsive 2-column grid (`minmax(560px, 1fr)`), ordered by `score`. Card: header strip tinted by severity with rank and stage; sprite well (subject: species front, trainer pic, item icon; fights show trainer + party); title; one-sentence detail; big value with ± margin; gauge with the target band; change chip vs the baseline; "What if" LCD box from `suggestion`; buttons Reproduce (toggles the `repro.cmd` strip with Copy), Try what-if (Duel lab pre-filled), Open (fight, species or table row).
2. **Journey**. Header with metric toggle (Win rate / HP left / TP left) and legend. One wide chart card: x = stages (badge circle filled when the stage has a gym badge, name, level range under the axis), y = 0 to 100%, green target band behind (band changes with fight class: trainer, rival, gym), cyan average line, yellow skilled line, one dot per fight (round trainers, square gyms, green inside the band, coral outside, selected has a coral ring), gym leader faces over their dots. Hover or focus a dot selects it. Below: Selected fight card (face, party icons with levels, average, skilled, wanted, verdict, Open fight, Duel now) plus two small cards: level curve (player vs trainers) and fights before you must rest (bars, wanted 8 to 15 shaded). The chart box scrolls horizontally under 1180px.
3. **Fight detail**. Header: back, trainer pic, name, place, stage, verdict, "Run duel now". Left column: team cards (front sprite x2, level, types, moves), win rate card (Average and Skilled gauges), by-starter rows (starter front, gauge, verdict; footer states the gap vs the allowed 15 pts), cost card (HP lost, TP used, items per fight, blackout rate as bars with icons). Right column: Replay (battle frame, message box, prev/next/new sample, turn list; turn text is from `sample_log`), Run duel now (starter segmented with icons, level, fights, skill, Run, live result).
4. **Species dex + page**. Dex: header with Average/Skilled and a find box; Stage card with slider and the colour legend; type filter row; grid of 151 tiles (`repeat(auto-fill, minmax(88px, 1fr))`) next to a sticky preview card (front sprite, type, verdict chip, strength ± margin, mini gauge, link). Tile click opens the Species page. Keyboard: tab through tiles, Enter opens. Species page: header (front x2, types, evolution line with icons and levels, base stat bars), "Strength over the game" line chart with evolution markers, Beats these easily / Struggles against lists (foe icon, win bar), flags on this species, Moves it learns table (level, name, type icon, TP pips, how often it is used, DeltaBar, verdict).
5. **Moves & items**. Two cards. Moves: verdict filter chips in the header, sortable columns (Level, TP cost, Used, Fights won thanks to it); row = move, who learns it (icon), type icon, level, pips, usage bar (coral above 60%), DeltaBar, verdict. Items: icon x2, first sold, price, DeltaBar, "per 1000" value, verdict.
6. **Duel lab**. Three columns: Your team (3 to 6 slots: front sprite, name, level, move chips), Set up + Result (number of fights 100/1,000/10,000/100,000, skill, seed, Fight, Fight once with log; result with stacked bar, win rate ± margin, turns, mons lost, progress), Their team (Custom or A trainer, which loads `trainers.party`). Below, the sprite picker (icon grid, search); clicking a slot chooses which side/slot the picker fills. Result buttons: Save as what-if, Show a sample log, Compare with a run.
7. **Run compare**. Before / After run pickers and a "what changed in the game" card (commit range, summary lines). Four total tiles (red flags before to after, fixed, new problems, amber). Table "Biggest swings" with a dumbbell per row, change in pts, status chip; filter Everything / Got worse / Got better. Rows only show changes larger than both margins.

## Interaction rules

- Everything is clickable through to its detail: flag card -> its fight/species/move; journey dot -> Fight detail; dex tile -> Species page; move row -> species page; compare row -> the matching card.
- Hover and keyboard focus show the same preview (journey dot, dex tile). Escape clears a selection.
- Filters and the Average/Skilled choice persist in the URL hash (shareable) and localStorage (convenience only).
- Long jobs (new run, big duels) show a progress bar from `/api/run/<id>/progress` or streamed duel chunks; results update live; the margin shrinks as fights finish. Run buttons turn into Stop.
- "Reproduce" copies `repro.cmd` to the clipboard and shows a toast "Copied". "Try what-if" opens Duel lab with the flag's subject loaded.
- Destructive or slow actions (new run over 1M fights) ask once with the estimated time.
- Responsive: single column below 900px, tables and the journey chart scroll inside their cards; page never scrolls sideways.
- Accessibility: real buttons and links, labels on inputs, text 4.5:1 or better, charts have aria summaries, dots have names, focus ring is the 3px coral outline.

## Data mapping (report.json to UI)

- Flag card <- `flags[]` (sorted by `score`; `metric.value/ci/target` feed the gauge; `change.status` the chip; `suggestion`; `repro.cmd`).
- Journey <- `stages[]`, `fights[].by_band[*].win_rate`, `fights[].target`, `level_curve[]`, `tp[]`.
- Fight detail <- `fights[]` (`party`, `by_band`, `by_starter`, `target`) plus `/api/duel` sample log.
- Dex <- `species[]` filtered by stage and band (`rating`, `win_rate`, `ci`, `z`); colour from `z` (below -1.4, -0.6, 0.8, 1.5 thresholds, tune later).
- Moves/items tables <- `moves[]`, `items[]` (`delta_win`, `ci`, `verdict`, `usage`, `tp_cost`, `price`, `delta_per_1000`).
- Compare <- two reports joined on `flags[].id`; changes below the combined ci are dropped.
