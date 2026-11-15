"""A box of the city's collision as one region file, for flightsim's Colliders (docs/design/flightsim.md, Colliders).

    python -P -E tools/flightsim/region_pack.py x0,y0,x1,y1 -o <region.ltbfregion> [--margin 20]
    python -P -E tools/flightsim/region_pack.py --regions tools/flightsim/terrain_regions.tsv [--force]

--regions packs every box of the table (name, x0, y0, x1, y1, what) into REGIONS/<name>.ltbfregion, skipping the
ones already there unless --force.

What the car's rays hit in game, from the nav map bake's inputs (tools/navmap_bake; needs numpy and scipy):
- the sector triangles (sectors.py output) of the presets that block vehicles, as spans.py bin keeps them, less the
  blockers the plugin moves away (src/navigation/blockers.txt with the blockers.py scan) and the hidden colliders
  listed by hand (tools/world/hidden_colliders.json), through spans.py's own functions;
- LTBF's proxy colliders (proxy_colliders.py's colliders.npz);
- the terrain height fields as triangles, two a cell. Sector levels 2 to 6 hold the same terrain at falling detail;
  a coarser field's cell is left out where a finer level's field covers it (its whole footprint, holes too, so a
  tunnel mouth's hole does not show the coarse terrain through it). The diagonal is always the same one: the
  tessellation flag PhysX keeps per sample is not in the bake.

The region file (Colliders.hpp reads it): "LTBFREG1", uint32 version 2, uint32 triangle count n, float32 bounds
x0 y0 z0 x1 y1 z1, then n x 9 float32 (the corners a, b, c), then n uint8 kinds (0 collision, 1 terrain, 2 LTBF's
proxy colliders), then n uint32 mesh ids: which PhysX geometry of the game the triangle belongs to (a sector's shape,
from sectors.py's shape array; a terrain height field; the proxy colliders all one), for flightsim's PhysXWorld,
which adds each as its own mesh as the game does. Sectors baked before shape ids get one id each. Little endian.
"""

import argparse
import json
import os
import re
import struct
import sys

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.dirname(HERE))  # tools/, for common
sys.path.insert(0, os.path.join(os.path.dirname(HERE), "navmap_bake"))
from common import paths  # noqa: E402
from presets import load_lookup  # noqa: E402
from spans import drop_hand, drop_removed, hand_removed, removed_blockers  # noqa: E402

BAKE = os.path.join(paths.WORK, "heightmap", "bake_2026-10-26")
SECTORS = os.path.join(BAKE, "sectors")
PRESETS = os.path.join(paths.WORK, "heightmap", "presets_table.json")
PROXY = os.path.join(paths.WORK, "proxycol", "colliders.npz")
REMOVED = os.path.join(paths.REPO, "src", "navigation", "blockers.txt")
BLOCKER_SCAN = os.path.join(paths.WORK, "blockers", "scan")
HAND = os.path.join(paths.TOOLS, "world", "hidden_colliders.json")
REGIONS = os.path.join(paths.WORK, "flightsim_regions")

COLLISION, TERRAIN, PROXY_KIND = 0, 1, 2


def in_box(tris, lo, hi):
    """Triangles whose xy bounds overlap the box."""
    tmin, tmax = tris[:, :, :2].min(1), tris[:, :, :2].max(1)
    return np.all(tmax >= lo, 1) & np.all(tmin <= hi, 1)


