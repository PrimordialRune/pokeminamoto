#!/usr/bin/env bash
# Rebuild Masara Town (gTileset_Kanto + gTileset_MasaraTown + LAYOUT_MASARA_TOWN)
# from Omar's mockup. Run from WSL or Linux:
#
#   WATER_STYLE=soft WATER_SEED=80 tools/porytiles/masara.sh MAP.png BUILDINGS.png
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
python3 "$HERE/water_anim.py" "$out/fit.png" "$out/masara.png" "$out/anim/water" \
    --style "${WATER_STYLE:-soft}" --seed "${WATER_SEED:-80}" --preview "$out/water.gif"
# Crops (north field, lab garden, Red's planter) and the plaza flower bushes.
flowers=()
for b in 338,319,351 225,320,231 190,334,207 366,334,383 194,383,207 369,384,375 222,398,239 338,399,351; do
    IFS=, read -r x0 y0 x1 <<< "$b"
    flowers+=(--rect "flowers:$((x0-3)),$((y0-4)),$((x1+4)),$((y0+5)):20")
done
python3 "$HERE/sway_anim.py" "$out/masara.png" "$out/masara.png" "$out/anim" \
    --rect crops:336,32,416,96:16 --rect crops:512,271,576,303:16 --rect crops:144,54,160,96:8 \
    --leans crops:0,1,-1 "${flowers[@]}" --leans flowers:0,1,-1

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
cp -r "$out/anim/water" "$out/anim/crops" "$out/anim/flowers" "$anim/"
# Each animation gets its own run of tile slots (Porytiles rewrites the offsets on compile).
python3 - "$anim" <<'PY'
import json, os, sys
from PIL import Image
anim = sys.argv[1]
order = {
    'water': sorted((f[:-4] for f in os.listdir(os.path.join(anim, 'water')) if f[0] == 'f'), key=lambda f: int(f[1:])),
    'crops': ['f0', 'f0', 'f1', 'f1', 'f0', 'f0', 'f2', 'f2'],   # slow: 32 frames per pose
    'flowers': ['f0', 'f1', 'f0', 'f2'],
}
out, offset = {}, 1
for name, fo in order.items():
    n = Image.open(os.path.join(anim, name, 'key.png')).width // 8
    out[name] = {'frames': sorted(set(fo), key=fo.index), 'frame_order': fo, 'tile_offset': offset}
    offset += n
json.dump(out, open(os.path.join(anim, 'anim.json'), 'w'), indent=2)
PY
"$HERE/tileset.sh" compile gTileset_Kanto gTileset_MasaraTown
python3 "$HERE/render_check.py" data/tilesets/primary/kanto/porytiles_bin \
    data/tilesets/secondary/masara_town/porytiles_bin "$out/map.bin" 40 "$out/masara.png"

mkdir -p data/layouts/MasaraTown
# Walkable: the pier deck and the ROUTE 1 gap. Blocked: the lab's roof.
python3 "$HERE/collision.py" "$out/masara.png" "$out/map.bin" data/layouts/MasaraTown/map.bin \
    --walk 17,30,19,33 --walk 15,0,16,0 --block 24,15,30,19 > "$out/collision.txt"
mv data/layouts/MasaraTown/collision.png "$out/"
echo "Masara rebuilt. Budget: $out/report.md  Collision preview: $out/collision.png"
