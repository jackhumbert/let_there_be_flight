"""Resolve a pad kit's hashed mesh appearance names (cooked nodes keep only the CName hash, and
WolvenKit cannot write an unresolved CName). Reads each affected mesh's appearance list from the
game archives and matches FNV-1a 64 hashes; writes the kit back with names filled in.

  python pad_appearances.py kit.json [--archives <content dir>] [--work <scratch dir>]
"""
import argparse
import json
import os
import shutil
import subprocess
import tempfile

DEFAULT_WOLVENKIT = r"D:\cp2077_work\build\tools\wolvenkit-10.0.0-beta.1\WolvenKit.CLI.exe"
CONTENT = r"D:\Games\SteamLibrary\steamapps\common\Cyberpunk 2077\archive\pc\content"


def fnv1a64(s):
    h = 0xCBF29CE484222325
    for b in s.encode("utf-8"):
        h ^= b
        h = (h * 0x100000001B3) & 0xFFFFFFFFFFFFFFFF
    return h


def part_mesh(p):
    inner = p.get("inner") if isinstance(p.get("inner"), dict) else {}
    return p.get("mesh") or inner.get("mesh")


def part_appearance(p):
    inner = p.get("inner") if isinstance(p.get("inner"), dict) else {}
    return p.get("appearance") or inner.get("meshAppearance")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("kit")
    ap.add_argument("--archives", default=CONTENT)
    ap.add_argument("--wolvenkit", default=DEFAULT_WOLVENKIT)
    args = ap.parse_args()
    kit = json.load(open(args.kit))
    meshes = sorted({part_mesh(p) for p in kit["parts"] if str(part_appearance(p) or "").isdigit()})
    tmp = tempfile.mkdtemp(prefix="padapp_")
    try:
        names = {}
        for m in meshes:
            regex = "^" + m.replace("\\", "\\\\").replace(".", "\\.") + "$"
            subprocess.run([args.wolvenkit, "unbundle", args.archives, "-o", tmp, "-r", regex],
                           stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        files = [os.path.join(tmp, m) for m in meshes if os.path.exists(os.path.join(tmp, m))]
        js = os.path.join(tmp, "json")
        os.makedirs(js)
        subprocess.run([args.wolvenkit, "cr2w", "-s", *files, "-o", js],
                       stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        for m in meshes:
            jp = os.path.join(js, os.path.basename(m) + ".json")
            if not os.path.exists(jp):
                print("not read:", m)
                continue
            rc = json.load(open(jp, encoding="utf-8"))["Data"]["RootChunk"]
            for a in rc.get("appearances", []):
                n = a["Data"]["name"]["$value"]
                names[(m, str(fnv1a64(n)))] = n
    finally:
        shutil.rmtree(tmp, ignore_errors=True)
    fixed = unresolved = 0
    for p in kit["parts"]:
        app = str(part_appearance(p) or "")
        if not app.isdigit():
            continue
        n = names.get((part_mesh(p), app))
        if n:
            p["appearance"] = n
            fixed += 1
        else:
            p["appearance"] = "default"
            unresolved += 1
    json.dump(kit, open(args.kit, "w"), indent=1)
    print(f"{len(meshes)} meshes; {fixed} appearances resolved, {unresolved} set to default")


if __name__ == "__main__":
    main()
