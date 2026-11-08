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
- scrape_trail_metal: the same, drawn from scrape_trail_metal_d.xbm: the gouges with a wide dark scuff along each, so
  they show on a light metal plate.
- scrape_sparks_hard, scrape_sparks_metal, scrape_sparks_glass: the vanilla scratch sparks, every emitter's birth rate
  scaled (metal the most, and hotter).
- scrape_dust_soft: the vanilla concrete scratch with its sparks off and its dust tan, thicker and more of it.
- scrape_trail_soft: a rut, not a gouge (scrape_rut_d.xbm, scrape_rut_n.xbm): a trough of damp earth with berms of
  turned soil either side. scrape_trail_grass: a streak of flattened turf torn to earth now and then
  (scrape_grass_d.xbm). scrape_turf_grass: the game's tyre spray on grass, driven by the scrape (tyre()).
- The sparks and the dust take the scrape's inputs (INPUTS, driven()): their emission scales with its force, their
  velocity and size follow its speed and press. The trails come in three depths each (DEPTHS: _light, as named,
  _deep), picked by its press: a parameter on a trail changed the whole line already laid.
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
CONCRETE = "base\\fx\\vehicles\\_damage\\scratches\\car_scratches_concrete"
SKID_TEX = "base\\fx\\_textures\\vehicles\\fx_skid_marks_radial_sport_1x2_d"
NORMAL_TEMPLATE = "base\\fx\\_textures\\decals\\impact_holes\\fx_bullet_hole_concrete_01_2x2_n"
SCRATCH_TEX = "user\\jackhumbert\\effects\\scrape_trail_d"
SCRATCH_METAL_TEX = "user\\jackhumbert\\effects\\scrape_trail_metal_d"
SCRATCH_NORMAL = "user\\jackhumbert\\effects\\scrape_trail_n"
SOFT_TEX = "user\\jackhumbert\\effects\\scrape_rut_d"
SOFT_NORMAL = "user\\jackhumbert\\effects\\scrape_rut_n"
GRASS_TEX = "user\\jackhumbert\\effects\\scrape_grass_d"
GRASS_TYRE = "base\\fx\\vehicles\\_wheels\\_surface_fx\\grass\\v_tire_grass_m_01"
TRAIL_MATERIAL = "base\\materials\\trail_decal_normal_color.mt"


def draw_scratch(path, normal_path=None, seed=7, rim=(165, 1.15, 0.35, 0.7), fill=222.0):
    """The trail textures: 512 square, two frames stacked (the skid mark's 1x2 layout: the trail runs along U, V
    crosses it). Each frame is a thruster's corners scraping: two deep gouges near the edges and a hairline beside each,
    transparent between, every one tiling along U. They are cut into a height field too, and its slopes are the
    normal map (written when normal_path is given). rim is the gouges' rims: their colour, centre and half width (in
    gouge widths from the gouge's middle) and alpha; fill the colour under alpha 0."""
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
        col = np.full((half, size, 3), float(fill))
        # a thruster's two corners, near the trail's edges: each a deep gouge (bright where cut through, a little darker
        # along its rims) with a hairline beside it, and nothing between them
        for centre, width in ((rng.uniform(0.17, 0.23), 0.03), (rng.uniform(0.77, 0.83), 0.034),
                              (rng.uniform(0.24, 0.27), 0.008), (rng.uniform(0.72, 0.75), 0.008)):
            # wanders a little across the trail and breaks up along it now and then
            wander = 0.008 * tiling_noise(4)
            strength = np.clip(0.85 + 0.6 * tiling_noise(6), 0, 1)
            d = np.abs(v[:, None] - centre - wander[None, :]) / width
            g = np.clip(1 - d, 0, 1) ** 0.6 * strength[None, :]
            r = np.clip(1 - np.abs(d - rim[1]) / rim[2], 0, 1) * strength[None, :]
            col = col * (1 - r[..., None]) + rim[0] * r[..., None]
            col = col * (1 - g[..., None]) + rng.uniform(215, 235) * g[..., None]
            a = np.maximum(a, np.maximum(g, rim[3] * r))
            # cut in, with a burr thrown up beside it
            burr = np.clip(1 - np.abs(d - 1.4) / 0.6, 0, 1) * strength[None, :]
            h += -g + 0.35 * burr
        rows = slice(frame * half, (frame + 1) * half)
        rgb[rows] = col
        alpha[rows] = a
        height[rows] = h
    img = np.dstack([rgb, alpha * 255]).clip(0, 255).astype("uint8")
    Image.fromarray(img, "RGBA").save(path)
    if not normal_path:
        return
    # slopes per pixel to a tangent-space normal in R (along U) and G (along V); flat is 128
    dv, du = np.gradient(height)
    nx, ny = -du * 2.5, -dv * 2.5
    length = np.maximum(1.0, np.hypot(nx, ny) / 0.9)
    nx, ny = nx / length, ny / length
    n = np.dstack([128 + 127 * nx, 128 + 127 * ny, np.zeros((size, size)), np.full((size, size), 255.0)])
    Image.fromarray(n.clip(0, 255).astype("uint8"), "RGBA").save(normal_path)