def collision(args, lo, hi):
    """The sector triangles a vehicle collides with, in the box, less what the plugin moves away, and each one's mesh
    id: the sector's place in the bounds table above bit 20, its shape below."""
    lookup = load_lookup(args.presets)
    bounds = json.load(open(os.path.join(args.sectors, "_bounds.json")))  # collision_at.py writes it
    removed = removed_blockers(args.removed, args.blocker_scan) if args.removed else {}
    hand = hand_removed(args.hand) if args.removed else {}
    out, ids = [], []
    for k, (stem, b) in enumerate(bounds.items()):
        if b[3] < lo[0] or b[0] > hi[0] or b[4] < lo[1] or b[1] > hi[1]:
            continue
        path = os.path.join(args.sectors, stem + ".npz")
        if not os.path.exists(path):
            continue
        d = np.load(path)
        names = [str(n) for n in d["preset_names"]]
        keep = np.array([lookup.get(n, {"blocks_vehicles": True})["blocks_vehicles"] for n in names])
        shape = d["shape"] if "shape" in d.files else np.zeros(len(d["tris"]), np.uint32)
        mesh = (np.uint32(k + 1) << np.uint32(20)) | (shape.astype(np.uint32) & np.uint32(0xFFFFF))
        tris, preset, mesh = drop_removed(stem, d["tris"], names, d["preset"], removed, mesh)
        tris, preset, mesh = drop_hand(stem, tris, names, preset, hand, mesh)
        sel = keep[preset]
        tris, mesh = tris[sel], mesh[sel]
        box = in_box(tris, lo, hi)
        out.append(tris[box])
        ids.append(mesh[box])
    if not out:
        return np.zeros((0, 3, 3), np.float32), np.zeros(0, np.uint32)
    return np.concatenate(out), np.concatenate(ids)


def proxies(path, lo, hi):
    """LTBF's proxy colliders in the box, one mesh id for all of them."""
    if not path or not os.path.exists(path):
        return np.zeros((0, 3, 3), np.float32), np.zeros(0, np.uint32)
    tris = np.load(path)["tris"]
    tris = tris[in_box(tris, lo, hi)]
    return tris, np.full(len(tris), 0xE0000000, np.uint32)


def field_points(xform, heights):
    """World positions (rows, cols, 3) of one terrain height field's samples (rasterize.py's terrain_points, every
    sample)."""
    rot = xform[:9].reshape(3, 3)
    pos = xform[9:12]
    row_scale, height_scale, col_scale = xform[12:15]
    r, c = np.indices(heights.shape)
    local = np.stack([r * row_scale, heights.astype(np.float64) * height_scale, c * col_scale], axis=-1)
    return local @ rot.T + pos


def terrain(sectors, lo, hi):
    """The terrain in the box as triangles, the finest level winning, and each one's mesh id (its height field's)."""
    fields = []  # (level, xy min, xy max, points, holes)
    for f in os.listdir(sectors):
        m = re.match(r"exterior_-?\d+_-?\d+_-?\d+_(\d+)\.terrain\.npz$", f)
        if not m:
            continue
        d = np.load(os.path.join(sectors, f))
        for i, xform in enumerate(d["xform"]):
            h = d[f"h{i}"]
            # the footprint from the corner samples alone, before the whole field is placed
            span = np.array([[0, 0], [h.shape[0] - 1, 0], [0, h.shape[1] - 1], [h.shape[0] - 1, h.shape[1] - 1]])
            rot, pos, rs, cs = xform[:9].reshape(3, 3), xform[9:12], xform[12], xform[14]
            xy = np.array([(np.array([a * rs, 0, b * cs]) @ rot.T + pos)[:2] for a, b in span])
            fmin, fmax = xy.min(0), xy.max(0)
            if fmax[0] < lo[0] or fmin[0] > hi[0] or fmax[1] < lo[1] or fmin[1] > hi[1]:
                continue
            fields.append((int(m.group(1)), fmin, fmax, field_points(xform, h), d[f"hole{i}"]))
    out, ids = [], []
    for field, (level, fmin, fmax, p, holes) in enumerate(fields):
        a, b, c, e = p[:-1, :-1], p[1:, :-1], p[:-1, 1:], p[1:, 1:]
        quads = np.stack([a, b, c, e], axis=2).reshape(-1, 4, 3)
        keep = ~holes[:-1, :-1].ravel()
        qmin, qmax = quads[:, :, :2].min(1), quads[:, :, :2].max(1)
        keep &= np.all(qmax >= lo, 1) & np.all(qmin <= hi, 1)
        centre = quads[:, :, :2].mean(1)
        for other, omin, omax, _, _ in fields:
            if other < level:
                keep &= ~(np.all(centre >= omin, 1) & np.all(centre <= omax, 1))
        q = quads[keep]
        out.append(np.concatenate([q[:, [0, 1, 2]], q[:, [1, 3, 2]]]))
        ids.append(np.full(2 * len(q), 0xF0000000 | field, np.uint32))
    if not out:
        return np.zeros((0, 3, 3), np.float32), np.zeros(0, np.uint32)
    return np.concatenate(out).astype(np.float32), np.concatenate(ids)


