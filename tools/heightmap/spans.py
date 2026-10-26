"""Multi-level span map: per 2 m column, the free gaps a car's centre can occupy.

Pipeline (each step is a subcommand):

  bin    Read sector triangles (sectors.py output), keep presets that block vehicles, and bin
         them into 256 m tiles on disk (a triangle crossing tiles goes into each).
  build  Per tile: voxelize at --cell x --vz (2 m x 0.5 m), sampling triangles finely enough
         in 3D that walls come out continuous. Mark solid below the lowest surface of each
         column (you cannot get under the world) and below the terrain. Inflate the solid by
         the car (a cylinder: --radius sideways, --half-height up and down) and store the free
         runs of every column, plus the raw solid runs for validation.
  merge  Join tiles into one file: free.npz / solid.npz with per-column run counts and the
         runs as int16 voxel indices (z = z0 + i * vz); a run ending at nz is open sky.
  reach  Link gaps of neighbouring columns that overlap vertically, keep the components that
         touch open sky (free.npz gets a per-run "reachable" flag). Enclosed spaces such as
         building interiors drop out.
  preview  PNG of reachable gaps per column (1 = open to the sky, 2+ = something overhead).

Terrain comes from rasterize.py --no-collision at the same cell size (terrain.npy + meta.json),
so the grids line up.

Usage:
  python spans.py bin --sectors S --presets presets_table.json --terrain-dir T -o WORK
  python spans.py build WORK [--jobs 8]
  python spans.py merge WORK
  python spans.py reach WORK
  python spans.py preview WORK -o gaps.png
"""

import argparse
import collections
import glob
import json
import os
import re
import sys
from concurrent.futures import ProcessPoolExecutor

import numpy as np
from scipy import ndimage

from presets import load_lookup

TILE_CELLS = 128
Z0, Z1 = -200.0, 1000.0  # voxel range; geometry outside is clipped (sky quest spaces sit at 1.6 km+)
FLUSH_TRIS = 20_000_000
MAX_POINTS = 30_000_000


def tile_params(meta, cell):
    return meta["origin"][0], meta["origin"][1], cell, TILE_CELLS * cell


# ----------------------------------------------------------------------------------------- bin


