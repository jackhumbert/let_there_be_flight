"""Turn the cooked collision of streaming sectors into world-space triangles.

Each worldCookedCollisionNode (game 2.5; worldCollisionNode before) holds inner nodes with
actors. An actor has a world transform, an optional uniform scale and a run of shapes; a shape is
a box, capsule, convex or triangle mesh with an optional local position / rotation. Convex and
triangle meshes come from the geometry cache by hash (see geometry_cache.py).

World vertex = actor.pos + actor.rot * (scale * (shape.pos + shape.rot * v)).

WolvenKit 10 (first version that reads 2.5 sectors) serializes each sector to JSON; we keep
only the collision and delete the JSON. Output per sector: <name>.npz with
tris (N, 3, 3) float32 world positions, preset (N,) uint16 index into preset_names (collision
preset CName: the name when WolvenKit resolves it, else the decimal hash).
Terrain (worldTerrainCollisionNode, sector levels 2 to 6) is a PhysX height field instead:
<name>.terrain.npz holds xform (n, 15: rotation 3x3, position, rowScale, heightScale,
columnScale) and per node h<i> (rows, cols) int16 samples and hole<i> masks.
Building proxies (worldBuildingProxyMeshNode: mesh path + nodeData placement) go to
<name>.proxies.json for proxies.py; visible buildings often have no vehicle collision up high.
A summary of node types per sector goes to nodes.json, to spot collision we do not read yet.

Usage:
  python sectors.py --sectors <dir with .streamingsector> --geometry geometry.npz -o <out dir>
                    [--wolvenkit WolvenKit.CLI.exe] [--jobs 6] [--match REGEX]
"""

import argparse
import base64
import collections
import json
import math
import os
import re
import shutil
import struct
import subprocess
import sys
import tempfile
from concurrent.futures import ProcessPoolExecutor, as_completed

import numpy as np

from geometry_cache import GeometryLibrary

DEFAULT_WOLVENKIT = r"D:\cp2077_work\build\tools\wolvenkit-10.0.0-beta.1\WolvenKit.CLI.exe"
FIXED_POINT = 1.0 / 131072.0  # WorldPosition FixedPoint.Bits, 17 fractional bits
COLLISION_NODE_TYPES = ("worldCookedCollisionNode", "worldCollisionNode")
BATCH = 80

_BOX_CORNERS = np.array(
    [[x, y, z] for x in (-1, 1) for y in (-1, 1) for z in (-1, 1)], np.float32
)
# 12 triangles of a unit cube over _BOX_CORNERS (index = x*4 + y*2 + z)
_BOX_TRIS = np.array(
    [
        [0, 1, 3], [0, 3, 2], [4, 6, 7], [4, 7, 5],  # -x, +x
        [0, 4, 5], [0, 5, 1], [2, 3, 7], [2, 7, 6],  # -y, +y
        [0, 2, 6], [0, 6, 4], [1, 5, 7], [1, 7, 3],  # -z, +z
    ],
    np.int32,
)


def quat_matrix(q):
    x, y, z, w = q["i"], q["j"], q["k"], q["r"]
    return np.array(
        [
            [1 - 2 * (y * y + z * z), 2 * (x * y - z * w), 2 * (x * z + y * w)],
            [2 * (x * y + z * w), 1 - 2 * (x * x + z * z), 2 * (y * z - x * w)],
            [2 * (x * z - y * w), 2 * (y * z + x * w), 1 - 2 * (x * x + y * y)],
        ],
        np.float64,
    )


def vec3(v):
    return np.array([v["X"], v["Y"], v["Z"]], np.float64)


def box_mesh(half):
    return _BOX_CORNERS * np.asarray(half, np.float32), _BOX_TRIS


def shape_mesh(key, geometry, missing):
    t = key["$type"]
    if t == "PhysicsGeometryBoxData":
        return box_mesh(vec3(key["size"]))
    if t == "PhysicsGeometryCapsuleData":
        # size.X = radius, size.Y = half height, axis along local Z (not PhysX's X). Checked
        # against dense in-game raycasts on a leaning tree (a chain of tilted capsules) and a
        # horizontal lamp arm; X and Y axes put both in the wrong place. A box hull is plenty
        # for a 2 m height map.
        r, hh = key["size"]["X"], key["size"]["Y"]
        return box_mesh((r, r, hh + r))
    if t == "PhysicsGeometrySphereData":
        r = key["size"]["X"]
        return box_mesh((r, r, r))
    if t in ("PhysicsGeometryTriangleMeshData", "PhysicsGeometryConvexMeshData"):
        h = int(key["hash"])
        g = geometry.get(h)
        if g is None:
            missing[t] += 1
        return g
    missing[t] += 1
    return None


