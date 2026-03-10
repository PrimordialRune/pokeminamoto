#!/usr/bin/env bash
# inc_to_pory.sh — Bulk-convert existing .inc script files to Poryscript raw-wrap format.
#
# For each data/maps/*/scripts.inc and data/scripts/*.inc:
#   - If no .pory already exists, wrap the .inc content in a raw `...` block
#   - Validate the .pory compiles with the local poryscript binary
#   - Print [OK], [SKIP], or [ERR] for each file
#
# Exit code: 0 if all conversions succeeded (or were skipped), 1 if any failed.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
PORYSCRIPT="$SCRIPT_DIR/poryscript/poryscript"
PORYSCRIPT_CC="$SCRIPT_DIR/poryscript/command_config.json"
PORYSCRIPT_FC="$SCRIPT_DIR/poryscript/font_config.json"

if [[ ! -x "$PORYSCRIPT" ]]; then
  echo "ERROR: poryscript binary not found at $PORYSCRIPT" >&2
  exit 1
fi

ok=0
skipped=0
failed=0

convert_file() {
  local inc_file="$1"
  local pory_file="${inc_file%.inc}.pory"

  if [[ -f "$pory_file" ]]; then
    echo "[SKIP] $pory_file (already exists)"
    skipped=$((skipped + 1))
    return
  fi

  # Wrap .inc content in a Poryscript raw backtick block
  {
    printf 'raw `\n'
    cat "$inc_file"
    printf '`\n'
  } > "$pory_file"

  # Validate the new .pory compiles successfully
  local tmpout
  tmpout="$(mktemp /tmp/poryscript_XXXXXX.inc)"
  local compile_err
  if compile_err=$("$PORYSCRIPT" -i "$pory_file" -o "$tmpout" \
      -cc "$PORYSCRIPT_CC" -fc "$PORYSCRIPT_FC" -lm=false 2>&1); then
    rm -f "$tmpout"
    echo "[OK]  $pory_file"
    ok=$((ok + 1))
  else
    rm -f "$pory_file" "$tmpout"
    echo "[ERR] $pory_file" >&2
    echo "      $compile_err" >&2
    failed=$((failed + 1))
  fi
}

cd "$PROJECT_ROOT"

# Per-map scripts
for inc_file in data/maps/*/scripts.inc; do
  [[ -f "$inc_file" ]] || continue
  convert_file "$inc_file"
done

# Global scripts under data/scripts/
for inc_file in data/scripts/*.inc; do
  [[ -f "$inc_file" ]] || continue
  convert_file "$inc_file"
done

echo ""
echo "Done: $ok converted, $skipped skipped, $failed failed"

[[ $failed -eq 0 ]]
