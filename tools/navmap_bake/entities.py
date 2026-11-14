"""Collision of street furniture placed as entities and destructible meshes, for the nav map.

The cooked world collision (sectors.py) holds only what the level designers baked into
worldCookedCollisionNode. Street lamps, traffic lights and sign gantries are placed as entities
(worldEntityNode / worldDeviceNode with an .ent template), railings and similar props as
worldInstancedDestructibleMeshNode / worldPhysicalDestructionNode meshes; their collision comes
from the template's components and the meshes' physics, created when they stream in. Without them
the planner flew lines through highway lamps and sign gantries.

What counts (per shape):
- entColliderComponent: its colliders (box, capsule, sphere, convex, triangle mesh).
- entPhysicalMeshComponent: the bodies of its mesh's physics (meshMeshParamPhysics).
- entPhysicalDestructionComponent / worldPhysicalDestructionNode: the intact bodies of the
  destruction mesh (the Kinematic or Static ones; the Dynamic ones are the broken pieces), with
  the first destruction level's filter.
- worldInstancedDestructibleMeshNode: its mesh's bodies at every instance transform.
Left out: Dynamic components and nodes (props that move when hit), entPhysicalSkinnedMeshComponent
(swinging lanterns and curtains: World Dynamic physical scenes), entSimpleColliderComponent (the
interaction volumes of switches and gates), query-only shapes and bodies,
and shapes whose filter does not touch a car. A shape touches a car when its own groups
(simulation mask1) include VehicleBlocker or its collides-with groups (mask2) include Vehicle:
World Static does the first, Destructible (an intact lamp pole) the second, Debris and Bullet
logic neither. Destructibles count as solid: hitting one still costs the car most of its speed
(the broken pole stays in front of it).

Transforms: world = placement * component local (parent bindings to other components followed)
* (visualScale) body localToModel * shape localToBody. A binding to a bone or slot is taken as
the parent component's own transform. entEntityInstanceData overrides on a placement are not read.

Templates: a Soft path is unbundled from the archives; an Embedded one (Flags "Embedded") lives in
the sector's EmbeddedFiles. A template's appearance (.app) components count for the placement's
appearance (its appearanceName, else the template's defaultAppearance), and included templates
add their parts.

Steps (W = work folder; the sectors are the raw ones the full bake unbundled):
  python entities.py scan --sectors <raw sector dir> -o W [--jobs 10] [--match REGEX]
  python entities.py templates W         # unbundle + read the .ent / .app templates
  python entities.py meshes W [--depot D] # mesh physics (depot first, else unbundled)
  python entities.py build W [--min-size 0.6]  # -> W/entities.npz for spans.py bin --extra
  python entities.py report W [--near x,y,r]

entities.npz: tris (N, 3, 3) float32, preset (N,) uint16 into preset_names, blocks (len
preset_names) bool (spans.py bin uses it in place of the preset table: the table goes by the
VehicleBlocker group only, which drops Destructible), source (N,) uint32 into source_names
(template or mesh path), kind (N,) uint8 (0 static, 1 kinematic, 2 destructible intact body).
"""

import argparse
import base64
import collections
import glob
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile
from concurrent.futures import ProcessPoolExecutor, as_completed

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))  # tools/, for common
from common import paths  # noqa: E402
from common.lazy import Lazy  # noqa: E402

from geometry_cache import parse_convex, parse_mesh  # noqa: E402
from presets import fnv1a64  # noqa: E402

DEFAULT_WOLVENKIT = Lazy(lambda: paths.WOLVENKIT)
FIXED_POINT = 1.0 / 131072.0
BATCH = 60

# collision group bits (engine collision_groups.json)
GROUP_VEHICLE = 1 << 4
GROUP_VEHICLE_BLOCKER = 1 << 15

KIND_STATIC, KIND_KINEMATIC, KIND_DESTRUCTIBLE = 0, 1, 2
MESH_NODE_TYPES = ("worldInstancedDestructibleMeshNode", "worldPhysicalDestructionNode")

_BOX_CORNERS = np.array([[x, y, z] for x in (-1, 1) for y in (-1, 1) for z in (-1, 1)], np.float64)
_BOX_TRIS = np.array(
    [[0, 1, 3], [0, 3, 2], [4, 6, 7], [4, 7, 5], [0, 4, 5], [0, 5, 1],
     [2, 3, 7], [2, 7, 6], [0, 2, 6], [0, 6, 4], [1, 5, 7], [1, 7, 3]],
    np.int32,
)


