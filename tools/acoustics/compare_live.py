"""The acoustics comparison log (red4ext/logs/ltbf_acoustics_<time>.csv, LtbfBridge.AcousticsLive):
for each near surface (floor, ceiling, nearest wall left and right), over the probes with live rays
on (by who cast them: the scripts or the plugin through PhysX), how often the game's own collision answered, how far its distance is from the nav map's, and
where only one of the two found a surface; then the materials hit and the probe's cost.

    python tools/acoustics/compare_live.py LOG.csv

The nav map's distance is pushed out by the car-sized shell it was grown by; the live one is the
hit itself, so they should agree within the map's 2 m cells where both see the same surface."""

import collections
import csv
import math
import sys

import numpy as np


SOURCES = {"1": "scripts", "2": "PhysX"}


def main():
    rows = [r for r in csv.DictReader(open(sys.argv[1])) if int(r["live_rays"]) >= 0]
    by_source = {}
    for r in rows:
        by_source.setdefault(r.get("source", "1"), []).append(r)
    for source, part in sorted(by_source.items()):
        print(f"== rays cast by {SOURCES.get(source, source)}")
        report(part)


def report(rows):
    print(f"{len(rows)} probes with live rays on")

    def f(r, k):
        return float(r[k]) if r[k] not in ("", "inf") else math.inf

    for s in ("floor", "ceiling", "left", "right"):
        live, cmap = np.array([f(r, s) for r in rows]), np.array([f(r, s + "_map") for r in rows])
        # with a live hit the probe takes it; without one the value is the map's
        has = np.array([bool(r.get(s + "_material")) for r in rows]) if s != "ceiling" else live != cmap
        both = has & np.isfinite(cmap)
        d = live[both] - cmap[both]
        only_live = has & ~np.isfinite(cmap)
        only_map = ~has & np.isfinite(cmap) & (cmap < 100)
        line = f"{s:8s} live hit {has.mean():5.1%}"
        if len(d):
            line += (f" | live - map: median {np.median(d):+.2f} m, p5 {np.percentile(d, 5):+.2f}, p95 {np.percentile(d, 95):+.2f},"
                     f" within 1 m {np.mean(np.abs(d) < 1):.0%}, within 3 m {np.mean(np.abs(d) < 3):.0%}")
        line += f" | only live {only_live.mean():.1%}, only map (< 100 m) {only_map.mean():.1%}"
        print(line)
    c = collections.Counter(r[k] for r in rows for k in ("floor_material", "left_material", "right_material") if r[k])
    print("materials:", ", ".join(f"{m} {n}" for m, n in c.most_common(10)))
    ms = np.array([float(r["probe_ms"]) for r in rows])
    print(f"probe: median {np.median(ms):.2f} ms, p99 {np.percentile(ms, 99):.2f} ms")


if __name__ == "__main__":
    main()