def collision_triangles(node, geometry, missing):
    tris, presets = [], []
    for inner in node["compiledData"]["Data"]["CollisionNodes"]:
        shapes = inner["shapes"] or []
        positions = inner["shapePositions"] or []
        rotations = inner["shapeRotations"] or []
        scales = inner["scales"] or []
        preset_names = inner["presets"] or []
        for actor in inner["actors"] or []:
            tr = actor["transform"]
            pos = tr["Position"]
            apos = np.array([pos[c]["Bits"] for c in "xyz"], np.float64) * FIXED_POINT
            arot = quat_matrix(tr["Orientation"])
            # scaleIndex -1 = unscaled (a Python [-1] would silently pick the last scale)
            si = actor["scaleIndex"]
            scale = vec3(scales[si]) if 0 <= si < len(scales) else np.ones(3)
            first = actor["shapeStartIndex"]
            for shape in shapes[first : first + actor["shapeCount"]]:
                key = shape["collisionSourceKey"]
                if not key:
                    continue
                g = shape_mesh(key, geometry, missing)
                if g is None:
                    continue
                verts, idx = g
                v = verts.astype(np.float64)
                if shape["shapeRotationIndex"] >= 0:
                    v = v @ quat_matrix(rotations[shape["shapeRotationIndex"]]).T
                if shape["shapePositionIndex"] >= 0:
                    v = v + vec3(positions[shape["shapePositionIndex"]])
                v = (v * scale) @ arot.T + apos
                tris.append(v[idx].astype(np.float32))
                pi = shape["shapePresetIndex"]
                # CNames come back as the string when WolvenKit knows it, else the hash
                preset = str(preset_names[pi]["$value"]) if 0 <= pi < len(preset_names) else ""
                presets.append((preset, len(idx)))
    return tris, presets


HF_HOLE = 127  # PxHeightFieldMaterial::eHOLE


def parse_heightfield(blob):
    """PhysX 3.4 cooked height field ("NXS\\x01HFHF" v1) -> (rows, cols) int16 heights and
    a hole mask. Layout after the 8 byte tag: version, rows, cols (u32); rowLimit, colLimit,
    nbColumns, thickness, convexEdgeThreshold (f32); flags, format (u16); local bounds
    (6 f32); sampleStride, nbSamples (u32); minHeight, maxHeight (f32); samples of
    (i16 height, u8 material0, u8 material1)."""
    if blob[:8] != b"NXS\x01HFHF":
        raise ValueError("not a cooked height field")
    version, rows, cols = struct.unpack_from("<3I", blob, 8)
    if version != 1:
        raise ValueError(f"height field version {version}")
    stride, count = struct.unpack_from("<2I", blob, 70)
    if stride != 4 or count != rows * cols:
        raise ValueError(f"height field stride {stride} samples {count}")
    s = np.frombuffer(blob, np.dtype([("h", "<i2"), ("m0", "u1"), ("m1", "u1")]), count, 86)
    s = s.reshape(rows, cols)
    holes = ((s["m0"] & 0x7F) == HF_HOLE) & ((s["m1"] & 0x7F) == HF_HOLE)
    return s["h"].copy(), holes


def terrain_heightfield(node):
    """World mapping of a worldTerrainCollisionNode: PhysX local (row * rowScale,
    height * heightScale, col * columnScale), rotated and placed by actorTransform."""
    blob = base64.b64decode(node["heightfieldGeometry"]["Bytes"])
    heights, holes = parse_heightfield(blob)
    tr = node["actorTransform"]
    pos = np.array([tr["Position"][c]["Bits"] for c in "xyz"], np.float64) * FIXED_POINT
    xform = np.concatenate(
        [
            quat_matrix(tr["Orientation"]).ravel(),
            pos,
            [node["rowScale"], node["heightScale"], node["columnScale"]],
        ]
    )
    return xform, heights, holes


def building_proxies(rc):
    """Placed worldBuildingProxyMeshNode meshes: (depot path, position, quaternion i j k r,
    scale) per nodeData instance. These are simplified shells of whole buildings and cover
    visible geometry that has no vehicle collision (upper floors, roofs, background towers)."""
    instances = collections.defaultdict(list)
    for d in rc["nodeData"]["Data"]:
        instances[int(d["nodeIdx"])].append(d)
    out = []
    for i, handle in enumerate(rc["nodes"]):
        node = handle["Data"]
        if node["$type"] != "worldBuildingProxyMeshNode":
            continue
        mesh = node["mesh"]["DepotPath"]["$value"]
        for d in instances[i]:
            tr = d["transform"]
            out.append(
                [
                    str(mesh),
                    [tr["position"][c] for c in "XYZ"],
                    [tr["orientation"][c] for c in ("i", "j", "k", "r")],
                    [d["scale"][c] for c in "XYZ"],
                ]
            )
    return out


def process_json(path, geometry):
    rc = json.load(open(path, encoding="utf-8"))["Data"]["RootChunk"]
    node_types = collections.Counter()
    missing = collections.Counter()
    tris, presets, terrain = [], [], []
    for handle in rc["nodes"]:
        node = handle["Data"]
        t = node["$type"]
        node_types[t] += 1
        if t in COLLISION_NODE_TYPES:
            nt, npre = collision_triangles(node, geometry, missing)
            tris += nt
            presets += npre
        elif t == "worldTerrainCollisionNode":
            terrain.append(terrain_heightfield(node))
    return node_types, missing, tris, presets, terrain, building_proxies(rc)


