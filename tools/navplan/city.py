"""The whole city from above, as a height map from the nav map (navmap.bin), with routes over it.

Background: per pixel (--res metres, default 8), the height of the open-sky floor there (roofs,
streets, terrain) from the highest cell in the pixel, light is high; water and empty map are
blank. Each route (a CSV with x,y,z columns, as navplan writes them; an LTBF log for a flown path)
is a coloured line with its start (circle) and goal (square) marked, and its name in the legend:
give it as name=path. --pads marks the landing pads from helipads.tsv.

    python tools/navplan/city.py NAVMAP.bin [name=]route.csv ... [-o city.png] [--res 8]
            [--extent x0,y0,x1,y1] [--pads src/navigation/helipads.tsv] [--grid 500]
"""

import argparse

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np

from plot import NavMap, load

# the built-up city and the edge of the Badlands; the map itself runs much further
CITY = (-3400.0, -3600.0, 3600.0, 2600.0)


def floor_map(nav, res, extent):
    """Highest open-sky floor per pixel of res metres over extent, NaN where there is none."""
    step = max(1, int(round(res / nav.cell)))
    x0, y0, x1, y1 = extent
    cx0, cy0 = nav.cell_of(x0, y0)
    cx1, cy1 = nav.cell_of(x1, y1)
    cx0, cy0 = max(cx0, 0), max(cy0, 0)
    cx1, cy1 = min(cx1, nav.w - 1), min(cy1, nav.h - 1)
    w, h = (cx1 - cx0) // step + 1, (cy1 - cy0) // step + 1
    out = np.full((h, w), np.nan, np.float32)
    t = nav.tile
    for ty in range(cy0 // t, cy1 // t + 1):
        for tx in range(cx0 // t, cx1 // t + 1):
            tile = nav.tile_data(tx, ty)
            if tile is None:
                continue
            counts, gaps, _ = tile
            counts = counts.astype(np.int64)
            last = np.cumsum(counts) - 1
            has = counts > 0
            top = np.full(t * t, np.nan, np.float32)
            idx = last[has]
            open_sky = gaps[idx, 1] >= nav.nz
            vals = np.where(open_sky, nav.z0 + gaps[idx, 0].astype(np.float32) * nav.vz, np.nan)
            top[has] = vals
            top = top.reshape(t, t)
            # cells of this tile inside the extent, on the step grid
            gy = np.arange(ty * t, ty * t + t)
            gx = np.arange(tx * t, tx * t + t)
            sel_y = (gy >= cy0) & (gy <= cy1) & ((gy - cy0) % step == 0)
            sel_x = (gx >= cx0) & (gx <= cx1) & ((gx - cx0) % step == 0)
            if not sel_y.any() or not sel_x.any():
                continue
            block = top[np.ix_(sel_y, sel_x)]
            oy = (gy[sel_y] - cy0) // step
            ox = (gx[sel_x] - cx0) // step
            out[np.ix_(oy, ox)] = block
    real = (cx0 * nav.cell + nav.ox, (cx1 + 1) * nav.cell + nav.ox, cy0 * nav.cell + nav.oy, (cy1 + 1) * nav.cell + nav.oy)
    return out, real


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("navmap")
    ap.add_argument("routes", nargs="*")
    ap.add_argument("-o", "--out", default="city.png")
    ap.add_argument("--res", type=float, default=8.0)
    ap.add_argument("--extent", help="x0,y0,x1,y1 (default: the city)")
    ap.add_argument("--pads", help="helipads.tsv: mark the landing pads")
    ap.add_argument("--grid", type=float, default=500.0, help="grid line spacing, m (0: none)")
    args = ap.parse_args()

    nav = NavMap(args.navmap)
    extent = tuple(map(float, args.extent.split(","))) if args.extent else CITY
    floor, real = floor_map(nav, args.res, extent)

    w = real[1] - real[0]
    h = real[3] - real[2]
    fig, ax = plt.subplots(figsize=(14, 14 * h / w), dpi=120)
    lo, hi = np.nanpercentile(floor, 1), np.nanpercentile(floor, 99.5)
    ax.imshow(floor, origin="lower", extent=real, cmap="Greys_r", vmin=lo, vmax=hi, interpolation="nearest")

    if args.pads:
        px, py = [], []
        for line in open(args.pads):
            if line.startswith("#") or not line.strip():
                continue
            f = line.rstrip("\n").split("\t")
            px.append(float(f[1]))
            py.append(float(f[2]))
        ax.scatter(px, py, s=10, marker="^", color="#a0a0a0", edgecolors="#404040", linewidths=0.4, label="landing pads", zorder=3)

    # categorical palette (validated set), one hue per route in a fixed order
    colours = ["#2a78d6", "#eb6834", "#1baf7a", "#a35ad6", "#d6a21b", "#d63a5a"]
    for i, spec in enumerate(args.routes):
        name, path = spec.split("=", 1) if "=" in spec else (spec, spec)
        pts = load(path)
        if len(pts) == 0:
            continue
        c = colours[i % len(colours)]
        length = np.sum(np.linalg.norm(np.diff(pts, axis=0), axis=1))
        ax.plot(pts[:, 0], pts[:, 1], color="white", lw=4.2, solid_capstyle="round", zorder=4)
        ax.plot(pts[:, 0], pts[:, 1], color=c, lw=2.2, solid_capstyle="round", zorder=5,
                label=f"{name}: {length / 1000:.1f} km, {pts[:, 2].min():.0f} to {pts[:, 2].max():.0f} m up")
        ax.plot(*pts[0, :2], "o", ms=9, color=c, mec="white", mew=1.5, zorder=6)
        ax.plot(*pts[-1, :2], "s", ms=9, color=c, mec="white", mew=1.5, zorder=6)

    if args.grid > 0:
        ax.set_xticks(np.arange(np.ceil(real[0] / args.grid) * args.grid, real[1], args.grid))
        ax.set_yticks(np.arange(np.ceil(real[2] / args.grid) * args.grid, real[3], args.grid))
        ax.grid(color="#ffffff", alpha=0.12, lw=0.6)
        ax.tick_params(labelsize=7, colors="#555555")
        plt.setp(ax.get_xticklabels(), rotation=90)
    ax.set_xlim(real[0], real[1])
    ax.set_ylim(real[2], real[3])
    ax.set_aspect("equal")
    sm = plt.cm.ScalarMappable(cmap="Greys_r", norm=plt.Normalize(lo, hi))
    cb = fig.colorbar(sm, ax=ax, fraction=0.025, pad=0.01)
    cb.set_label("open-sky floor height (m)", fontsize=8)
    cb.ax.tick_params(labelsize=7)
    if args.routes or args.pads:
        ax.legend(loc="lower left", fontsize=8, framealpha=0.85)
    ax.set_title("Night City from the nav map (x east, y north; circle = start, square = goal)", fontsize=10, loc="left")
    fig.tight_layout()
    fig.savefig(args.out)
    print(f"wrote {args.out}")


if __name__ == "__main__":
    main()
