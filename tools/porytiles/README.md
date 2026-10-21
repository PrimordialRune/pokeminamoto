# Tilesets from a map mockup

This takes a flattened map mockup (like `MyRes/Maps/Pallet_Town_v3.png`) to a pokeemerald primary + secondary tileset pair that uses as few palettes as possible. Nobody has to place palettes by hand.

Run it in WSL or Linux. `tools/setup-cloud.sh` installs what it needs:

- [Porytiles](https://github.com/grunt-lucas/porytiles) 2.0.0, into `tools/porytiles/bin/`;
- the Python packages `pillow` and `ortools`.

## Workflow

```bash
# 1. Once per town: cut the mockup and create the tilesets.
tools/porytiles/tileset.sh import MyRes/Maps/Pallet_Town_v3.png MyRes/Tilesets/pallet.png \
    gTileset_Kanto gTileset_MasaraTown
#    -> data/tilesets/{primary/kanto,secondary/masara_town}/porytiles_src/{bottom,middle,top}.png
#    -> build/porytiles/masara_town/{report.md,overlay.png,map.bin}

# 2. Edit the sheets in porytiles_src: layers, behaviors in attributes.csv, art fixes.

# 3. After every edit: optimal palettes, then compile.
tools/porytiles/tileset.sh compile gTileset_Kanto gTileset_MasaraTown
```

## The scripts

| Script | What it does |
|---|---|
| `map2porytiles.py` | Cuts the mockup into 16x16 metatiles and merges duplicates. Splits them between the primary and the secondary: a cell goes to the secondary if it holds a tile from the building sheet, or borders one and uses a color the open ground doesn't. `--split-mask` overrides that per cell. Writes the Porytiles sheets, `map.bin`, and a budget `report.md` with an `overlay.png` that marks one-off tiles. |
| `palopt.py` | Solves palette packing exactly with OR-Tools: fewest primary palettes first, then fewest secondary. Writes the result as Porytiles palette hints so `compile-tileset` reproduces it. `--suggest-merges` lists color merges that would free a palette. |
| `render_check.py` | Redraws the map from the compiled `tiles.png`, palettes and `metatiles.bin`, and counts the pixels that differ from the mockup. |
| `tileset.sh` | Wraps the above with Porytiles (`import` and `compile`). |

## Limits (include/fieldmap.h)

| | Primary | Secondary |
|---|---|---|
| Tiles (8x8) | 512 | 512 |
| Metatiles | 512 | 512 |
| Palettes (15 colors + transparent) | 6 (0-5) | 7 (6-12) |

The primary is shared by every Kanto town, so its palettes are the scarcest. `palopt.py --keep-free-primary N` keeps N primary palettes free for later towns.
