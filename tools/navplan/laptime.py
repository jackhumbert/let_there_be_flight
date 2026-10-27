"""Estimate how fast the Autopilot's Race option can fly a route (CSV x,y,z), offline.

Same limits as FlightModeAutopilot's race code: corner speed sqrt(lateral x radius) with the
radius taken over +-15 m (lateral: the lift banked into the turn, at raceMargin), a backward pass
braking with reverse surge, the brake and drag, a forward pass accelerating with surge less drag,
top speed where surge meets drag. Defaults are the Archer Quartz measured in game (mass 1115,
air resistance 1.8, brake friction 1.4).

    python tools/navplan/laptime.py route.csv [more.csv ...] [--start-speed 0] [--navmap navmap.bin]

With --navmap it also reports the line's clearance: per point, the distance to the nearest blocked
space (the map already holds the car's size) in 8 directions across and above / below, out to 10 m.
"""

import argparse
import csv
import math

import numpy as np

G = 9.81
SURGE, LIFT, MARGIN, SPAN = 15.0, 25.0, 0.85, 15.0


def load(path):
    with open(path) as f:
        return np.array([[float(r["x"]), float(r["y"]), float(r["z"])] for r in csv.DictReader(f)])


def estimate(points, mass=1115.0, air=1.8, brake=1.4, start=0.0, step=2.0):
    seg = np.linalg.norm(np.diff(points, axis=0), axis=1)
    s = np.concatenate([[0.0], np.cumsum(seg)])
    d = np.arange(0.0, s[-1], step)
    p = np.stack([np.interp(d, s, points[:, k]) for k in range(3)], 1)
    k_air = 1.2 * air / mass
    top = 0.97 * math.sqrt(SURGE / k_air)
    lateral = MARGIN * math.sqrt((G + LIFT) ** 2 - G * G)
    n = len(d)
    off = max(1, int(SPAN / step))
    radius = np.full(n, 1e6)
    for i in range(off, n - off):
        a, b, c = p[i - off], p[i], p[i + off]
        cr = np.linalg.norm(np.cross(b - a, c - b))
        if cr > 1e-6:
            radius[i] = np.linalg.norm(b - a) * np.linalg.norm(c - b) * np.linalg.norm(c - a) / (2 * cr)
    v = np.minimum(top, np.sqrt(lateral * radius))
    v[-1] = 0.0
    for i in range(n - 2, -1, -1):  # braking
        dec = MARGIN * (SURGE + brake * v[i + 1] + k_air * v[i + 1] ** 2)
        v[i] = min(v[i], math.sqrt(v[i + 1] ** 2 + 2 * dec * step))
    v[0] = min(v[0], start)
    for i in range(1, n):  # accelerating
        acc = max(SURGE - k_air * v[i - 1] ** 2, 0.0)
        v[i] = min(v[i], math.sqrt(v[i - 1] ** 2 + 2 * acc * step))
    t = np.sum(step / np.maximum((v[:-1] + v[1:]) / 2, 0.5))
    return {"length": s[-1], "time": t, "top": top, "radius": radius, "speed": v, "d": d}


def clearance(nav, points, step=5.0):
    """Distance (m, up to 10) from each point, every `step` metres, to the nearest blocked space."""
    seg = np.linalg.norm(np.diff(points, axis=0), axis=1)
    s = np.concatenate([[0.0], np.cumsum(seg)])
    d = np.arange(0.0, s[-1], step)
    p = np.stack([np.interp(d, s, points[:, k]) for k in range(3)], 1)

    def free(q):
        return any(b <= q[2] < t for b, t in nav.gaps(*nav.cell_of(q[0], q[1])))

    dirs = [np.array([math.cos(a), math.sin(a), 0.0]) for a in np.arange(8) * math.pi / 4] + [np.array([0, 0, 1.0]), np.array([0, 0, -1.0])]
    out = []
    for q in p:
        best = 10.0
        for n in dirs:
            for r in (1, 2, 3, 4, 5, 6, 8, 10):
                if r >= best:
                    break
                if not free(q + n * r):
                    best = r
                    break
        out.append(best)
    return np.array(out)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("routes", nargs="+")
    ap.add_argument("--start-speed", type=float, default=0.0)
    ap.add_argument("--navmap")
    args = ap.parse_args()
    nav = None
    if args.navmap:
        import os, sys
        sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
        from plot import NavMap
        nav = NavMap(args.navmap)
    for path in args.routes:
        r = estimate(load(path), start=args.start_speed)
        near_top = np.mean(r["speed"] > 0.9 * r["top"]) * 100
        tight = np.sum(r["radius"] < (0.9 * r["top"]) ** 2 / (MARGIN * math.sqrt((G + LIFT) ** 2 - G * G))) * 2.0
        print(f"{path.replace(chr(92), '/').split('/')[-1]:24s} {r['length']:6.0f} m  {r['time']:5.1f} s  "
              f"mean {r['length'] / r['time']:4.1f} m/s  {near_top:3.0f}% at 90% of top ({r['top']:.0f} m/s)  "
              f"{tight:4.0f} m of bends too tight for it")
        if nav:
            c = clearance(nav, load(path))
            print(f"{'':24s} clearance: {np.mean(c < 3) * 100:4.1f}% of points under 3 m, {np.mean(c < 5) * 100:4.1f}% under 5 m, "
                  f"{np.mean(c < 8) * 100:4.1f}% under 8 m")


if __name__ == "__main__":
    main()