# ------------------------------------------------------------------------------------- JSON


def handle_table(doc):
    """HandleId -> object, over a whole WolvenKit document (an embedded template's handles are
    numbered with its sector's)."""
    ids = {}
    stack = [doc]
    while stack:
        o = stack.pop()
        if isinstance(o, dict):
            if "HandleId" in o:
                ids[o["HandleId"]] = o.get("Data")
            stack.extend(o.values())
        elif isinstance(o, list):
            stack.extend(o)
    return ids


def deref(o, ids):
    if isinstance(o, dict):
        if "HandleRefId" in o:
            return ids.get(o["HandleRefId"])
        if "HandleId" in o:
            return o.get("Data")
    return o


def cname(o):
    return str(o["$value"]) if isinstance(o, dict) and "$value" in o else ""


def depot_path(ref):
    """A resource reference's path; a path WolvenKit could not name comes back as '#<hash>'."""
    if not isinstance(ref, dict):
        return ""
    p = ref.get("DepotPath", ref)
    if not isinstance(p, dict):
        return ""
    v = str(p.get("$value", ""))
    if p.get("$storage") == "uint64":
        return "" if v == "0" else "#" + v
    return v


def path_hash(path):
    return int(path[1:]) if path.startswith("#") else fnv1a64(path.lower())


def quat(q):
    return [float(q["i"]), float(q["j"]), float(q["k"]), float(q["r"])]


def xform(t):
    """Transform / WorldTransform -> [x, y, z, qi, qj, qk, qr]."""
    if t is None:
        return [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0]
    if "Position" in t:  # WorldTransform: fixed point
        p = [t["Position"][c]["Bits"] * FIXED_POINT for c in "xyz"]
        return p + quat(t["Orientation"])
    p = t["position"]
    return [float(p["X"]), float(p["Y"]), float(p["Z"])] + quat(t["orientation"])


def vec3(v, default=1.0):
    if not isinstance(v, dict):
        return [default] * 3
    return [float(v["X"]), float(v["Y"]), float(v["Z"])]


def filter_of(fd, ids):
    """physicsFilterData -> [preset, mask1, mask2] (simulation filter), or None."""
    fd = deref(fd, ids)
    if not isinstance(fd, dict):
        return None
    sim = fd.get("simulationFilter") or {}
    return [cname(fd.get("preset")), int(sim.get("mask1", 0)), int(sim.get("mask2", 0))]


def touches_car(f):
    return bool(f) and bool((f[1] & GROUP_VEHICLE_BLOCKER) or (f[2] & GROUP_VEHICLE))


# ------------------------------------------------------------------------------------ shapes


def shape_compact(s, ids):
    """A physicsCollider* -> compact dict, or None when it cannot collide."""
    if not isinstance(s, dict) or s.get("isQueryShapeOnly"):
        return None
    t = s["$type"]
    out = {"x": xform(s.get("localToBody")), "f": filter_of(s.get("filterData"), ids)}
    if t == "physicsColliderBox":
        out.update(t="box", half=vec3(s["halfExtents"]))
    elif t == "physicsColliderCapsule":
        out.update(t="capsule", r=float(s.get("radius", 0)), h=float(s.get("height", 0)))
    elif t == "physicsColliderSphere":
        out.update(t="sphere", r=float(s.get("radius", 0)))
    elif t in ("physicsColliderConvex", "physicsColliderMesh"):
        buf = s.get("compiledGeometryBuffer") or {}
        if not buf.get("Bytes"):
            return {"t": "missing:" + t}
        out.update(t="convex" if t == "physicsColliderConvex" else "mesh", blob=buf["Bytes"])
    else:
        return {"t": "unknown:" + t}
    return out


def shape_geometry(shape):
    """Compact shape -> (verts (n, 3), tris (m, 3)) in body space before localToBody."""
    t = shape["t"]
    if t == "box":
        return _BOX_CORNERS * np.asarray(shape["half"]), _BOX_TRIS
    if t == "capsule":
        # axis along local Z, as world capsules (sectors.py); a box hull is plenty at 2 m
        r, hh = shape["r"], shape["h"] / 2
        return _BOX_CORNERS * np.array([r, r, hh + r]), _BOX_TRIS
    if t == "sphere":
        return _BOX_CORNERS * shape["r"], _BOX_TRIS
    blob = base64.b64decode(shape["blob"])
    v, tr = parse_convex(blob) if t == "convex" else parse_mesh(blob)
    return v.astype(np.float64), tr


