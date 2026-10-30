"""Render a height map (rasterize.py output) as a shaded PNG: color = height, light from the NW.

Usage: python preview.py <dir with top.npy + meta.json> -o preview.png [--max-px 4000]
"""

import argparse
import json
import os

import matplotlib
import numpy as np

matplotlib.use("Agg")
from matplotlib import cm, colors  # noqa: E402
from PIL import Image  # noqa: E402


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("dir")
    ap.add_argument("-o", "--out", required=True)
    ap.add_argument("--max-px", type=int, default=4000)
    ap.add_argument("--zmin", type=float)
    ap.add_argument("--zmax", type=float)
    args = ap.parse_args()

    top = np.load(os.path.join(args.dir, "top.npy"))
    meta = json.load(open(os.path.join(args.dir, "meta.json")))
    step = max(1, int(np.ceil(max(top.shape) / args.max_px)))
    if step > 1:  # block max keeps thin tall things visible
        h, w = (top.shape[0] // step) * step, (top.shape[1] // step) * step
        t = top[:h, :w].reshape(h // step, step, w // step, step)
        top = np.nanmax(np.nanmax(t, axis=3), axis=1)
    cell = meta["cell"] * step

    empty = ~np.isfinite(top)
    z = np.where(empty, np.nanmin(top), top)
    zmin = args.zmin if args.zmin is not None else np.nanpercentile(top, 1)
    zmax = args.zmax if args.zmax is not None else np.nanpercentile(top, 99.8)
    rgb = cm.turbo(colors.Normalize(zmin, zmax, clip=True)(z))[:, :, :3]
    shade = colors.LightSource(azdeg=315, altdeg=40).hillshade(z, vert_exag=1.0, dx=cell, dy=cell)
    rgb = rgb * (0.35 + 0.65 * shade[:, :, None])
    rgb[empty] = 0.08
    # rows run +y; flip so north is up
    img = Image.fromarray((np.flipud(rgb) * 255).astype(np.uint8))
    img.save(args.out)
    print(f"{img.size[0]}x{img.size[1]} px, {cell} m/px, z {zmin:.0f}..{zmax:.0f} m")


if __name__ == "__main__":
    main()