def tiling_noise_2d(rng, half, size, terms):
    """Noise over a frame (half rows across the trail, size columns along it) that tiles along U: a sum of waves, each
    a whole number of times along U. terms: (along, across, amplitude) per wave, along an int, across in cycles."""
    import numpy as np
    u = np.arange(size)[None, :] / size
    v = np.arange(half)[:, None] / half
    out = np.zeros((half, size))
    for along, across, amp in terms:
        out += amp * np.sin(2 * np.pi * (along * u + across * v + rng.random()))
    return out / max(1e-6, sum(abs(a) for _, _, a in terms))


def normals_from(height, path, gain=2.5):
    """A height field's slopes as a tangent-space normal map in R (along U) and G (along V); flat is 128."""
    import numpy as np
    from PIL import Image
    size = height.shape[0]
    dv, du = np.gradient(height)
    nx, ny = -du * gain, -dv * gain
    length = np.maximum(1.0, np.hypot(nx, ny) / 0.9)
    nx, ny = nx / length, ny / length
    n = np.dstack([128 + 127 * nx, 128 + 127 * ny, np.zeros((size, size)), np.full((size, size), 255.0)])
    Image.fromarray(n.clip(0, 255).astype("uint8"), "RGBA").save(path)


def draw_rut(path, normal_path, seed=11):
    """Soft ground (dirt, sand, mud): a pod dragged through it ploughs a rut, not a gouge. A broad trough of dark,
    damp turned earth with soft edges, and a berm of drier, lighter soil thrown up along each side, broken into clods;
    every part tiles along U. Its height field (trough down, berms up, clods) is the normal map. The skid mark's 1x2
    layout, as draw_scratch."""
    import numpy as np
    from PIL import Image

    rng = np.random.default_rng(seed)
    size, half = 512, 256
    rgb = np.zeros((size, size, 3))
    alpha = np.zeros((size, size))
    height = np.zeros((size, size))
    v = np.arange(half)[:, None] / half
    for frame in range(2):
        wander = 0.02 * tiling_noise_2d(rng, 1, size, [(1, 0, 1), (2, 0, 0.5), (3, 0, 0.3)])
        off = np.abs(v - 0.5 - wander)
        grain = tiling_noise_2d(rng, half, size, [(f, rng.uniform(4, 30), 1 / (1 + i)) for i, f in
                                                 enumerate((3, 5, 8, 13, 21, 34))])
        # lumps at every angle (few waves made diagonal ticks)
        clods = np.clip(tiling_noise_2d(rng, half, size, [(int(rng.integers(6, 30)), rng.uniform(-24, 24), 1.0)
                                                         for _ in range(14)]) * 2.4 - 0.25, 0, 1)
        trough = np.clip(1 - (off / 0.28) ** 2, 0, 1)
        berm = np.clip(1 - np.abs(off - 0.37) / 0.11, 0, 1) ** 1.2
        # damp earth in the trough, drier soil on the berms, darker clods over them
        col = np.full((half, size, 3), (100.0, 82.0, 62.0))
        floor = np.array((72.0, 57.0, 42.0))[None, None, :] + 14.0 * grain[..., None]
        bank = np.array((138.0, 114.0, 86.0))[None, None, :] + 18.0 * grain[..., None]
        col = col * (1 - trough[..., None]) + floor * trough[..., None]
        col = col * (1 - berm[..., None]) + bank * berm[..., None]
        lump = (clods * berm)[..., None]
        col = col * (1 - 0.6 * lump) + np.array((62.0, 49.0, 36.0)) * 0.6 * lump
        a = np.clip(np.maximum(0.9 * trough ** 0.5, 0.75 * berm) * (0.85 + 0.15 * grain), 0, 1)
        h = -trough + 0.55 * berm + 0.2 * grain * (trough + berm) + 0.35 * clods * berm
        rows = slice(frame * half, (frame + 1) * half)
        rgb[rows], alpha[rows], height[rows] = col, a, h
    Image.fromarray(np.dstack([rgb, alpha * 255]).clip(0, 255).astype("uint8"), "RGBA").save(path)
    normals_from(height, normal_path, gain=3.0)


