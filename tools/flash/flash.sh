#!/usr/bin/env bash
# Flash pokeminamoto to a GBA flash cart with the FlashGBX command line (build -> pick ROM -> flash -> verify).
#
#   tools/flash/flash.sh [options]
#
# Target (Omar, 2026-10): ChisFlash V1.1 (128 KB Flash save + Seiko RTC) written with a GBFlash flasher.
# Everything here is read-only until the "flash" step, and --dry-run never touches a cart: it prints each
# command, runs the ROM checks, and exits 0 with or without a cart, FlashGBX or a built ROM.
# Walk-through for a first flash: docs/engineering/real-hardware.md.
#
# Options
#   --dry-run          print what would run; run no build, no FlashGBX call (the ROM checks still run)
#   --rom FILE         flash this ROM instead of picking one
#   --no-build         use the ROM that is already built
#   --kind release|modern   which build to make and offer (default release: no debug menus)
#   --cart-type NAME   FlashGBX flash cart profile (default: autodetect; set FLASHGBX_CART_TYPE to keep one)
#   --port DEV         serial port of the flasher, e.g. /dev/ttyACM0 (default: FlashGBX finds it)
#   --no-save-backup   do not read the cart's save before writing (default: back it up to tools/flash/backups/)
#   --no-readback      skip the extra read-back compare (FlashGBX's own write verify always runs)
#   --info             only show the cart FlashGBX sees (header, flash chip, save type, RTC), then stop
#   --yes              no prompts (pick the default ROM, flash without asking)
#   -h, --help         this text
#
# Environment: FLASHGBX = command to run FlashGBX (default: the `FlashGBX` command, else python3 -m FlashGBX).
#
# ChisFlash RTC note (what the script can and cannot do): FlashGBX writes the ROM and the save, not the
# clock. The cart's Seiko S-3511A keeps running on its CR1616 and the game reads it as it is; after a
# flash or a save write it can show 2000-01-01 (power-failure flag or a reset). That is harmless to the
# game's own logic (it starts a new clock at 00:00 on a blank save), but if you want the clock to show
# the real time, set it with the GBA RTC tool for the flasher (GBA_RTCRead) before the first play; see
# the RTC section of docs/engineering/real-hardware.md. Check it with --info: "Real Time Clock".

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BACKUPS="$ROOT/tools/flash/backups"

DRY=0
ROM=""
BUILD=1
KIND=release
CART_TYPE="${FLASHGBX_CART_TYPE:-autodetect}"
PORT=""
SAVE_BACKUP=1
READBACK=1
INFO_ONLY=0
YES=0

die() { echo "flash.sh: $*" >&2; exit 1; }
note() { echo "== $*"; }
warn() { echo "WARNING: $*" >&2; }

usage() { sed -n '2,/^set -euo/p' "${BASH_SOURCE[0]}" | sed -e '$d' -e 's/^# \{0,1\}//'; }

while [ $# -gt 0 ]; do
    case "$1" in
        --dry-run) DRY=1 ;;
        --rom) ROM="${2:?--rom needs a file}"; shift ;;
        --no-build) BUILD=0 ;;
        --kind) KIND="${2:?--kind needs release or modern}"; shift ;;
        --cart-type) CART_TYPE="${2:?--cart-type needs a name}"; shift ;;
        --port) PORT="${2:?--port needs a device}"; shift ;;
        --no-save-backup) SAVE_BACKUP=0 ;;
        --no-readback) READBACK=0 ;;
        --info) INFO_ONLY=1 ;;
        --yes|-y) YES=1 ;;
        -h|--help) usage; exit 0 ;;
        *) die "unknown option $1 (see --help)" ;;
    esac
    shift
done

case "$KIND" in
    release) BUILT_ROM="$ROOT/pokeemerald_modern_release.gba"; MAKE_ARGS=(release) ;;
    modern)  BUILT_ROM="$ROOT/pokeemerald_modern.gba";         MAKE_ARGS=() ;;
    *) die "--kind must be release or modern" ;;
esac

# Print a command; run it unless this is a dry run.
run() {
    if [ "$DRY" = 1 ]; then
        echo "[dry-run] $*"
    else
        echo "+ $*"
        "$@"
    fi
}

# --- 0. environment ---------------------------------------------------------------------------------
note "0. environment"
if grep -qi microsoft /proc/version 2>/dev/null; then
    echo "WSL detected. WSL cannot see USB devices by default: attach the flasher with usbipd from an"
    echo "elevated Windows PowerShell, once per plug-in:"
    echo "    usbipd list                       (find the GBFlash / CH340 / Arduino-like entry)"
    echo "    usbipd bind --busid <BUSID>       (first time only)"
    echo "    usbipd attach --wsl --busid <BUSID>"
    echo "then 'ls /dev/ttyACM* /dev/ttyUSB*' here must show a port. On Linux add yourself to the dialout group."
fi

if [ -n "${FLASHGBX:-}" ]; then
    # shellcheck disable=SC2206
    FGBX=($FLASHGBX)
