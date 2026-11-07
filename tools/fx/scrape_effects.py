"""Builds the scrape effects (docs/design/scrape-effects.md) from the game's own, so they can be made again after a
patch: each is a vanilla effect, unbundled from the game, serialized to JSON, edited and written back as CR2W into
src/wolvenkit/source/archive/user/jackhumbert/effects/ (the WolvenKit CLI round trip gives the vanilla files back byte
for byte). The plugin spawns them per scrape (src/red4ext/Physics/ScrapeEffects.cpp).

    python tools/fx/scrape_effects.py [--cli <WolvenKit.CLI.exe 9.x>] [--cli10 <WolvenKit.CLI.exe 10.x>] [--work <dir>]

- scrape_trail_hard, scrape_trail_metal: the tyres' sport skid mark (a particle trail drawn as a projected decal),
  moved to trail_decal_normal_color.mt with our own textures drawn from one groove height field: scrape_trail_d.xbm
  (a thruster's corners: two deep gouges near the edges, bright where cut through with dark rims, a hairline beside
  each, transparent between) and scrape_trail_n.xbm (their normals, so the gouges catch the light). The decal paints texture x particle colour x DiffuseColor over the ground
  at the texture's alpha (the shader in the game's cache), so the texture's brightness sets how light the mark can be;
  its normal map is tangent space in R and G, scaled by NormalStrength and written with the same alpha.
- scrape_sparks_hard, scrape_sparks_metal, scrape_sparks_glass: the vanilla scratch sparks, every emitter's birth rate
  scaled (metal the most).
The new files are tracked with git add -f (source/archive is gitignored, its files tracked).
"""

import argparse
import json
import os
import shutil
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
REPO = HERE.parent.parent
sys.path.insert(0, str(REPO / "tools"))
from common import paths  # noqa: E402

sys.path.insert(0, str(HERE))
import bc3  # noqa: E402

OUT = REPO / "src" / "wolvenkit" / "source" / "archive" / "user" / "jackhumbert" / "effects"
OUT_DEPOT = "user\\jackhumbert\\effects\\"
SKID = "base\\fx\\vehicles\\_wheels\\skid_marks\\sport\\v_skid_mark_s_m_01"
SPARKS = "base\\fx\\vehicles\\_damage\\scratches\\car_scratches_sparks"
SKID_TEX = "base\\fx\\_textures\\vehicles\\fx_skid_marks_radial_sport_1x2_d"
NORMAL_TEMPLATE = "base\\fx\\_textures\\decals\\impact_holes\\fx_bullet_hole_concrete_01_2x2_n"
SCRATCH_TEX = "user\\jackhumbert\\effects\\scrape_trail_d"
SCRATCH_NORMAL = "user\\jackhumbert\\effects\\scrape_trail_n"
TRAIL_MATERIAL = "base\\materials\\trail_decal_normal_color.mt"


