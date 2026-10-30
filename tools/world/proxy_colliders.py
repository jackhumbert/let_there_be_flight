"""Collision for proxy-only geometry: parts of the city the game draws only as proxy meshes and
never gives collision (e.g. the raised complex at (-1600, -680), 150 to 350 m up: flying cars
pass through it).

Steps:

  scan     Every proxy placement in the exterior sectors (worldCookedGenericProxyMeshNode inner
           nodes, worldBuildingProxyMeshNode, worldEntityProxyMeshNode ...): mesh depot path,
           transform, scale, sector, globalNodeId, auto hide distances -> placements.json.
  export   Export the referenced proxy meshes to .glb (as proxies.py export).
  audit    Place every proxy (levels --levels, finest first), sample its triangles and keep
           those with no real collision (sectors.py <sector>.npz) and no already kept finer
           proxy within --tol. A placement whose kept area is at least --min-area m2 is
           "uncovered". Writes uncovered.npz (kept triangles, world space, with the placement
           index of each) and audit.json (per placement: kept area, total area), plus a preview
           (--preview) of kept triangles over the city.
  force    The placements listed in proxy_collider_force.json (buildings drawn without wall
           collision that audit counted as covered: a surface under the top real surface of its
           column counts as covered there), their triangles inside each entry's box not within --tol
           of real collision -> forced.npz, for build --force.
  build    Cut the kept triangles (and with --force the forced ones) into --tile m tiles, cook each tile's triangles (PhysX 3.4,
           tools/packaging/physx_cook) relative to the tile centre and write one collider entity per tile
           (.ent WolvenKit JSON, entColliderComponent with one physicsColliderMesh) plus
           tiles.json (entity name, position) for placing them (pad_sector.py "colliders").

  python proxy_colliders.py scan --sectors <raw ep1 _compiled\\default> -o W\\proxycol [--jobs 10]
  python proxy_colliders.py export W\\proxycol --raw <unbundled proxy meshes>
  python proxy_colliders.py audit W\\proxycol --tris <sectors.py output> --city x0,y0,x1,y1
      [--preview W\\proxycol\\audit.png]
  python proxy_colliders.py build W\\proxycol --entities <out dir> [--tile 128]
"""

import argparse
import collections
import glob
import json
import math
import os
import re
import shutil
import subprocess
import sys
import tempfile
from concurrent.futures import ProcessPoolExecutor, ThreadPoolExecutor, as_completed

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))  # tools/, for common
from common import paths  # noqa: E402
from common import toolpath  # noqa: E402,F401 (modules from other tool folders)

from glb import read_glb_triangles
from proxies import ARCHIVES, glb_path
from sectors import DEFAULT_WOLVENKIT, quat_matrix

PROXY_TYPES = (
    "worldCookedGenericProxyMeshNode",
    "worldBuildingProxyMeshNode",
    "worldEntityProxyMeshNode",
    "worldDestructibleEntityProxyMeshNode",
    "worldDestructibleProxyMeshNode",
    "worldTerrainProxyMeshNode",
)
BATCH = 40


# ---------------------------------------------------------------------------------------- scan


def _mesh_of(node, inner, d):
    if inner is not None:
        i = int(d["instanceIndex"])
        if i >= len(inner):
            return None, {}
        n = inner[i]
    else:
        n = node
    mesh = n.get("mesh", {}).get("DepotPath", {}).get("$value")
    extra = {k: n[k] for k in ("nearAutoHideDistance", "forcedAutoHideDistance") if k in n}
    return mesh, extra


def scan_json(path, sector):
    rc = json.load(open(path, encoding="utf-8"))["Data"]["RootChunk"]
    nodes = rc["nodes"]
    out = []
    for k, d in enumerate(rc["nodeData"]["Data"]):
        node = nodes[int(d["nodeIdx"])]["Data"]
        t = node["$type"]
        if t not in PROXY_TYPES:
            continue
        buf = node.get("buffer")
        inner = buf.get("Data", {}).get("InnerNodes") if isinstance(buf, dict) else None
        mesh, extra = _mesh_of(node, inner, d)
        if not mesh or not str(mesh).endswith(".mesh"):
            continue
        tr = d["transform"]
        out.append(
            dict(
                sector=sector,
                index=k,
                type=t,
                mesh=mesh,
                pos=[tr["position"][c] for c in "XYZ"],
                quat=[tr["orientation"][c] for c in ("i", "j", "k", "r")],
                scale=[d["scale"][c] for c in "XYZ"],
                gid=d["globalNodeId"]["hash"],
                streaming=d.get("streamingDistance"),
                **extra,
            )
        )
    return out