def draw_grass(path, seed=23):
    """Grass: a pod dragged over turf flattens it rather than cutting it. A broad, soft-edged band of grass laid along
    the slide (fine streaks along U: the blades bent one way, light where they catch the sky, dark where crushed),
    with the turf torn through to earth under the pod's two corners now and then; every part tiles along U. Drawn in
    colour, so the trail's own colour is near white."""
    import numpy as np
    from PIL import Image

    rng = np.random.default_rng(seed)
    size, half = 512, 256
    rgb = np.zeros((size, size, 3))
    alpha = np.zeros((size, size))
    v = np.arange(half)[:, None] / half
    for frame in range(2):
        wander = 0.015 * tiling_noise_2d(rng, 1, size, [(1, 0, 1), (2, 0, 0.6)])
        off = np.abs(v - 0.5 - wander)
        band = np.clip(1 - (off / 0.36) ** 4, 0, 1)
        # blades: many thin waves across, few along, so they run with the slide
        blades = tiling_noise_2d(rng, half, size, [(rng.integers(1, 5), rng.uniform(40, 130), 1.0) for _ in range(9)])
        patches = tiling_noise_2d(rng, half, size, [(f, rng.uniform(2, 8), 1.0) for f in (2, 3, 5)])
        sheen = np.clip(0.5 + 0.9 * blades, 0, 1)[..., None]
        laid = np.array((38.0, 56.0, 24.0)) * (1 - sheen) + np.array((150.0, 166.0, 96.0)) * sheen
        laid = laid * (0.9 + 0.12 * patches[..., None])
        col = np.full((half, size, 3), (88.0, 104.0, 58.0))
        col = col * (1 - band[..., None]) + laid * band[..., None]
        a = band * np.clip(0.68 + 0.2 * np.abs(blades) + 0.1 * patches, 0, 1)
        # torn through to earth under the pod's corners, here and there along the trail
        for centre in (rng.uniform(0.26, 0.3), rng.uniform(0.7, 0.74)):
            along = np.clip(tiling_noise_2d(rng, 1, size, [(f, 0, 1.0 / f) for f in (1, 2, 3)]) * 2.2 + 0.2, 0, 1)
            tear = np.clip(1 - np.abs(v - centre - wander) / 0.055, 0, 1) ** 0.6 * along
            col = col * (1 - tear[..., None]) + np.array((84.0, 66.0, 48.0)) * tear[..., None]
            a = np.maximum(a, 0.9 * tear)
        rows = slice(frame * half, (frame + 1) * half)
        rgb[rows], alpha[rows] = col, a
    Image.fromarray(np.dstack([rgb, alpha * 255]).clip(0, 255).astype("uint8"), "RGBA").save(path)


