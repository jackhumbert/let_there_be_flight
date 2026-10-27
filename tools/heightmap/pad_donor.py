"""Inventory everything placed in a complete landing pad's footprint (docs/helipads.md, step 8).

A visual-only pad gets a real deck by copying a complete pad of the same prefab: this lists the
pieces of the donor, i.e. every node instance whose position lies in the prefab's footprint
around the donor's proxy placement (in the proxy's frame), from the sectors around it.

  python pad_donor.py --sectors <raw ep1 _compiled\\default> --at X Y Z --yaw DEG
      [--half 14 14] [--below 3] [--above 13] -o donor.json

Output: one row per instance (sector, node index, node type, mesh / resource, nodeData index,
local position and yaw in the donor's frame, scale) plus collision actors (shape types, preset).
"""
import argparse
import json
import math
import os
import shutil
import subprocess
import tempfile

DEFAULT_WOLVENKIT = r"D:\cp2077_work\build\tools\wolvenkit-10.0.0-beta.1\WolvenKit.CLI.exe"
FIXED_POINT = 1.0 / 131072.0
LEVELS = (0, 1, 2, 3)


def sectors_around(x, y, z):
    names = set()
    for level in LEVELS:
        size = 64.0 * (1 << level)
        cx, cy, cz = (int(round(v / size)) for v in (x, y, z))
        for dx in (-1, 0, 1):
            for dy in (-1, 0, 1):
                for dz in (-1, 0, 1):
                    names.add(f"exterior_{cx + dx}_{cy + dy}_{cz + dz}_{level}")
        # interiors (shops, garages) are on a grid of half the size, floored: (-937, -84) is
        # interior_-15_-2_0_1 (mesh_scan.py, 2026-10-27)
        size = 32.0 * (1 << level)
        cx, cy, cz = (int(math.floor(v / size)) for v in (x, y, z))
        for dx in (-1, 0, 1):
            for dy in (-1, 0, 1):
                for dz in (-1, 0, 1):
                    names.add(f"interior_{cx + dx}_{cy + dy}_{cz + dz}_{level}")
    return names


def val(o):
    if isinstance(o, dict):
        if "$value" in o:
            return o["$value"]
        if "DepotPath" in o:
            return val(o["DepotPath"])
    return o


def yaw_of(q):
    return math.degrees(2.0 * math.atan2(q.get("k", 0.0), q.get("r", 1.0)))


def qmul(a, b):
    ai, aj, ak, ar = a
    bi, bj, bk, br = b
    return [ar * bi + ai * br + aj * bk - ak * bj,
            ar * bj - ai * bk + aj * br + ak * bi,
            ar * bk + ai * bj - aj * bi + ak * br,
            ar * br - ai * bi - aj * bj - ak * bk]


def local_quat(q, yaw):
    """Orientation in the pad's frame: rotate by -yaw about Z first. Returns [i, j, k, r]."""
    h = math.radians(-yaw) / 2.0
    out = qmul([0.0, 0.0, math.sin(h), math.cos(h)], [q.get("i", 0.0), q.get("j", 0.0), q.get("k", 0.0), q.get("r", 1.0)])
    return [round(v, 6) for v in out]


def vec(v):
    return [v.get("X", 0.0), v.get("Y", 0.0), v.get("Z", 0.0)] if isinstance(v, dict) else None


def world_pos(p):
    """Vector4 / Vector3 or WorldPosition (fixed point)."""
    if "x" in p and isinstance(p["x"], dict):
        return [p[a]["Bits"] * FIXED_POINT for a in ("x", "y", "z")]
    return [p.get("X", 0.0), p.get("Y", 0.0), p.get("Z", 0.0)]


def to_local(pos, origin, yaw):
    dx, dy, dz = pos[0] - origin[0], pos[1] - origin[1], pos[2] - origin[2]
    c, s = math.cos(math.radians(-yaw)), math.sin(math.radians(-yaw))
    return [dx * c - dy * s, dx * s + dy * c, dz]