def cmd_bin(args):
    meta = json.load(open(os.path.join(args.terrain_dir, "meta.json")))
    if meta["cell"] != args.cell:
        sys.exit(f"terrain cell {meta['cell']} != --cell {args.cell}")
    x0, y0, _, tile = tile_params(meta, args.cell)
    h, w = meta["shape"]
    ntx, nty = -(-w // TILE_CELLS), -(-h // TILE_CELLS)
    lookup = load_lookup(args.presets)
    margin = args.margin

    os.makedirs(os.path.join(args.out, "bins"), exist_ok=True)
    config = {
        "origin": [x0, y0],
        "cell": args.cell,
        "vz": args.vz,
        "z0": Z0,
        "z1": Z1,
        "tiles": [ntx, nty],
        "shape": [h, w],
        "terrain_dir": os.path.abspath(args.terrain_dir),
        "margin": margin,
    }
    json.dump(config, open(os.path.join(args.out, "config.json"), "w"), indent=1)

    pending = collections.defaultdict(list)
    pending_n = 0
    part = collections.Counter()

    def flush():
        nonlocal pending_n
        for (tx, ty), chunks in pending.items():
            k = part[(tx, ty)]
            part[(tx, ty)] += 1
            np.save(os.path.join(args.out, "bins", f"t_{tx}_{ty}_{k}.npy"), np.concatenate(chunks))
        pending.clear()
        pending_n = 0

    files = sorted(
        f for f in os.listdir(args.sectors) if f.endswith(".npz") and not f.endswith(".terrain.npz")
    )
    for k, f in enumerate(files):
        d = np.load(os.path.join(args.sectors, f))
        names = [str(n) for n in d["preset_names"]]
        keep = np.array([lookup.get(n, {"blocks_vehicles": True})["blocks_vehicles"] for n in names])
        tris = d["tris"][keep[d["preset"]]]
        if not len(tris):
            continue
        z = tris[:, :, 2]
        tris = tris[(z.max(1) >= Z0) & (z.min(1) <= Z1)]
        mn = tris[:, :, :2].min(1)
        mx = tris[:, :, :2].max(1)
        tx0 = np.floor((mn[:, 0] - margin - x0) / tile).astype(int).clip(0, ntx - 1)
        tx1 = np.floor((mx[:, 0] + margin - x0) / tile).astype(int).clip(0, ntx - 1)
        ty0 = np.floor((mn[:, 1] - margin - y0) / tile).astype(int).clip(0, nty - 1)
        ty1 = np.floor((mx[:, 1] + margin - y0) / tile).astype(int).clip(0, nty - 1)
        single = (tx0 == tx1) & (ty0 == ty1)
        key = ty0[single] * ntx + tx0[single]
        order = np.argsort(key, kind="stable")
        ks, starts = np.unique(key[order], return_index=True)
        st = tris[single][order]
        for i, kk in enumerate(ks):
            end = starts[i + 1] if i + 1 < len(starts) else len(st)
            pending[(int(kk % ntx), int(kk // ntx))].append(st[starts[i] : end])
        for i in np.nonzero(~single)[0]:
            for ty in range(ty0[i], ty1[i] + 1):
                for tx in range(tx0[i], tx1[i] + 1):
                    pending[(tx, ty)].append(tris[i : i + 1])
        pending_n += len(tris)
        if pending_n > FLUSH_TRIS:
            flush()
        if (k + 1) % 1000 == 0:
            print(f"{k + 1}/{len(files)} sectors binned", file=sys.stderr)
    flush()
    print(f"{len(part)} tiles with triangles", file=sys.stderr)


# --------------------------------------------------------------------------------------- build


def barycentric_grid(n):
    i, j = np.meshgrid(np.arange(n + 1), np.arange(n + 1), indexing="ij")
    keep = i + j <= n
    return np.stack([1.0 - (i[keep] + j[keep]) / n, i[keep] / n, j[keep] / n], 1).astype(np.float32)


def sample_points(tris, cell, vz):
    """Points on the triangles, spaced under 0.7 of a cell sideways and 0.7 voxel vertically."""
    d = np.stack([tris[:, 1] - tris[:, 0], tris[:, 2] - tris[:, 1], tris[:, 0] - tris[:, 2]], 1)
    steps = np.maximum(
        np.linalg.norm(d[:, :, :2], axis=2) / (0.7 * cell), np.abs(d[:, :, 2]) / (0.7 * vz)
    ).max(1)
    n = np.maximum(1, np.ceil(steps)).astype(np.int64)
    for level in np.unique(n):
        group = tris[n == level]
        w = barycentric_grid(int(level))
        per = max(1, MAX_POINTS // len(w))
        for i in range(0, len(group), per):
            yield np.einsum("kv,tvc->tkc", w, group[i : i + per]).reshape(-1, 3)


def runs_of(mask):
    """mask (cols, nz) bool -> counts (cols,) and runs (n, 2) [start, end) per column, in order."""
    pad = np.zeros((mask.shape[0], 1), bool)
    edges = np.diff(np.concatenate([pad, mask, pad], 1).astype(np.int8), axis=1)
    sc, sz = np.nonzero(edges == 1)
    ec, ez = np.nonzero(edges == -1)
    counts = np.bincount(sc, minlength=mask.shape[0]).astype(np.uint8)
    return counts, np.stack([sz, ez], 1).astype(np.int16)


def build_tile(work, tx, ty, radius, half_height):
    cfg = json.load(open(os.path.join(work, "config.json")))
    cell, vz = cfg["cell"], cfg["vz"]
    x0, y0 = cfg["origin"]
    h, w = cfg["shape"]
    nz = int(round((cfg["z1"] - cfg["z0"]) / vz))
    m = int(np.ceil(cfg["margin"] / cell))
    c0x, c0y = tx * TILE_CELLS - m, ty * TILE_CELLS - m
    n = TILE_CELLS + 2 * m

    occ = np.zeros((n, n, nz), bool)  # [y, x, z]
    for path in glob.glob(os.path.join(work, "bins", f"t_{tx}_{ty}_*.npy")):
        tris = np.load(path)
        for pts in sample_points(tris, cell, vz):
            cx = np.floor((pts[:, 0] - x0) / cell).astype(np.int64) - c0x
            cy = np.floor((pts[:, 1] - y0) / cell).astype(np.int64) - c0y
            cz = np.floor((pts[:, 2] - cfg["z0"]) / vz).astype(np.int64)
            ok = (cx >= 0) & (cx < n) & (cy >= 0) & (cy < n) & (cz >= 0) & (cz < nz)
            occ[cy[ok], cx[ok], cz[ok]] = True

    terrain = np.load(os.path.join(cfg["terrain_dir"], "terrain.npy"), mmap_mode="r")
    tgrid = np.full((n, n), np.nan, np.float32)
    ys, xs = slice(max(c0y, 0), min(c0y + n, h)), slice(max(c0x, 0), min(c0x + n, w))
    tgrid[ys.start - c0y : ys.stop - c0y, xs.start - c0x : xs.stop - c0x] = terrain[ys, xs]

    has_data = occ.any(2) | np.isfinite(tgrid)
    if not has_data.any():
        return None
    # solid below the lowest surface in each column, and below the terrain
    zi = np.arange(nz)[None, None, :]
    lowest = np.where(occ.any(2), occ.argmax(2), -1)
    tz = np.floor((np.nan_to_num(tgrid, nan=cfg["z0"] - 1) - cfg["z0"]) / vz)
    solid = occ | (zi < lowest[:, :, None]) | (zi <= tz[:, :, None])

    # inflate by the car: disc of `radius` sideways, `half_height` up and down
    r = radius / cell
    ri = int(np.ceil(r))
    yy, xx = np.mgrid[-ri : ri + 1, -ri : ri + 1]
    disc = (xx * xx + yy * yy) <= r * r + 1e-6
    hz = int(np.ceil(half_height / vz))
    grown = ndimage.binary_dilation(solid, structure=disc[:, :, None])
    grown = ndimage.binary_dilation(grown, structure=np.ones((1, 1, 2 * hz + 1), bool))

    inner = (slice(m, m + TILE_CELLS), slice(m, m + TILE_CELLS))
    solid_c, grown_c = solid[inner], grown[inner]
    data_c = has_data[inner]
    free = ~grown_c
    free[~data_c] = False  # no data: unknown, not free
    sc, sr = runs_of(solid_c.reshape(-1, nz))
    fc, fr = runs_of(free.reshape(-1, nz))
    out = os.path.join(work, "tiles", f"t_{tx}_{ty}.npz")
    np.savez_compressed(out, solid_counts=sc, solid_runs=sr, free_counts=fc, free_runs=fr)
    return int(fc.max()) if len(fc) else 0


def _build_job(a):
    work, tx, ty, radius, half_height = a
    try:
        return tx, ty, build_tile(work, tx, ty, radius, half_height), None
    except Exception as err:
        return tx, ty, None, repr(err)


def cmd_build(args):
    cfg = json.load(open(os.path.join(args.work, "config.json")))
    cfg.update({"radius": args.radius, "half_height": args.half_height})
    json.dump(cfg, open(os.path.join(args.work, "config.json"), "w"), indent=1)
    os.makedirs(os.path.join(args.work, "tiles"), exist_ok=True)
    ntx, nty = cfg["tiles"]
    jobs = [
        (args.work, tx, ty, args.radius, args.half_height)
        for ty in range(nty)
        for tx in range(ntx)
        if args.redo or not os.path.exists(os.path.join(args.work, "tiles", f"t_{tx}_{ty}.npz"))
    ]
    if args.only:
        tx, ty = map(int, args.only.split(","))
        jobs = [(args.work, tx, ty, args.radius, args.half_height)]
    print(f"{len(jobs)} tiles, {args.jobs} jobs", file=sys.stderr)
    done = 0
    with ProcessPoolExecutor(args.jobs) as pool:
        for tx, ty, most, err in pool.map(_build_job, jobs, chunksize=1):
            done += 1
            if err:
                print(f"tile {tx},{ty}: {err}", file=sys.stderr)
            if done % 100 == 0 or done == len(jobs):
                print(f"{done}/{len(jobs)} tiles", file=sys.stderr)


# --------------------------------------------------------------------------------------- merge


def cmd_merge(args):
    cfg = json.load(open(os.path.join(args.work, "config.json")))
    ntx, nty = cfg["tiles"]
    h, w = cfg["shape"]
    for kind in ("free", "solid"):
        counts = np.zeros((nty * TILE_CELLS, ntx * TILE_CELLS), np.uint8)
        runs_by_cell = {}
        for ty in range(nty):
            for tx in range(ntx):
                p = os.path.join(args.work, "tiles", f"t_{tx}_{ty}.npz")
                if not os.path.exists(p):
                    continue
                d = np.load(p)
                c = d[f"{kind}_counts"].reshape(TILE_CELLS, TILE_CELLS)
                counts[ty * TILE_CELLS : (ty + 1) * TILE_CELLS, tx * TILE_CELLS : (tx + 1) * TILE_CELLS] = c
                runs_by_cell[(tx, ty)] = d[f"{kind}_runs"]
        counts = counts[:h, :w]
        # runs in row-major cell order: gather per tile row by row
        offsets = np.zeros(h * w + 1, np.int64)
        offsets[1:] = np.cumsum(counts.ravel(), dtype=np.int64)
        runs = np.zeros((offsets[-1], 2), np.int16)
        for (tx, ty), r in runs_by_cell.items():
            c = np.load(os.path.join(args.work, "tiles", f"t_{tx}_{ty}.npz"))[f"{kind}_counts"]
            starts = np.concatenate([[0], np.cumsum(c, dtype=np.int64)])
            for row in range(TILE_CELLS):
                gy = ty * TILE_CELLS + row
                if gy >= h:
                    break
                gx0 = tx * TILE_CELLS
                gx1 = min(gx0 + TILE_CELLS, w)
                a, b = starts[row * TILE_CELLS], starts[row * TILE_CELLS + (gx1 - gx0)]
                dst = offsets[gy * w + gx0]
                runs[dst : dst + (b - a)] = r[a:b]
        np.savez_compressed(os.path.join(args.work, f"{kind}.npz"), counts=counts, runs=runs)
        print(f"{kind}: {len(runs)} runs, max {counts.max()} per column", file=sys.stderr)


# --------------------------------------------------------------------------------------- reach


def dense_gaps(counts, runs, offsets, y0, y1):
    """Rows [y0, y1) as dense (rows, w, k) lo/hi arrays (lo = hi = 0 for missing gaps) plus ids."""
    c = counts[y0:y1]
    rows, w = c.shape
    k = max(1, int(c.max()))
    lo = np.zeros((rows, w, k), np.int16)
    hi = np.zeros((rows, w, k), np.int16)
    ids = np.full((rows, w, k), -1, np.int64)
    base = offsets[y0 * w : y1 * w].reshape(rows, w)
    for j in range(k):
        has = c > j
        g = base[has] + j
        lo[:, :, j][has] = runs[g, 0]
        hi[:, :, j][has] = runs[g, 1]
        ids[:, :, j][has] = g
    return lo, hi, ids


def overlap_edges(lo_a, hi_a, id_a, lo_b, hi_b, id_b):
    """Edges between gap lists of paired columns (same leading shape) that overlap vertically."""
    ov = (np.maximum(lo_a[..., :, None], lo_b[..., None, :]) < np.minimum(hi_a[..., :, None], hi_b[..., None, :]))
    ov &= (id_a[..., :, None] >= 0) & (id_b[..., None, :] >= 0)
    ga = id_a.reshape(-1, id_a.shape[-1])
    gb = id_b.reshape(-1, id_b.shape[-1])
    flat = ov.reshape(-1, ov.shape[-2], ov.shape[-1])
    col, ka, kb = np.nonzero(flat)
    return np.stack([ga[col, ka], gb[col, kb]], 1)


def cmd_reach(args):
    from scipy.sparse import coo_matrix
    from scipy.sparse.csgraph import connected_components

    cfg = json.load(open(os.path.join(args.work, "config.json")))
    nz = int(round((cfg["z1"] - cfg["z0"]) / cfg["vz"]))
    path = os.path.join(args.work, "free.npz")
    d = dict(np.load(path))
    counts, runs = d["counts"], d["runs"]
    h, w = counts.shape
    offsets = np.zeros(h * w + 1, np.int64)
    offsets[1:] = np.cumsum(counts.ravel(), dtype=np.int64)
    n = len(runs)
    edges = []
    stripe = 64
    for y0 in range(0, h, stripe):
        y1 = min(h, y0 + stripe + 1)  # one extra row for the y neighbour
        lo, hi, ids = dense_gaps(counts, runs, offsets, y0, y1)
        rows = min(stripe, h - y0)
        # x neighbours
        edges.append(
            overlap_edges(lo[:rows, :-1], hi[:rows, :-1], ids[:rows, :-1], lo[:rows, 1:], hi[:rows, 1:], ids[:rows, 1:])
        )
        # y neighbours
        if y1 - y0 > 1:
            r = y1 - y0 - 1
            edges.append(overlap_edges(lo[:r], hi[:r], ids[:r], lo[1 : r + 1], hi[1 : r + 1], ids[1 : r + 1]))
        if (y0 // stripe) % 10 == 0:
            print(f"row {y0}/{h}, {sum(len(e) for e in edges)} edges", file=sys.stderr)
    e = np.concatenate(edges).astype(np.int32)
    graph = coo_matrix((np.ones(len(e), np.int8), (e[:, 0], e[:, 1])), shape=(n, n))
    ncomp, labels = connected_components(graph, directed=False)
    sky = runs[:, 1] >= nz
    reach_labels = np.unique(labels[sky])
    reachable = np.isin(labels, reach_labels)
    d["reachable"] = reachable
    np.savez_compressed(path, **d)
    print(f"{n} gaps, {len(e)} edges, {ncomp} components, {reachable.mean():.4f} reachable", file=sys.stderr)


# ------------------------------------------------------------------------------------- preview


def cmd_preview(args):
    from PIL import Image

    cfg = json.load(open(os.path.join(args.work, "config.json")))
    d = np.load(os.path.join(args.work, "free.npz"))
    c = d["counts"]
    if "reachable" in d:  # count only gaps a car can get to from open sky
        offsets = np.concatenate([[0], np.cumsum(c.ravel(), dtype=np.int64)])
        r = d["reachable"].astype(np.int64)
        cr = np.concatenate([[0], np.cumsum(r)])
        c = (cr[offsets[1:]] - cr[offsets[:-1]]).reshape(c.shape).astype(np.uint8)
    palette = np.array(
        [[20, 20, 24], [40, 70, 130], [250, 200, 40], [240, 90, 30], [200, 30, 160]], np.uint8
    )
    img = palette[np.minimum(c, len(palette) - 1)]
    if args.bounds:
        bx0, by0, bx1, by1 = map(float, args.bounds.split(","))
        x0, y0 = cfg["origin"]
        cs = cfg["cell"]
        img = img[int((by0 - y0) / cs) : int((by1 - y0) / cs), int((bx0 - x0) / cs) : int((bx1 - x0) / cs)]
    Image.fromarray(np.flipud(img)).save(args.out)
    shares = np.bincount(np.minimum(c.ravel(), 4), minlength=5) / c.size
    print("columns with 0/1/2/3/4+ free gaps:", " ".join(f"{s:.4f}" for s in shares))


def main():
    ap = argparse.ArgumentParser()
    sub = ap.add_subparsers(dest="cmd", required=True)
    b = sub.add_parser("bin")
    b.add_argument("--sectors", required=True)
    b.add_argument("--presets", required=True)
    b.add_argument("--terrain-dir", required=True)
    b.add_argument("-o", "--out", required=True)
    b.add_argument("--cell", type=float, default=2.0)
    b.add_argument("--vz", type=float, default=0.5)
    b.add_argument("--margin", type=float, default=8.0, help="tile overlap for inflation, m")
    u = sub.add_parser("build")
    u.add_argument("work")
    u.add_argument("--jobs", type=int, default=6)
    u.add_argument("--radius", type=float, default=3.0, help="car footprint radius + margin, m")
    u.add_argument("--half-height", type=float, default=1.5, help="half car height + margin, m")
    u.add_argument("--redo", action="store_true")
    u.add_argument("--only", help="tx,ty: build one tile")
    g = sub.add_parser("merge")
    g.add_argument("work")
    r = sub.add_parser("reach")
    r.add_argument("work")
    p = sub.add_parser("preview")
    p.add_argument("work")
    p.add_argument("-o", "--out", required=True)
    p.add_argument("--bounds")
    args = ap.parse_args()
    {"bin": cmd_bin, "build": cmd_build, "merge": cmd_merge, "reach": cmd_reach, "preview": cmd_preview}[
        args.cmd
    ](args)


if __name__ == "__main__":
    main()
