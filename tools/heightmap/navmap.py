"""Write the runtime nav map (navmap.bin) the plugin loads, from spans.py output.

Only gaps reachable from open sky are kept: per 2 m column, the vertical ranges a car's centre
can occupy (the bake already inflated solids by the car's radius and half height).

Layout, all little endian (src/red4ext/Navigation/NavMap.hpp reads it):

  header (68 bytes)
    char[8]  magic "LTBFNAV1"
    u32      version (1)
    u32      flags (bit 0: tiles are zlib streams)
    f32      origin x, origin y      world position of cell (0, 0)'s corner
    f32      cell                    cell edge, m
    f32      z0, vz                  voxel z = z0 + i * vz; a gap ending at nz is open sky
    u32      nz
    u32      width, height           cells
    u32      tile                    cells per tile edge (128)
    u32      tiles x, tiles y
    f32      radius, half_height     car size the bake was inflated by
  tile index: tiles x * tiles y entries (row major, y then x) of
    u64      offset from file start (0 = no data in this tile)
    u32      compressed size
    u32      uncompressed size
  tiles: zlib stream of
    u8       count[tile * tile]      gaps per column, row major (y then x)
    u16      gaps[sum(count)][2]     (bottom, top) voxel indices, bottom up, column after column

Usage: python navmap.py SPANS_DIR -o navmap.bin [--level 9]
"""

import argparse
import json
import os
import struct
import sys
import zlib

import numpy as np

MAGIC = b"LTBFNAV1"
VERSION = 1
HEADER = struct.Struct("<8sII5fIIII II2f")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("spans")
    ap.add_argument("-o", "--out", required=True)
    ap.add_argument("--level", type=int, default=9)
    args = ap.parse_args()

    cfg = json.load(open(os.path.join(args.spans, "config.json")))
    d = np.load(os.path.join(args.spans, "free.npz"))
    counts, runs, reach = d["counts"], d["runs"], d["reachable"]
    h, w = counts.shape
    tile = 128
    ntx, nty = -(-w // tile), -(-h // tile)
    nz = int(round((cfg["z1"] - cfg["z0"]) / cfg["vz"]))

    # keep reachable gaps only
    col = np.repeat(np.arange(h * w), counts.ravel().astype(np.int64))
    keep = reach.astype(bool)
    rcounts = np.bincount(col[keep], minlength=h * w).reshape(h, w)
    if rcounts.max() > 255:
        sys.exit("more than 255 gaps in a column")
    rruns = runs[keep].astype(np.uint16)
    offsets = np.zeros(h * w + 1, np.int64)
    offsets[1:] = np.cumsum(rcounts.ravel())

    header = HEADER.pack(
        MAGIC, VERSION, 1,
        cfg["origin"][0], cfg["origin"][1], cfg["cell"], cfg["z0"], cfg["vz"],
        nz, w, h, tile, ntx, nty,
        cfg.get("radius", 0.0), cfg.get("half_height", 0.0),
    )
    index = bytearray(16 * ntx * nty)
    blobs = []
    pos = len(header) + len(index)
    raw_total = 0
    for ty in range(nty):
        for tx in range(ntx):
            c = np.zeros((tile, tile), np.uint8)
            y0, x0 = ty * tile, tx * tile
            y1, x1 = min(y0 + tile, h), min(x0 + tile, w)
            c[: y1 - y0, : x1 - x0] = rcounts[y0:y1, x0:x1]
            k = ty * ntx + tx
            if not c.any():
                struct.pack_into("<QII", index, 16 * k, 0, 0, 0)
                continue
            parts = [c.tobytes()]
            for y in range(y0, y1):
                a, b = offsets[y * w + x0], offsets[y * w + x1]
                parts.append(rruns[a:b].tobytes())
            raw = b"".join(parts)
            blob = zlib.compress(raw, args.level)
            struct.pack_into("<QII", index, 16 * k, pos, len(blob), len(raw))
            blobs.append(blob)
            pos += len(blob)
            raw_total += len(raw)

    with open(args.out, "wb") as f:
        f.write(header)
        f.write(index)
        for b in blobs:
            f.write(b)
    print(
        f"{args.out}: {pos / 1e6:.1f} MB ({raw_total / 1e6:.1f} MB raw), {len(blobs)} tiles, "
        f"{len(rruns)} gaps, grid {w} x {h}",
        file=sys.stderr,
    )


if __name__ == "__main__":
    main()