def build_textures(cli10, raw, work):
    """Draws the trail textures and imports each over a vanilla one with the settings it needs: the colour over the
    skid mark's (DXT5, gamma), the normals over a bullet hole's (BC5 normal map, linear)."""
    tex_raw, arch = work / "tex_raw", work / "tex_archive"
    pairs = [(SCRATCH_TEX, SKID_TEX), (SCRATCH_METAL_TEX, SKID_TEX), (SCRATCH_NORMAL, NORMAL_TEMPLATE),
             (SOFT_TEX, SKID_TEX), (SOFT_NORMAL, NORMAL_TEMPLATE), (GRASS_TEX, SKID_TEX)]
    colours = (SCRATCH_TEX, SCRATCH_METAL_TEX, SOFT_TEX, GRASS_TEX)

    def local(depot):
        return depot.replace("\\", os.sep)

    for ours, template in pairs:
        for d in (tex_raw, arch):
            (d / local(ours)).parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(raw / f"{template}.xbm", arch / f"{local(ours)}.xbm")
    draw_scratch(tex_raw / f"{local(SCRATCH_TEX)}.png", tex_raw / f"{local(SCRATCH_NORMAL)}.png")
    # metal: the same gouges (one normal map for both), bright where cut with a wide dark scuff along each (paint and
    # grime dragged along), so they show on a light plate where the light trail did not
    draw_scratch(tex_raw / f"{local(SCRATCH_METAL_TEX)}.png", rim=(55, 1.6, 0.8, 0.85), fill=200.0)
    draw_rut(tex_raw / f"{local(SOFT_TEX)}.png", tex_raw / f"{local(SOFT_NORMAL)}.png")
    draw_grass(tex_raw / f"{local(GRASS_TEX)}.png")
    # exits 3 ("no buffers found to rebuild") even when it imported: check the files instead
    subprocess.run([str(cli10), "import", str(tex_raw), "-k", "-o", str(arch)], stdout=subprocess.DEVNULL)
    for ours, template in pairs:
        out = arch / f"{local(ours)}.xbm"
        if out.read_bytes() == (raw / f"{template}.xbm").read_bytes():
            sys.exit(f"texture import changed nothing: {ours}")
    # the colours' data written again, not premultiplied (bc3.py: the import's premultiply darkened the gouges)
    from PIL import Image
    import numpy as np
    js = work / "tex_json"
    js.mkdir(parents=True, exist_ok=True)
    for tex in colours:
        colour = arch / f"{local(tex)}.xbm"
        subprocess.run([str(cli10), "cr2w", "-s", str(colour), "-o", str(js)], check=True, stdout=subprocess.DEVNULL)
        rgba = np.asarray(Image.open(tex_raw / f"{local(tex)}.png").convert("RGBA"))
        bc3.replace_data(js / f"{colour.name}.json", rgba)
        subprocess.run([str(cli10), "cr2w", "-d", str(js / f"{colour.name}.json"), "-o", str(colour.parent)],
                       check=True, stdout=subprocess.DEVNULL)
    for ours, _ in pairs:
        out = arch / f"{local(ours)}.xbm"
        shutil.copyfile(out, OUT / out.name)
        print(f"{ours}.xbm")


