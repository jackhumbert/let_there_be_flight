"""Rasterize world collision triangles (sectors.py output) into a top-surface height map.

Each cell holds the highest collision Z inside it ("skyline": road, roof, overpass, sign,
whichever is on top), NaN where there is no collision. Triangles are sampled on a barycentric
grid finer than the cell, so every cell a triangle crosses gets at least one sample; vertical
walls project to their footprint line.

Terrain height fields (<sector>.terrain.npz) are rasterized separately, finest sector level
winning, and merged in with a max.

Outputs in --out:
  top.npy        float32 (rows = y, cols = x), NaN = empty
  terrain.npy    float32 terrain only, NaN = no terrain
  meta.json      origin (world x, y of cell [0, 0] corner), cell size, shape, preset counts
  presets.json   triangles per collision preset name (to decide --exclude)

Usage: python rasterize.py --sectors <sectors.py out> -o <dir> [--cell 2] [--exclude "Name,..."]
       [--presets presets_table.json] [--bounds xmin,ymin,xmax,ymax]
"""

import argparse
import collections
import json
import os
import re
import sys

import numpy as np

from presets import load_lookup

SECTOR_SIZE = 64.0  # level 0 exterior sector edge, metres
CHUNK_SAMPLES = 20_000_000


def sector_bounds(names):
    xs, ys = [], []
    for n in names:
        m = re.match(r"exterior_(-?\d+)_(-?\d+)_(-?\d+)_0\.npz$", n)
        if m:
            xs.append(int(m.group(1)))
            ys.append(int(m.group(2)))
    return (
        min(xs) * SECTOR_SIZE,
        min(ys) * SECTOR_SIZE,
        (max(xs) + 1) * SECTOR_SIZE,
        (max(ys) + 1) * SECTOR_SIZE,
    )


def barycentric_grid(n):
    """Barycentric weights (k, 3) of an n-subdivided triangle, k = (n+1)(n+2)/2."""
    i, j = np.meshgrid(np.arange(n + 1), np.arange(n + 1), indexing="ij")
    keep = i + j <= n
    a = i[keep] / n
    b = j[keep] / n
    return np.stack([1.0 - a - b, a, b], axis=1).astype(np.float32)


