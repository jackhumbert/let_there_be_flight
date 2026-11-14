"""World boundaries: the areas the game keeps the player out of, for the Autopilot to keep out of too.

The game marks the edges of the playable area with gameWorldBoundaryNode (a worldAreaShapeNode: an
outline extruded upwards by a height) in the ordinary exterior streaming sectors; the native
gameWorldBoundarySystem acts on them, and the game's own autodrive stops at one
(AutoDriveSystem.OnReachedWorldBoundary). They are fences of oriented boxes along the Badlands and
the hills and out to sea, not one closed outline: terrain and the sea close the gaps between them.

Some of them belong to a sector variant that a quest switches on or off (questTogglePrefabVariant).
The streaming block names each sector's variants (worldStreamingSectorDescriptor.variants: the
prefab, the variant name, the range of the sector's nodes it holds, whether it is on by default).
The plugin cannot ask the game for a variant's state, so each variant is mapped to the quest fact
that decides it, read from the quest phases that toggle it (VARIANT_GATES below).

    python tools/navmap_bake/boundaries.py --sectors <dir with .streamingsector> --block <all.streamingblock> \
        -o src/navigation/boundaries.tsv [--preview boundaries.png]

--sectors: the exterior sectors as the nav map bake unbundles them (tools/navmap_bake/README.md, step 1:
ep1_1_nightcity.archive overrides nearly all of the base game's). --block: the streaming block from the
same archive, base/worlds/03_night_city/_compiled/default/blocks/all.streamingblock:

    WolvenKit.CLI unbundle <game>/archive/pc/ep1/ep1_1_nightcity.archive -o <dir> -r "default.blocks.all\\.streamingblock$"

Only the sectors holding a boundary node are converted (about 50 of 18k, found by their bytes).

The fences end open where terrain or the sea closes the gap for a car on the ground. The closures drawn by hand in
boundary_closures.tsv (beside this script) close them for a car in the air: each polyline is widened into one area
per segment, always in force, the segments lengthened by half the width at both ends so they overlap at the bends.

boundaries.tsv, one area per line: name, gate, z_min, z_max, then the outline's x y pairs (world
metres, in order). gate is "-" for always, else "<fact><op><value>" (op one of > < >= <= == !=):
the area is there while the fact compares true. Read by src/red4ext/Navigation/Boundaries.cpp.
"""

import argparse
import glob
import json
import math
import os
import subprocess
import sys
import tempfile

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))  # tools/, for common
from common import paths  # noqa: E402

NODE_TYPE = "gameWorldBoundaryNode"
CLOSURES = os.path.join(os.path.dirname(os.path.abspath(__file__)), "boundary_closures.tsv")
# the closures' z range: unused by the plugin (a masked column is blocked at every height)
CLOSURE_Z = (-200.0, 1500.0)

# variant name -> gate, from the quest phases that toggle it; None: never on in the game (left out)
VARIANT_GATES = {
    # off by default. open_world/phases/open_world_content starts phases/prolog_roadblocks/lch_prolog_roadblocks
    # while watson_prolog_lock > 0; that phase shows the variant at its start and hides it once the fact is 0.
    # q001_ride_to_mb sets the fact to 1 (Watson locked down), q101 sets it to 0 (Act 2).
    "prolog_blockades": "watson_prolog_lock>0",
    # on by default; ep1/openworld/ep1_open_world hides it once ep1_installed > 0 (Phantom Liberty)
    "boundaries_spaceport_invisible": "ep1_installed==0",
    # off by default and every quest node that names it (sq021, sq029) hides it: never shown
    "var_vr_game": None,
}


def convert(wolvenkit, files, out_dir):
    """CR2W to json with WolvenKit (cr2w -s), into out_dir; the json paths, by input stem."""
    os.makedirs(out_dir, exist_ok=True)  # WolvenKit refuses an output folder that is not there
    subprocess.run([wolvenkit, "cr2w", "-s", *files, "-o", out_dir, "-v", "Minimal"],
                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, check=False)
    return {os.path.basename(p).split(".")[0]: p for p in glob.glob(os.path.join(out_dir, "*.json"))}


