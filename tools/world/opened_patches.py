"""The space a bake change opened (free in the new nav map, solid in the old), in patches, and for
each patch whether it is inside a building the game draws. In-game collision cannot tell these
apart (the opened space has none either way); what is drawn can (the proxy meshes,
proxy_colliders.py scan / export):

- uncovered surface (the verdict): drawn surface within NEAR_OPENED of the opened space with no
  real collision within NEAR_SOLID (the new bake's raw solid), as a share of the drawn surface
  there. A hollow building's walls are drawn without collision; the open air under an overhang is
  bounded by real collision. Checked against the two known cases: the Dogtown combat tower 0.35
  (a building), the North highway ghost 0.02 (open air). Water (ocean patches) is not counted.
- hit share (for reference): rays from points in the opened space that hit drawn geometry.
  Proxies are not closed shells, so rays leak through their gaps.

    python tools/world/opened_patches.py --old <spans dir> --new <spans dir> -o <out dir>
        [--proxycol W\\proxycol] [--min-area 200] [--jobs 12]

Writes <out>/patches.json: per patch its centre, area, the opened height range, the drawn surface
bounding it and how much of it has no collision (m2 and share), the verdict (building / open /
unsure), the proxy placements with the most uncovered surface (the ones to give collision), and
<out>/labels.npy (patch id per column). Spot checks in game: put the car beside the patches the report lists.
"""

import argparse
import collections
import json
import math
import os
import sys
from concurrent.futures import ProcessPoolExecutor

import numpy as np
from scipy import ndimage

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))  # tools/, for common
from common import paths  # noqa: E402
from common import toolpath  # noqa: E402,F401

from proxy_colliders import _placed, sample, sector_level  # noqa: E402

RAY_LENGTH = 150.0
POINTS = 12
SAMPLE = 1.5        # m between samples on drawn surfaces
NEAR_OPENED = 4.0   # m from the opened space a drawn surface counts as bounding it
NEAR_SOLID = 1.5    # m from real collision a drawn sample counts as covered
VZ = 1.0            # m, height step of the local grids
DIRS = np.array([[1, 0, 0], [-1, 0, 0], [0, 1, 0], [0, -1, 0], [0, 0, 1], [0, 0, -1],
                 [1, 1, 0], [1, -1, 0], [-1, 1, 0], [-1, -1, 0],
                 [1, 0, 1], [-1, 0, 1], [0, 1, 1], [0, -1, 1]], np.float64)
DIRS /= np.linalg.norm(DIRS, axis=1, keepdims=True)


def free_runs(path):
    d = np.load(path)
    counts = d["counts"]
    off = np.concatenate([[0], np.cumsum(counts.ravel())]).astype(np.int64)
    return counts, d["runs"].astype(np.int64), d["reachable"].astype(bool), off


def column_free(spans, k, nz):
    counts, runs, reach, off = spans
    r = runs[off[k]:off[k + 1]][reach[off[k]:off[k + 1]]]
    return [(a, min(b, nz)) for a, b in r]


def gained(old_iv, new_iv):
    """Voxel ranges free in new and not in old."""
    out = []
    for a, b in new_iv:
        cuts = [(a, b)]
        for c, d in old_iv:
            nxt = []
            for x, y in cuts:
                if d <= x or c >= y:
                    nxt.append((x, y))
                    continue
                if c > x:
                    nxt.append((x, c))
                if d < y:
                    nxt.append((d, y))
            cuts = nxt
        out += cuts
    return out


def total(spans, nz):
    counts, runs, reach, off = spans
    h = (np.minimum(runs[:, 1], nz) - runs[:, 0]) * reach
    hc = np.concatenate([[0], np.cumsum(h)])
    return (hc[off[1:]] - hc[off[:-1]]).reshape(counts.shape)


