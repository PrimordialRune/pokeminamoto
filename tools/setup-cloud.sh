#!/usr/bin/env bash
# Bootstrap a fresh Debian/Ubuntu box (Claude Code cloud env, CI, new machine)
# so that `make modern` and the classic `make` build pokeminamoto.
#
#   tools/setup-cloud.sh            # from anywhere inside the repo
#   SKIP_APT=1 tools/setup-cloud.sh # toolchain already installed
#
# Idempotent: skips apt packages and agbcc when they are already present.
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$REPO_ROOT"

SUDO=""
if [ "$(id -u)" -ne 0 ] && command -v sudo >/dev/null 2>&1; then
    SUDO="sudo"
fi

# --- 1. System packages -------------------------------------------------------
APT_PKGS=(build-essential git libpng-dev pkg-config
          binutils-arm-none-eabi gcc-arm-none-eabi libnewlib-arm-none-eabi)

if [ -z "${SKIP_APT:-}" ]; then
    missing=()
    for p in "${APT_PKGS[@]}"; do
        dpkg -s "$p" >/dev/null 2>&1 || missing+=("$p")
    done
    if [ ${#missing[@]} -gt 0 ]; then
        echo ">> apt install ${missing[*]}"
        export DEBIAN_FRONTEND=noninteractive
        $SUDO apt-get update -qq
        $SUDO apt-get install -y -qq --no-install-recommends "${missing[@]}"
    else
        echo ">> apt packages already installed"
    fi
fi

# --- 2. agbcc (classic compiler, not committed) -------------------------------
if [ -x tools/agbcc/bin/agbcc ] && [ -x tools/agbcc/bin/old_agbcc ]; then
    echo ">> tools/agbcc already installed"
else
    echo ">> building pret/agbcc"
    tmp="$(mktemp -d)"
    trap 'rm -rf "$tmp"' EXIT
    git clone --depth 1 https://github.com/pret/agbcc "$tmp/agbcc"
    (cd "$tmp/agbcc" && ./build.sh >/dev/null && ./install.sh "$REPO_ROOT")
fi

# Committed prebuilt binaries must be executable (older commits stored them 644).
chmod +x tools/poryscript/poryscript tools/poryscript/poryscript-pls 2>/dev/null || true

# --- 3. MCP server deps (optional, only if node is available) -----------------
if command -v node >/dev/null 2>&1 && \
   ! node -e 'process.exit(+process.versions.node.split(".")[0] >= 20 ? 0 : 1)'; then
    echo "!! node $(node -v) found; porymap-mcp needs node >= 20 (import.meta.dirname)"
fi
if command -v npm >/dev/null 2>&1; then
    for d in tools/porymap-mcp tools/gba-debug-mcp; do
        [ -f "$d/package-lock.json" ] && [ ! -d "$d/node_modules" ] || continue
        echo ">> npm ci in $d"
        (cd "$d" && npm ci --silent --no-audit --no-fund) || \
            echo "!! npm ci failed in $d; that MCP server will be unavailable"
    done
fi

# --- 4. Porytiles (tileset compiler, not committed: the binary is ~100 MB) ------
PORYTILES_VERSION=v2.0.0
PORYTILES_SHA256=68c904c3ff707ed2d6de0372073f165e627ec3ee7855eedbb3363184cc9212a4
if [ "$(uname -m)" = "x86_64" ] && ! [ -x tools/porytiles/bin/porytiles ]; then
    echo ">> downloading Porytiles $PORYTILES_VERSION"
    ptmp="$(mktemp -d)"
    if curl -fsSL -o "$ptmp/pt.zip" \
        "https://github.com/grunt-lucas/porytiles/releases/download/$PORYTILES_VERSION/porytiles-linux-amd64.zip" &&
       echo "$PORYTILES_SHA256  $ptmp/pt.zip" | sha256sum -c --quiet; then
        mkdir -p tools/porytiles/bin
        python3 -c "import zipfile,sys; zipfile.ZipFile(sys.argv[1]).extract('porytiles-linux-amd64/porytiles', sys.argv[2])" \
            "$ptmp/pt.zip" "$ptmp"
        install -m 755 "$ptmp/porytiles-linux-amd64/porytiles" tools/porytiles/bin/porytiles
    else
        echo "!! Porytiles download failed; tools/porytiles/tileset.sh will be unavailable"
    fi
    rm -rf "$ptmp"
fi
# Python deps for tools/porytiles/*.py (optional)
python3 -c "import PIL, ortools" 2>/dev/null || \
    python3 -m pip install --quiet pillow ortools 2>/dev/null || \
    python3 -m pip install --quiet --break-system-packages pillow ortools 2>/dev/null || \
    echo "!! pip install pillow ortools failed; tools/porytiles/*.py will be unavailable"

echo ">> done. Build with: make modern -j\$(nproc)   (classic: make -j\$(nproc))"
