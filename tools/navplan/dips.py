"""Where a racing line makes the drone-style Autopilot slow down for nothing: the speed it can fly
(turns at the banked lateral limit, crests at the push-over limit, pull-ups at the lift's, then
speeding up and braking), and every dip in it (a slowdown it speeds up out of again), outside the
landing's approach. The line's look matters more than its time (Jack, 2026-10-29: it does not need
to slow down), so this reports the dips, not a lap time.

    python tools/navplan/dips.py line.csv [more.csv ...]
"""

import argparse
import csv
import math

import numpy as np

TOP, LATERAL, PUSH_OVER, PULL_UP, ACCEL, DECEL = 120.0, 22.7, 21.0, 40.0, 10.0, 15.0
SPAN, STEP, APPROACH, DIP = 10.0, 2.0, 300.0, 5.0


def load(path):
    with open(path) as f:
        return np.array([[float(r["x"]), float(r["y"]), float(r["z"])] for r in csv.DictReader(f)])


def profile(points):
    seg = np.linalg.norm(np.diff(points, axis=0), axis=1)
    s = np.concatenate([[0.0], np.cumsum(seg)])
    d = np.arange(0.0, s[-1], STEP)
    p = np.stack([np.interp(d, s, points[:, k]) for k in range(3)], 1)
    n, k = len(d), int(SPAN / STEP)
    limit = np.full(n, TOP)
    why = [""] * n
    for i in range(k, n - k):
        a, b, c = p[i - k], p[i], p[i + k]
        h1, h2 = np.linalg.norm((b - a)[:2]), np.linalg.norm((c - b)[:2])
        if h1 < 1.0 or h2 < 1.0:
            continue
        t1, t2 = (b - a)[:2] / h1, (c - b)[:2] / h2
        turn = math.acos(max(-1.0, min(1.0, float(t1 @ t2))))
        kh = turn / (0.5 * (h1 + h2))
        kv = ((c[2] - b[2]) / h2 - (b[2] - a[2]) / h1) / (0.5 * (h1 + h2))
        for lim, tag in ((math.sqrt(LATERAL / kh) if kh > 1e-6 else TOP, "turn"),
                         (math.sqrt(PUSH_OVER / -kv) if kv < -1e-6 else TOP, "crest"),
                         (math.sqrt(PULL_UP / kv) if kv > 1e-6 else TOP, "pull-up")):
            if lim < limit[i]:
                limit[i], why[i] = lim, tag
    v = limit.copy()
    v[0] = 0.0
    for i in range(1, n):
        v[i] = min(v[i], math.sqrt(v[i - 1] ** 2 + 2 * ACCEL * STEP))
    v[-1] = 0.0
    for i in range(n - 2, -1, -1):
        v[i] = min(v[i], math.sqrt(v[i + 1] ** 2 + 2 * DECEL * STEP))
    return d, v, limit, why


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
        d, v, limit, why = profile(load(path))
        found = dips(d, v, why)
        lost = sum(t - lo for _, t, lo, _ in found)
        name = path.replace(chr(92), "/").split("/")[-1]
        print(f"{name:24s} {d[-1]:6.0f} m  {len(found):2d} dips, {lost:5.0f} m/s lost  "
              + "  ".join(f"{at:.0f} m {t:.0f}->{lo:.0f} ({w})" for at, t, lo, w in found[:6]))


if __name__ == "__main__":
    main()
