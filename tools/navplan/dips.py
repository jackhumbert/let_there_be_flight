"""Where the line makes the drone-style Autopilot slow down for nothing: every dip in its speed
(a slowdown it speeds up out of again), outside the landing's approach. The line's look matters
more than its time (Jack, 2026-10-29: it does not need to slow down), so this reports the dips, not
a lap time.

The speeds come from navplan's <prefix>_profile.csv (the car's limits in
src/red4ext/Navigation/Vehicle.hpp, the same the planner and the Autopilot use); this file holds
no physics of its own. A RouteDump CSV from the game (columns s, speed) works too: the Autopilot's
own profile.

    python tools/navplan/dips.py prefix_profile.csv [more.csv ...]
"""

import argparse
import csv

import numpy as np

APPROACH, DIP = 300.0, 5.0  # m before the end not counted (the landing), m/s a dip must lose


def load(path):
    """Distance, speed and limit reason along a profile CSV (navplan) or a route dump (game)."""
    with open(path) as f:
        rows = list(csv.DictReader(f))
    if rows and "v" in rows[0]:
        return (np.array([float(r["d"]) for r in rows]), np.array([float(r["v"]) for r in rows]),
                [r.get("why", "") for r in rows])
    if rows and "speed" in rows[0]:
        return (np.array([float(r["s"]) for r in rows]), np.array([float(r["speed"]) for r in rows]), [""] * len(rows))
    raise SystemExit(f"{path}: no speeds (navplan's _profile.csv or a RouteDump CSV)")


def dips(d, v, why):
    out, end = [], d[-1] - APPROACH
    i, n = 1, len(v)
    while i < n - 1:
        if v[i] < v[i - 1] - 1e-6:
            top = v[i - 1]
            j = i
            while j + 1 < n and v[j + 1] <= v[j] + 1e-6:
                j += 1
            low = j
            while j + 1 < n and v[j + 1] >= v[j] - 1e-6:
                j += 1
            if d[low] < end and top - v[low] >= DIP and v[j] > v[low] + DIP:
                out.append((d[low], top, v[low], why[low] or "?"))
            i = j + 1
        else:
            i += 1
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("lines", nargs="+")
    args = ap.parse_args()
    for path in args.lines:
        d, v, why = load(path)
        found = dips(d, v, why)
        lost = sum(t - lo for _, t, lo, _ in found)
        name = path.replace(chr(92), "/").split("/")[-1]
        print(f"{name:24s} {d[-1]:6.0f} m  {len(found):2d} dips, {lost:5.0f} m/s lost  "
              + "  ".join(f"{at:.0f} m {t:.0f}->{lo:.0f} ({w})" for at, t, lo, w in found[:6]))


if __name__ == "__main__":
    main()