def write(path, parts):
    tris = np.concatenate([t for (t, _), _ in parts]).astype("<f4")
    kinds = np.concatenate([np.full(len(t), k, np.uint8) for (t, _), k in parts])
    meshes = np.concatenate([m for (_, m), _ in parts]).astype("<u4")
    p = tris.reshape(-1, 3)
    bounds = np.concatenate([p.min(0), p.max(0)]) if len(p) else np.zeros(6)
    os.makedirs(os.path.dirname(os.path.abspath(path)), exist_ok=True)
    with open(path, "wb") as f:
        f.write(b"LTBFREG1")
        f.write(struct.pack("<II", 2, len(tris)))
        f.write(struct.pack("<6f", *bounds))
        f.write(tris.tobytes())
        f.write(kinds.tobytes())
        f.write(meshes.tobytes())


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("box", nargs="?", help="x0,y0,x1,y1 (m, world)")
    ap.add_argument("-o", "--out", help=f"region file (regions live in {REGIONS})")
    ap.add_argument("--regions", help="a table of boxes (terrain_regions.tsv), each packed to REGIONS/<name>.ltbfregion")
    ap.add_argument("--force", action="store_true", help="with --regions: pack the ones already there again")
    ap.add_argument("--margin", type=float, default=20.0, help="m added round the box")
    ap.add_argument("--sectors", default=SECTORS)
    ap.add_argument("--presets", default=PRESETS)
    ap.add_argument("--proxy", default=PROXY, help="LTBF's proxy colliders (empty: none)")
    ap.add_argument("--removed", default=REMOVED, help="blockers the plugin moves away (empty: keep them)")
    ap.add_argument("--blocker-scan", default=BLOCKER_SCAN)
    ap.add_argument("--hand", default=HAND)
    args = ap.parse_args()

    if args.regions:
        with open(args.regions, encoding="utf-8") as f:
            rows = [line.rstrip("\n").split("\t") for line in f if line.strip() and not line.startswith("#")]
        for name, x0, y0, x1, y1, *_ in rows:
            out = os.path.join(REGIONS, name + ".ltbfregion")
            if os.path.exists(out) and not args.force:
                print(f"{out}: there")
                continue
            pack(args, (float(x0), float(y0), float(x1), float(y1)), out)
        return
    if not args.box or not args.out:
        ap.error("give a box and -o, or --regions")
    pack(args, tuple(float(v) for v in args.box.split(",")), args.out)


def pack(args, box, out):
    x0, y0, x1, y1 = box
    lo = np.array([min(x0, x1) - args.margin, min(y0, y1) - args.margin])
    hi = np.array([max(x0, x1) + args.margin, max(y0, y1) + args.margin])
    parts = [(collision(args, lo, hi), COLLISION), (terrain(args.sectors, lo, hi), TERRAIN),
             (proxies(args.proxy, lo, hi), PROXY_KIND)]
    write(out, parts)
    counts = ", ".join(f"{n} {len(t)} ({len(np.unique(m))} meshes)"
                       for ((t, m), _), n in zip(parts, ("collision", "terrain", "proxy")))
    print(f"{out}: {counts} triangles, {os.path.getsize(out) / 1e6:.1f} MB")


if __name__ == "__main__":
    main()