def hits(origins, tris, length):
    """Per origin and direction: index of the nearest triangle hit within length, -1 for none."""
    out = np.full((len(origins), len(DIRS)), -1, np.int64)
    v0, e1, e2 = tris[:, 0], tris[:, 1] - tris[:, 0], tris[:, 2] - tris[:, 0]
    for oi, o in enumerate(origins):
        for di, d in enumerate(DIRS):
            p = np.cross(d, e2)
            det = np.einsum("ij,ij->i", e1, p)
            ok = np.abs(det) > 1e-9
            inv = np.where(ok, 1.0 / np.where(ok, det, 1.0), 0.0)
            s = o - v0
            u = np.einsum("ij,ij->i", s, p) * inv
            q = np.cross(s, e1)
            v = (q @ d) * inv
            t = np.einsum("ij,ij->i", e2, q) * inv
            m = ok & (u >= 0) & (v >= 0) & (u + v <= 1) & (t > 0) & (t < length)
            if m.any():
                out[oi, di] = np.nonzero(m)[0][np.argmin(t[m])]
    return out


def local_grids(lab, pid, sl, old, new, solid, nz, cfg):
    """The patch's surroundings on a local grid (cell x cell x VZ): the opened space grown by
    NEAR_OPENED, and the new bake's raw solid grown by NEAR_SOLID; with the grid's corner."""
    cell, vz, z0 = cfg["cell"], cfg["vz"], cfg["z0"]
    h, w = lab.shape
    pad = int(math.ceil(NEAR_OPENED / cell)) + 2
    y0, y1 = max(sl[0].start - pad, 0), min(sl[0].stop + pad, h)
    x0, x1 = max(sl[1].start - pad, 0), min(sl[1].stop + pad, w)
    opened_iv = {}
    for cy in range(sl[0].start, sl[0].stop):
        for cx in range(sl[1].start, sl[1].stop):
            if lab[cy, cx] == pid:
                k = cy * w + cx
                opened_iv[(cy, cx)] = [(z0 + a * vz, z0 + b * vz) for a, b in gained(column_free(old, k, nz), column_free(new, k, nz))]
    zs = [iv for ivs in opened_iv.values() for iv in ivs]
    if not zs:
        return None
    zlo = min(a for a, _ in zs) - NEAR_OPENED - 2
    zhi = max(b for _, b in zs) + NEAR_OPENED + 2
    nzl = int(math.ceil((zhi - zlo) / VZ))
    opened = np.zeros((y1 - y0, x1 - x0, nzl), bool)
    covered = np.zeros_like(opened)

    def mark(grid, gy, gx, a, b):
        i0, i1 = max(int(math.floor((a - zlo) / VZ)), 0), min(int(math.ceil((b - zlo) / VZ)), nzl)
        if i1 > i0:
            grid[gy, gx, i0:i1] = True

    for (cy, cx), ivs in opened_iv.items():
        for a, b in ivs:
            mark(opened, cy - y0, cx - x0, a, b)
    counts, runs, off = solid
    for cy in range(y0, y1):
        for cx in range(x0, x1):
            k = cy * w + cx
            for a, b in runs[off[k]:off[k + 1]]:
                mark(covered, cy - y0, cx - x0, z0 + int(a) * vz, z0 + min(int(b), nz) * vz)
    r = int(math.ceil(NEAR_OPENED / cell))
    opened = ndimage.binary_dilation(opened, np.ones((2 * r + 1, 2 * r + 1, 2 * int(NEAR_OPENED / VZ) + 1), bool))
    rs = int(math.ceil(NEAR_SOLID / cell))
    covered = ndimage.binary_dilation(covered, np.ones((2 * rs + 1, 2 * rs + 1, 2 * int(math.ceil(NEAR_SOLID / VZ)) + 1), bool))
    return (cfg["origin"][0] + x0 * cell, cfg["origin"][1] + y0 * cell, zlo, cell, opened, covered)