elif command -v FlashGBX >/dev/null 2>&1; then
    FGBX=(FlashGBX)
elif command -v flashgbx >/dev/null 2>&1; then
    FGBX=(flashgbx)
elif python3 -c 'import FlashGBX' >/dev/null 2>&1; then
    FGBX=(python3 -m FlashGBX)
else
    FGBX=(FlashGBX)
    if [ "$DRY" = 1 ]; then
        warn "FlashGBX is not installed (a real run would stop here). Install: pipx install FlashGBX  (or: python3 -m pip install --user FlashGBX)"
    else
        die "FlashGBX is not installed. Install it: pipx install FlashGBX  (or: python3 -m pip install --user FlashGBX), then run again."
    fi
fi
echo "FlashGBX command: ${FGBX[*]}"
FGBX_ARGS=(--cli --mode agb)
[ -n "$PORT" ] && FGBX_ARGS+=(--device-port "$PORT")

PORTS=$(ls /dev/ttyACM* /dev/ttyUSB* 2>/dev/null || true)
if [ -n "$PORTS" ]; then
    echo "serial ports: $(echo "$PORTS" | tr '\n' ' ')"
elif [ "$DRY" = 1 ]; then
    echo "serial ports: none (fine for a dry run)"
elif [ -z "$PORT" ]; then
    die "no serial port (/dev/ttyACM*, /dev/ttyUSB*): plug in the flasher and attach it to WSL (see above), or pass --port"
fi

# --- 1. build -------------------------------------------------------------------------------------------
note "1. build ($KIND)"
if [ "$BUILD" = 1 ] && [ -z "$ROM" ]; then
    if [ "$DRY" = 1 ]; then
        echo "[dry-run] (cd $ROOT && make ${MAKE_ARGS[*]} -j\$(nproc))"
    else
        (cd "$ROOT" && make "${MAKE_ARGS[@]}" -j"$(nproc)")
    fi
else
    echo "skipped"
fi

# --- 2. pick the ROM --------------------------------------------------------------------------------------
note "2. ROM"
if [ -z "$ROM" ]; then
    CANDIDATES=()
    for f in "$ROOT/pokeemerald_modern_release.gba" "$ROOT/pokeemerald_modern.gba"; do
        [ -f "$f" ] && CANDIDATES+=("$f")
    done
    if [ "${#CANDIDATES[@]}" -eq 0 ]; then
        if [ "$DRY" = 1 ]; then
            warn "no built ROM yet ($BUILT_ROM); a real run builds it in step 1"
            ROM="$BUILT_ROM"
        else
            die "no ROM found; run without --no-build"
        fi
    elif [ "$YES" = 1 ] || [ ! -t 0 ] || [ "${#CANDIDATES[@]}" -eq 1 ]; then
        ROM="$BUILT_ROM"
        [ -f "$ROM" ] || ROM="${CANDIDATES[0]}"
    else
        echo "Which ROM? (release has no debug menus; modern has them)"
        select choice in "${CANDIDATES[@]}"; do
            [ -n "${choice:-}" ] && { ROM="$choice"; break; }
        done
    fi
fi
echo "ROM: $ROM"
case "$(basename "$ROM")" in
    *regress*|*_test*) die "$ROM is a REGRESS/TEST build (headless harness hooks, not for a cart)" ;;
esac

# --- 3. ROM checks -----------------------------------------------------------------------------------------
note "3. ROM checks"
ROM_SIZE=0
if [ -f "$ROM" ]; then
    ROM_SIZE=$(python3 - "$ROM" <<'PY'
import hashlib, sys
p = sys.argv[1]
d = open(p, "rb").read()
errs, notes = [], []
size = len(d)
if size > 32 * 1024 * 1024:
    errs.append(f"{size} bytes is over 32 MiB (the cart's limit)")
if size < 0xC0:
    errs.append("too small to be a GBA ROM")
else:
    code = d[0xAC:0xB0].decode("ascii", "replace")
    notes.append(f"game code {code}, title {d[0xA0:0xAC].decode('ascii', 'replace').strip()}")
    chk = (-sum(d[0xA0:0xBD]) - 0x19) & 0xFF
    if chk != d[0xBD]:
        errs.append(f"header checksum byte {d[0xBD]:#04x}, should be {chk:#04x} (gbafix not run?)")
    if d[0xB2] != 0x96:
        errs.append("header fixed byte 0xB2 is not 0x96")
    if code != "BPEE":
        errs.append(f"game code {code}, expected BPEE (the Flash-save and RTC detection of the cart key off it)")
    if b"FLASH1M_V103" in d:
        notes.append("save type: FLASH1M_V103 (128 KB Flash), what the ChisFlash V1.1 has")
    else:
        errs.append("no FLASH1M_V103 tag: the cart would pick the wrong save chip")
    if b"SIIRTC_V001" in d:
        notes.append("RTC: SIIRTC_V001 library present (Seiko S-3511A)")
    else:
        errs.append("no SIIRTC_V001 tag")
    if b"RGRS" in d and b"gRegress" in d:
        errs.append("REGRESS hooks present")
if size & (size - 1):
    notes.append(f"size {size} is not a power of two; FlashGBX pads on write")
notes.append(f"size {size} bytes ({size / 1048576:.2f} MiB)")
notes.append("sha256 " + hashlib.sha256(d).hexdigest())
for n in notes:
    print("  " + n, file=sys.stderr)
for e in errs:
    print("  ERROR: " + e, file=sys.stderr)
print(size)
sys.exit(1 if errs else 0)
PY
    ) || die "the ROM failed its checks (see above)"