def scan_batch(wolvenkit, files):
    tmp = tempfile.mkdtemp(prefix="pc_")
    try:
        subprocess.run([wolvenkit, "cr2w", "-s", *files, "-o", tmp, "-v", "Minimal"],
                       stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, check=False)
        out, bad = [], []
        for f in files:
            name = os.path.basename(f)
            jp = os.path.join(tmp, name + ".json")
            try:
                out += scan_json(jp, name[: -len(".streamingsector")])
            except Exception as err:
                bad.append((name, repr(err)[:80]))
        return out, bad
    finally:
        shutil.rmtree(tmp, ignore_errors=True)


def cmd_scan(args):
    os.makedirs(args.work, exist_ok=True)
    pat = re.compile(args.match)
    files = sorted(os.path.join(args.sectors, f) for f in os.listdir(args.sectors) if pat.search(f))
    batches = [files[i : i + BATCH] for i in range(0, len(files), BATCH)]
    print(f"{len(files)} sectors in {len(batches)} batches", file=sys.stderr)
    rows, bad, done = [], [], 0
    with ProcessPoolExecutor(args.jobs) as pool:
        futs = [pool.submit(scan_batch, args.wolvenkit, b) for b in batches]
        for f in as_completed(futs):
            r, b = f.result()
            rows += r
            bad += b
            done += 1
            if done % 10 == 0:
                print(f"{done}/{len(batches)} batches, {len(rows)} placements", file=sys.stderr)
    rows.sort(key=lambda r: (r["sector"], r["index"]))
    json.dump(rows, open(os.path.join(args.work, "placements.json"), "w"))
    print(f"{len(rows)} placements, {len(bad)} sectors failed: {bad[:5]}", file=sys.stderr)


# -------------------------------------------------------------------------------------- export


def cmd_export(args):
    rows = json.load(open(os.path.join(args.work, "placements.json")))
    depots = sorted({r["mesh"] for r in rows})
    raw = args.raw
    missing = [d for d in depots if not os.path.exists(os.path.join(raw, d))]
    if missing:
        print(f"unbundling ({len(missing)} meshes not in {raw})", file=sys.stderr)
        for a in ARCHIVES:
            subprocess.run([args.wolvenkit, "unbundle", a, "-o", raw, "-r", r"_external.proxy.*\.mesh$",
                            "-v", "Minimal"], stdout=subprocess.DEVNULL, check=False)
        # the rest (prefab proxies, shop signs ...) by exact path, from every game archive
        missing = [d for d in depots if not os.path.exists(os.path.join(raw, d))
                   and not d.startswith("base\\worlds\\03_night_city\\_global")]  # terrain proxies
        folders = [os.path.join(os.path.dirname(os.path.dirname(ARCHIVES[0])), x) for x in ("content", "ep1")]
        for i in range(0, len(missing), 25):
            rx = "^(" + "|".join(re.escape(d) for d in missing[i : i + 25]) + ")$"
            subprocess.run([args.wolvenkit, "unbundle", *folders, "-o", raw, "-r", rx, "-v", "Minimal"],
                           stdout=subprocess.DEVNULL, check=False)
    todo = collections.defaultdict(list)
    for d in depots:
        src = os.path.join(raw, d)
        if os.path.exists(src) and not os.path.exists(glb_path(args.work, d)):
            todo[d.replace("/", "\\").split("\\")[-2]].append(src)
    # names clash only across folders: a batch never holds two files with the same name
    # and the command line stays under Windows' 32k characters
    jobs, batch, names, chars = [], [], set(), 0
    for folder, all_fs in sorted(todo.items()):
        for k in range(0, len(all_fs), 40):
            fs = all_fs[k : k + 40]
            base = {os.path.basename(f) for f in fs}
            size = sum(len(f) + 3 for f in fs)
            if batch and (chars + size > 20000 or names & base):
                jobs.append(batch)
                batch, names, chars = [], set(), 0
            batch.append((folder, fs))
            names |= base
            chars += size
    if batch:
        jobs.append(batch)
    print(f"{len(depots)} meshes, {sum(map(len, todo.values()))} to export", file=sys.stderr)

    def run(batch):
        tmp = os.path.join(args.work, "tmp", batch[0][0])
        os.makedirs(tmp, exist_ok=True)
        subprocess.run([args.wolvenkit, "export", *[f for _, fs in batch for f in fs], "-o", tmp,
                        "-v", "Minimal"], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, check=False)
        for folder, fs in batch:
            out = os.path.join(args.work, "glb", folder)
            os.makedirs(out, exist_ok=True)
            for f in fs:
                g = os.path.join(tmp, os.path.basename(f)[: -len(".mesh")] + ".glb")
                if os.path.exists(g):
                    os.replace(g, os.path.join(out, os.path.basename(g)))
        return sum(len(fs) for _, fs in batch)

    done = 0
    with ThreadPoolExecutor(args.jobs) as pool:
        for n in pool.map(run, jobs):
            done += n
            print(f"{done} exported", file=sys.stderr)
    have = sum(os.path.exists(glb_path(args.work, d)) for d in depots)
    print(f"{have} of {len(depots)} meshes have a glb", file=sys.stderr)


