r"""The landing pad number signs for the mod archive (docs/design/landing-pads.md, "Pad numbers"): the vanilla
"28" sign of landing_pad_b_01.mesh with its digits taken out (the ring), and one sign per digit 0 to 9, drawn in
the vanilla sign's pixel style (glyphs 22 x 57 px, 6 to 7 px strokes, 10 px bars; the 2 and 8 are the vanilla
ones). All are 128 x 128 like the vanilla sign (its diode_pavements material draws it as an LED grid, so it is low
resolution), the digit centred: a digit is the vanilla quad moved sideways (pad_numbers.py places them).

Each PNG is imported into a copy of the vanilla sign texture (helipad_b_01.xbm, `import --keep`, so it keeps that
texture's setup), and each texture gets a mesh: a copy of landing_pad_b_01.mesh (the same 5 x 5 m quad and
diode_pavements material) whose SignTexture is ours. Output: OUT_DIR in the archive source (pack it with
tools/packaging/pack_archive.py).

  python pad_number_assets.py --work <scratch dir> [--preview sheet.png]

The vanilla files come from the unbundled game files (the DEPOT and DEPOT_TEX settings).
"""

import argparse
import json
import os
import shutil
import subprocess
import sys

import numpy as np
from PIL import Image, ImageDraw

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))  # tools/, for common
from common import paths  # noqa: E402

HERE = os.path.dirname(os.path.abspath(__file__))
DEPOT_DIR = r"user\jackhumbert\pads"
OUT_DIR = os.path.join(HERE, "..", "..", "src", "wolvenkit", "source", "archive", *DEPOT_DIR.split("\\"))
SIGNAGES = r"base\environment\decoration\public_utility\signages"
VANILLA_MESH = SIGNAGES + r"\landing_pad\landing_pad_b_01.mesh"
VANILLA_XBM = SIGNAGES + r"\textures\helipad_b_01.xbm"
VANILLA_PNG = SIGNAGES + r"\landing_pad\landing_pad_b_01.mesh_embedded\14509486360423988535.png"
RING = "ltbf_pad_ring"
DIGIT = "ltbf_pad_digit_{}"

SIZE = 128
COLOR = (255, 190, 82)
W, H = 22, 57  # a glyph
# the vanilla 2 and 8 (x 35..56 and 72..93, y 35..91 in the vanilla sign; the pair is centred on x 64)
VANILLA_GLYPHS = {2: 35, 8: 72}
GLYPH_X, GLYPH_Y = (SIZE - W) // 2, 35


def depot_path(name, ext):
    return DEPOT_DIR + "\\" + name + ext


def glyph(d):
    """Digit d as a 22 x 57 mask in the vanilla style: top bar 10 px, bottom bar 11, verticals 6 to 7, middle bar
    from y 21 to 29."""
    sc = 8
    im = Image.new("L", (W * sc, H * sc), 0)
    g = ImageDraw.Draw(im)

    def r(x0, y0, x1, y1):
        g.rectangle([x0 * sc, y0 * sc, x1 * sc - 1, y1 * sc - 1], fill=255)

    top, bot, mid0, mid1, v = 10, 46, 21, 29, 6.5
    if d == 0:
        r(0, 0, W, top); r(0, bot, W, H); r(0, 0, v, H); r(W - v, 0, W, H)
    elif d == 1:
        r(8, 0, 15, H); r(2, 0, 15, 8)
    elif d == 3:
        r(0, 0, W, top); r(0, bot, W, H); r(W - v, 0, W, H); r(5, mid0, W, mid1)
    elif d == 4:
        r(0, 0, v, mid1); r(W - v, 0, W, H); r(0, mid0, W, mid1)
    elif d == 5:
        r(0, 0, W, top); r(0, 0, v, mid1); r(0, mid0, W, mid1); r(W - v, mid0, W, H); r(0, bot, W, H)
    elif d == 6:
        r(0, 0, W, top); r(0, 0, v, H); r(0, mid0, W, mid1); r(W - v, mid0, W, H); r(0, bot, W, H)
    elif d == 7:
        r(0, 0, W, top)
        g.polygon([(x * sc, y * sc) for x, y in ((W - 7, top), (W, top), (9, H), (2, H))], fill=255)
    elif d == 9:
        r(0, 0, W, top); r(0, 0, v, mid1); r(W - v, 0, W, H); r(0, mid0, W, mid1); r(0, bot, W, H)
    return np.array(im.resize((W, H), Image.BOX)) >= 128