def quat_matrix(q):
    x, y, z, w = q
    return np.array(
        [
            [1 - 2 * (y * y + z * z), 2 * (x * y - z * w), 2 * (x * z + y * w)],
            [2 * (x * y + z * w), 1 - 2 * (x * x + z * z), 2 * (y * z - x * w)],
            [2 * (x * z - y * w), 2 * (y * z + x * w), 1 - 2 * (x * x + y * y)],
        ],
        np.float64,
    )


def apply(x, v, scale=None):
    """World of v under transform x ([pos, quat]) with an optional scale before the rotation."""
    if scale is not None:
        v = v * np.asarray(scale)
    return v @ quat_matrix(x[3:]).T + np.asarray(x[:3])


def compose(a, b):
    """a * b as one [pos, quat] (no scale)."""
    ra = quat_matrix(a[3:])
    p = ra @ np.asarray(b[:3]) + np.asarray(a[:3])
    x1, y1, z1, w1 = a[3:]
    x2, y2, z2, w2 = b[3:]
    q = [
        w1 * x2 + x1 * w2 + y1 * z2 - z1 * y2,
        w1 * y2 - x1 * z2 + y1 * w2 + z1 * x2,
        w1 * z2 + x1 * y2 - y1 * x2 + z1 * w2,
        w1 * w2 - x1 * x2 - y1 * y2 - z1 * z2,
    ]
    return list(p) + q


# --------------------------------------------------------------------------------- templates


PHYSICAL_MESH_TYPES = ("entPhysicalMeshComponent",)


def component_parts(components, ids, unknown):
    """Collision parts of a list of components. Part: kind ('collider' | 'mesh' | 'destruction'),
    name, x (local transform with parent bindings followed), sim, f (component filter), source
    (filterDataSource), scale, mesh, shapes."""
    byname = {cname(c.get("name")): c for c in components}

    def local(c, depth=0):
        x = xform(c.get("localTransform"))
        b = deref(c.get("parentTransform"), ids)
        if isinstance(b, dict) and depth < 8:
            parent = byname.get(cname(b.get("bindName")))
            if parent is not None and parent is not c:
                return compose(local(parent, depth + 1), x)
        return x

    parts = []
    for c in components:
        t = c.get("$type", "")
        sim = str(c.get("simulationType", ""))
        base = {"name": cname(c.get("name")), "x": local(c), "sim": sim}
        if t == "entColliderComponent":
            shapes = [shape_compact(deref(s, ids), ids) for s in c.get("colliders") or []]
            base.update(kind="collider", f=filter_of(c.get("filterData"), ids), shapes=[s for s in shapes if s])
        elif t in PHYSICAL_MESH_TYPES:
            base.update(
                kind="mesh",
                f=filter_of(c.get("filterData"), ids),
                source=str(c.get("filterDataSource", "")),
                scale=vec3(c.get("visualScale")),
                mesh=depot_path(c.get("mesh")),
                app=cname(c.get("meshAppearance")),
            )
        elif t == "entPhysicalDestructionComponent":
            levels = c.get("destructionLevelData") or []
            f = filter_of(levels[0].get("filterData"), ids) if levels else None
            base.update(kind="destruction", f=f, mesh=depot_path(c.get("mesh")), scale=[1.0, 1.0, 1.0])
        else:
            if "Physic" in t or "Collider" in t:
                unknown[t] += 1
            continue
        parts.append(base)
    return parts


def template_compact(root, ids, unknown):
    """entEntityTemplate RootChunk -> {parts, apps: {name: [app path, app appearance]}, default,
    includes}."""
    return {
        "parts": component_parts(root.get("components") or [], ids, unknown),
        "apps": {
            cname(a.get("name")): [depot_path(a.get("appearanceResource")), cname(a.get("appearanceName"))]
            for a in root.get("appearances") or []
        },
        "default": cname(root.get("defaultAppearance")),
        "includes": [depot_path(i) for i in root.get("includes") or []],
    }


