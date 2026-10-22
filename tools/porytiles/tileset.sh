#!/usr/bin/env bash
# Mockup PNG -> Porytiles tileset pair with the fewest palettes.
#
#   tools/porytiles/tileset.sh import MAP.png REF.png gTileset_Kanto gTileset_MasaraTown
#       Cut MAP.png into primary/secondary sheets (REF.png = building sheet with a
#       transparent background), create both tilesets if they don't exist, copy the
#       sheets into their porytiles_src folders and write the layout to
#       build/porytiles/<secondary>/map.bin. Refuses to overwrite sheets you have
#       already edited unless FORCE=1. SPLIT_MASK=mask.png is passed to
#       map2porytiles.py --split-mask (red cells -> secondary, blue -> primary),
#       BEHAVIORS=cells.csv to --behaviors.
#
#   tools/porytiles/tileset.sh compile gTileset_Kanto gTileset_MasaraTown
#       Solve the minimum palette layout (palopt.py), write it as Porytiles palette
#       hints, then compile the primary and the secondary. Run this after every art edit.
#
# Run from WSL or Linux, never Windows. Needs python3 with pillow and ortools
# (pip install pillow ortools) and Porytiles 2.0.0: $PORYTILES, tools/porytiles/bin/porytiles
# (tools/setup-cloud.sh installs it there) or porytiles on PATH.
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
HERE="$REPO_ROOT/tools/porytiles"
cd "$REPO_ROOT"

PT="${PORYTILES:-}"
[ -n "$PT" ] || { [ -x "$HERE/bin/porytiles" ] && PT="$HERE/bin/porytiles"; } || true
[ -n "$PT" ] || PT="$(command -v porytiles || true)"
[ -n "$PT" ] || { echo "porytiles not found: run tools/setup-cloud.sh or set PORYTILES" >&2; exit 1; }

# gTileset_MasaraTown -> masara_town (the folder Porytiles creates)
folder() { echo "${1#gTileset_}" | sed -E 's/([a-z0-9])([A-Z])/\1_\2/g' | tr 'A-Z' 'a-z'; }

usage() { sed -n '2,19p' "$0" | sed 's/^# \{0,1\}//'; exit 1; }

cmd="${1:-}"; shift || true
case "$cmd" in
import)
    [ $# -eq 4 ] || usage
    map="$1"; ref="$2"; prim="$3"; sec="$4"
    psrc="data/tilesets/primary/$(folder "$prim")/porytiles_src"
    ssrc="data/tilesets/secondary/$(folder "$sec")/porytiles_src"
    out="build/porytiles/$(folder "$sec")"
    mkdir -p "$out"
    python3 "$HERE/map2porytiles.py" "$map" --secondary-ref "$ref" --out "$out" \
        ${SPLIT_MASK:+--split-mask "$SPLIT_MASK"} ${BEHAVIORS:+--behaviors "$BEHAVIORS"}
    [ -d "$psrc" ] || "$PT" create-tileset "$prim"
    [ -d "$ssrc" ] || "$PT" create-tileset --secondary --primary-pairing-mode manual \
        --primary-pairing-partners="$prim" -- "$sec"
    for pair in "$psrc:$out/primary" "$ssrc:$out/secondary"; do
        dst="${pair%%:*}"; src="${pair#*:}"
        if [ -z "${FORCE:-}" ] && [ -f "$dst/.from_mockup" ] && ! cmp -s "$dst/bottom.png" "$dst/.from_mockup"; then
            echo "$dst/bottom.png was edited by hand; rerun with FORCE=1 to overwrite it" >&2; exit 1
        fi
        rm -rf "$dst/anim"   # drop the sample flower animation create-tileset adds
        cp "$src"/{bottom,middle,top}.png "$src/attributes.csv" "$dst/"
        cp "$src/bottom.png" "$dst/.from_mockup"
    done
    echo "Sheets imported. Budget: $out/report.md  Layout: $out/map.bin"
    ;;
compile)
    [ $# -eq 2 ] || usage
    prim="$1"; sec="$2"
    psrc="data/tilesets/primary/$(folder "$prim")/porytiles_src"
    ssrc="data/tilesets/secondary/$(folder "$sec")/porytiles_src"
    python3 "$HERE/palopt.py" "$psrc" "$ssrc" \
        --primary-config "porytiles/tilesets/$prim/config.yaml" \
        --secondary-config "porytiles/tilesets/$sec/config.yaml"
    "$PT" compile-tileset "$prim"
    "$PT" compile-tileset --primary-pairing-mode manual --primary-pairing-partners="$prim" -- "$sec"
    ;;
*)
    usage
    ;;
esac
