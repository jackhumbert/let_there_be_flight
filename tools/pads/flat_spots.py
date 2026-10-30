"""Find big flat, open-sky spots in a region of navmap.bin, for hand-placed landing pads.

Renders the floor of the highest gap per 2 m column with the flat areas outlined, and prints the
largest flat areas' centres (deck heights). A column is flat when its top floor varies by at most
0.5 m and has open sky within --radius m. Used for the spaceport pads in helipads_overrides.json
(docs/archive/helipads.md).

Usage: python flat_spots.py navmap.bin x0 y0 x1 y1 -o region.png [--radius 10] [--count 25]
"""

import argparse

import matplotlib
import numpy as np
from scipy import ndimage


matplotlib.use("Agg")
import matplotlib.pyplot as plt  # noqa: E402

import os
import sys

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))  # tools/, for common
from common.navmap import NavMap  # noqa: E402


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("navmap")
    ap.add_argument("bounds", nargs=4, type=float, metavar=("X0", "Y0", "X1", "Y1"))
    ap.add_argument("-o", "--out", required=True)
    ap.add_argument("--radius", type=float, default=10.0)
    ap.add_argument("--count", type=int, default=25)
    args = ap.parse_args()
    x0, y0, x1, y1 = args.bounds

    nav = NavMap(args.navmap)
    xs = np.arange(x0, x1, nav.cell)
    ys = np.arange(y0, y1, nav.cell)
    top = np.full((len(ys), len(xs)), np.nan)
    sky = np.zeros_like(top, bool)
    for j, y in enumerate(ys):
        for i, x in enumerate(xs):
            g = nav.gaps(x + nav.cell / 2, y + nav.cell / 2)
            if g:
                b, t = g[-1]
                top[j, i] = b - nav.half
                sky[j, i] = t == float("inf")

    size = 2 * int(args.radius / nav.cell) + 1
    finite = np.isfinite(top)
    hi = ndimage.maximum_filter(np.where(finite, top, -1e4), size=size)
    lo = ndimage.minimum_filter(np.where(finite, top, 1e4), size=size)
    open_sky = ndimage.minimum_filter(sky.astype(np.uint8), size=size).astype(bool)
    flat = (hi - lo <= 0.5) & open_sky & finite

    labels, n = ndimage.label(flat)
    spots = []
    for lab in range(1, n + 1):
        jj, ii = np.nonzero(labels == lab)
        c = np.argmin((jj - jj.mean()) ** 2 + (ii - ii.mean()) ** 2)  # member nearest the centroid
        cx, cy = xs[ii[c]] + nav.cell / 2, ys[jj[c]] + nav.cell / 2
        spots.append((len(jj) * nav.cell * nav.cell, cx, cy, top[jj[c], ii[c]]))
    spots.sort(reverse=True)
    spots = spots[: args.count]
    for area, x, y, z in spots:
        print(f"{area:9.0f} m2 of centres  ({x:8.1f}, {y:8.1f}, {z:6.1f})")

    fig, ax = plt.subplots(figsize=(14, 14 * (y1 - y0) / (x1 - x0)), dpi=110)
    ax.imshow(top, origin="lower", extent=[x0, x1, y0, y1], cmap="turbo")
    ax.contour(xs + nav.cell / 2, ys + nav.cell / 2, flat, levels=[0.5], colors="white", linewidths=0.6)
    for _, x, y, z in spots:
        ax.plot(x, y, "w+", ms=10)
        ax.annotate(f"{z:.0f}", (x, y), color="white", fontsize=8, xytext=(4, 4), textcoords="offset points")
    fig.savefig(args.out, bbox_inches="tight")


if __name__ == "__main__":
    main()