def app_compact(root, ids, unknown):
    """appearanceAppearanceResource RootChunk -> {appearance name: parts}."""
    out = {}
    for a in root.get("appearances") or []:
        a = deref(a, ids)
        if isinstance(a, dict):
            out[cname(a.get("name"))] = component_parts(a.get("components") or [], ids, unknown)
    return out


# -------------------------------------------------------------------------------------- scan


def sector_placements(doc):
    """A sector's entity and destructible mesh placements, and its embedded templates."""
    rc = doc["Data"]["RootChunk"]
    ids = handle_table(doc)
    unknown = collections.Counter()
    instances = collections.defaultdict(list)
    for d in rc["nodeData"]["Data"]:
        instances[int(d["nodeIdx"])].append(d)
    embedded = {}
    for e in doc["Data"].get("EmbeddedFiles") or []:
        content = e.get("Content") or {}
        if content.get("$type") == "entEntityTemplate":
            embedded[depot_path({"DepotPath": e["FileName"]})] = template_compact(content, ids, unknown)
    entities, meshes = [], []
    for i, handle in enumerate(rc["nodes"]):
        node = handle["Data"]
        t = node["$type"]
        if "entityTemplate" in node:
            tpl = node["entityTemplate"]
            path = depot_path(tpl)
            if not path:
                continue
            for d in instances[i]:
                tr = d["transform"]
                entities.append(
                    [path, tpl.get("Flags") == "Embedded", xform(tr), vec3(d.get("scale")),
                     cname(node.get("appearanceName")), t]
                )
        elif t in MESH_NODE_TYPES:
            if t == "worldPhysicalDestructionNode":
                levels = node.get("destructionLevelData") or []
                f = filter_of(levels[0].get("filterData"), ids) if levels else None
                inst = [[0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0]]
                kind = "destruction"
            else:
                f = filter_of(node.get("filterData"), ids)
                inst = [xform(x) for x in node.get("instanceTransforms") or []]
                kind = "mesh"
            for d in instances[i]:
                meshes.append(
                    {"kind": kind, "mesh": depot_path(node.get("mesh")), "x": xform(d["transform"]),
                     "scale": vec3(d.get("scale")), "f": f, "source": str(node.get("filterDataSource", "")),
                     "sim": str(node.get("simulationType", "")), "inst": inst, "node": t}
                )
    return {"entities": entities, "meshes": meshes, "embedded": embedded, "unknown": dict(unknown)}


def serialize(wolvenkit, files, tmp_root):
    """cr2w -s a batch into a fresh folder; returns (folder, {input file: json path or None})."""
    tmp = tempfile.mkdtemp(prefix="ent_", dir=tmp_root)
    cmd = [str(wolvenkit), "cr2w", "-s", *files, "-o", tmp, "-v", "Minimal"]
    subprocess.run(cmd, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, check=False)
    out = {}
    for f in files:
        jp = os.path.join(tmp, os.path.basename(f) + ".json")
        out[f] = jp if os.path.exists(jp) else None
    return tmp, out


def scan_batch(wolvenkit, files, out_dir, tmp_root):
    tmp, jsons = serialize(wolvenkit, files, tmp_root)
    results = {}
    try:
        for f, jp in jsons.items():
            name = os.path.basename(f)
            if jp is None:
                results[name] = {"error": "not serialized"}
                continue
            try:
                p = sector_placements(json.load(open(jp, encoding="utf-8")))
            except Exception as err:
                results[name] = {"error": repr(err)}
                continue
            finally:
                os.remove(jp)
            if p["entities"] or p["meshes"]:
                json.dump(p, open(os.path.join(out_dir, name[: -len(".streamingsector")] + ".json"), "w"))
            results[name] = {"entities": len(p["entities"]), "meshes": len(p["meshes"]), "unknown": p["unknown"]}
        return results
    finally:
        shutil.rmtree(tmp, ignore_errors=True)


