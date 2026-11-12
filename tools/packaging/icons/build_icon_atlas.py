"""Build an icon's textures and atlas for the mod archive from its PNGs: the landing pad icon
(landing_pad_icon.py) or the AUTOPILOT logo (autopilot_logo.py), see ICONS.

Steps (WolvenKit.Console 10, the WOLVENKIT setting):
  1. Each PNG is imported into a copy of a vanilla texture (`import --keep`), so it gets the vanilla
     UI texture setup (TEXG_Generic_UI, TCM_QualityColor, gamma, no mips), then the setup is
     matched exactly (not streamable, downgrade allowed) through a JSON round trip.
  2. An inkatlas from the vanilla one (3 slots: 4K / 1080p / 720p textures) pointing at ours: the
     landing pad icon gets one part covering each whole texture; the AUTOPILOT logo keeps vanilla
     autodrive.inkatlas's parts, since its textures are vanilla's own with the logo repainted.
     (A first logo build put it alone in 210 x 34 / 106 x 18 / 72 x 12 textures made from the
     mappin template: the HUD drew it as a solid black box. Same-sized copies of vanilla's work.)
Output goes into src/wolvenkit/source/archive/user/jackhumbert/textures/ (pack it with
tools/packaging/pack_archive.py). Scripts use it with
  inkImageRef.SetAtlasResource(widget, r"user\\jackhumbert\\textures\\landing_pad_icons.inkatlas")
  inkImageRef.SetTexturePart(widget, n"landing_pad")

Usage: python build_icon_atlas.py [--icon landing_pad|autopilot] --pngs <dir with the PNGs> --work <scratch dir>
"""

import argparse
import json
import os
import shutil
import subprocess
import sys

from PIL import Image

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))  # tools/, for common
from common import paths  # noqa: E402

REPO = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", ".."))
DEPOT_DIR = r"user\jackhumbert\textures"
OUT = os.path.join(REPO, r"src\wolvenkit\source\archive", DEPOT_DIR)
MAPPIN_DIR = r"base\gameplay\gui\common\icons"
AUTODRIVE_DIR = r"base\gameplay\gui\widgets\autodrive"
# per icon: our atlas; the vanilla folder, atlas and per-slot textures it is made from (slot
# order 4K, 1080p, 720p); our PNG / texture names per slot; our part name (None: keep vanilla's)
ICONS = {
    "landing_pad": ("landing_pad_icons.inkatlas", MAPPIN_DIR, "mappin_icons.inkatlas",
                    ("mappin_icons", "mappin_icons", "mappin_icons"),
                    ("landing_pad_icon_64", "landing_pad_icon_32", "landing_pad_icon_22"), "landing_pad"),
    "autopilot": ("autopilot_logo.inkatlas", AUTODRIVE_DIR, "autodrive.inkatlas",
                  ("autodrive", "autodrive_1080p", "autodrive_720p"),
                  ("autopilot_logo", "autopilot_logo_1080p", "autopilot_logo_720p"), None),
}


def wk(*args):
    r = subprocess.run([paths.WOLVENKIT, *args], capture_output=True, text=True)
    # `import --keep` exits 3 ("No buffers found to rebuild") even when every file was imported
    imported = args[0] == "import" and "Imported" in r.stdout and "/" in r.stdout
    if r.returncode != 0 and not (imported and r.returncode == 3):
        raise SystemExit(f"WolvenKit {' '.join(args[:1])} failed ({r.returncode}):\n{r.stdout[-2000:]}\n{r.stderr[-2000:]}")
    return r.stdout


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--icon", choices=ICONS, default="landing_pad")
    ap.add_argument("--pngs", required=True)
    ap.add_argument("--work", required=True)
    args = ap.parse_args()
    atlas, vanilla_dir, vanilla_atlas, templates, names, part_name = ICONS[args.icon]
    work = os.path.abspath(args.work)
    shutil.rmtree(work, ignore_errors=True)
    vanilla = os.path.join(work, "vanilla")
    stems = "|".join(sorted({vanilla_atlas.split(".")[0], *templates}))
    wk("unbundle", os.path.join(paths.ARCHIVES, "content"), "-o", vanilla, "-r", vanilla_dir.replace("\\", ".") + r".(" + stems + r")\.(xbm|inkatlas)$")

    # 1. textures
    raw = os.path.join(work, "raw", DEPOT_DIR)
    arc = os.path.join(work, "archive", DEPOT_DIR)
    os.makedirs(raw)
    os.makedirs(arc)
    for name, template in zip(names, templates):
        shutil.copy(os.path.join(args.pngs, name + ".png"), os.path.join(raw, name + ".png"))
        shutil.copy(os.path.join(vanilla, vanilla_dir, template + ".xbm"), os.path.join(arc, name + ".xbm"))
    wk("import", os.path.join(work, "raw"), "-k", "-o", os.path.join(work, "archive"))
    js = os.path.join(work, "json")
    os.makedirs(js)
    os.makedirs(os.path.join(work, "final"))  # cr2w -o needs an existing folder
    os.makedirs(OUT, exist_ok=True)
    for name in names:
        size = Image.open(os.path.join(args.pngs, name + ".png")).size
        wk("cr2w", "-s", os.path.join(arc, name + ".xbm"), "-o", js)
        path = os.path.join(js, name + ".xbm.json")
        doc = json.load(open(path, encoding="utf-8"))
        rc = doc["Data"]["RootChunk"]
        assert (rc["width"], rc["height"]) == size, (name, rc["width"], rc["height"])
        rc["setup"]["isStreamable"] = 0
        rc["setup"]["allowTextureDowngrade"] = 1
        json.dump(doc, open(path, "w", encoding="utf-8"), indent=2)
        wk("cr2w", "-d", path, "-o", os.path.join(work, "final"))
        shutil.copy(os.path.join(work, "final", name + ".xbm"), os.path.join(OUT, name + ".xbm"))

    # 2. atlas, from the vanilla one: same slots, our textures
    wk("cr2w", "-s", os.path.join(vanilla, vanilla_dir, vanilla_atlas), "-o", js)
    doc = json.load(open(os.path.join(js, vanilla_atlas + ".json"), encoding="utf-8"))
    slots = doc["Data"]["RootChunk"]["slots"]["Elements"]
    assert len(slots) == len(names)
    for slot, name in zip(slots, names):
        slot["texture"]["DepotPath"] = {"$type": "ResourcePath", "$storage": "string",
                                        "$value": f"{DEPOT_DIR}\\{name}.xbm"}
        if part_name:
            part = slot["parts"][0]
            part["partName"] = {"$type": "CName", "$storage": "string", "$value": part_name}
            part["clippingRectInUVCoords"].update({"Left": 0, "Top": 0, "Right": 1, "Bottom": 1})
            slot["parts"] = [part]
            slot["slices"] = []
    doc["Header"]["ArchiveFileName"] = ""
    path = os.path.join(js, atlas + ".json")
    json.dump(doc, open(path, "w", encoding="utf-8"), indent=2)
    wk("cr2w", "-d", path, "-o", os.path.join(work, "final"))
    shutil.copy(os.path.join(work, "final", atlas), os.path.join(OUT, atlas))
    for f in sorted(os.listdir(OUT)):
        if f.startswith(names[0]) or f == atlas:
            print(os.path.join(OUT, f), os.path.getsize(os.path.join(OUT, f)))


if __name__ == "__main__":
    main()