# --------------------------------------------------------------------------------------- audit


def sector_level(sector):
    return int(sector.rsplit("_", 1)[1])


def tri_area(t):
    return 0.5 * np.linalg.norm(np.cross(t[:, 1] - t[:, 0], t[:, 2] - t[:, 0]), axis=1)


def sample(tris, step):
    """Points on each triangle at most about `step` apart (a barycentric grid, vertices
    included), and the triangle index of each point."""
    t = tris.astype(np.float64)
    edge = np.max(np.linalg.norm(t - np.roll(t, 1, axis=1), axis=2), axis=1)
    n = np.clip(np.ceil(edge / step), 1, 200).astype(int)
    pts, ids = [], []
    for k in np.unique(n):
        sel = np.nonzero(n == k)[0]
        a, b = np.meshgrid(np.arange(k + 1), np.arange(k + 1), indexing="ij")
        m = a + b <= k
        u, v = a[m] / k, b[m] / k
        w = 1.0 - u - v
        p = (w[None, :, None] * t[sel, 0][:, None] + u[None, :, None] * t[sel, 1][:, None]
             + v[None, :, None] * t[sel, 2][:, None])
        pts.append(p.reshape(-1, 3))
        ids.append(np.repeat(sel, len(u)))
    return np.concatenate(pts), np.concatenate(ids)


def _solid_file(path, lookup, voxel, tile):
    """Voxels (int16 x y z indices) of one sector's vehicle-blocking collision, per xy tile."""
    d = np.load(path)
    t = d["tris"]
    if not len(t):
        return {}
    names = [str(n) for n in d["preset_names"]]
    keep = np.array([lookup.get(n, {"blocks_vehicles": True})["blocks_vehicles"] for n in names])
    t = t[keep[d["preset"]]]
    if not len(t):
        return {}
    out = {}
    for i in range(0, len(t), 200000):
        pts = sample(t[i : i + 200000], voxel * 0.75)[0]
        v = np.floor(pts / voxel).astype(np.int64)
        v = v[(np.abs(v) < 32000).all(1)]
        tk = np.floor(v[:, :2] * voxel / tile).astype(np.int64)
        key = (tk[:, 0] + 1000) * 2000 + (tk[:, 1] + 1000)
        for k in np.unique(key):
            out.setdefault(int(k), []).append(np.unique(v[key == k], axis=0).astype(np.int16))
    return {k: np.unique(np.concatenate(v), axis=0) for k, v in out.items()}


def _tile_name(key):
    return f"{key // 2000 - 1000}_{key % 2000 - 1000}"