def cmd_scan(args):
    out = os.path.join(args.work, "placements")
    os.makedirs(out, exist_ok=True)
    tmp_root = paths.scratch("entities_")
    summary_path = os.path.join(args.work, "scan.json")
    summary = json.load(open(summary_path)) if os.path.exists(summary_path) else {}
    pattern = re.compile(args.match)
    files = sorted(
        os.path.join(args.sectors, f)
        for f in os.listdir(args.sectors)
        if pattern.search(f) and (f not in summary or "error" in summary[f])
    )
    batches = [files[i : i + BATCH] for i in range(0, len(files), BATCH)]
    print(f"{len(files)} sectors in {len(batches)} batches, {args.jobs} jobs", file=sys.stderr)
    try:
        with ProcessPoolExecutor(args.jobs) as pool:
            futures = [pool.submit(scan_batch, str(args.wolvenkit), b, out, tmp_root) for b in batches]
            for k, fut in enumerate(as_completed(futures)):
                summary.update(fut.result())
                if k % 10 == 9 or k + 1 == len(batches):
                    json.dump(summary, open(summary_path, "w"))
                    print(f"{k + 1}/{len(batches)} batches", file=sys.stderr)
    finally:
        shutil.rmtree(tmp_root, ignore_errors=True)
    json.dump(summary, open(summary_path, "w"), indent=0)
    errors = [k for k, v in summary.items() if "error" in v]
    total = sum(v.get("entities", 0) for v in summary.values())
    meshes = sum(v.get("meshes", 0) for v in summary.values())
    print(f"{total} entity placements, {meshes} mesh placements, {len(errors)} errors", file=sys.stderr)


# --------------------------------------------------------------------- resources (unbundled)


def unbundle(wolvenkit, paths_, out_dir):
    """Unbundle resources by path hash from the game's content and ep1 archives."""
    if not paths_:
        return
    os.makedirs(out_dir, exist_ok=True)
    listing = os.path.join(out_dir, "_hashes.txt")
    with open(listing, "w") as f:
        for p in paths_:
            f.write(f"{path_hash(p)}\n")
    pc = paths.ARCHIVES
    cmd = [str(wolvenkit), "unbundle", os.path.join(pc, "content"), os.path.join(pc, "ep1"),
           "-o", out_dir, "--hash", listing, "-v", "Minimal"]
    subprocess.run(cmd, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, check=False)


def local_file(path, *roots):
    """Where an unbundled resource is, by its depot path (WolvenKit names unnamed ones by hash)."""
    if path.startswith("#"):
        candidates = [path[1:] + ".bin", path[1:]]
    else:
        candidates = [path.replace("\\", os.sep)]
    for root in roots:
        for c in candidates:
            p = os.path.join(root, c)
            if os.path.exists(p):
                return p
    return None


def read_resources(wolvenkit, wanted, roots, tmp_root, read):
    """Serialize the unbundled files of `wanted` (depot paths) in batches and read each with
    read(path, doc); returns {path: result} (None where missing or unreadable)."""
    files = {}
    for p in wanted:
        lf = local_file(p, *roots)
        files[p] = lf
    out = {p: None for p in wanted}
    present = [(p, f) for p, f in files.items() if f]
    for i in range(0, len(present), BATCH):
        chunk = present[i : i + BATCH]
        # two files of one name in a batch would collide in the flat output folder
        by_name = collections.defaultdict(list)
        for p, f in chunk:
            by_name[os.path.basename(f)].append((p, f))
        rounds = max(len(v) for v in by_name.values())
        for r in range(rounds):
            sub = [v[r] for v in by_name.values() if len(v) > r]
            tmp, jsons = serialize(wolvenkit, [f for _, f in sub], tmp_root)
            try:
                for p, f in sub:
                    jp = jsons[f]
                    if jp:
                        try:
                            out[p] = read(p, json.load(open(jp, encoding="utf-8")))
                        except Exception as err:
                            print(f"  {p}: {err!r}", file=sys.stderr)
            finally:
                shutil.rmtree(tmp, ignore_errors=True)
        print(f"  {min(i + BATCH, len(present))}/{len(present)} read", file=sys.stderr)
    return out


def placement_files(work):
    return sorted(glob.glob(os.path.join(work, "placements", "*.json")))


def read_placements(work):
    """Every sector's placements; a file still being written by a running scan is skipped (the
    templates and meshes steps are rerun for what they missed)."""
    for f in placement_files(work):
        try:
            yield json.load(open(f))
        except json.JSONDecodeError:
            print(f"  skipped {os.path.basename(f)} (being written?)", file=sys.stderr)