def rotate(q, v):
    """v rotated by the quaternion q (i j k r)."""
    x, y, z, w = q["i"], q["j"], q["k"], q["r"]
    vx, vy, vz = v
    tx, ty, tz = 2 * (y * vz - z * vy), 2 * (z * vx - x * vz), 2 * (x * vy - y * vx)
    return (vx + w * tx + (y * tz - z * ty), vy + w * ty + (z * tx - x * tz), vz + w * tz + (x * ty - y * tx))


def variant_ranges(descriptor):
    """rangeIndex -> variant of a sector's streaming block descriptor."""
    return {int(v["rangeIndex"]): v for v in (descriptor["variants"] if descriptor else [])}


def areas_of(sector, descriptor):
    """The boundary areas placed in one sector json: dicts of name, variant, z_min, z_max, outline, key."""
    root = sector["Data"]["RootChunk"]
    starts = [int(s) for s in root["variantIndices"]]  # first nodeData index of each node range
    variants = variant_ranges(descriptor)
    out = []
    for index, placement in enumerate(root["nodeData"]["Data"]):
        node = root["nodes"][int(placement["nodeIdx"])]["Data"]
        if node["$type"] != NODE_TYPE:
            continue
        rng = max(i for i, s in enumerate(starts) if s <= index)
        variant = variants.get(rng) if rng > 0 else None
        outline = node["outline"]["Data"]
        scale, transform = placement["scale"], placement["transform"]
        at = transform["position"]
        corners = []
        for p in outline["points"]:
            v = rotate(transform["orientation"], (p["X"] * scale["X"], p["Y"] * scale["Y"], p["Z"] * scale["Z"]))
            corners.append((at["X"] + v[0], at["Y"] + v[1], at["Z"] + v[2]))
        z_min = min(c[2] for c in corners)
        out.append({
            "name": node["debugName"]["$value"],
            "variant": variant["name"]["$value"] if variant else None,
            "z_min": z_min,
            "z_max": z_min + outline["height"] * scale["Z"],
            "outline": [(c[0], c[1]) for c in corners],
            # one placement can be in several sectors (levels of detail): the same node at the same place
            "key": (placement["patchedNode"]["$value"], round(at["X"], 1), round(at["Y"], 1)),
        })
    return out


def bake(sectors_dir, block_path, wolvenkit):
    files = []
    for path in sorted(glob.glob(os.path.join(sectors_dir, "*.streamingsector"))):
        with open(path, "rb") as f:
            if NODE_TYPE.encode() in f.read():
                files.append(path)
    if not files:
        sys.exit(f"no sector in {sectors_dir} holds a {NODE_TYPE}")
    with tempfile.TemporaryDirectory() as tmp:
        jsons = convert(wolvenkit, files, tmp)
        block_json = convert(wolvenkit, [block_path], os.path.join(tmp, "block"))
        if len(block_json) != 1:
            sys.exit(f"WolvenKit could not convert {block_path}")
        with open(next(iter(block_json.values()))) as f:
            descriptors = json.load(f)["Data"]["RootChunk"]["descriptors"]
        by_sector = {}
        for d in descriptors:
            name = d["data"]["DepotPath"]["$value"].replace("\\", "/").rsplit("/", 1)[-1].split(".")[0]
            by_sector[name] = d
        areas, seen = [], set()
        for stem in sorted(jsons):
            with open(jsons[stem]) as f:
                sector = json.load(f)
            for area in areas_of(sector, by_sector.get(stem)):
                if area["key"] not in seen:
                    seen.add(area["key"])
                    areas.append(area)
    print(f"{len(areas)} areas in {len(files)} sectors")
    return areas


