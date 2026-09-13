"""Check every RED4ext address hash used by the plugin (and optionally the in-tree
dependency plugins) against the game's shipped cyberpunk2077_addresses.json.

Usage:
  python tools/check_hashes.py [--game-dir "C:/.../Cyberpunk 2077"] [--deps]

Exit code 1 if any hash is missing. Commented-out lines are ignored.
"""
import argparse
import glob
import json
import os
import re
import sys

DEFAULT_GAME_DIR = r"C:/Program Files (x86)/Steam/steamapps/common/Cyberpunk 2077"
PATTERNS = [
    re.compile(r"REGISTER_\w*HOOK_HASH\(\s*[^,]+,\s*(\d+)\s*,\s*(\w+)"),
    re.compile(r"UniversalReloc(?:Func|Ptr)<[^;]*?>\s*(?P<name>\w+)?\s*[({]\s*(?P<hash>\d+)\s*[)}]"),
    re.compile(r"UniversalRelocBase::Resolve\((?P<hash2>\d+)\)"),
    re.compile(r"Red::Hook<[^>]*>\s*\(\s*(?P<hash3>\d+)"),
    re.compile(r"(?:AddressLib|UniversalRelocFunc|Hook)\s*\(\s*(?P<hash4>\d{6,})"),
]


def collect(roots):
    found = {}
    for root in roots:
        for f in glob.glob(os.path.join(root, "**", "*.[ch]pp"), recursive=True):
            with open(f, encoding="utf-8", errors="ignore") as fh:
                for line in fh:
                    if line.lstrip().startswith("//"):
                        continue
                    for pat in PATTERNS:
                        for m in pat.finditer(line):
                            gd = m.groupdict()
                            h = gd.get("hash") or gd.get("hash2") or gd.get("hash3") or gd.get("hash4") or m.group(1)
                            name = gd.get("name") or (m.group(2) if m.lastindex and m.lastindex >= 2 and not gd else None) or "call"
                            found.setdefault(int(h), set()).add(f"{os.path.relpath(f)}:{name}")
    return found


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--game-dir", default=DEFAULT_GAME_DIR)
    ap.add_argument("--deps", action="store_true", help="also scan deps/input_loader, deps/mod_settings, deps/red_lib")
    args = ap.parse_args()
    here = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    os.chdir(here)
    roots = ["src/red4ext"]
    if args.deps:
        roots += ["deps/input_loader/src", "deps/mod_settings/src", "deps/red_lib"]
    hashes = collect(roots)
    with open(os.path.join(args.game_dir, "bin/x64/cyberpunk2077_addresses.json")) as fh:
        data = json.load(fh)
    known = {int(a["hash"]): a.get("offset", "") for a in data["Addresses"]}
    print(f"addresses.json: {data.get('Linker map timestamp')} ({len(known)} entries); {len(hashes)} hashes referenced")
    missing = []
    for h, users in sorted(hashes.items()):
        ok = h in known
        print(f"{h:>11} {'OK     ' if ok else 'MISSING'} {known.get(h, ''):<16} <- {', '.join(sorted(users))}")
        if not ok:
            missing.append(h)
    print("MISSING:", missing or "none")
    sys.exit(1 if missing else 0)


if __name__ == "__main__":
    main()