def cmd_solid(args):
    """Real collision as surface voxels per --tile xy tile: work/solid/<tx>_<ty>_<part>.npy."""
    from presets import load_lookup

    lookup = load_lookup(args.presets)
    out_dir = os.path.join(args.work, "solid")
    os.makedirs(out_dir, exist_ok=True)
    files = [f for f in glob.glob(os.path.join(args.tris, "*.npz")) if os.path.basename(f).count(".") == 1]
    pending, n, part = collections.defaultdict(list), 0, collections.Counter()

    def flush():
        for k, chunks in pending.items():
            np.save(os.path.join(out_dir, f"{_tile_name(k)}_{part[k]}.npy"), np.concatenate(chunks))
            part[k] += 1
        pending.clear()

    with ProcessPoolExecutor(args.jobs) as pool:
        futs = [pool.submit(_solid_file, f, lookup, args.voxel, args.tile) for f in files]
        for i, f in enumerate(as_completed(futs)):
            for k, v in f.result().items():
                pending[k].append(v)
                n += len(v)
            if n > 30_000_000:
                flush()
                n = 0
            if (i + 1) % 1000 == 0:
                print(f"{i + 1}/{len(files)} sector files", file=sys.stderr)
    flush()
    json.dump(dict(voxel=args.voxel, tile=args.tile), open(os.path.join(out_dir, "meta.json"), "w"))
    print(f"{len(part)} tiles", file=sys.stderr)


_glb_cache = {}


def _mesh_tris(work, mesh):
    g = glb_path(work, mesh)
    if g not in _glb_cache:
        if len(_glb_cache) > 3000:
            _glb_cache.clear()
        try:
            _glb_cache[g] = read_glb_triangles(g) if os.path.exists(g) else None
        except Exception:
            _glb_cache[g] = None
    return _glb_cache[g]


def _placed(work, r):
    t = _mesh_tris(work, r["mesh"])
    if t is None or not len(t):
        return None
    rot = quat_matrix(dict(zip(("i", "j", "k", "r"), r["quat"])))
    return (t.astype(np.float64) * np.asarray(r["scale"])) @ rot.T + np.asarray(r["pos"])