def surfaces(tris, owner, local):
    """Drawn surface bounding the opened space, and the part with no real collision near (m2),
    and per placement its uncovered m2."""
    gx0, gy0, gz0, cell, opened, covered = local
    if not len(tris):
        return 0.0, 0.0, collections.Counter()
    pts, ids = sample(tris, SAMPLE)
    ix = np.floor((pts[:, 0] - gx0) / cell).astype(np.int64)
    iy = np.floor((pts[:, 1] - gy0) / cell).astype(np.int64)
    iz = np.floor((pts[:, 2] - gz0) / VZ).astype(np.int64)
    ok = (ix >= 0) & (iy >= 0) & (iz >= 0) & (ix < opened.shape[1]) & (iy < opened.shape[0]) & (iz < opened.shape[2])
    near = np.zeros(len(pts), bool)
    near[ok] = opened[iy[ok], ix[ok], iz[ok]]
    cov = np.zeros(len(pts), bool)
    cov[ok] = covered[iy[ok], ix[ok], iz[ok]]
    # each triangle's area shared by its samples
    area = 0.5 * np.linalg.norm(np.cross(tris[:, 1] - tris[:, 0], tris[:, 2] - tris[:, 0]), axis=1)
    per = area[ids] / np.bincount(ids, minlength=len(tris))[ids]
    bare = near & ~cov
    by = collections.Counter()
    for o, a in zip(owner[ids[bare]].tolist(), per[bare].tolist()):
        by[o] += a
    return float(per[near].sum()), float(per[bare].sum()), by


_rows = None


def _init(proxycol):
    global _rows
    _rows = json.load(open(os.path.join(proxycol, "placements.json")))