def draw_scratch(path, normal_path, seed=7):
    """The trail textures: 512 square, two frames stacked (the skid mark's 1x2 layout: the trail runs along U, V
    crosses it). Each frame is a thruster's corners scraping: two deep gouges near the edges and a hairline beside each,
    transparent between, every one tiling along U. They are cut into a height field too, and its slopes are the
    normal map."""
    import numpy as np
    from PIL import Image

    rng = np.random.default_rng(seed)
    size, half = 512, 256
    u = np.arange(size) / size
    rgb = np.zeros((size, size, 3))
    alpha = np.zeros((size, size))
    height = np.zeros((size, size))

    def tiling_noise(octaves):
        out = np.zeros(size)
        for f in range(1, octaves + 1):
            out += np.sin(2 * np.pi * (f * u + rng.random())) / f
        return out / sum(1 / f for f in range(1, octaves + 1))

    for frame in range(2):
        v = np.arange(half) / half
        a = np.zeros((half, size))
        h = np.zeros((half, size))
        # the colour under alpha 0 too: filtering and the mips average colour without alpha, so a grey there (and dark
        # rims) bled into the gouges, and from a car's distance they read darker than the road
        col = np.full((half, size, 3), 222.0)
        # a thruster's two corners, near the trail's edges: each a deep gouge (bright where cut through, a little darker
        # along its rims) with a hairline beside it, and nothing between them
        for centre, width in ((rng.uniform(0.17, 0.23), 0.03), (rng.uniform(0.77, 0.83), 0.034),
                              (rng.uniform(0.24, 0.27), 0.008), (rng.uniform(0.72, 0.75), 0.008)):
            # wanders a little across the trail and breaks up along it now and then
            wander = 0.008 * tiling_noise(4)
            strength = np.clip(0.85 + 0.6 * tiling_noise(6), 0, 1)
            d = np.abs(v[:, None] - centre - wander[None, :]) / width
            g = np.clip(1 - d, 0, 1) ** 0.6 * strength[None, :]
            rim = np.clip(1 - np.abs(d - 1.15) / 0.35, 0, 1) * strength[None, :]
            col = col * (1 - rim[..., None]) + 165 * rim[..., None]
            col = col * (1 - g[..., None]) + rng.uniform(215, 235) * g[..., None]
            a = np.maximum(a, np.maximum(g, 0.7 * rim))
            # cut in, with a burr thrown up beside it
            burr = np.clip(1 - np.abs(d - 1.4) / 0.6, 0, 1) * strength[None, :]
            h += -g + 0.35 * burr
        rows = slice(frame * half, (frame + 1) * half)
        rgb[rows] = col
        alpha[rows] = a
        height[rows] = h
    img = np.dstack([rgb, alpha * 255]).clip(0, 255).astype("uint8")
    Image.fromarray(img, "RGBA").save(path)
    # slopes per pixel to a tangent-space normal in R (along U) and G (along V); flat is 128
    dv, du = np.gradient(height)
    nx, ny = -du * 2.5, -dv * 2.5
    length = np.maximum(1.0, np.hypot(nx, ny) / 0.9)
    nx, ny = nx / length, ny / length
    n = np.dstack([128 + 127 * nx, 128 + 127 * ny, np.zeros((size, size)), np.full((size, size), 255.0)])
    Image.fromarray(n.clip(0, 255).astype("uint8"), "RGBA").save(normal_path)


def build_textures(cli10, raw, work):
    """Draws the trail textures and imports each over a vanilla one with the settings it needs: the colour over the
    skid mark's (DXT5, gamma), the normals over a bullet hole's (BC5 normal map, linear)."""
    tex_raw, arch = work / "tex_raw", work / "tex_archive"
    pairs = [(SCRATCH_TEX, SKID_TEX), (SCRATCH_NORMAL, NORMAL_TEMPLATE)]

    def local(depot):
        return depot.replace("\\", os.sep)

    for ours, template in pairs:
        for d in (tex_raw, arch):
            (d / local(ours)).parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(raw / f"{template}.xbm", arch / f"{local(ours)}.xbm")
    draw_scratch(tex_raw / f"{local(SCRATCH_TEX)}.png", tex_raw / f"{local(SCRATCH_NORMAL)}.png")
    # exits 3 ("no buffers found to rebuild") even when it imported: check the files instead
    subprocess.run([str(cli10), "import", str(tex_raw), "-k", "-o", str(arch)], stdout=subprocess.DEVNULL)
    for ours, template in pairs:
        out = arch / f"{local(ours)}.xbm"
        if out.read_bytes() == (raw / f"{template}.xbm").read_bytes():
            sys.exit(f"texture import changed nothing: {ours}")
    # the colour's data written again, not premultiplied (bc3.py: the import's premultiply darkened the gouges)
    colour = arch / f"{local(SCRATCH_TEX)}.xbm"
    js = work / "tex_json"
    js.mkdir(parents=True, exist_ok=True)
    subprocess.run([str(cli10), "cr2w", "-s", str(colour), "-o", str(js)], check=True, stdout=subprocess.DEVNULL)
    from PIL import Image
    import numpy as np
    rgba = np.asarray(Image.open(tex_raw / f"{local(SCRATCH_TEX)}.png").convert("RGBA"))
    bc3.replace_data(js / f"{colour.name}.json", rgba)
    subprocess.run([str(cli10), "cr2w", "-d", str(js / f"{colour.name}.json"), "-o", str(colour.parent)], check=True,
                   stdout=subprocess.DEVNULL)
    for ours, _ in pairs:
        out = arch / f"{local(ours)}.xbm"
        shutil.copyfile(out, OUT / out.name)
        print(f"{ours}.xbm")


