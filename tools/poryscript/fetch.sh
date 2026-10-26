#!/usr/bin/env bash
# Download the pinned Poryscript release into tools/poryscript/poryscript (not committed).
# Used by tools/setup-cloud.sh and by the Makefile when the binary is missing.
# To bump: change the version and sha256 of poryscript-linux.zip below.
set -euo pipefail

PORYSCRIPT_VERSION=3.6.0
PORYSCRIPT_ZIP_SHA256=6d060f849b4f09bad8395926a4f60eee28b3f4592798b1d26ee5bbfb05e55ae3

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
if [ -x "$DIR/poryscript" ]; then
    exit 0
fi
if [ "$(uname -s)" != Linux ] || [ "$(uname -m)" != x86_64 ]; then
    echo "!! no prebuilt Poryscript for $(uname -s)/$(uname -m); build $PORYSCRIPT_VERSION from" >&2
    echo "!! https://github.com/huderlem/poryscript and put it at $DIR/poryscript" >&2
    exit 1
fi
tmp="$(mktemp -d)"
trap 'rm -rf "$tmp"' EXIT
echo ">> downloading Poryscript $PORYSCRIPT_VERSION" >&2
curl -fsSL --retry 3 -o "$tmp/p.zip" \
    "https://github.com/huderlem/poryscript/releases/download/$PORYSCRIPT_VERSION/poryscript-linux.zip"
echo "$PORYSCRIPT_ZIP_SHA256  $tmp/p.zip" | sha256sum -c --quiet
python3 -I -c "import zipfile,sys; zipfile.ZipFile(sys.argv[1]).extract('poryscript-linux/poryscript', sys.argv[2])" \
    "$tmp/p.zip" "$tmp"
install -m 755 "$tmp/poryscript-linux/poryscript" "$DIR/poryscript"
