#!/bin/bash
# LTBF R41. Point let_there_be_flight.dylib at the copies of spdlog, fmt and FMOD that sit next to it in the plugin folder
# (red4ext/plugins/let_there_be_flight/), add @loader_path as an rpath (libfmodstudio itself asks for @rpath/libfmod.dylib),
# then re-sign ad hoc (install_name_tool invalidates the signature). Same pattern as cp2077-codeware/macos/fix-install-names.sh.
# Usage: fix-install-names.sh <path to let_there_be_flight.dylib>
set -eu
DYLIB="$1"
for lib in libspdlog libfmt libfmodstudio libfmod.dylib; do
  old=$(otool -L "$DYLIB" | awk -v pat="$lib" 'NR > 1 && index($1, pat) { print $1; exit }')
  if [[ -z "$old" ]]; then
    echo "fix-install-names: no $lib dependency found"
    continue
  fi
  case "$old" in
    @loader_path/*) echo "fix-install-names: $lib already $old" ;;
    *)
      new="@loader_path/$(basename "$old")"
      install_name_tool -change "$old" "$new" "$DYLIB"
      echo "fix-install-names: $old -> $new"
      ;;
  esac
done
if ! otool -l "$DYLIB" | grep -A2 LC_RPATH | grep -q '@loader_path'; then
  install_name_tool -add_rpath @loader_path "$DYLIB"
  echo "fix-install-names: rpath @loader_path added"
fi
codesign -f -s - "$DYLIB"