def cmd_templates(args):
    """Read every Soft template the placements name, the .app files and includes they reach."""
    raw = os.path.join(args.work, "raw")
    tmp_root = paths.scratch("entities_")
    unknown = collections.Counter()
    templates_path = os.path.join(args.work, "templates.json")
    store = json.load(open(templates_path)) if os.path.exists(templates_path) else {"ent": {}, "app": {}}
    wanted_ent = set()
    for p in read_placements(args.work):
        for path, embedded, *_ in p["entities"]:
            if not embedded:
                wanted_ent.add(path)
    try:
        wanted_app = set()
        for _ in range(4):  # includes reach more templates
            new_ent = sorted(p for p in wanted_ent if p not in store["ent"])
            new_app = sorted(p for p in wanted_app if p not in store["app"])
            if not new_ent and not new_app:
                break
            print(f"{len(new_ent)} templates, {len(new_app)} appearance files to read", file=sys.stderr)
            unbundle(args.wolvenkit, [p for p in new_ent + new_app if not local_file(p, raw)], raw)

            def read_ent(p, doc):
                return template_compact(doc["Data"]["RootChunk"], handle_table(doc), unknown)

            def read_app(p, doc):
                return app_compact(doc["Data"]["RootChunk"], handle_table(doc), unknown)

            store["ent"].update(read_resources(args.wolvenkit, new_ent, [raw], tmp_root, read_ent))
            store["app"].update(read_resources(args.wolvenkit, new_app, [raw], tmp_root, read_app))
            for t in store["ent"].values():
                if t:
                    wanted_ent.update(t["includes"])
                    wanted_app.update(a for a, _ in t["apps"].values() if a)
            for p in read_placements(args.work):
                for t in p["embedded"].values():
                    wanted_ent.update(t["includes"])
                    wanted_app.update(a for a, _ in t["apps"].values() if a)
            json.dump(store, open(templates_path + ".tmp", "w"))
            os.replace(templates_path + ".tmp", templates_path)
    finally:
        shutil.rmtree(tmp_root, ignore_errors=True)
    missing = [p for p, t in store["ent"].items() if t is None]
    print(f"{len(store['ent'])} templates ({len(missing)} unreadable), {len(store['app'])} appearance files; "
          f"physics components not read: {dict(unknown)}", file=sys.stderr)


# ------------------------------------------------------------------------------------ meshes


def mesh_compact(doc):
    """A mesh's physics bodies: [{sim, query, x (localToModel), shapes}]."""
    rc = doc["Data"]["RootChunk"]
    ids = handle_table(doc)
    bodies = []
    for p in rc.get("parameters") or []:
        p = deref(p, ids)
        if not isinstance(p, dict) or p.get("$type") != "meshMeshParamPhysics":
            continue
        res = deref(p.get("physicsData"), ids)
        if not isinstance(res, dict):
            continue
        for b in res.get("bodies") or []:
            b = deref(b, ids)
            if not isinstance(b, dict):
                continue
            params = b.get("params") or {}
            shapes = [shape_compact(deref(s, ids), ids) for s in b.get("collisionShapes") or []]
            bodies.append(
                {"sim": str(params.get("simulationType", "")), "query": bool(b.get("isQueryBodyOnly")),
                 "x": xform(b.get("localToModel")), "shapes": [s for s in shapes if s]}
            )
    return bodies


def entity_parts(store, embedded, path, is_embedded, appearance, depth=0):
    """All collision parts of one placed template (its components, its appearance's, includes')."""
    t = embedded.get(path) if is_embedded else store["ent"].get(path)
    if not t:
        return []
    parts = list(t["parts"])
    name = appearance if appearance and appearance not in ("default", "None") else t["default"]
    app = t["apps"].get(name) or t["apps"].get(t["default"])
    if app is None and len(t["apps"]) == 1:
        app = next(iter(t["apps"].values()))
    if app and app[0]:
        parts += (store["app"].get(app[0]) or {}).get(app[1], [])
    if depth < 4:
        for inc in t["includes"]:
            parts += entity_parts(store, embedded, inc, False, appearance, depth + 1)
    return parts