def trail(color, size, life, normal_strength, texture=SCRATCH_TEX, normal=SCRATCH_NORMAL):
    def edit(node):
        t = node.get("$type") if isinstance(node, dict) else None
        if t == "CMaterialInstance":
            node["baseMaterial"]["DepotPath"]["$value"] = TRAIL_MATERIAL
            values = node.setdefault("values", [])
            for v in values:
                if "DiffuseColor" in v:
                    v["DiffuseColor"].update({"Red": color[0], "Green": color[1], "Blue": color[2], "Alpha": color[3]})
                if "DiffuseTexture" in v:
                    v["DiffuseTexture"]["DepotPath"]["$value"] = f"{texture}.xbm"
            values.append({"$type": "rRef:ITexture", "NormalTexture": {
                "DepotPath": {"$type": "ResourcePath", "$storage": "string", "$value": f"{normal}.xbm"},
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


def emitters(**kinds):
    """An edit per kind of emitter, told apart by its material's texture: sparks (the spark streaks), smoke (the
    puffs) and chunks (the mesh debris): kinds=dict(sparks=(birth scale, {material value: new value}), ...). Birth and
    material values are what an effect keeps unbaked; its size and colour curves are baked into each emitter's render
    blob."""
    def kind_of(emitter):
        text = json.dumps(emitter.get("localMaterialInstance", {}).get("Data", {}).get("values", []))
        if "sparks" in text:
            return "sparks"
        if "smoke" in text:
            return "smoke"
        return "chunks"

    def edit(node):
        if not (isinstance(node, dict) and node.get("$type") == "CParticleEmitter"):
            return
        kind = kinds.get(kind_of(node))
        if not kind:
            return
        scale, values = kind
        for lod in node.get("lods", []):
            data = lod.get("birthRate", {}).get("Data", {})
            if data.get("$type") == "CEvaluatorFloatConst":
                data["value"] = data["value"] * scale
        for v in node.get("localMaterialInstance", {}).get("Data", {}).get("values", []):
            for name, value in values.items():
                if name in v:
                    if isinstance(value, tuple):
                        v[name].update({"Red": value[0], "Green": value[1], "Blue": value[2], "Alpha": 255})
                    else:
                        v[name] = value
    return edit


# The scrape's inputs, set on each live effect every frame (Physics/ScrapeEffects.cpp, Flight::FxInputsOf): force, how
# much it throws (0..2, 1 the effect as drawn); press, its load (0..1 of a full load); speed, its slide (0..1 of 12
# m/s).
INPUTS = ("scrape_force", "scrape_press", "scrape_speed")


def cname(value):
    return {"$type": "CName", "$storage": "string", "$value": value}


def curve(points):
    return {"$type": "CEvaluatorFloatCurve", "curves": {
        "InterpolationType": "Linear", "LinkType": "ESLT_Normal",
        "Elements": [{"Point": p, "Value": v} for p, v in points]}, "numberOfCurveSamples": 16}


def driven(inner, velocity=((0.0, 0.5), (1.0, 1.6)), size=((0.0, 0.7), (1.0, 1.4))):
    """inner, then the effect takes the scrape's inputs: its particles' emission is multiplied by force (keeping the
    effect's own emission over time, which fades the sparks out once the loop is broken), and their velocity and size
    are curves of speed and press, as the game's vehicle effects map veh_impact_force and veh_speed (the parameter the
    curve's input, not a multiplier)."""
    def edit(node):
        inner(node)
        if not isinstance(node, dict):
            return
        if node.get("$type") == "worldEffect":
            node["inputParameterNames"] = [cname(n) for n in INPUTS]
        elif node.get("$type") == "effectTrackItemParticles":
            node["emissionScale"]["inputParameterOverride"] = cname("scrape_force")
            node["emissionScale"]["inputParameterIsPostMultiplier"] = 1
            for field, name, points in (("velocity", "scrape_speed", velocity), ("size", "scrape_press", size)):
                node[field]["evaluator"]["Data"] = curve(points)
                node[field]["inputParameterOverride"] = cname(name)
                node[field]["inputParameterIsPostMultiplier"] = 0
    return edit


# A trail's depths, light to deep (Flight::TrailDepth picks one by the scrape's press; a change starts a new segment):
# its width, its colour's alpha and its normals' strength, each a factor on the trail's own. A parameter on the trail
# (its alpha or size) changed the whole line already laid, not what was laid after it, and none of the game's skid
# marks take one: they come in sizes instead.
DEPTHS = (("_light", 0.65, 0.55, 0.6), ("", 1.0, 1.0, 1.0), ("_deep", 1.4, 1.0, 1.4))


def trails(name, color, size, life, normal_strength, texture=SCRATCH_TEX, normal=SCRATCH_NORMAL):
    """The trail called name at each depth (DEPTHS): {name + suffix: (SKID, edit)}."""
    return {name + suffix: (SKID, trail((color[0], color[1], color[2], round(color[3] * alpha)), size * width, life,
                                        normal_strength * normals, texture, normal))
            for suffix, width, alpha, normals in DEPTHS}


# the game's tyre effects' inputs, and the scrape's that stand for them
TYRE_INPUTS = {"veh_wheel_angular_speed": "scrape_speed", "veh_speed": "scrape_speed", "veh_total_slip": "scrape_press",
               "veh_lat_slip": "scrape_press", "veh_long_slip": "scrape_press"}


def tyre(inner=lambda node: None):
    """A tyre surface effect (made to run under a sliding wheel, its emission, velocity and size curves of the wheel's
    speed and slip, 0..1) driven by the scrape instead: its inputs renamed to the scrape's (TYRE_INPUTS), the curves
    kept."""
    def edit(node):
        inner(node)
        if not isinstance(node, dict):
            return
        if node.get("$type") == "worldEffect":
            node["inputParameterNames"] = [cname(n) for n in INPUTS]
        elif node.get("$type", "").startswith("effectEffectParameterEvaluator"):
            name = node["inputParameterOverride"]["$value"]
            if name in TYRE_INPUTS:
                node["inputParameterOverride"] = cname(TYRE_INPUTS[name])
    return edit


# name: (vanilla source without extension, edit)
EFFECTS = {
    **trails("scrape_trail_hard", (235, 232, 225, 255), 1.0, 45.0, 1.0),
    **trails("scrape_trail_metal", (235, 238, 245, 255), 0.8, 45.0, 1.0, SCRATCH_METAL_TEX),
    # soft ground (dirt, sand, mud): a rut ploughed through it, berms of turned soil either side (draw_rut)
    **trails("scrape_trail_soft", (235, 235, 235, 255), 1.4, 30.0, 0.8, SOFT_TEX, SOFT_NORMAL),
    # grass: a streak of flattened turf, torn to earth under the pod's corners (draw_grass), on the rut's normals,
    # faintly
    **trails("scrape_trail_grass", (235, 235, 235, 255), 1.5, 40.0, 0.25, GRASS_TEX, SOFT_NORMAL),
    # grass: the game's own spray of grass and earth from a tyre spinning on turf, by the scrape's speed and press
    "scrape_turf_grass": (GRASS_TYRE, tyre()),
    "scrape_sparks_hard": (SPARKS, driven(birth(1.3))),
    # metal: many more sparks and hotter ones (the blackbody shader's Temperature), so they read by day
    "scrape_sparks_metal": (SPARKS, driven(emitters(sparks=(3.0, {"Temperature": 80.0})))),
    "scrape_sparks_glass": (SPARKS, driven(birth(0.35))),
    # soft ground: the concrete scratch's dust, tan, thicker and more of it, with fewer clods and no sparks; it rolls
    # out slower than sparks fly
    "scrape_dust_soft": (CONCRETE, driven(emitters(sparks=(0.0, {}), chunks=(0.6, {}),
                                                   smoke=(6.0, {"LightSmoke": (200, 176, 142),
                                                                "DarkSmoke": (140, 118, 92), "maxAlpha": 0.75})),
                                          velocity=((0.0, 0.6), (1.0, 1.3)), size=((0.0, 0.8), (1.0, 1.5)))),
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