def resource_of(node):
    for key in ("mesh", "resource", "meshRef", "entityTemplate", "decal", "material"):
        if key in node:
            v = val(node[key])
            if v:
                return key, v
    return None, None


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--sectors", required=True)
    ap.add_argument("--at", type=float, nargs=3, required=True)
    ap.add_argument("--yaw", type=float, required=True)
    ap.add_argument("--half", type=float, nargs=2, default=[14.0, 14.0])
    ap.add_argument("--below", type=float, default=3.0)
    ap.add_argument("--above", type=float, default=13.0)
    ap.add_argument("--wolvenkit", default=DEFAULT_WOLVENKIT)
    ap.add_argument("-o", "--out", required=True)
    args = ap.parse_args()

    origin = args.at
    files = [os.path.join(args.sectors, n + ".streamingsector")
             for n in sorted(sectors_around(*origin))]
    files = [f for f in files if os.path.exists(f)]
    tmp = tempfile.mkdtemp(prefix="donor_")
    rows = []
    try:
        subprocess.run([args.wolvenkit, "cr2w", "-s", *files, "-o", tmp, "-v", "Minimal"],
                       stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, check=False)
        for f in files:
            sector = os.path.basename(f)[: -len(".streamingsector")]
            jp = os.path.join(tmp, os.path.basename(f) + ".json")
            if not os.path.exists(jp):
                print("not serialized:", sector)
                continue
            rc = json.load(open(jp, encoding="utf-8"))["Data"]["RootChunk"]
            nodes = rc["nodes"]
            for k, nd in enumerate(rc["nodeData"]["Data"]):
                node = nodes[int(nd["nodeIdx"])]["Data"]
                ntype = node["$type"]
                pos = world_pos(nd["transform"]["position"])
                local = to_local(pos, origin, args.yaw)
                inside = (abs(local[0]) <= args.half[0] and abs(local[1]) <= args.half[1]
                          and -args.below <= local[2] <= args.above)
                if not inside and ntype not in ("worldCookedCollisionNode", "worldInstancedMeshNode"):
                    continue
                key, res = resource_of(node)
                inner = None
                inner_nodes = (node.get("buffer") or {}).get("Data", {}).get("InnerNodes") if isinstance(node.get("buffer"), dict) else None
                if inner_nodes is not None and "instanceIndex" in nd:
                    try:
                        inn = inner_nodes[nd["instanceIndex"]]
                        inner = {k2: val(v) for k2, v in inn.items() if isinstance(v, (dict, str, int, float))
                                 and k2 not in ("$type",)}
                    except (IndexError, TypeError):
                        inner = None
                if inside:
                    rows.append({
                        "sector": sector, "node": int(nd["nodeIdx"]), "data": k, "type": ntype,
                        "debugName": val(node.get("debugName")), key or "resource": res,
                        "inner": inner, "instanceIndex": nd.get("instanceIndex"),
                        "local": [round(v, 3) for v in local],
                        "yaw": round((yaw_of(nd["transform"]["orientation"]) - args.yaw + 540) % 360 - 180, 2),
                        "quat": local_quat(nd["transform"]["orientation"], args.yaw),
                        "scale": [nd["scale"].get(a, 1.0) for a in ("X", "Y", "Z")] if "scale" in nd else None,
                        "globalNodeId": val(nd.get("globalNodeId", {}).get("hash")) if isinstance(nd.get("globalNodeId"), dict) else None,
                    })
            for i, handle in enumerate(nodes):
                node = handle["Data"]
                if node["$type"] == "worldInstancedMeshNode":
                    # instances live in cookedWorldTransformsBuffer, TransformRanges[j] per inner mesh
                    data = node["buffer"]["Data"]
                    transforms = node["cookedWorldTransformsBuffer"]["Data"]["Transforms"]
                    for j, inn in enumerate(data.get("InnerNodes") or []):
                        r = data["TransformRanges"][j]
                        for t in transforms[r["transformIndex"]: r["transformIndex"] + r["transformCount"]]:
                            local = to_local(world_pos(t["transform"]["Position"]), origin, args.yaw)
                            if (abs(local[0]) <= args.half[0] and abs(local[1]) <= args.half[1]
                                    and -args.below <= local[2] <= args.above):
                                _, res = resource_of(inn)
                                rows.append({"sector": sector, "node": i, "type": "worldInstancedMeshNode",
                                             "inner": j, "mesh": res, "appearance": val(inn.get("meshAppearance")),
                                             "quat": local_quat(t["transform"]["Orientation"], args.yaw),
                                             "local": [round(v, 3) for v in local],
                                             "yaw": round((yaw_of(t["transform"]["Orientation"]) - args.yaw + 540) % 360 - 180, 2),
                                             "scale": t.get("scale")})
                elif node["$type"] in ("worldCookedCollisionNode", "worldCollisionNode"):
                    for c, inner in enumerate(node["compiledData"]["Data"]["CollisionNodes"]):
                        shapes = inner["shapes"] or []
                        presets = inner["presets"] or []
                        positions = inner["shapePositions"] or []
                        rotations = inner["shapeRotations"] or []
                        scales = inner["scales"] or []
                        for a, actor in enumerate(inner["actors"] or []):
                            local = to_local(world_pos(actor["transform"]["Position"]), origin, args.yaw)
                            if not (abs(local[0]) <= args.half[0] + 4 and abs(local[1]) <= args.half[1] + 4
                                    and -args.below - 4 <= local[2] <= args.above):
                                continue
                            first = actor["shapeStartIndex"]
                            kinds = []
                            for s in shapes[first: first + actor["shapeCount"]]:
                                pi = s["shapePresetIndex"]
                                spi, sri = s["shapePositionIndex"], s["shapeRotationIndex"]
                                kinds.append({"type": s.get("shapeType"), "key": s.get("collisionSourceKey"),
                                              "preset": val(presets[pi]) if 0 <= pi < len(presets) else None,
                                              "pos": vec(positions[spi]) if 0 <= spi < len(positions) else None,
                                              "rot": rotations[sri] if 0 <= sri < len(rotations) else None})
                            rows.append({"sector": sector, "node": i, "type": "collisionActor", "inner": c,
                                         "actor": a, "local": [round(v, 3) for v in local],
                                         "yaw": round((yaw_of(actor["transform"]["Orientation"]) - args.yaw + 540) % 360 - 180, 2),
                                         "quat": local_quat(actor["transform"]["Orientation"], args.yaw),
                                         "scale": vec(scales[actor["scaleIndex"]]) if 0 <= actor["scaleIndex"] < len(scales) else None,
                                         "shapes": kinds})
            os.remove(jp)
    finally:
        shutil.rmtree(tmp, ignore_errors=True)
    json.dump({"origin": origin, "yaw": args.yaw, "rows": rows}, open(args.out, "w"), indent=1)
    from collections import Counter
    print(len(rows), "instances in the footprint;", dict(Counter(r["type"] for r in rows)))


if __name__ == "__main__":
    main()
