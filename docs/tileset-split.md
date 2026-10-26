# Tileset split: proposal (#16)

Status: proposal only. Nothing in `include/fieldmap.h` or the tilesets has
changed. A global switch would break the Emerald tilesets the opening maps still
use, and those tilesets (`data/tilesets`, `src/data/tilesets`) belong to another
thread right now.

## Where we are (rev/hw, 2026-10-25)

| | primary | secondary | total |
|---|---:|---:|---:|
| BG tiles (`NUM_TILES_*`) | 512 | 512 | 1024 |
| metatiles | 512 | 512 | 1024 |
| BG palettes for the map | 6 (0-5) | 8 (6-13) | 14 |

What Kanto uses today:

| tileset | tiles | metatiles | palettes |
|---|---:|---:|---|
| gTileset_Kanto (primary) | 501 / 512 | 328 / 512 | 5 of 6 (palopt) |
| gTileset_MasaraTown (secondary) | 488 / 512 | 176 / 512 | 1 of 8 |

Palettes are not the bottleneck for Masara yet: the town fits in 6 of its 14
palettes. **Primary tiles are**: 11 tiles are left for the rest of Kanto's
shared overworld (water edges, ledges, signs, fences, the other towns'
paths).

## The FireRed layout and what a 7/7 split means

FireRed uses 640/384 tiles, 640/384 metatiles and 7/6 palettes. The reason is
that Kanto towns reuse a lot (houses, roofs, fences, trees, water), so the
shared part lives in the primary and each town's own buildings stay small.

Recommended target for Minamoto:

- **Tiles 640 / 384.** That is 139 more primary tiles for the shared Kanto set.
  Masara's secondary (488) must come down to 384 or less by moving its reusable
  pieces (the player's and rival's houses, the fence, the mailbox, the tree
  line) into the primary. 501 + ~110 is about 610 of 640.
- **Metatiles 640 / 384.** Kanto has 328 primary and Masara has 176 secondary.
  Both fit with room to spare.
- **Palettes 7 / 7 (14 in total).** BG palette 13 is already a map palette
  here (commit c8c39d796), so the secondary keeps 7, one more than FireRed. The
  seventh primary palette goes to shared colors that towns now duplicate in
  their secondary palettes (water and sand, for example). Day/night,
  weather and the LCD profile already handle palettes 0-13 as one range,
  whatever the split.

VRAM is unchanged. There are still 1024 tiles in charblocks 0-1
(0x06000000-0x06007FFF), with the map screenblocks 28-31 at 0x0600E000, so
charblock 0-1 never reaches them. The secondary tile base moves from tile 512
to tile 640 (0x06005000). Nothing else is placed in that gap.

## Why not now

A global change of the three constants moves the secondary's tiles, metatile
ids and palettes for every map at once:

1. **Emerald secondaries in use don't fit 384 tiles.** These are
   BrendansMaysHouse 480, Lab 500, PokemonCenter 478 and GenericBuilding 509
   (with 512 metatiles). Mystery events house 509, Shop 502 and others are
   the same if they come back.
2. **Every secondary metatile id shifts.** Secondary ids start at
   `NUM_METATILES_IN_PRIMARY`. Every `map.bin` / `border.bin` that uses a
   secondary metatile needs +128, and so do the metatile labels and any
   C or script code that names a metatile (`METATILE_*` constants, e.g. the
   door and PC metatiles).
3. **Every secondary's palette indices shift by one.** Metatile tiles carry
   absolute palette numbers (6-13). With 7 primary palettes, an Emerald
   secondary's palette 6 would show the primary's seventh palette. Every
   Emerald secondary would need its palettes and metatiles renumbered.

All three are data changes in `data/tilesets`, `src/data/tilesets` and the map
layouts, which are off-limits for this thread. They would also be thrown away
when the Emerald opening maps are removed.

### Per-primary split (considered, rejected)

Give each primary its own split (Kanto 640/7, General 512/6). The game side
is about 80 uses of the three constants in fieldmap.c, field_camera.c,
overworld.c, tileset_anims.c, decoration.c, shop.c and trainer_hill.c. They
would become lookups on the current primary, plus new `struct Tileset` fields
in the off-limits headers. Porytiles can take per-tileset overrides
(`--num-tiles-in-primary` etc.), but Porymap reads one split from fieldmap.h
for the whole project. It would draw and edit the other family wrong. Two
splits in one project would cost every map edit until the Emerald maps are
gone, so it isn't worth it.

## Plan when the last Emerald map tileset goes

One commit, in the thread that owns the tilesets:

1. `include/fieldmap.h`: `NUM_TILES_IN_PRIMARY 640`, `NUM_METATILES_IN_PRIMARY 640`,
   `NUM_PALS_IN_PRIMARY 7`. Porytiles, palopt.py, map2porytiles.py and
   render_check.py read these.
2. Re-cut the Masara mockup with a split mask that sends the reusable
   buildings to the primary (`SPLIT_MASK=... tools/porytiles/tileset.sh import`),
   then `tileset.sh compile gTileset_Kanto gTileset_MasaraTown`. The original
   mockup PNG used for Masara is not in the repo (only `porytiles_src`
   is), so this needs the art source from `PKMN_Minamoto/MyRes`.
3. Re-run porytiles for every other tileset that pairs with gTileset_Kanto.
   Remap Masara's `map.bin` and borders: map2porytiles writes a new layout.
4. Check the Kanto tile animations. Their VRAM destinations come from the
   porytiles `anim/` folders and `TILE_OFFSET_4BPP(NUM_TILES_IN_PRIMARY + n)`,
   so they follow the constant, but frame counts must still fit the
   6 KB-per-VBlank anim DMA cap.
5. Also update the palette cycles' entries (`src/palette_cycle.c`). They name
   a palette number, and Masara's palette 6 becomes 7.

palopt.py's packing for this note: Kanto needs 5 primary palettes and Masara
needs 1 secondary palette, so a 7/7 palette split changes nothing for Masara
today. The 640/384 tile split can't be checked without re-cutting the art
(step 2).
