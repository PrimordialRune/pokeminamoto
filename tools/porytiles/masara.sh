#!/usr/bin/env bash
# Rebuild Masara Town (gTileset_Kanto + gTileset_MasaraTown + LAYOUT_MASARA_TOWN)
# from Omar's mockup. Run from WSL or Linux:
#
#   tools/porytiles/masara.sh MAP.png BUILDINGS.png
#
# MAP.png = MyRes/Maps/Pallet_Town_v4.png without the Red sprite baked in by his
# door, BUILDINGS.png = MyRes/Tilesets/pallet_v2.png. Steps: merge near-identical
# tiles to fit the 512-tile budget (fit_tiles.py), draw the wave animation into
# the water (water_anim.py), cut and compile the tilesets (tileset.sh), then
# guess collision (collision.py). Intermediate files go to build/porytiles/masara_town.
set -euo pipefail
REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
HERE="$REPO_ROOT/tools/porytiles"
cd "$REPO_ROOT"
[ $# -eq 2 ] || { sed -n '2,11p' "$0" | sed 's/^# \{0,1\}//'; exit 1; }
map="$1"; ref="$2"
out=build/porytiles/masara_town
mkdir -p "$out"

python3 "$HERE/fit_tiles.py" "$map" "$ref" "$out/fit.png"
python3 "$HERE/water_anim.py" "$out/fit.png" "$out/masara.png" "$out/anim/water" --preview "$out/water.gif"

# The ROUTE 1 sign goes to the secondary so the primary keeps a little room.
python3 - "$out/mask.png" <<'PY'
import sys
from PIL import Image, ImageDraw
im = Image.new('RGB', (640, 576))
ImageDraw.Draw(im).rectangle((272, 16, 319, 47), fill=(255, 0, 0))
im.save(sys.argv[1])
PY
FORCE=1 SPLIT_MASK="$out/mask.png" "$HERE/tileset.sh" import "$out/masara.png" "$ref" gTileset_Kanto gTileset_MasaraTown

anim=data/tilesets/primary/kanto/porytiles_src/anim
rm -rf "$anim"; mkdir -p "$anim"
cp -r "$out/anim/water" "$anim/"
cat > "$anim/anim.json" <<'JSON'
{
  "water": {
    "frames": ["f0", "f1", "f2", "f3", "f4", "f5", "f6", "f7"],
    "frame_order": ["f0", "f1", "f2", "f3", "f4", "f5", "f6", "f7"],
    "tile_offset": 1
  }
}
JSON
"$HERE/tileset.sh" compile gTileset_Kanto gTileset_MasaraTown
python3 "$HERE/render_check.py" data/tilesets/primary/kanto/porytiles_bin \
    data/tilesets/secondary/masara_town/porytiles_bin "$out/map.bin" 40 "$out/masara.png"

mkdir -p data/layouts/MasaraTown
# Walkable: the pier deck and the ROUTE 1 gap. Blocked: the lab's roof.
python3 "$HERE/collision.py" "$out/masara.png" "$out/map.bin" data/layouts/MasaraTown/map.bin \
    --walk 17,30,19,33 --walk 15,0,16,0 --block 24,15,30,19 > "$out/collision.txt"
mv data/layouts/MasaraTown/collision.png "$out/"
echo "Masara rebuilt. Budget: $out/report.md  Collision preview: $out/collision.png"
