"""What collision is in a box: for checking a hidden collider (something the nav map or the car
treats as solid where the game shows nothing) back to the collision behind it.

    python tools/world/collision_at.py x0,y0,z0 x1,y1,z1 [--actors] [--json out.json]
    python tools/world/collision_at.py --case <name>        a case from collider_cases.json

Step 1, always: the bake's per-sector world triangles (navmap_bake/sectors.py output) and the
extra triangle files the bake took (proxy_colliders.py's colliders.npz, LTBF's own) whose
triangles reach into the box, counted by sector and collision preset. A per-sector bounds index
is written next to the sectors the first time (a few minutes over the 14 GB).

Step 2, --actors: each of those sectors serialized again (WolvenKit) and walked actor by actor
as sectors.py does, listing every actor with a shape in the box: collision node, group (the
ArchiveXL nodeDeletions index is that group's nodeData placement), actor, its stored position
(what Blockers.cpp matches), shapes and presets, and the world bounds of its shapes in the box.

A triangle counts when its bounding box overlaps the box (enough to name the source; a long
diagonal triangle can be listed from just outside).
"""

import argparse
import collections
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))  # tools/, for common
sys.path.insert(0, os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "navmap_bake"))
from common import paths  # noqa: E402

BAKE = os.path.join(paths.WORK, "heightmap", "bake_2026-10-26")
DEFAULT_SECTORS = os.path.join(BAKE, "sectors")
DEFAULT_RAW = os.path.join(BAKE, "raw", "ep1", "base", "worlds", "03_night_city", "_compiled", "default")
DEFAULT_EXTRA = [os.path.join(paths.WORK, "proxycol", "colliders.npz")]
DEFAULT_GEOMETRY = os.path.join(paths.WORK, "heightmap", "geometry.npz")
PRESETS_TABLE = os.path.join(paths.WORK, "heightmap", "presets_table.json")
CASES = os.path.join(os.path.dirname(os.path.abspath(__file__)), "collider_cases.json")


def preset_names():
    names = {}
    if os.path.exists(PRESETS_TABLE):
        names = {str(p["hash"]): p["name"] for p in json.load(open(PRESETS_TABLE))}
    return lambda v: names.get(str(v), str(v))


def bounds_index(sectors):
    """{sector stem: [x0, y0, z0, x1, y1, z1]} of each sector's triangles, cached."""
    path = os.path.join(sectors, "_bounds.json")
    if os.path.exists(path):
        return json.load(open(path))
    out = {}
    files = sorted(n for n in os.listdir(sectors) if n.endswith(".npz") and not n.endswith(".terrain.npz"))
    for k, n in enumerate(files):
        tris = np.load(os.path.join(sectors, n))["tris"]
        if len(tris):
            p = tris.reshape(-1, 3)
            out[n[:-4]] = [float(v) for v in (*p.min(0), *p.max(0))]
        if k % 2000 == 0:
            print(f"  indexing sectors {k}/{len(files)}", file=sys.stderr, flush=True)
    json.dump(out, open(path, "w"))
    return out


def in_box(tris, lo, hi):
    tmin, tmax = tris.min(1), tris.max(1)
    return np.all(tmax >= lo, 1) & np.all(tmin <= hi, 1)


def triangles(args, lo, hi):
    name = preset_names()
    found = []
    for stem, b in bounds_index(args.sectors).items():
        if any(b[i + 3] < lo[i] or b[i] > hi[i] for i in range(3)):
            continue
        d = np.load(os.path.join(args.sectors, stem + ".npz"))
        sel = in_box(d["tris"], lo, hi)
        if sel.any():
            names = [str(x) for x in d["preset_names"]]
            t = d["tris"][sel].reshape(-1, 3)
            for pi, n in collections.Counter(d["preset"][sel].tolist()).items():
                found.append({"source": stem, "preset": name(names[pi]), "triangles": n,
                              "bounds": [round(float(v), 1) for v in (*t.min(0), *t.max(0))]})
    for path in args.extra:
        if not os.path.exists(path):
            continue
        d = np.load(path)
        sel = in_box(d["tris"], lo, hi)
        if sel.any():
            t = d["tris"][sel].reshape(-1, 3)
            found.append({"source": os.path.basename(path) + " (LTBF's own)", "preset": "",
                          "triangles": int(sel.sum()),
                          "bounds": [round(float(v), 1) for v in (*t.min(0), *t.max(0))]})
    return found