else
    echo "  (ROM not built yet; checks run on a real run)"
fi

# --- 4. cart ------------------------------------------------------------------------------------------------
note "4. cart"
INFO_CMD=("${FGBX[@]}" "${FGBX_ARGS[@]}" --action info --flashcart-type "$CART_TYPE")
if [ "$DRY" = 1 ]; then
    echo "[dry-run] ${INFO_CMD[*]}"
    echo "[dry-run] expect: a Game Boy Advance cart, 'Real Time Clock' line present (ChisFlash V1.1: yes), flash cart detected"
else
    echo "+ ${INFO_CMD[*]}"
    "${INFO_CMD[@]}"
fi
if [ "$INFO_ONLY" = 1 ]; then
    echo "--info: stopping here."
    exit 0
fi

# --- 5. save backup -----------------------------------------------------------------------------------------
note "5. save backup"
if [ "$SAVE_BACKUP" = 1 ]; then
    [ "$DRY" = 1 ] || mkdir -p "$BACKUPS"
    BACKUP="$BACKUPS/save_$(date +%Y%m%d_%H%M%S).sav"
    if [ "$DRY" = 1 ]; then
        echo "[dry-run] ${FGBX[*]} ${FGBX_ARGS[*]} --action backup-save --agb-savetype flash1m $BACKUP"
    else
        echo "+ ${FGBX[*]} ${FGBX_ARGS[*]} --action backup-save --agb-savetype flash1m $BACKUP"
        "${FGBX[@]}" "${FGBX_ARGS[@]}" --action backup-save --agb-savetype flash1m "$BACKUP" \
            || warn "could not read the save (a blank cart has none); continuing"
    fi
else
    echo "skipped"
fi

# --- 6. flash -------------------------------------------------------------------------------------------------
note "6. flash"
if [ "$DRY" = 0 ] && [ "$YES" = 0 ]; then
    read -r -p "Write $ROM to the cart now? This erases the ROM on the cart (the save is kept). [y/N] " ans
    case "$ans" in y|Y|yes) ;; *) die "cancelled" ;; esac
fi
# FlashGBX verifies what it wrote unless --no-verify-write is given; we never give it.
run "${FGBX[@]}" "${FGBX_ARGS[@]}" --action flash-rom --flashcart-type "$CART_TYPE" "$ROM"

# --- 7. verify ------------------------------------------------------------------------------------------------
note "7. verify"
echo "FlashGBX's own write verify ran in step 6 (a failure stops the script)."
if [ "$READBACK" = 1 ]; then
    SIZE_MB=$(python3 -c "
import sys
n = int(sys.argv[1]) or 16 * 1048576
p = 1
while p < n:
    p *= 2
print(max(p // 1048576, 1))" "$ROM_SIZE")
    READ="$(mktemp -t minamoto_readback.XXXXXX.gba)"
    trap 'rm -f "$READ"' EXIT
    if [ "$DRY" = 1 ]; then
        echo "[dry-run] ${FGBX[*]} ${FGBX_ARGS[*]} --action backup-rom --overwrite --agb-romsize ${SIZE_MB}mb $READ"
        echo "[dry-run] cmp -n $ROM_SIZE $ROM $READ"
    else
        "${FGBX[@]}" "${FGBX_ARGS[@]}" --action backup-rom --overwrite --agb-romsize "${SIZE_MB}mb" "$READ"
        if cmp -n "$ROM_SIZE" "$ROM" "$READ"; then
            echo "read-back matches the ROM ($ROM_SIZE bytes)"
        else
            die "READ-BACK MISMATCH: the cart does not hold this ROM. Reseat the cart, clean the contacts, flash again."
        fi
    fi
else
    echo "read-back skipped"
fi

# --- 8. next -------------------------------------------------------------------------------------------------
note "done"
cat <<'EOF'
Next (details and the pass/fail list: docs/engineering/real-hardware.md):
  1. Power the GBA with the cart. Title screen first, then NEW GAME.
  2. Save from the start menu, power off, power on, CONTINUE: the save must be there.
  3. Look at day/night: the clock starts at 00:00 on a new game, so the first hours are NITE.
  4. If a "battery has run dry" message shows at the main menu, the cart's clock lost power: reseat it,
     set the RTC (docs: RTC section) and try again. The game still plays (it falls back to play time).
EOF
[ "$DRY" = 1 ] && echo "(dry run: nothing was built, written or read from a cart)"
exit 0