def cmd_meshes(args):
    store = json.load(open(os.path.join(args.work, "templates.json")))
    wanted = set()

    def add(parts):
        for part in parts:
            if part["kind"] != "collider" and part["mesh"]:
                wanted.add(part["mesh"])

    for t in store["ent"].values():
        if t:
            add(t["parts"])
    for a in store["app"].values():
        for parts in (a or {}).values():
            add(parts)
    for p in read_placements(args.work):
        for t in p["embedded"].values():
            add(t["parts"])
        for m in p["meshes"]:
            if m["mesh"]:
                wanted.add(m["mesh"])
    meshes_path = os.path.join(args.work, "meshes.json")
    done = json.load(open(meshes_path)) if os.path.exists(meshes_path) else {}
    new = sorted(p for p in wanted if p not in done)
    raw = os.path.join(args.work, "raw")
    roots = [args.depot, raw] if args.depot else [raw]
    print(f"{len(wanted)} meshes, {len(new)} to read", file=sys.stderr)
    unbundle(args.wolvenkit, [p for p in new if not local_file(p, *roots)], raw)
    tmp_root = paths.scratch("entities_")
    try:
        done.update(read_resources(args.wolvenkit, new, roots, tmp_root, lambda p, doc: mesh_compact(doc)))
    finally:
        shutil.rmtree(tmp_root, ignore_errors=True)
    json.dump(done, open(meshes_path, "w"))
    print(f"{sum(1 for v in done.values() if v is None)} meshes missing or unreadable, "
          f"{sum(1 for v in done.values() if v)} with physics", file=sys.stderr)


# ------------------------------------------------------------------------------------- build


class Builder:
    """Placed parts -> world triangles, one array per shape."""

    def __init__(self, meshes, min_size=0.0):
        self.meshes = meshes
        self.min_size = min_size
        self.tris, self.preset, self.kind, self.source = [], [], [], []
        self.geometry = {}
        self.stats = collections.Counter()

    def _shape(self, shape, world, body_x, scale, filt, kind, source):
        if shape["t"].startswith(("unknown:", "missing:")):
            self.stats[shape["t"]] += 1
            return
        if not touches_car(filt):
            self.stats["shapes not touching a car"] += 1
            return
        key = shape.get("blob") or json.dumps(shape, sort_keys=True)
        g = self.geometry.get(key)
        if g is None:
            try:
                g = shape_geometry(shape)
            except ValueError:
                g = (np.zeros((0, 3)), np.zeros((0, 3), np.int32))
            self.geometry[key] = g
        v, idx = g
        if not len(idx):
            self.stats["degenerate shapes"] += 1
            return
        v = apply(shape["x"], v)
        if body_x is not None:
            v = apply(body_x, v)
        v = apply(world, v, scale)
        self.tris.append(v[idx].astype(np.float32))
        self.preset.append((filt[0], len(idx)))
        self.kind.append((kind, len(idx)))
        self.source.append((source, len(idx)))
        self.stats["shapes"] += 1

    def mesh(self, mesh, world, scale, filt, source_mode, sim, destruction, source):
        bodies = self.meshes.get(mesh)
        if not bodies:
            self.stats["meshes without physics"] += 1
            return
        for b in bodies:
            if b["query"]:
                continue
            if destruction and b["sim"] == "Dynamic":
                continue  # the broken pieces
            kind = KIND_DESTRUCTIBLE if destruction else (KIND_KINEMATIC if sim == "Kinematic" else KIND_STATIC)
            for s in b["shapes"]:
                if source_mode == "Parent" and filt:
                    f = filt
                else:
                    f = s.get("f") or filt
                self._shape(s, world, b["x"], scale, f, kind, source)

    def part(self, part, placement, pscale, source):
        if part["sim"] == "Dynamic":
            self.stats["dynamic parts"] += 1
            return
        world = compose(placement, part["x"])
        if part["kind"] == "collider":
            kind = KIND_KINEMATIC if part["sim"] == "Kinematic" else KIND_STATIC
            for s in part["shapes"]:
                self._shape(s, world, None, pscale, s.get("f") or part["f"], kind, source)
        else:
            scale = np.asarray(pscale) * np.asarray(part.get("scale") or [1, 1, 1])
            self.mesh(part["mesh"], world, scale, part["f"], part.get("source", ""), part["sim"],
                      part["kind"] == "destruction", source)

    def placed(self, start):
        """Drop what one placement added since `start` when all of it fits in a min_size cube: a car
        rolls over bottles and cans, and the grown solid of one would lift the floor by a voxel."""
        if self.min_size <= 0 or len(self.tris) == start:
            return
        v = np.concatenate(self.tris[start:]).reshape(-1, 3)
        if (v.max(0) - v.min(0)).max() < self.min_size:
            for lst in (self.tris, self.preset, self.kind, self.source):
                del lst[start:]
            self.stats["small placements"] += 1

    def arrays(self):
        if not self.tris:
            return None
        names = sorted({p for p, _ in self.preset})
        lookup = {p: i for i, p in enumerate(names)}
        sources = sorted({s for s, _ in self.source})
        slookup = {s: i for i, s in enumerate(sources)}
        return {
            "tris": np.concatenate(self.tris),
            "preset": np.concatenate([np.full(n, lookup[p], np.uint16) for p, n in self.preset]),
            "preset_names": np.array(names),
            "blocks": np.ones(len(names), bool),
            "kind": np.concatenate([np.full(n, k, np.uint8) for k, n in self.kind]),
            "source": np.concatenate([np.full(n, slookup[s], np.uint32) for s, n in self.source]),
            "source_names": np.array(sources),
            "shape": np.concatenate([np.full(len(t), i, np.uint32) for i, t in enumerate(self.tris)]),
        }