def to_image(mask):
    out = np.zeros((SIZE, SIZE, 4), np.uint8)
    out[mask, :3] = COLOR
    out[mask, 3] = 255
    return Image.fromarray(out, "RGBA")


def signs(vanilla_png):
    """{name: RGBA image} for the ring and the ten digits."""
    on = np.array(Image.open(vanilla_png).convert("RGBA"))[..., 3] > 0
    yy, xx = np.mgrid[:SIZE, :SIZE]
    out = {RING: to_image(on & (np.hypot(xx - 63.5, yy - 63.5) >= 45))}
    for d in range(10):
        m = on[GLYPH_Y:GLYPH_Y + H, VANILLA_GLYPHS[d]:VANILLA_GLYPHS[d] + W] if d in VANILLA_GLYPHS else glyph(d)
        full = np.zeros((SIZE, SIZE), bool)
        full[GLYPH_Y:GLYPH_Y + H, GLYPH_X:GLYPH_X + W] = m
        out[DIGIT.format(d)] = to_image(full)
    return out


def wk(*args):
    r = subprocess.run([paths.WOLVENKIT, *args], capture_output=True, text=True)
    # `import --keep` exits 3 ("No buffers found to rebuild") even when every file was imported
    imported = args[0] == "import" and "Imported" in r.stdout
    if r.returncode != 0 and not (imported and r.returncode == 3):
        raise SystemExit(f"WolvenKit {args[0]} failed ({r.returncode}):\n{r.stdout[-2000:]}\n{r.stderr[-2000:]}")
    return r.stdout


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--work", required=True, help="scratch folder (emptied first)")
    ap.add_argument("--preview")
    args = ap.parse_args()
    work = os.path.abspath(args.work)
    shutil.rmtree(work, ignore_errors=True)
    raw, arc, js = (os.path.join(work, d) for d in ("raw", "archive", "json"))
    sub = DEPOT_DIR.split("\\")
    for d in (raw, arc):
        os.makedirs(os.path.join(d, *sub))
    os.makedirs(js)

    images = signs(os.path.join(paths.DEPOT_TEX, VANILLA_PNG))
    for name, im in images.items():
        im.save(os.path.join(raw, *sub, name + ".png"))
        shutil.copy(os.path.join(paths.DEPOT, VANILLA_XBM), os.path.join(arc, *sub, name + ".xbm"))
    wk("import", raw, "-k", "-o", arc)

    # the meshes: the vanilla one with our texture in place of its embedded one
    wk("cr2w", "-s", os.path.join(paths.DEPOT, VANILLA_MESH), "-o", js)
    template = json.load(open(os.path.join(js, "landing_pad_b_01.mesh.json"), encoding="utf-8"))
    for name in images:
        doc = json.loads(json.dumps(template))
        doc["Data"]["EmbeddedFiles"] = []
        done = 0
        for mat in doc["Data"]["RootChunk"]["preloadLocalMaterialInstances"]:
            for value in mat["Data"]["values"]:
                if "SignTexture" in value:
                    value["SignTexture"]["DepotPath"]["$value"] = depot_path(name, ".xbm")
                    value["SignTexture"]["Flags"] = "Default"
                    done += 1
        if done != 1:
            raise SystemExit(f"the vanilla mesh has {done} SignTexture values, not 1: its layout changed")
        with open(os.path.join(js, name + ".mesh.json"), "w", encoding="utf-8") as f:
            json.dump(doc, f, indent=1)
    wk("cr2w", "-d", *(os.path.join(js, name + ".mesh.json") for name in images), "-o", os.path.join(arc, *sub))

    os.makedirs(OUT_DIR, exist_ok=True)
    for name in images:
        for ext in (".xbm", ".mesh"):
            src = os.path.join(arc, *sub, name + ext)
            if not os.path.exists(src):
                raise SystemExit(f"not made: {src}")
            shutil.copy(src, os.path.join(OUT_DIR, name + ext))
    if args.preview:
        sheet = Image.new("RGBA", (SIZE * 6, SIZE * 2), (40, 42, 46, 255))
        for i, im in enumerate(images.values()):
            sheet.alpha_composite(im, ((i % 6) * SIZE, (i // 6) * SIZE))
        sheet.resize((sheet.width * 2, sheet.height * 2), Image.NEAREST).save(args.preview)
    print(f"wrote {len(images)} textures and meshes to {os.path.normpath(OUT_DIR)}")


if __name__ == "__main__":
    main()