class TopRaster:
    def __init__(self, x0, y0, cell, w, h):
        self.x0, self.y0, self.cell, self.w, self.h = x0, y0, cell, w, h
        self.top = np.full(w * h, -np.inf, np.float32)

    def splat(self, pts):
        cx = np.floor((pts[:, 0] - self.x0) / self.cell).astype(np.int64)
        cy = np.floor((pts[:, 1] - self.y0) / self.cell).astype(np.int64)
        ok = (cx >= 0) & (cx < self.w) & (cy >= 0) & (cy < self.h)
        np.maximum.at(self.top, cy[ok] * self.w + cx[ok], pts[ok, 2])

    def add(self, tris):
        # subdivision so that sample spacing along each edge is below 0.7 cells
        e = np.stack(
            [
                np.linalg.norm(tris[:, 1, :2] - tris[:, 0, :2], axis=1),
                np.linalg.norm(tris[:, 2, :2] - tris[:, 1, :2], axis=1),
                np.linalg.norm(tris[:, 0, :2] - tris[:, 2, :2], axis=1),
            ],
            axis=1,
        ).max(axis=1)
        n = np.maximum(1, np.ceil(e / (0.7 * self.cell))).astype(np.int64)
        for level in np.unique(n):
            group = tris[n == level]
            w = barycentric_grid(int(level))
            per = max(1, CHUNK_SAMPLES // len(w))
            for i in range(0, len(group), per):
                g = group[i : i + per]
                pts = np.einsum("kv,tvc->tkc", w, g).reshape(-1, 3)
                self.splat(pts)

    def result(self):
        t = self.top.reshape(self.h, self.w)
        t[np.isneginf(t)] = np.nan
        return t


def terrain_points(xform, heights, holes):
    """World positions (k, 3) of the non-hole samples of one terrain height field."""
    rot = xform[:9].reshape(3, 3)
    pos = xform[9:12]
    row_scale, height_scale, col_scale = xform[12:15]
    r, c = np.nonzero(~holes)
    local = np.stack(
        [r * row_scale, heights[r, c].astype(np.float64) * height_scale, c * col_scale], axis=1
    )
    return (local @ rot.T + pos).astype(np.float32)


def rasterize_terrain(dirs, x0, y0, cell, w, h):
    """Terrain height per cell. Sector levels 2 to 6 hold the same terrain at falling detail,
    so levels are applied coarse to fine and the finest level present wins."""
    by_level = collections.defaultdict(list)
    for d in dirs:
        for f in os.listdir(d):
            m = re.match(r"exterior_-?\d+_-?\d+_-?\d+_(\d+)\.terrain\.npz$", f)
            if m:
                by_level[int(m.group(1))].append(os.path.join(d, f))
    terrain = np.full(w * h, np.nan, np.float32)
    for level in sorted(by_level, reverse=True):
        layer = TopRaster(x0, y0, cell, w, h)
        for path in by_level[level]:
            d = np.load(path)
            for i, xform in enumerate(d["xform"]):
                layer.splat(terrain_points(xform, d[f"h{i}"], d[f"hole{i}"]))
        got = np.isfinite(layer.top)
        terrain[got] = layer.top[got]
        print(f"terrain level {level}: {len(by_level[level])} sectors", file=sys.stderr)
    return terrain.reshape(h, w)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--sectors", required=True)
    ap.add_argument("-o", "--out", required=True)
    ap.add_argument("--cell", type=float, default=2.0)
    ap.add_argument("--exclude", default="", help="comma separated preset names to skip")
    ap.add_argument(
        "--presets",
        help="presets_table.json (presets.py); keeps only presets that block vehicles. "
        "Presets missing from the table are kept and listed in meta.json.",
    )
    ap.add_argument("--bounds", help="xmin,ymin,xmax,ymax world metres (default: all sectors)")
    ap.add_argument(
        "--terrain",
        nargs="*",
        help="extra dirs with <sector>.terrain.npz (default: --sectors itself)",
    )
    ap.add_argument("--no-collision", action="store_true", help="terrain only")
    args = ap.parse_args()

    files = sorted(
        f
        for f in os.listdir(args.sectors)
        if f.endswith(".npz") and not f.endswith(".terrain.npz")
    )
    if args.bounds:
        x0, y0, x1, y1 = map(float, args.bounds.split(","))
    else:
        x0, y0, x1, y1 = sector_bounds(files)
    w = int(np.ceil((x1 - x0) / args.cell))
    h = int(np.ceil((y1 - y0) / args.cell))
    print(f"grid {w} x {h} cells of {args.cell} m from ({x0}, {y0})", file=sys.stderr)
    raster = TopRaster(x0, y0, args.cell, w, h)
    exclude = {s.strip() for s in args.exclude.split(",") if s.strip()}
    lookup = load_lookup(args.presets) if args.presets else None
    unknown = collections.Counter()

    def skip(name):
        if name in exclude:
            return True
        if lookup is None:
            return False
        p = lookup.get(name)
        if p is None:
            unknown[name] += 1
            return False
        return p["name"] in exclude or not p["blocks_vehicles"]

    counts = collections.Counter()
    for k, f in enumerate([] if args.no_collision else files):
        d = np.load(os.path.join(args.sectors, f))
        tris, preset, names = d["tris"], d["preset"], d["preset_names"]
        for i, name in enumerate(names):
            counts[str(name)] += int((preset == i).sum())
        if exclude or lookup:
            drop = np.array([skip(str(nm)) for nm in names])
            tris = tris[~drop[preset]]
        if args.bounds:
            mn, mx = tris[:, :, :2].min(axis=1), tris[:, :, :2].max(axis=1)
            keep = (mx[:, 0] >= x0) & (mn[:, 0] <= x1) & (mx[:, 1] >= y0) & (mn[:, 1] <= y1)
            tris = tris[keep]
        if len(tris):
            raster.add(tris)
        if (k + 1) % 500 == 0:
            print(f"{k + 1}/{len(files)} sectors", file=sys.stderr)

    os.makedirs(args.out, exist_ok=True)
    top = raster.result()
    terrain = rasterize_terrain(args.terrain or [args.sectors], x0, y0, args.cell, w, h)
    np.save(os.path.join(args.out, "terrain.npy"), terrain)
    top = np.fmax(top, terrain)  # NaN-aware max
    np.save(os.path.join(args.out, "top.npy"), top)
    meta = {
        "origin": [x0, y0],
        "cell": args.cell,
        "shape": [h, w],
        "exclude": sorted(exclude),
        "vehicle_presets_only": bool(lookup),
        "unknown_presets": sorted(unknown),
        "filled": float(np.isfinite(top).mean()),
        "z_range": [float(np.nanmin(top)), float(np.nanmax(top))],
    }
    json.dump(meta, open(os.path.join(args.out, "meta.json"), "w"), indent=1)
    json.dump(dict(counts.most_common()), open(os.path.join(args.out, "presets.json"), "w"), indent=1)
    print(json.dumps(meta), file=sys.stderr)


if __name__ == "__main__":
    main()