def classify(task):
    pid, points, proxycol, cand, local = task
    gx0, gy0, gz0, cell, opened, _ = local
    glo = np.array([gx0, gy0, gz0])
    ghi = glo + np.array([opened.shape[1] * cell, opened.shape[0] * cell, opened.shape[2] * VZ])
    lo = np.minimum(points.min(0) - RAY_LENGTH, glo)
    hi = np.maximum(points.max(0) + RAY_LENGTH, ghi)
    tris, owner = [], []
    for i in cand:
        r = _rows[i]
        if sector_level(r["sector"]) > 4 or r["type"] == "worldTerrainProxyMeshNode" or "ocean_patch" in r["mesh"]:
            continue
        w = _placed(proxycol, r)
        if w is None:
            continue
        m = np.all(w.max(1) >= lo, 1) & np.all(w.min(1) <= hi, 1)
        if m.any():
            tris.append(w[m])
            owner.append(np.full(m.sum(), i, np.int64))
    if not tris:
        return pid, 0.0, [], 0.0, 0.0, []
    tris, owner = np.concatenate(tris), np.concatenate(owner)
    h = hits(points, tris, RAY_LENGTH)
    share = float((h >= 0).mean())
    top = collections.Counter(owner[h[h >= 0]].tolist()).most_common(6)
    m = np.all(tris.max(1) >= glo, 1) & np.all(tris.min(1) <= ghi, 1)
    drawn, bare, by = surfaces(tris[m], owner[m], local)
    return pid, share, top, drawn, bare, by.most_common(40)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--old", required=True)
    ap.add_argument("--new", required=True)
    ap.add_argument("-o", "--out", required=True)
    ap.add_argument("--proxycol", default=os.path.join(paths.WORK, "proxycol"))
    ap.add_argument("--min-area", type=float, default=200.0, help="m2: smaller patches are left out")
    ap.add_argument("--min-gain", type=float, default=10.0, help="m of free height gained for a column to count")
    ap.add_argument("--building-share", type=float, default=0.1, help="uncovered share of the drawn surface for a building")
    ap.add_argument("--open-share", type=float, default=0.05, help="uncovered share below which it is open air")
    ap.add_argument("--min-uncovered", type=float, default=200.0, help="uncovered m2 a building needs at least")
    ap.add_argument("--jobs", type=int, default=12)
    args = ap.parse_args()
    os.makedirs(args.out, exist_ok=True)
    cfg = json.load(open(os.path.join(args.new, "config.json")))
    cell, vz, z0 = cfg["cell"], cfg["vz"], cfg["z0"]
    x0, y0 = cfg["origin"]
    nz = int(round((cfg["z1"] - z0) / vz))
    old, new = free_runs(os.path.join(args.old, "free.npz")), free_runs(os.path.join(args.new, "free.npz"))
    sd = np.load(os.path.join(args.new, "solid.npz"))
    solid = (sd["counts"], sd["runs"], np.concatenate([[0], np.cumsum(sd["counts"].ravel())]).astype(np.int64))
    gain = (total(new, nz) - total(old, nz)) * vz
    mask = gain > args.min_gain
    lab, n = ndimage.label(mask)
    np.save(os.path.join(args.out, "labels.npy"), lab.astype(np.int32))
    sizes = np.bincount(lab.ravel(), minlength=n + 1)
    keep = [k for k in range(1, n + 1) if sizes[k] * cell * cell >= args.min_area]
    print(f"{n} patches, {len(keep)} of at least {args.min_area:.0f} m2", file=sys.stderr)
    w = lab.shape[1]
    objects = ndimage.find_objects(lab)
    rng = np.random.default_rng(0)
    patches, tasks = {}, []
    rows = json.load(open(os.path.join(args.proxycol, "placements.json")))
    pos = np.array([r["pos"][:2] for r in rows])
    for k in keep:
        sl = objects[k - 1]
        ys, xs = np.nonzero(lab[sl] == k)
        ys, xs = ys + sl[0].start, xs + sl[1].start
        zlo, zhi, pts = [], [], []
        for j in rng.choice(len(ys), min(len(ys), 200), replace=False):
            col = ys[j] * w + xs[j]
            iv = gained(column_free(old, col, nz), column_free(new, col, nz))
            if not iv:
                continue
            a, b = max(iv, key=lambda ab: ab[1] - ab[0])
            zlo.append(z0 + a * vz)
            zhi.append(z0 + b * vz)
            pts.append((x0 + (xs[j] + 0.5) * cell, y0 + (ys[j] + 0.5) * cell, z0 + 0.5 * (a + b) * vz))
        if not pts:
            continue
        local = local_grids(lab, k, sl, old, new, solid, nz, cfg)
        if local is None:
            continue
        pts = np.array(pts)
        pick = pts[rng.choice(len(pts), min(len(pts), POINTS), replace=False)]
        c = pts.mean(0)
        reach = RAY_LENGTH + 600.0  # proxies are placed by their pivot; big ones reach far from it
        cand = np.nonzero((np.abs(pos[:, 0] - c[0]) < reach) & (np.abs(pos[:, 1] - c[1]) < reach))[0].tolist()
        patches[k] = {"id": int(k), "x": round(float(c[0])), "y": round(float(c[1])),
                      "area_m2": int(sizes[k] * cell * cell), "z": [round(float(np.median(zlo))), round(float(np.median(zhi)))],
                      "gained_m3": int(gain[ys, xs].sum() * cell * cell)}
        tasks.append((int(k), pick, args.proxycol, cand, local))
    print(f"{len(tasks)} patches to classify", file=sys.stderr, flush=True)
    done = 0
    with ProcessPoolExecutor(args.jobs, initializer=_init, initargs=(args.proxycol,)) as pool:
        for pid, share, top, drawn, bare, by in pool.map(classify, tasks):
            p = patches[pid]
            p["hit_share"] = round(share, 2)
            p["drawn_m2"] = round(drawn)
            p["uncovered_m2"] = round(bare)
            p["uncovered_share"] = round(bare / drawn, 2) if drawn > 0 else 0.0
            p["verdict"] = ("building" if p["uncovered_share"] >= args.building_share and bare >= args.min_uncovered
                            else "open" if p["uncovered_share"] < args.open_share or bare < args.min_uncovered else "unsure")
            p["proxies"] = [{"row": i, "mesh": rows[i]["mesh"], "sector": rows[i]["sector"], "uncovered_m2": round(a)} for i, a in by]
            p["ray_proxies"] = [{"row": i, "mesh": rows[i]["mesh"], "hits": c} for i, c in top]
            done += 1
            if done % 25 == 0:
                print(f"{done}/{len(tasks)} patches", file=sys.stderr, flush=True)
    out = sorted(patches.values(), key=lambda p: -p["gained_m3"])
    json.dump(out, open(os.path.join(args.out, "patches.json"), "w"), indent=1)
    v = collections.Counter(p.get("verdict") for p in out)
    print(f"{len(out)} patches: {dict(v)}", file=sys.stderr)


if __name__ == "__main__":
    main()