def closures(path):
    """The hand-drawn closures as areas: one rectangle per polyline segment, lengthened by half the width at both
    ends so neighbouring segments overlap at a bend."""
    out = []
    with open(path) as f:
        for line in f:
            if line.startswith("#") or not line.strip():
                continue
            fields = line.rstrip("\n").split("\t")
            name, width = fields[0], float(fields[1])
            xy = [float(v) for v in fields[2:]]
            points = list(zip(xy[0::2], xy[1::2]))
            for k, (a, b) in enumerate(zip(points, points[1:])):
                length = math.hypot(b[0] - a[0], b[1] - a[1])
                ux, uy = (b[0] - a[0]) / length, (b[1] - a[1]) / length
                hx, hy = -uy * width / 2, ux * width / 2  # half the width, across
                ex, ey = ux * width / 2, uy * width / 2  # half the width, along
                a, b = (a[0] - ex, a[1] - ey), (b[0] + ex, b[1] + ey)
                outline = [(a[0] + hx, a[1] + hy), (b[0] + hx, b[1] + hy), (b[0] - hx, b[1] - hy), (a[0] - hx, a[1] - hy)]
                out.append({"name": f"{name}_{k}" if len(points) > 2 else name, "variant": None, "z_min": CLOSURE_Z[0],
                            "z_max": CLOSURE_Z[1], "outline": outline, "key": (name, k)})
    return out


def gate_of(area):
    if area["variant"] is None:
        return "-"
    if area["variant"] not in VARIANT_GATES:
        sys.exit(f"{area['name']} is in variant {area['variant']}, which has no gate: find the quest phases that toggle "
                 f"it and add it to VARIANT_GATES")
    return VARIANT_GATES[area["variant"]]


def write(areas, out_path):
    lines = [
        "# World boundaries: the game's gameWorldBoundaryNode areas, generated by tools/navmap_bake/boundaries.py.",
        "# name, gate (\"-\" always, else <fact><op><value>: there while the fact compares true), z_min, z_max (m),",
        "# then the outline's x y pairs (world metres, in order)",
    ]
    kept = 0
    for area in sorted(areas, key=lambda a: (a["variant"] or "", a["name"], str(a["key"]))):
        gate = gate_of(area)
        if gate is None:
            continue
        kept += 1
        xy = "\t".join(f"{x:.2f}\t{y:.2f}" for x, y in area["outline"])
        lines.append(f"{area['name']}\t{gate}\t{area['z_min']:.1f}\t{area['z_max']:.1f}\t{xy}")
    with open(out_path, "w", newline="\n") as f:
        f.write("\n".join(lines) + "\n")
    print(f"wrote {kept} areas to {out_path} ({len(areas) - kept} never on, left out)")


def preview(areas, out_path):
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    fig, ax = plt.subplots(figsize=(12, 12))
    for area in areas:
        gate = gate_of(area)
        closure = area["name"].startswith("closure")
        colour = ("magenta" if closure else "red") if gate == "-" else "grey" if gate is None else "orange"
        xs = [p[0] for p in area["outline"]] + [area["outline"][0][0]]
        ys = [p[1] for p in area["outline"]] + [area["outline"][0][1]]
        ax.fill(xs, ys, color=colour, alpha=0.35)
        ax.plot(xs, ys, color=colour, lw=0.5)
    ax.set_aspect("equal")
    ax.grid(True, alpha=0.3)
    ax.set_title("world boundaries: red always, magenta closures, orange gated by a fact, grey never on")
    fig.savefig(out_path, dpi=80, bbox_inches="tight")


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--sectors", required=True, help="folder with the exterior .streamingsector files")
    ap.add_argument("--block", required=True, help="all.streamingblock of the same archive")
    ap.add_argument("-o", "--out", required=True, help="boundaries.tsv to write")
    ap.add_argument("--preview", help="a png of the areas, top down")
    ap.add_argument("--closures", default=CLOSURES, help="the hand-drawn closures (default: boundary_closures.tsv here)")
    ap.add_argument("--wolvenkit", default=None, help="WolvenKit.CLI.exe 10.x (default: the WOLVENKIT setting)")
    args = ap.parse_args()
    areas = bake(args.sectors, args.block, args.wolvenkit or paths.WOLVENKIT)
    drawn = closures(args.closures)
    print(f"{len(drawn)} closure areas from {args.closures}")
    areas += drawn
    write(areas, args.out)
    if args.preview:
        preview(areas, args.preview)


if __name__ == "__main__":
    main()
