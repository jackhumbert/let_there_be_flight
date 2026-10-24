#!/bin/bash
# LTBF R41 (macOS). After the link: (1) the RED4ext entry points and ltbf_registerNatives are exported, (2) the dylib has no symbol that
# is looked up dynamically. The link flag -undefined dynamic_lookup (game functions are reached through address hashes, not symbol
# names) would also hide a function the plugin declares and never compiles in; its first call would crash. Everything LTBF calls in
# the game goes through an address, so that list must be empty (macos/undefined-allowed.txt can name exceptions).
# Same check as Mod Settings' macos/check-undefined.sh. Usage: check-undefined.sh <let_there_be_flight.dylib>
set -eu
DYLIB="$1"
ALLOW="$(dirname "$0")/undefined-allowed.txt"
for sym in _Main _Query _Supports _ltbf_registerNatives; do
  nm -gU "$DYLIB" | grep -q " T $sym\$" || { echo "check-undefined: FAILED. export $sym is missing" >&2; exit 1; }
done
BAD=$(nm -m -u "$DYLIB" | grep "dynamically looked up" | sed -E 's/^ +\(undefined\) external //; s/ \(dynamically looked up\)$//' || true)
if [[ -f "$ALLOW" && -n "$BAD" ]]; then
  BAD=$(echo "$BAD" | grep -vxFf <(grep -v '^#' "$ALLOW" | grep -v '^$') || true)
fi
if [[ -n "$BAD" ]]; then
  echo "check-undefined: FAILED. Symbols looked up dynamically at load (declared, not compiled in):" >&2
  echo "$BAD" | c++filt >&2
  exit 1
fi
echo "check-undefined: ok (Main, Query, Supports, ltbf_registerNatives exported; no dynamically looked up symbols)"
