# FRLG import

Brings FireRed/LeafGreen maps and overworld sprites from a
[pokefirered](https://github.com/pret/pokefirered) checkout into this repo as
placeholders.

```bash
git clone --depth 1 https://github.com/pret/pokefirered /tmp/pokefirered
python3 tools/frlg_import/import_maps.py /tmp/pokefirered
python3 tools/frlg_import/import_sprites.py /tmp/pokefirered
make -j$(nproc)
```

Both scripts can be rerun; they replace what they generated before (marked
`FRLG import begin/end` blocks, `imported_layouts.txt`, the `frlg_*` tileset
folders, `graphics/object_events/frlg/`, `src/data/object_events/frlg/`).
Rerunning `import_maps.py` overwrites the imported maps' `map.json` and
`scripts.pory`, so hand edits to those maps are lost: once maps are being
edited, stop rerunning it.

## What comes over

- **Maps:** the 256 mainland Kanto maps (towns, routes, interiors, dungeons;
  Sevii Islands, Trainer Tower, Navel/Birth Island, Mt. Ember, link rooms and
  prototypes are skipped), in map groups appended after ours
  (`gMapGroup_KantoTownsAndRoutes`, `gMapGroup_KantoDungeons`,
  `gMapGroup_IndoorPallet`...). Layout, border, warps, connections, NPC
  placement and signs are kept. Scripts, trainers, flags, items, hidden items
  and coord triggers are not: NPCs and signs have no script yet. Warps to maps
  that weren't imported go to `MAP_DYNAMIC`. Music uses the `MUS_RG_*` songs.
  Pokémon Centers come over as-is although Minamoto replaces them with inns.
  Layouts whose id or folder clashed with ours got a `Frlg` prefix
  (`LAYOUT_FRLG_HOUSE1`, `LAYOUT_FRLG_POKEMON_CENTER_1F`...).
- **Tilesets:** `gTileset_FrlgGeneral`, `gTileset_FrlgBuilding` and 49
  secondaries (`data/tilesets/*/frlg_*`), converted to this engine's 512/512
  split (see the docstring in `import_maps.py`): every tile sits at the VRAM
  index it had in FRLG, so `map.bin` files are FRLG's byte for byte. Metatile
  behaviors are mapped by name; FRLG-only ones (signs, cabinets, spinners...)
  are listed in `BEHAVIOR_MAP` or become `MB_NORMAL`. FRLG's tile animations
  are in `src/data/tileset_anims_frlg.h`.
- **Sprites:** all 152 FRLG object event graphics as `OBJ_EVENT_GFX_FRLG_*`.
  Graphics ids are u16 for this (`struct ObjectEvent`, `ObjectEventTemplate`;
  save format 3, with a migration from 2).

Warp to any imported map with the debug menu (hold R, press START, Utilities,
Warp: group 4 = Kanto dungeons, 5 = Kanto towns and routes, 6 and up = indoor
groups; see `include/constants/map_groups.h`).