def _audit_tile(work, key, rows, ids, voxel, tile, reach, box, split_edge, terrain_dir, tol):
    """Uncovered triangles of the proxies whose triangle centres fall in this tile."""
    from scipy import ndimage

    tx, ty = key // 2000 - 1000, key % 2000 - 1000
    tris, owner = [], []
    for i in ids:
        w = _placed(work, rows[i])
        if w is None:
            continue
        c = w.mean(1)
        m = (np.floor(c[:, 0] / tile) == tx) & (np.floor(c[:, 1] / tile) == ty)
        if box is not None:
            m &= (c[:, 0] >= box[0]) & (c[:, 0] <= box[2]) & (c[:, 1] >= box[1]) & (c[:, 1] <= box[3])
        if m.any():
            tris.append(w[m])
            owner.append(np.full(m.sum(), i, np.int32))
    if not tris:
        return None
    tris, owner = np.concatenate(tris), np.concatenate(owner)
    q = np.floor(sample(tris, voxel * 0.75)[0] / voxel).astype(np.int64)
    lo, hi = q.min(0) - reach, q.max(0) + reach
    grid = np.zeros(tuple(hi - lo + 1), bool)
    for dx in (-1, 0, 1):
        for dy in (-1, 0, 1):
            for f in glob.glob(os.path.join(work, "solid", f"{tx + dx}_{ty + dy}_*.npy")):
                v = np.load(f).astype(np.int64)
                v = v[np.all((v >= lo) & (v <= hi), axis=1)]
                grid[tuple((v - lo).T)] = True
    solid = ndimage.binary_dilation(grid, np.ones((3, 3, 3), bool), iterations=reach)
    # under the top real surface of a column counts as covered: proxy undersides and bottoms
    # below streets and decks are out of reach (and the rule keeps them from making bumps)
    top = np.where(grid.any(2), grid.shape[2] - 1 - np.argmax(grid[:, :, ::-1], axis=2), -1)
    top = top.astype(np.int64)
    shape = np.array(grid.shape)

    ground, tmeta = None, None
    if terrain_dir:  # rasterize.py --no-collision output: per 2 m cell the terrain height (-inf: none)
        ground = np.load(os.path.join(terrain_dir, "terrain.npy"), mmap_mode="r")
        tmeta = json.load(open(os.path.join(terrain_dir, "meta.json")))

    def uncovered(t):
        """Per triangle: has an uncovered sample, has a covered one; the samples' voxels."""
        p, ids = sample(t, voxel * 0.75)
        v = np.clip(np.floor(p / voxel).astype(np.int64) - lo, 0, shape - 1)
        cov = solid[tuple(v.T)] | (v[:, 2] < top[v[:, 0], v[:, 1]])
        if ground is not None:  # on or under the terrain
            cx = np.clip(np.floor((p[:, 0] - tmeta["origin"][0]) / tmeta["cell"]).astype(np.int64), 0, ground.shape[1] - 1)
            cy = np.clip(np.floor((p[:, 1] - tmeta["origin"][1]) / tmeta["cell"]).astype(np.int64), 0, ground.shape[0] - 1)
            cov |= p[:, 2] < ground[cy, cx] + tol
        some_out = np.zeros(len(t), bool)
        some_in = np.zeros(len(t), bool)
        some_out[ids[~cov]] = True
        some_in[ids[cov]] = True
        return some_out, some_in, v, ids

    level = np.array([sector_level(rows[i]["sector"]) for i in owner])
    kept_t, kept_o = [], []
    for L in sorted(set(level.tolist())):
        t, o = tris[level == L], owner[level == L]
        out_t, out_o, out_v = [], [], []
        while len(t):
            some_out, some_in, v, ids = uncovered(t)
            edge = np.max(np.linalg.norm(t - np.roll(t, 1, axis=1), axis=2), axis=1)
            split = some_out & some_in & (edge > split_edge)
            keep = some_out & ~split
            out_t.append(t[keep])
            out_o.append(o[keep])
            out_v.append(v[keep[ids]])
            # partly covered: cut in four at the edge midpoints and test the pieces
            s = t[split]
            m01, m12, m20 = (s[:, 0] + s[:, 1]) / 2, (s[:, 1] + s[:, 2]) / 2, (s[:, 2] + s[:, 0]) / 2
            t = np.concatenate([np.stack(x, 1) for x in ((s[:, 0], m01, m20), (m01, s[:, 1], m12),
                                                         (m20, m12, s[:, 2]), (m01, m12, m20))])
            o = np.tile(o[split], 4)
        kt = np.concatenate(out_t)
        kept_t.append(kt)
        kept_o.append(np.concatenate(out_o))
        if len(kt):  # finer kept proxies cover the coarser ones after them
            g = np.zeros_like(grid)
            g[tuple(np.concatenate(out_v).T)] = True
            solid |= ndimage.binary_dilation(g, np.ones((3, 3, 3), bool), iterations=reach)
    kt, ko = np.concatenate(kept_t), np.concatenate(kept_o)
    return (kt.astype(np.float32), ko, np.bincount(owner, tri_area(tris), len(rows)),
            np.bincount(ko, tri_area(kt), len(rows)))