_geometry = None


def _init(geometry_path):
    global _geometry
    _geometry = GeometryLibrary(geometry_path)


def run_batch(wolvenkit, files, out_dir):
    tmp = tempfile.mkdtemp(prefix="hm_")
    try:
        cmd = [wolvenkit, "cr2w", "-s", *files, "-o", tmp, "-v", "Minimal"]
        subprocess.run(cmd, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, check=False)
        results = {}
        for f in files:
            name = os.path.basename(f)
            jp = os.path.join(tmp, name + ".json")
            if not os.path.exists(jp):
                results[name] = {"error": "not serialized"}
                continue
            try:
                node_types, missing, tris, presets, terrain, proxies = process_json(jp, _geometry)
            except Exception as err:  # keep going; report per sector
                results[name] = {"error": repr(err)}
                continue
            finally:
                os.remove(jp)
            stem = name[: -len(".streamingsector")]
            if tris:
                names = sorted({p for p, _ in presets})
                lookup = {p: i for i, p in enumerate(names)}
                np.savez(
                    os.path.join(out_dir, stem + ".npz"),
                    tris=np.concatenate(tris),
                    preset=np.concatenate(
                        [np.full(n, lookup[p], np.uint16) for p, n in presets]
                    ),
                    preset_names=np.array(names),
                )
            if terrain:
                arrays = {"xform": np.stack([x for x, _, _ in terrain])}
                for i, (_, h, holes) in enumerate(terrain):
                    arrays[f"h{i}"] = h
                    arrays[f"hole{i}"] = holes
                np.savez(os.path.join(out_dir, stem + ".terrain.npz"), **arrays)
            if proxies:
                json.dump(proxies, open(os.path.join(out_dir, stem + ".proxies.json"), "w"))
            results[name] = {
                "nodes": dict(node_types),
                "missing": dict(missing),
                "triangles": int(sum(len(t) for t in tris)),
                "terrain": len(terrain),
            }
        return results
    finally:
        shutil.rmtree(tmp, ignore_errors=True)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--sectors", required=True)
    ap.add_argument("--geometry", required=True)
    ap.add_argument("-o", "--out", required=True)
    ap.add_argument("--wolvenkit", default=DEFAULT_WOLVENKIT)
    ap.add_argument("--jobs", type=int, default=max(1, (os.cpu_count() or 2) // 2))
    ap.add_argument("--match", default=r"^exterior_.*\.streamingsector$")
    ap.add_argument(
        "--redo",
        choices=["errors", "terrain", "proxies", "all"],
        help="reprocess sectors already in nodes.json: failed ones, ones with terrain, ones "
        "with building proxies, or all",
    )
    args = ap.parse_args()

    os.makedirs(args.out, exist_ok=True)
    summary_path = os.path.join(args.out, "nodes.json")
    summary = json.load(open(summary_path)) if os.path.exists(summary_path) else {}

    def wanted(name):
        done = summary.get(name)
        if done is None or args.redo == "all":
            return True
        if args.redo == "errors":
            return "error" in done
        if args.redo == "terrain":
            return bool(done.get("nodes", {}).get("worldTerrainCollisionNode"))
        if args.redo == "proxies":
            return bool(done.get("nodes", {}).get("worldBuildingProxyMeshNode"))
        return False

    pattern = re.compile(args.match)
    files = sorted(
        os.path.join(args.sectors, f)
        for f in os.listdir(args.sectors)
        if pattern.search(f) and wanted(f)
    )
    batches = [files[i : i + BATCH] for i in range(0, len(files), BATCH)]
    print(f"{len(files)} sectors in {len(batches)} batches, {args.jobs} jobs", file=sys.stderr)

    done = 0
    with ProcessPoolExecutor(args.jobs, initializer=_init, initargs=(args.geometry,)) as pool:
        futures = [pool.submit(run_batch, args.wolvenkit, b, args.out) for b in batches]
        for fut in as_completed(futures):
            summary.update(fut.result())
            done += 1
            if done % 5 == 0 or done == len(batches):
                json.dump(summary, open(summary_path, "w"))
                print(f"{done}/{len(batches)} batches", file=sys.stderr)
    json.dump(summary, open(summary_path, "w"), indent=0)

    errors = {k: v["error"] for k, v in summary.items() if "error" in v}
    missing = collections.Counter()
    nodes = collections.Counter()
    for v in summary.values():
        missing.update(v.get("missing", {}))
        nodes.update(v.get("nodes", {}))
    total = sum(v.get("triangles", 0) for v in summary.values())
    print(f"{total} triangles, {len(errors)} errors, missing shapes {dict(missing)}", file=sys.stderr)
    for t, c in nodes.most_common():
        if "ollision" in t or "errain" in t:
            print(f"  {t}: {c}", file=sys.stderr)
    for k, e in list(errors.items())[:10]:
        print(f"  error {k}: {e}", file=sys.stderr)


if __name__ == "__main__":
    main()
