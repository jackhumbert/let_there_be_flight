"""Build a landing pad entity (<name>.ent + <name>.app, WolvenKit JSON) from a pad kit
(docs/archive/helipads.md, step 8).

The .app's appearance "default" holds one mesh component per visual part of the kit; the .ent
holds one collider component (preset World Static) with a collider per collision shape, all in
the pad's own frame (origin = the prefab's proxy origin, +Y forward). Shapes:

- boxes stay boxes; triangle / convex meshes become physicsColliderMesh / physicsColliderConvex
  with the cooked blob from pad_geometry.py inline;
- a scaled actor cannot scale a mesh collider, so its mesh shapes become the box around the
  scaled vertices (decoded geometry.npz), and its boxes get scaled half extents (--boxes-only
  does that for every mesh shape);
- shapes with the query-only "Complex Environment Collision" preset are left out.

In game (2026-10-26): colliders work in the .ent, meshes only show from the .app. An entity can
hold at most 192 placed components without a parentTransform: Entity::
ConnectFloatingComponentsToRootTransform collects them into a StaticArray<..., 192> via
ent::helper::CollectFloatingComponent (2.5 IDB 0x140406680) with no bounds check, so the 193rd
overruns a stack buffer (the crash at 0x140406729, from the minidumps). So meshes are split over
several entities (--max-meshes, default 100) all placed at the pad's transform; <name>.parts.json
lists them. (Binding the meshes to a root component would also keep them out of that array.)

  python pad_entity.py kit.json --blobs kit_geometry.npz --geometry geometry.npz
      --name ltbf_landing_pad_v2 [--depot user\\jackhumbert\\entities] -o <out dir>
  then: WolvenKit.CLI cr2w -d <out dir>\\<name>.ent.json <out dir>\\<name>.app.json -o <archive dir>
"""
import argparse
import base64
import copy
import json
import math
import os

import numpy as np

import sys

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))  # tools/, for common
from common import paths  # noqa: E402
from common import toolpath  # noqa: E402,F401 (modules from other tool folders)

from geometry_cache import GeometryLibrary

HERE = os.path.dirname(os.path.abspath(__file__))
COMPLEX = "4499208264152541502"  # Complex Environment Collision: query only
MESH_TYPES = ("worldCookedStaticMeshNode", "worldInstancedMeshNode", "worldStaticMeshNode")
FIXED = 131072.0


def qmul(a, b):
    ai, aj, ak, ar = a
    bi, bj, bk, br = b
    return [ar * bi + ai * br + aj * bk - ak * bj,
            ar * bj - ai * bk + aj * br + ak * bi,
            ar * bk + ai * bj - aj * bi + ak * br,
            ar * br - ai * bi - aj * bj - ak * bk]


def qmat(q):
    i, j, k, r = q
    return np.array([[1 - 2 * (j * j + k * k), 2 * (i * j - k * r), 2 * (i * k + j * r)],
                     [2 * (i * j + k * r), 1 - 2 * (i * i + k * k), 2 * (j * k - i * r)],
                     [2 * (i * k - j * r), 2 * (j * k + i * r), 1 - 2 * (i * i + j * j)]])


def qlist(q):
    if q is None:
        return [0.0, 0.0, 0.0, 1.0]
    if isinstance(q, dict):
        return [q.get("i", 0.0), q.get("j", 0.0), q.get("k", 0.0), q.get("r", 1.0)]
    return list(q)


def v3(v, default=(0.0, 0.0, 0.0)):
    if v is None:
        return np.array(default, float)
    if isinstance(v, dict):
        return np.array([v.get("X", 0.0), v.get("Y", 0.0), v.get("Z", 0.0)], float)
    return np.array(v, float)


def cname(s):
    s = str(s)
    if s.isdigit():
        return {"$type": "CName", "$storage": "uint64", "$value": s}
    return {"$type": "CName", "$storage": "string", "$value": s}