def build_placements(placements, store, meshes, builder):
    """Add one sector's placements (sector_placements output) to builder."""
    for path, embedded, x, scale, appearance, _node in placements["entities"]:
        start = len(builder.tris)
        for part in entity_parts(store, placements["embedded"], path, embedded, appearance):
            builder.part(part, x, scale, path)
        builder.placed(start)
    for m in placements["meshes"]:
        if m["sim"] == "Dynamic":
            builder.stats["dynamic parts"] += 1
            continue
        for inst in m["inst"]:
            start = len(builder.tris)
            world = compose(m["x"], inst)
            builder.mesh(m["mesh"], world, m["scale"], m["f"], m["source"], m["sim"],
                         m["kind"] == "destruction", m["mesh"])
            builder.placed(start)


def cmd_build(args):
    store = json.load(open(os.path.join(args.work, "templates.json")))
    meshes = json.load(open(os.path.join(args.work, "meshes.json")))
    builder = Builder(meshes, args.min_size)
    for f in placement_files(args.work):
        build_placements(json.load(open(f)), store, meshes, builder)
    arrays = builder.arrays()
    out = args.out or os.path.join(args.work, "entities.npz")
    np.savez_compressed(out, **arrays)
    kinds = np.bincount(arrays["kind"], minlength=3)
    print(f"{len(arrays['tris'])} triangles (static {kinds[0]}, kinematic {kinds[1]}, destructible {kinds[2]}) "
          f"-> {out}; {dict(builder.stats)}", file=sys.stderr)


def cmd_report(args):
    d = np.load(args.npz or os.path.join(args.work, "entities.npz"))
    tris, source = d["tris"], d["source"]
    names = d["source_names"]
    if args.near:
        x, y, r = map(float, args.near.split(","))
        c = tris.mean(1)
        sel = np.hypot(c[:, 0] - x, c[:, 1] - y) <= r
        tris, source = tris[sel], source[sel]
    counts = collections.Counter(source.tolist())
    for s, n in counts.most_common(args.top):
        t = tris[source == s]
        print(f"{n:8d}  z {t[:, :, 2].min():7.1f} .. {t[:, :, 2].max():7.1f}  {names[s]}")


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest="cmd", required=True)
    s = sub.add_parser("scan")
    s.add_argument("--sectors", required=True)
    s.add_argument("-o", "--work", required=True)
    s.add_argument("--jobs", type=int, default=max(1, (os.cpu_count() or 2) // 2))
    s.add_argument("--match", default=r"^(exterior|interior)_.*\.streamingsector$")
    for name in ("templates", "meshes", "build"):
        p = sub.add_parser(name)
        p.add_argument("work")
        if name == "meshes":
            p.add_argument("--depot", default=None, help="unbundled game files to read meshes from first")
        if name == "build":
            p.add_argument("-o", "--out", default=None)
            p.add_argument("--min-size", type=float, default=0.6,
                           help="leave out placements that fit in a cube this size (m)")
    for p in (s, sub.choices["templates"], sub.choices["meshes"]):
        p.add_argument("--wolvenkit", default=DEFAULT_WOLVENKIT)
    r = sub.add_parser("report")
    r.add_argument("work")
    r.add_argument("--npz", default=None)
    r.add_argument("--near", default=None, help="x,y,radius")
    r.add_argument("--top", type=int, default=40)
    args = ap.parse_args()
    {"scan": cmd_scan, "templates": cmd_templates, "meshes": cmd_meshes, "build": cmd_build,
     "report": cmd_report}[args.cmd](args)


if __name__ == "__main__":
    main()