def cmd_audit(args):
    rows = json.load(open(os.path.join(args.work, "placements.json")))
    box = [float(x) for x in args.city.split(",")] if args.city else None
    levels = {int(x) for x in args.levels.split(",")}
    meta = json.load(open(os.path.join(args.work, "solid", "meta.json")))
    voxel, tile = meta["voxel"], meta["tile"]
    reach = max(1, int(math.ceil(args.tol / voxel)))
    pick = [i for i, r in enumerate(rows) if sector_level(r["sector"]) in levels
            and r["type"] not in args.skip_types.split(",") and not re.search(args.exclude, r["mesh"])]
    # the tiles each placement's triangles may land in, from its world bounds
    tiles = collections.defaultdict(list)
    for i in pick:
        w = _placed(args.work, rows[i])
        if w is None:
            continue
        f = w.reshape(-1, 3)
        lo, hi = f.min(0), f.max(0)
        if box is not None and (hi[0] < box[0] or lo[0] > box[2] or hi[1] < box[1] or lo[1] > box[3]):
            continue
        for tx in range(int(lo[0] // tile), int(hi[0] // tile) + 1):
            for ty in range(int(lo[1] // tile), int(hi[1] // tile) + 1):
                tiles[(tx + 1000) * 2000 + (ty + 1000)].append(i)
    print(f"{len(pick)} placements over {len(tiles)} tiles", file=sys.stderr)
    _glb_cache.clear()

    kept_t, kept_o = [], []
    total = np.zeros(len(rows))
    kept_area = np.zeros(len(rows))
    done = 0
    with ProcessPoolExecutor(args.jobs) as pool:
        futs = [pool.submit(_audit_tile, args.work, k, rows, ids, voxel, tile, reach, box, args.split,
                               args.terrain, args.tol)
                for k, ids in sorted(tiles.items())]
        for f in as_completed(futs):
            r = f.result()
            done += 1
            if done % 100 == 0:
                print(f"{done}/{len(futs)} tiles", file=sys.stderr)
            if r is None:
                continue
            kept_t.append(r[0])
            kept_o.append(r[1])
            total += r[2]
            kept_area += r[3]
    tris = np.concatenate(kept_t) if kept_t else np.zeros((0, 3, 3), np.float32)
    owner = np.concatenate(kept_o) if kept_o else np.zeros(0, np.int32)
    use = kept_area >= args.min_area
    m = use[owner]
    np.savez(os.path.join(args.work, "uncovered.npz"), tris=tris[m], owner=owner[m])
    audit = [dict(r, row=i, kept_area=round(float(kept_area[i]), 1), area=round(float(total[i]), 1))
             for i, r in enumerate(rows) if use[i]]
    json.dump(dict(placements=audit), open(os.path.join(args.work, "audit.json"), "w"))
    print(f"{use.sum()} placements uncovered ({m.sum()} triangles, {kept_area[use].sum() / 1e6:.2f} km2)",
          file=sys.stderr)
    if args.preview:
        preview(tris[m], box, args.preview)


# --------------------------------------------------------------------------------------- build


def cmd_force(args):
    from scipy import ndimage

    rows = json.load(open(os.path.join(args.work, "placements.json")))
    index = {(r["sector"], str(r["gid"]), r["mesh"]): i for i, r in enumerate(rows)}
    meta = json.load(open(os.path.join(args.work, "solid", "meta.json")))
    voxel, tile = meta["voxel"], meta["tile"]
    reach = max(1, int(math.ceil(args.tol / voxel)))
    entries = json.load(open(args.list))["buildings"]
    kept_t, kept_o, missing = [], [], 0
    for e in entries:
        lo, hi = np.array(e["box"][:3], float), np.array(e["box"][3:], float)
        tris, owner = [], []
        for pl in e["placements"]:
            i = index.get((pl["sector"], str(pl["gid"]), pl["mesh"]))
            w = _placed(args.work, rows[i]) if i is not None else None
            if w is None:
                missing += 1
                continue
            m = np.all(w.max(1) >= lo, 1) & np.all(w.min(1) <= hi, 1)
            tris.append(w[m])
            owner.append(np.full(m.sum(), i, np.int32))
        if not tris:
            continue
        tris, owner = np.concatenate(tris), np.concatenate(owner)
        if not len(tris):
            continue
        # real collision near the box (the audit's solid voxels), grown by --tol
        vlo = np.floor(lo / voxel).astype(np.int64) - reach - 1
        vhi = np.floor(hi / voxel).astype(np.int64) + reach + 1
        grid = np.zeros(tuple(vhi - vlo + 1), bool)
        for tx in range(int(lo[0] // tile) - 1, int(hi[0] // tile) + 2):
            for ty in range(int(lo[1] // tile) - 1, int(hi[1] // tile) + 2):
                for f in glob.glob(os.path.join(args.work, "solid", f"{tx}_{ty}_*.npy")):
                    v = np.load(f).astype(np.int64)
                    v = v[np.all((v >= vlo) & (v <= vhi), axis=1)]
                    grid[tuple((v - vlo).T)] = True
        solid = ndimage.binary_dilation(grid, np.ones((3, 3, 3), bool), iterations=reach)
        pts, ids = sample(tris, voxel * 0.75)
        v = np.clip(np.floor(pts / voxel).astype(np.int64) - vlo, 0, np.array(grid.shape) - 1)
        out = np.zeros(len(tris), bool)
        out[ids[~solid[tuple(v.T)]]] = True
        kept_t.append(tris[out].astype(np.float32))
        kept_o.append(owner[out])
        print(f"({e['near'][0]}, {e['near'][1]}): {out.sum()} of {len(tris)} triangles, {tri_area(tris[out]).sum():.0f} m2",
              file=sys.stderr)
    tris = np.concatenate(kept_t) if kept_t else np.zeros((0, 3, 3), np.float32)
    owner = np.concatenate(kept_o) if kept_o else np.zeros(0, np.int32)
    # overlapping boxes take the same triangle more than once
    _, first = np.unique(np.round(tris.reshape(len(tris), 9), 2), axis=0, return_index=True)
    first = np.sort(first)
    tris, owner = tris[first], owner[first]
    np.savez(os.path.join(args.work, "forced.npz"), tris=tris, owner=owner)
    print(f"{len(entries)} buildings, {len(tris)} triangles ({tri_area(tris).sum() / 1e6:.3f} km2), "
          f"{missing} placements not found", file=sys.stderr)


def cmd_build(args):
    import base64
    import copy

    from pad_entity import cname, renumber
    from physx_cook import cook

    d = np.load(os.path.join(args.work, "uncovered.npz"))
    tris = d["tris"]
    if args.force:
        tris = np.concatenate([tris, np.load(os.path.join(args.work, "forced.npz"))["tris"]])
    # the same triangles for the nav map bake (spans.py bin --extra)
    np.savez(os.path.join(args.work, "colliders.npz"), tris=tris, preset=np.zeros(len(tris), np.uint16),
             preset_names=np.array(["World Static"]))
    tris = tris.astype(np.float64)
    tpl = json.load(open(os.path.join(paths.TOOLS, "pads", "pad_templates.json"),
                         encoding="utf-8"))
    c = tris.mean(1)
    t = np.floor(c[:, :2] / args.tile).astype(int)
    keys = sorted({(int(a), int(b)) for a, b in t})
    tiles, meshes = [], []
    for tx, ty in keys:
        m = (t[:, 0] == tx) & (t[:, 1] == ty)
        tt = tris[m]
        origin = np.array([(tx + 0.5) * args.tile, (ty + 0.5) * args.tile, float(np.floor(tt[:, :, 2].min()))])
        verts, idx = np.unique(np.round((tt - origin).reshape(-1, 3), 3), axis=0, return_inverse=True)
        faces = idx.reshape(-1, 3)
        faces = faces[(faces[:, 0] != faces[:, 1]) & (faces[:, 1] != faces[:, 2]) & (faces[:, 0] != faces[:, 2])]
        reach = float(np.max(np.linalg.norm(verts[:, :2], axis=1)))
        tiles.append(dict(tile=[tx, ty], pos=origin.tolist(), tris=len(faces), reach=reach))
        meshes.append((verts, faces))
    blobs = cook(meshes)

    os.makedirs(args.entities, exist_ok=True)
    rows, failed = [], 0
    for k, (tile, blob) in enumerate(zip(tiles, blobs)):
        if not blob:
            failed += 1
            continue
        tx, ty = tile["tile"]
        name = f"ltbf_proxycol_{tx}_{ty}".replace("-", "m")
        ent = copy.deepcopy(tpl["ent"])
        rc = ent["Data"]["RootChunk"]
        rc["appearances"] = []  # collision only, the proxy stays the visual
        col = copy.deepcopy(tpl["collider_component"])
        col["name"] = cname("proxy_collision")
        col["id"] = str(0x3F00000000000000)
        mc = copy.deepcopy(tpl["mesh_collider"])
        mc["compiledGeometryBuffer"]["Bytes"] = base64.b64encode(blob).decode()
        mc["material"] = cname("concrete.physmat")
        col["colliders"] = [{"HandleId": "0", "Data": mc}]
        rc["components"] = [col]
        json.dump(renumber(ent), open(os.path.join(args.entities, f"{name}.ent.json"), "w", encoding="utf-8"), indent=1)
        rows.append(dict(entity=f"{args.depot}\\{name}.ent", pos=[round(v, 3) for v in tile["pos"]],
                         streaming_distance=round(tile["reach"] + args.margin, 1),
                         node_id=args.node_id + k, tris=tile["tris"]))
    out = dict(comment="proxy_colliders.py build: collision for proxy-only geometry, one collider "
                       "entity per tile (pad_sector.py \"colliders\")", tiles=rows)
    json.dump(out, open(args.tiles, "w", encoding="utf-8"), indent=1)
    print(f"{len(rows)} tiles ({sum(r['tris'] for r in rows)} triangles, "
          f"{sum(len(b) for b in blobs) / 1e6:.1f} MB cooked), {failed} failed to cook", file=sys.stderr)


def preview(kept, box, path):
    import matplotlib

    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    from matplotlib.collections import PolyCollection

    fig, ax = plt.subplots(figsize=(18, 18))
    z = kept[:, :, 2].max(1)
    pc = PolyCollection(kept[:, :, :2], array=z, cmap="plasma", edgecolor="none")
    ax.add_collection(pc)
    fig.colorbar(pc, ax=ax, shrink=0.5, label="top z of uncovered proxy triangles")
    allp = kept.reshape(-1, 3)
    lo, hi = (box[:2], box[2:]) if box else (allp.min(0)[:2], allp.max(0)[:2])
    ax.set_xlim(lo[0], hi[0])
    ax.set_ylim(lo[1], hi[1])
    ax.set_aspect("equal")
    ax.grid(alpha=0.3)
    plt.tight_layout()
    plt.savefig(path, dpi=80)


# ---------------------------------------------------------------------------------------- main


def main():
    ap = argparse.ArgumentParser()
    sub = ap.add_subparsers(dest="cmd", required=True)
    p = sub.add_parser("scan")
    p.add_argument("--sectors", required=True)
    p.add_argument("-o", "--work", required=True)
    p.add_argument("--match", default=r"^exterior_.*\.streamingsector$")
    p.add_argument("--jobs", type=int, default=10)
    p.add_argument("--wolvenkit", default=DEFAULT_WOLVENKIT)
    p = sub.add_parser("export")
    p.add_argument("work")
    p.add_argument("--raw", required=True)
    p.add_argument("--jobs", type=int, default=8)
    p.add_argument("--wolvenkit", default=DEFAULT_WOLVENKIT)
    p = sub.add_parser("solid")
    p.add_argument("work")
    p.add_argument("--tris", required=True, help="sectors.py output (<sector>.npz world triangles)")
    p.add_argument("--presets", required=True, help="presets.py table")
    p.add_argument("--voxel", type=float, default=2.0)
    p.add_argument("--tile", type=float, default=256.0)
    p.add_argument("--jobs", type=int, default=6)
    p = sub.add_parser("audit")
    p.add_argument("work")
    p.add_argument("--city", help="x0,y0,x1,y1: only placements and triangles inside")
    p.add_argument("--levels", default="1,2,3,4", help="sector levels to take proxies from, finest first")
    p.add_argument("--skip-types", default="worldTerrainProxyMeshNode")
    p.add_argument("--exclude", default=r"(?i)ocean_patch", help="regex on the mesh path: never solid (water)")
    p.add_argument("--terrain", help="rasterize.py --no-collision folder: terrain counts as covered")
    p.add_argument("--jobs", type=int, default=6)
    p.add_argument("--tol", type=float, default=4.0, help="a surface within this is covered (m)")
    p.add_argument("--split", type=float, default=6.0, help="cut partly covered triangles down to this edge (m)")
    p.add_argument("--min-area", type=float, default=50.0, help="uncovered m2 for a placement to count")
    p.add_argument("--preview")
    p = sub.add_parser("force")
    p.add_argument("work")
    p.add_argument("--list", default=os.path.join(os.path.dirname(os.path.abspath(__file__)), "proxy_collider_force.json"))
    p.add_argument("--tol", type=float, default=4.0, help="a surface within this of real collision is covered (m)")
    p = sub.add_parser("build")
    p.add_argument("work")
    p.add_argument("--force", action="store_true", help="add forced.npz (the force step)")
    p.add_argument("--entities", required=True, help="output folder for the .ent.json files")
    p.add_argument("--tiles", default=os.path.join(os.path.dirname(os.path.abspath(__file__)), "proxy_collider_tiles.json"))
    p.add_argument("--depot", default=r"user\jackhumbert\entities")
    p.add_argument("--tile", type=float, default=128.0)
    p.add_argument("--margin", type=float, default=300.0, help="streaming distance beyond the tile's reach (m)")
    p.add_argument("--node-id", type=int, default=1000000000010000)
    args = ap.parse_args()
    {"scan": cmd_scan, "export": cmd_export, "solid": cmd_solid, "audit": cmd_audit,
     "force": cmd_force, "build": cmd_build}[args.cmd](args)


if __name__ == "__main__":
    main()