def world_transform(pos, q):
    return {"$type": "WorldTransform",
            "Orientation": {"$type": "Quaternion", "i": q[0], "j": q[1], "k": q[2], "r": q[3]},
            "Position": {"$type": "WorldPosition",
                         **{a: {"$type": "FixedPoint", "Bits": int(round(float(v) * FIXED))} for a, v in zip("xyz", pos)}}}


def transform(pos, q):
    return {"$type": "Transform",
            "orientation": {"$type": "Quaternion", "i": q[0], "j": q[1], "k": q[2], "r": q[3]},
            "position": {"$type": "Vector4", "W": 0, "X": float(pos[0]), "Y": float(pos[1]), "Z": float(pos[2])}}


def renumber(doc):
    """HandleIds and BufferIds 0, 1, 2, ... in document order, as WolvenKit expects."""
    n = [0]
    b = [0]

    def walk(o):
        if isinstance(o, dict):
            if "HandleId" in o:
                o["HandleId"] = str(n[0])
                n[0] += 1
            if "BufferId" in o:
                o["BufferId"] = str(b[0])
                b[0] += 1
            for v in o.values():
                walk(v)
        elif isinstance(o, list):
            for v in o:
                walk(v)

    walk(doc)
    return doc


def mesh_components(parts, tpl):
    out = []
    for i, p in enumerate(p for p in parts if p["type"] in MESH_TYPES):
        inner = p.get("inner") if isinstance(p.get("inner"), dict) else {}
        mesh = p.get("mesh") or inner.get("mesh")
        if not mesh:
            continue
        appearance = p.get("appearance") or inner.get("meshAppearance") or "default"
        c = copy.deepcopy(tpl)
        c["id"] = str(0x3000000000000000 + i)
        c["name"] = cname(f"m{i}")
        c["mesh"]["DepotPath"]["$value"] = mesh
        c["meshAppearance"] = cname(appearance)
        c["localTransform"] = world_transform(p["local"], qlist(p.get("quat")))
        s = v3(p.get("scale"), (1.0, 1.0, 1.0))
        c["visualScale"] = {"$type": "Vector3", "X": float(s[0]), "Y": float(s[1]), "Z": float(s[2])}
        out.append(c)
    return out


