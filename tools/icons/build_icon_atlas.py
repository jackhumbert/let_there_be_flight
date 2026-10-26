"""Build the landing pad icon textures and atlas for the mod archive from landing_pad_icon.py's PNGs.

Steps (WolvenKit.Console 10; see the wolvenkit skill in E:\\src\\.claude\\skills):
  1. Each PNG is imported into a copy of the vanilla mappin_icons.xbm (`import --keep`), so it gets
     the vanilla UI texture setup (TEXG_Generic_UI, TCM_QualityColor, gamma, no mips), then the
     setup is matched exactly (not streamable, downgrade allowed) through a JSON round trip.
  2. An inkatlas like mappin_icons.inkatlas (3 slots: 4K / 1080p / 720p textures) with one part,
     `landing_pad`, covering each whole texture.
Output goes into src/wolvenkit/source/archive/user/jackhumbert/textures/ (pack it with
tools/pack_archive.py). Scripts use it with
  inkImageRef.SetAtlasResource(widget, r"user\\jackhumbert\\textures\\landing_pad_icons.inkatlas")
  inkImageRef.SetTexturePart(widget, n"landing_pad")

Usage: python build_icon_atlas.py --pngs <dir with landing_pad_icon_{64,32,22}.png> --work <scratch dir>
"""

import argparse
import json
import os
import shutil
import subprocess

GAME = r"D:\Games\SteamLibrary\steamapps\common\Cyberpunk 2077\archive\pc\content"
WOLVENKIT = r"D:\cp2077_work\build\tools\wolvenkit-10.0.0-beta.1\WolvenKit.CLI.exe"
REPO = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
DEPOT_DIR = r"user\jackhumbert\textures"
OUT = os.path.join(REPO, r"src\wolvenkit\source\archive", DEPOT_DIR)
VANILLA_XBM = r"base\gameplay\gui\common\icons\mappin_icons.xbm"
VANILLA_ATLAS = r"base\gameplay\gui\common\icons\mappin_icons.inkatlas"
SIZES = (64, 32, 22)  # slot order of mappin_icons.inkatlas: 4K, 1080p, 720p
PART = "landing_pad"
ATLAS = "landing_pad_icons.inkatlas"


def wk(*args):
    r = subprocess.run([WOLVENKIT, *args], capture_output=True, text=True)
    # `import --keep` exits 3 ("No buffers found to rebuild") even when every file was imported
    imported = args[0] == "import" and "Imported" in r.stdout and "/" in r.stdout
    if r.returncode != 0 and not (imported and r.returncode == 3):
        raise SystemExit(f"WolvenKit {' '.join(args[:1])} failed ({r.returncode}):\n{r.stdout[-2000:]}\n{r.stderr[-2000:]}")
    return r.stdout


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--pngs", required=True)
    ap.add_argument("--work", required=True)
    args = ap.parse_args()
    work = os.path.abspath(args.work)
    shutil.rmtree(work, ignore_errors=True)
    vanilla = os.path.join(work, "vanilla")
    wk("unbundle", GAME, "-o", vanilla, "-r", r"gui.common.icons.mappin_icons\.(xbm|inkatlas)$")

    # 1. textures
    raw = os.path.join(work, "raw", DEPOT_DIR)
    arc = os.path.join(work, "archive", DEPOT_DIR)
    os.makedirs(raw)
    os.makedirs(arc)
    for size in SIZES:
        name = f"landing_pad_icon_{size}"
        shutil.copy(os.path.join(args.pngs, name + ".png"), os.path.join(raw, name + ".png"))
        shutil.copy(os.path.join(vanilla, VANILLA_XBM), os.path.join(arc, name + ".xbm"))
    wk("import", os.path.join(work, "raw"), "-k", "-o", os.path.join(work, "archive"))
    js = os.path.join(work, "json")
    os.makedirs(js)
    os.makedirs(os.path.join(work, "final"))  # cr2w -o needs an existing folder
    os.makedirs(OUT, exist_ok=True)
    for size in SIZES:
        name = f"landing_pad_icon_{size}"
        wk("cr2w", "-s", os.path.join(arc, name + ".xbm"), "-o", js)
        path = os.path.join(js, name + ".xbm.json")
        doc = json.load(open(path, encoding="utf-8"))
        rc = doc["Data"]["RootChunk"]
        assert (rc["width"], rc["height"]) == (size, size), (name, rc["width"], rc["height"])
        rc["setup"]["isStreamable"] = 0
        rc["setup"]["allowTextureDowngrade"] = 1
        json.dump(doc, open(path, "w", encoding="utf-8"), indent=2)
        wk("cr2w", "-d", path, "-o", os.path.join(work, "final"))
        shutil.copy(os.path.join(work, "final", name + ".xbm"), os.path.join(OUT, name + ".xbm"))

    # 2. atlas, from the vanilla one: same slots, our textures, one part each
    wk("cr2w", "-s", os.path.join(vanilla, VANILLA_ATLAS), "-o", js)
    doc = json.load(open(os.path.join(js, "mappin_icons.inkatlas.json"), encoding="utf-8"))
    slots = doc["Data"]["RootChunk"]["slots"]["Elements"]
    assert len(slots) == len(SIZES)
    for slot, size in zip(slots, SIZES):
        slot["texture"]["DepotPath"] = {"$type": "ResourcePath", "$storage": "string",
                                        "$value": f"{DEPOT_DIR}\\landing_pad_icon_{size}.xbm"}
        part = slot["parts"][0]
        part["partName"] = {"$type": "CName", "$storage": "string", "$value": PART}
        part["clippingRectInUVCoords"].update({"Left": 0, "Top": 0, "Right": 1, "Bottom": 1})
        slot["parts"] = [part]
        slot["slices"] = []
    doc["Header"]["ArchiveFileName"] = ""
    path = os.path.join(js, ATLAS + ".json")
    json.dump(doc, open(path, "w", encoding="utf-8"), indent=2)
    wk("cr2w", "-d", path, "-o", os.path.join(work, "final"))
    shutil.copy(os.path.join(work, "final", ATLAS), os.path.join(OUT, ATLAS))
    for f in sorted(os.listdir(OUT)):
        if f.startswith("landing_pad"):
            print(os.path.join(OUT, f), os.path.getsize(os.path.join(OUT, f)))


if __name__ == "__main__":
    main()
