#!/bin/bash
# LTBF (macOS): offline tests. No game, nothing written outside a temporary folder.
#   1. matrix_test: the GetMatrixFromOrientation replacement against the SDK's quaternion rotation.
#   2. with a dylib (use the LTBF_MAC_NO_AUDIO build, which needs no FMOD next to it):
#      smoke:     dlopen it in a plain process and print what it queued at load (ltbf_mac_pending);
#      deps_test: the dependency check at Main and at ltbf_registerNatives, in a test tree with stand-in dependency plugins
#                 (present but not loaded at Main; missing entirely).
# Usage: run.sh [<let_there_be_flight.dylib>]
set -eu
HERE="$(cd "$(dirname "$0")" && pwd)"
LTBF="$(cd "$HERE/../.." && pwd)"
SDK="${RED4EXT_SDK_DIR:-$LTBF/../../RED4ext.SDK-macos}/include"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT
CXX=(clang++ -std=c++20 -arch arm64 -DRED4EXT_PLATFORM_MACOS -DRED4EXT_HEADER_ONLY -w)

"${CXX[@]}" -I "$LTBF/src/red4ext" -I "$SDK" "$HERE/matrix_test.cpp" -o "$TMP/matrix_test" -Wl,-undefined,dynamic_lookup
"$TMP/matrix_test"
[[ $# -ge 1 ]] || exit 0

DYLIB="$(cd "$(dirname "$1")" && pwd)/$(basename "$1")"
if otool -L "$DYLIB" | grep -q libfmod; then
  echo "run.sh: $DYLIB links FMOD; give the LTBF_MAC_NO_AUDIO build (macos/build-noaudio)" >&2
  exit 2
fi
# A test tree: <root>/red4ext/plugins/let_there_be_flight with the dylib and its Homebrew fmt / spdlog.
make_tree() {
  local root="$1"
  mkdir -p "$root/red4ext/plugins/let_there_be_flight"
  cp "$DYLIB" "$root/red4ext/plugins/let_there_be_flight/let_there_be_flight.dylib"
  cp -L /opt/homebrew/lib/libfmt.12.dylib /opt/homebrew/lib/libspdlog.1.17.dylib "$root/red4ext/plugins/let_there_be_flight/"
}

# smoke
make_tree "$TMP/smoke"
cat > "$TMP/smoke.c" <<'C'
#include <dlfcn.h>
#include <stdio.h>
int main(int argc, char** argv) {
  void* h = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
  if (!h) { printf("smoke: dlopen FAILED: %s\n", dlerror()); return 1; }
  void (*pending)(unsigned long*, unsigned long*, unsigned long*) = dlsym(h, "ltbf_mac_pending");
  if (!pending || !dlsym(h, "Main") || !dlsym(h, "Query") || !dlsym(h, "Supports") || !dlsym(h, "ltbf_registerNatives")) {
    printf("smoke: an export is missing\n"); return 1; }
  unsigned long engine = 0, reg = 0, desc = 0;
  pending(&engine, &reg, &desc);
  printf("smoke: ok (Engine registrars %lu, RedLib registrations %lu, descriptions %lu)\n", engine, reg, desc);
  return engine && reg ? 0 : 1;
}
C
clang -arch arm64 "$TMP/smoke.c" -o "$TMP/smoke_bin"
"$TMP/smoke_bin" "$TMP/smoke/red4ext/plugins/let_there_be_flight/let_there_be_flight.dylib"

# deps_test
"${CXX[@]}" "$HERE/deps_test.cpp" -o "$TMP/deps_test"
stub() { # stub <root> <name> <major> <minor> <patch>
  mkdir -p "$1/red4ext/plugins/$2"
  "${CXX[@]}" -I "$SDK" -dynamiclib -DSTUB_MAJOR=$3 -DSTUB_MINOR=$4 -DSTUB_PATCH=$5 "$HERE/stub_plugin.cpp" \
    -install_name "@rpath/$2.dylib" -o "$1/red4ext/plugins/$2/$2.dylib"
}
make_tree "$TMP/later"
stub "$TMP/later" ArchiveXL 1 27 4
stub "$TMP/later" TweakXL 1 10 0
stub "$TMP/later" mod_settings 0 2 21
"$TMP/deps_test" "$TMP/later" later 2> "$TMP/later.log" || { cat "$TMP/later.log"; exit 1; }
grep -E 'present, loads later|is not loaded|found with version' "$TMP/later.log" | sed 's/^/  log: /'
make_tree "$TMP/missing"
"$TMP/deps_test" "$TMP/missing" missing 2> "$TMP/missing.log" || { cat "$TMP/missing.log"; exit 1; }
grep -E 'not found|continuing' "$TMP/missing.log" | sed 's/^/  log: /'