def trail(color, size, life, normal_strength):
    def edit(node):
        t = node.get("$type") if isinstance(node, dict) else None
        if t == "CMaterialInstance":
            node["baseMaterial"]["DepotPath"]["$value"] = TRAIL_MATERIAL
            values = node.setdefault("values", [])
            for v in values:
                if "DiffuseColor" in v:
                    v["DiffuseColor"].update({"Red": color[0], "Green": color[1], "Blue": color[2], "Alpha": color[3]})
                if "DiffuseTexture" in v:
                    v["DiffuseTexture"]["DepotPath"]["$value"] = f"{SCRATCH_TEX}.xbm"
            values.append({"$type": "rRef:ITexture", "NormalTexture": {
                "DepotPath": {"$type": "ResourcePath", "$storage": "string", "$value": f"{SCRATCH_NORMAL}.xbm"},
                "Flags": "Default"}})
            values.append({"$type": "Float", "NormalStrength": normal_strength})
        elif t == "CParticleInitializerSize":
            node["scale"] = node["scale"] * size
        elif t == "CParticleInitializerLifeTime":
            data = node["lifeTime"]["Data"]
            if data.get("$type") == "CEvaluatorFloatConst":
                data["value"] = life
    return edit


def birth(scale):
    def edit(node):
        if isinstance(node, dict):
            for k, v in node.items():
                if k == "birthRate" and isinstance(v, dict) and v.get("Data", {}).get("$type") == "CEvaluatorFloatConst":
                    v["Data"]["value"] = v["Data"]["value"] * scale
                elif k == "birthRate" and isinstance(v, list):
                    node[k] = [x * scale for x in v]
    return edit


# name: (vanilla source without extension, edit)
EFFECTS = {
    "scrape_trail_hard": (SKID, trail((235, 232, 225, 255), 1.0, 45.0, 1.0)),
    "scrape_trail_metal": (SKID, trail((225, 230, 240, 255), 0.8, 45.0, 1.0)),
    "scrape_sparks_hard": (SPARKS, birth(1.3)),
    "scrape_sparks_metal": (SPARKS, birth(2.5)),
    "scrape_sparks_glass": (SPARKS, birth(0.35)),
}


def walk(node, edit):
    edit(node)
    if isinstance(node, dict):
        for v in node.values():
            walk(v, edit)
    elif isinstance(node, list):
        for v in node:
            walk(v, edit)


def run(cli, *args):
    subprocess.run([str(cli), *args], check=True, stdout=subprocess.DEVNULL)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--cli", default=os.environ.get("WOLVENKIT_CLI", r"D:\cp2077_work\build\tools\wolvenkit-9.0.1\WolvenKit.CLI.exe"))
    ap.add_argument("--cli10", default=os.environ.get("WOLVENKIT_CLI10", r"D:\cp2077_work\build\tools\wolvenkit-10.0.0-beta.1\WolvenKit.CLI.exe"))
    ap.add_argument("--work", default=os.path.join(paths.TMP, "scrape_effects"))
    args = ap.parse_args()
    work = Path(args.work)
    raw, js = work / "raw", work / "json"
    for d in (raw, js):
        d.mkdir(parents=True, exist_ok=True)
    sources = sorted({src for src, _ in EFFECTS.values()})
    regex = "|".join(s.replace("\\", ".") + r"\.(effect|particle)$" for s in sources)
    regex += "|" + "|".join(t.replace("\\", ".") + r"\.xbm$" for t in (SKID_TEX, NORMAL_TEMPLATE))
    run(args.cli, "unbundle", os.path.join(paths.ARCHIVES, "content"), "-o", str(raw), "-r", regex)
    for src in sources:
        for ext in ("effect", "particle"):
            run(args.cli, "cr2w", "-s", str(raw / f"{src}.{ext}"), "-o", str(js))
    OUT.mkdir(parents=True, exist_ok=True)
    build_textures(args.cli10, raw, work)
    for name, (src, edit) in EFFECTS.items():
        stem = src.split("\\")[-1]
        # the effect only: it embeds its particle system under the vanilla name and draws that copy; pointed at a
        # particle file of its own instead, it drew nothing in game
        for ext in ("effect",):
            data = json.loads((js / f"{stem}.{ext}.json").read_text(encoding="utf-8"))
            walk(data, edit)
            out_json = work / f"{name}.{ext}.json"
            out_json.write_text(json.dumps(data, indent=2), encoding="utf-8")
            run(args.cli, "cr2w", "-d", str(out_json), "-o", str(work))
            shutil.copyfile(work / f"{name}.{ext}", OUT / f"{name}.{ext}")
            print(f"{OUT_DEPOT}{name}.{ext}")
    shutil.rmtree(work, ignore_errors=True)


if __name__ == "__main__":
    main()
