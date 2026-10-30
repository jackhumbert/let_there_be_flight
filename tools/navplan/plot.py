"""Plot routes from above over the nav map (navmap.bin), to see how the Autopilot's path bends.

Background: per 2 m column, the height of the open-sky floor (roofs, streets), grey, darker is
lower. Columns where the route's own height is blocked are red, so a route through red is through
something. Each CSV (x,y,z, header row) is drawn as a line, coloured by height when it is the only
one; with several, one colour each, plus markers where a racing line leg is blocked.

    python tools/navplan/plot.py NAVMAP.bin a_route.csv b_line.csv [-o out.png] [--margin 80]
            [--zoom x,y,radius]

navplan (tools/navplan/navplan.cpp) writes <prefix>_route.csv and <prefix>_line.csv; an LTBF log draws the flown path from the
Autopilot telemetry in it;
LtbfBridge.RouteDump (tools/bridge) writes the in-game route (red4ext/logs/ltbf_route_<time>.csv, extra columns ignored).
"""

import argparse
import csv

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np

import os
import sys

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))  # tools/, for common
from common.navmap import NavMap  # noqa: E402,F401 (city.py and navviz.py import it from here)

def load(path):
    """A route CSV (x,y,z columns), or an LTBF log: the flown path from its Autopilot telemetry
    lines ([AutopilotA], aerobatics; time, then x y z). The Autopilot's telemetry CSV
    (red4ext/logs/ltbf_autopilot_<time>.csv) has x, y, z columns and loads as a route CSV."""
    if not path.lower().endswith(".csv"):
        pts = []
        for line in open(path, errors="replace"):
            for tag in ("[AutopilotA] ",):
                if tag in line:
                    f = line.split(tag, 1)[1].split()
                    pts.append([float(f[1]), float(f[2]), float(f[3])])
        return np.array(pts)
    with open(path) as f:
        rows = list(csv.DictReader(f))
    return np.array([[float(r["x"]), float(r["y"]), float(r["z"])] for r in rows])


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("navmap")
    ap.add_argument("routes", nargs="+")
    ap.add_argument("-o", "--out", default="routes.png")
    ap.add_argument("--margin", type=float, default=80.0)
    ap.add_argument("--zoom", help="x,y,radius: plot only around this point")
    args = ap.parse_args()

    nav = NavMap(args.navmap)
    routes = [load(p) for p in args.routes]
    allp = np.concatenate(routes)
    if args.zoom:
        zx, zy, zr = map(float, args.zoom.split(","))
        x0, x1, y0, y1 = zx - zr, zx + zr, zy - zr, zy + zr
    else:
        x0, y0 = allp[:, 0].min() - args.margin, allp[:, 1].min() - args.margin
        x1, y1 = allp[:, 0].max() + args.margin, allp[:, 1].max() + args.margin
    cx0, cy0 = nav.cell_of(x0, y0)
    cx1, cy1 = nav.cell_of(x1, y1)
    step = max(1, int(max(cx1 - cx0, cy1 - cy0) / 900))  # keep the raster near 900 px

    # route height per column (nearest route point), to mark where that height is blocked
    ref = routes[0]
    floor = np.full(((cy1 - cy0) // step + 1, (cx1 - cx0) // step + 1), np.nan)
    blocked = np.zeros_like(floor, dtype=bool)
    for iy, cy in enumerate(range(cy0, cy1 + 1, step)):
        for ix, cx in enumerate(range(cx0, cx1 + 1, step)):
            g = nav.column(cx, cy)
            if not g:
                continue
            floor[iy, ix] = g[-1][0] if np.isinf(g[-1][1]) else np.nan
    # blocked at route height: only near the route (within 60 m), where it matters
    for route in routes[:1]:
        for p in route:
            px, py = nav.cell_of(p[0], p[1])
            r = int(30 / nav.cell)
            for cy in range(py - r, py + r + 1, step):
                for cx in range(px - r, px + r + 1, step):
                    iy, ix = (cy - cy0) // step, (cx - cx0) // step
                    if 0 <= iy < floor.shape[0] and 0 <= ix < floor.shape[1]:
                        if not any(b <= p[2] <= t for b, t in nav.column(cx, cy)):
                            blocked[iy, ix] = True

    fig, ax = plt.subplots(figsize=(12, 12 * (y1 - y0) / max(x1 - x0, 1)), dpi=110)
    extent = (cx0 * nav.cell + nav.ox, (cx1 + 1) * nav.cell + nav.ox, cy0 * nav.cell + nav.oy, (cy1 + 1) * nav.cell + nav.oy)
    ax.imshow(floor, origin="lower", extent=extent, cmap="Greys_r", vmin=np.nanpercentile(floor, 2), vmax=np.nanpercentile(floor, 98))
    red = np.zeros(blocked.shape + (4,))
    red[blocked] = (0.85, 0.1, 0.1, 0.55)
    ax.imshow(red, origin="lower", extent=extent)

    colours = ["#2a9df4", "#f4a62a", "#3ccf4e", "#c84cf4"]
    for i, (path, route) in enumerate(zip(args.routes, routes)):
        if len(routes) == 1:
            sc = ax.scatter(route[:, 0], route[:, 1], c=route[:, 2], s=4, cmap="viridis")
            fig.colorbar(sc, ax=ax, label="height (m)", shrink=0.6)
        else:
            ax.plot(route[:, 0], route[:, 1], "-", color=colours[i % 4], lw=1.6, label=f"{path.split('/')[-1].split(chr(92))[-1]} ({len(route)} pts)")
            ax.plot(route[:, 0], route[:, 1], ".", color=colours[i % 4], ms=2)
    ax.plot(*routes[0][0, :2], "go", ms=8)
    ax.plot(*routes[0][-1, :2], "rs", ms=8)
    ax.set_xlim(x0, x1)
    ax.set_ylim(y0, y1)
    ax.set_aspect("equal")
    ax.set_xlabel("x (m)")
    ax.set_ylabel("y (m)")
    ax.set_title("Grey: open-sky floor (darker is lower). Red: blocked at the first route's height.")
    if len(routes) > 1:
        ax.legend(loc="best")
    fig.tight_layout()
    fig.savefig(args.out)
    print(f"wrote {args.out}")

    # side view: height against distance along each route, with the free gap under the first
    fig, ax = plt.subplots(figsize=(14, 4.5), dpi=110)
    for i, (path, route) in enumerate(zip(args.routes, routes)):
        s = np.concatenate([[0.0], np.cumsum(np.linalg.norm(np.diff(route, axis=0), axis=1))])
        if i == 0:
            fine = np.arange(0.0, s[-1], 2.0)
            xs, ys = np.interp(fine, s, route[:, 0]), np.interp(fine, s, route[:, 1])
            zs = np.interp(fine, s, route[:, 2])
            lo, hi = [], []
            for x, y, z in zip(xs, ys, zs):
                g = [gap for gap in nav.column(*nav.cell_of(x, y)) if gap[0] <= z + 3 and z - 3 <= gap[1]]
                lo.append(g[0][0] if g else np.nan)
                hi.append(min(g[0][1], z + 60) if g else np.nan)
            ax.fill_between(fine, lo, hi, color="#dddddd", label="free gap at the route")
        ax.plot(s, route[:, 2], "-", color=colours[i % 4], lw=1.5, label=path.split("/")[-1].split(chr(92))[-1])
    ax.set_xlabel("distance along the route (m)")
    ax.set_ylabel("height (m)")
    ax.legend(loc="best")
    fig.tight_layout()
    side = args.out.rsplit(".", 1)[0] + "_profile.png"
    fig.savefig(side)
    print(f"wrote {side}")


if __name__ == "__main__":
    main()