def colliders(parts, tpl, blobs, library, stats, boxes_only=False):
    out = []
    for p in parts:
        if p["type"] != "collisionActor":
            continue
        apos = np.array(p["local"], float)
        aq = qlist(p.get("quat"))
        ar = qmat(aq)
        scale = v3(p.get("scale"), (1.0, 1.0, 1.0))
        scaled = bool(np.any(np.abs(scale - 1.0) > 1e-4))
        for s in p["shapes"]:
            if s.get("preset") == COMPLEX:
                stats["complex skipped"] += 1
                continue
            key = s.get("key") or {}
            kind = key.get("$type")
            spos = v3(s.get("pos"))
            sq = qlist(s.get("rot"))
            sr = qmat(sq)
            if kind == "PhysicsGeometryBoxData":
                half = v3(key.get("size"))
                if scaled:
                    half = np.array([np.linalg.norm(scale * (sr[:, k] * half[k])) for k in range(3)])
                c = copy.deepcopy(tpl["box"])
                c["halfExtents"] = {"$type": "Vector3", "X": float(half[0]), "Y": float(half[1]), "Z": float(half[2])}
                c["localToBody"] = transform(apos + ar @ (scale * spos), qmul(aq, sq))
                out.append(c)
                stats["box"] += 1
            elif kind in ("PhysicsGeometryTriangleMeshData", "PhysicsGeometryConvexMeshData"):
                h = int(key["hash"])
                if scaled or boxes_only:
                    g = library.get(h)
                    if g is None:
                        stats["missing geometry"] += 1
                        continue
                    verts = scale * (spos + g[0].astype(float) @ sr.T)
                    lo, hi = verts.min(axis=0), verts.max(axis=0)
                    half = np.maximum((hi - lo) / 2.0, 0.01)
                    c = copy.deepcopy(tpl["box"])
                    c["halfExtents"] = {"$type": "Vector3", "X": float(half[0]), "Y": float(half[1]), "Z": float(half[2])}
                    c["localToBody"] = transform(apos + ar @ ((lo + hi) / 2.0), aq)
                    out.append(c)
                    stats["mesh -> box"] += 1
                    continue
                if f"h{h}" not in blobs.files:
                    stats["missing blob"] += 1
                    continue
                convex = kind == "PhysicsGeometryConvexMeshData"
                c = copy.deepcopy(tpl["convex_collider" if convex else "mesh_collider"])
                c["compiledGeometryBuffer"]["Bytes"] = base64.b64encode(blobs[f"h{h}"].tobytes()).decode()
                c["localToBody"] = transform(apos + ar @ spos, qmul(aq, sq))
                out.append(c)
                stats["convex" if convex else "mesh"] += 1
            else:
                stats[f"unknown {kind}"] += 1
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("kit")
    ap.add_argument("--blobs", help="pad_geometry.py npz (not needed when every shape is a box)")
    ap.add_argument("--geometry", help="geometry_cache npz (not needed when every shape is a box)")
    ap.add_argument("--name", required=True)
    ap.add_argument("--depot", default=r"user\jackhumbert\entities")
    ap.add_argument("--templates", default=os.path.join(HERE, "pad_templates.json"))
    ap.add_argument("--boxes-only", action="store_true",
                    help="every mesh shape as the box around its (scaled) vertices, no inline PhysX blobs")
    ap.add_argument("--max-meshes", type=int, default=100, help="mesh components per entity (engine limit: 192 unparented)")
    ap.add_argument("-o", "--out", required=True)
    args = ap.parse_args()
    tpl = json.load(open(args.templates, encoding="utf-8"))
    parts = json.load(open(args.kit))["parts"]
    blobs = np.load(args.blobs) if args.blobs else {}
    library = GeometryLibrary(args.geometry) if args.geometry else {}
    from collections import Counter
    stats = Counter()

    meshes = mesh_components(parts, tpl["mesh_component"])
    shapes = colliders(parts, tpl, blobs, library, stats, args.boxes_only)
    # at most 192 unparented placed components per entity (see the module docstring), so the pad
    # is several entities placed at the same transform: <name> (colliders + first meshes), <name>_1, ...
    chunks = [meshes[i:i + args.max_meshes] for i in range(0, len(meshes), args.max_meshes)] or [[]]
    os.makedirs(args.out, exist_ok=True)
    names = []
    for k, chunk in enumerate(chunks):
        name = args.name if k == 0 else f"{args.name}_{k}"
        names.append(name)
        app = copy.deepcopy(tpl["app"])
        look = app["Data"]["RootChunk"]["appearances"][0]["Data"]
        look["components"] = chunk
        app["Data"]["RootChunk"]["baseEntity"]["DepotPath"]["$value"] = f"{args.depot}\\{name}.ent"
        ent = copy.deepcopy(tpl["ent"])
        rc = ent["Data"]["RootChunk"]
        rc["components"] = []
        if k == 0:
            col = copy.deepcopy(tpl["collider_component"])
            col["name"] = cname("pad_collision")
            col["id"] = str(0x3F00000000000000)
            col["colliders"] = [{"HandleId": "0", "Data": c} for c in shapes]
            rc["components"] = [col]
        rc["appearances"][0]["appearanceResource"]["DepotPath"]["$value"] = f"{args.depot}\\{name}.app"
        for doc, fname in ((ent, f"{name}.ent.json"), (app, f"{name}.app.json")):
            json.dump(renumber(doc), open(os.path.join(args.out, fname), "w", encoding="utf-8"), indent=1)
    json.dump({"entities": [f"{args.depot}\\{n}.ent" for n in names]},
              open(os.path.join(args.out, f"{args.name}.parts.json"), "w"), indent=1)
    print(f"{len(meshes)} mesh components in {len(chunks)} entities ({', '.join(names)}); colliders {dict(stats)}")


if __name__ == "__main__":
    main()