def actors(args, sectors_found, lo, hi):
    from sectors import COLLISION_NODE_TYPES, FIXED_POINT, quat_matrix, shape_mesh, vec3  # noqa: E402
    from geometry_cache import GeometryLibrary  # noqa: E402

    name = preset_names()
    geometry = GeometryLibrary(args.geometry)
    tmp = tempfile.mkdtemp(prefix="cat_")
    out = []
    try:
        files = [os.path.join(args.raw, s + ".streamingsector") for s in sectors_found]
        subprocess.run([args.wolvenkit, "cr2w", "-s", *files, "-o", tmp, "-v", "Minimal"],
                       stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, check=False)
        for stem in sectors_found:
            jp = os.path.join(tmp, stem + ".streamingsector.json")
            if not os.path.exists(jp):
                out.append({"sector": stem, "error": "not serialized"})
                continue
            rc = json.load(open(jp, encoding="utf-8"))["Data"]["RootChunk"]
            placements = collections.defaultdict(list)
            for i, nd in enumerate(rc["nodeData"]["Data"]):
                placements[int(nd["nodeIdx"])].append(i)
            for ni, handle in enumerate(rc["nodes"]):
                node = handle["Data"]
                if node["$type"] not in COLLISION_NODE_TYPES:
                    continue
                groups = node["compiledData"]["Data"]["CollisionNodes"]
                for gi, inner in enumerate(groups):
                    shapes = inner["shapes"] or []
                    positions = inner["shapePositions"] or []
                    rotations = inner["shapeRotations"] or []
                    scales = inner["scales"] or []
                    presets = inner["presets"] or []
                    for ai, actor in enumerate(inner["actors"] or []):
                        tr = actor["transform"]
                        bits = [tr["Position"][c]["Bits"] for c in "xyz"]
                        apos = np.array(bits, np.float64) * FIXED_POINT
                        arot = quat_matrix(tr["Orientation"])
                        si = actor["scaleIndex"]
                        scale = vec3(scales[si]) if 0 <= si < len(scales) else np.ones(3)
                        hit = []
                        first = actor["shapeStartIndex"]
                        for shape in shapes[first : first + actor["shapeCount"]]:
                            key = shape["collisionSourceKey"]
                            if not key:
                                continue
                            g = shape_mesh(key, geometry, collections.Counter())
                            if g is None:
                                continue
                            v = g[0].astype(np.float64)
                            if shape["shapeRotationIndex"] >= 0:
                                v = v @ quat_matrix(rotations[shape["shapeRotationIndex"]]).T
                            if shape["shapePositionIndex"] >= 0:
                                v = v + vec3(positions[shape["shapePositionIndex"]])
                            v = (v * scale) @ arot.T + apos
                            t = v[g[1]]
                            if in_box(t, lo, hi).any():
                                pi = shape["shapePresetIndex"]
                                preset = name(presets[pi]["$value"]) if 0 <= pi < len(presets) else ""
                                kind = key["$type"].replace("PhysicsGeometry", "").replace("Data", "")
                                hit.append((kind, preset, v.min(0), v.max(0)))
                        if hit:
                            p = placements.get(ni, [])
                            out.append({
                                "sector": stem, "node": ni, "group": gi,
                                "index": p[gi] if len(p) == len(groups) else None,
                                "actor": ai, "position": [round(float(x), 2) for x in apos], "bits": bits,
                                "shapes": [{"kind": k, "preset": pr,
                                            "bounds": [round(float(x), 1) for x in (*a, *b)]} for k, pr, a, b in hit],
                            })
    finally:
        shutil.rmtree(tmp, ignore_errors=True)
    return out


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("lo", nargs="?", help="x0,y0,z0")
    ap.add_argument("hi", nargs="?", help="x1,y1,z1")
    ap.add_argument("--case", help="a case name from collider_cases.json")
    ap.add_argument("--actors", action="store_true", help="also list the actors (serializes the sectors)")
    ap.add_argument("--json", help="write the findings here")
    ap.add_argument("--sectors", default=DEFAULT_SECTORS)
    ap.add_argument("--raw", default=DEFAULT_RAW)
    ap.add_argument("--extra", action="append", default=None)
    ap.add_argument("--geometry", default=DEFAULT_GEOMETRY)
    ap.add_argument("--wolvenkit", help="default paths.WOLVENKIT")
    args = ap.parse_args()
    args.wolvenkit = args.wolvenkit or paths.WOLVENKIT
    args.extra = args.extra or DEFAULT_EXTRA
    if args.case:
        case = next(c for c in json.load(open(CASES))["cases"] if c["name"] == args.case)
        lo, hi = np.array(case["box"][:3], float), np.array(case["box"][3:], float)
    else:
        lo = np.array([float(v) for v in args.lo.split(",")])
        hi = np.array([float(v) for v in args.hi.split(",")])
    found = triangles(args, lo, hi)
    print(f"collision in {lo.tolist()} .. {hi.tolist()}:")
    for f in sorted(found, key=lambda f: -f["triangles"]):
        print(f"  {f['source']:42s} {f['preset']:32s} {f['triangles']:6d} tris  bounds {f['bounds']}")
    if not found:
        print("  none: the bake read no collision here (a nav map artefact, or collision it does not read)")
    result = {"box": [*lo.tolist(), *hi.tolist()], "triangles": found}
    if args.actors:
        stems = sorted({f["source"] for f in found if not f["source"].endswith("(LTBF's own)")})
        result["actors"] = actors(args, stems, lo, hi)
        for a in result["actors"]:
            if "error" in a:
                print(f"  {a['sector']}: {a['error']}")
                continue
            print(f"  {a['sector']} node {a['node']} group {a['group']} (nodeData {a['index']}) actor {a['actor']} at {a['position']}")
            for s in a["shapes"]:
                print(f"      {s['kind']:14s} {s['preset']:32s} bounds {s['bounds']}")
    if args.json:
        json.dump(result, open(args.json, "w"), indent=1)


if __name__ == "__main__":
    main()
