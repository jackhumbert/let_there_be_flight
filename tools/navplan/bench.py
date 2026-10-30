"""Plan the benchmark routes (routes.json) with navplan and print one table: the line's speed lost
in dips, bend jerk, sharpest bend, share within 3 m of blocked space, flown time and plan time per
route. With --against, a second run (another navplan build, or other parameters) is scored against
the first, as tune.py scores its trials (0 = as good, lower is better). The measures and the score
are measure.py's, shared with tune.py.

    python tools/navplan/bench.py [--out <dir>] [--vehicle shion] [--params a=1,b=2]
        [--against-navplan <exe>] [--against-params a=1,b=2] [--routes 0]

The navplan builds as cmake --build build --target navplan; copy build/navplan.exe somewhere first
to compare a change against the build before it.
"""

import argparse
import os
import sys
import tempfile

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import measure  # noqa: E402


def parse_params(text):
    out = {}
    for item in filter(None, (text or "").split(",")):
        k, v = item.split("=", 1)
        out[k] = v
    return out


def table(rts, res, base=None):
    print(f"{'route':10s} {'lost':>6s} {'jerk':>6s} {'sharp':>6s} {'tight%':>6s} {'time':>6s} {'ms':>6s} {'fixed':>5s}" + ("  score" if base else ""))
    scores = []
    for r, m in zip(rts, res):
        if not m or not m["ok"]:
            print(f"{r[0]:10s}  (not planned)")
            scores.append(10.0)
            continue
        line = (f"{r[0]:10s} {m['lost']:6.0f} {m['jerk']:6.2f} {m['sharpest']:6.1f} {m['tight']:6.1f} {m['time']:6.1f} "
                f"{m['ms']:6.0f} {m['repaired']:5d}")
        if base:
            s = measure.score(m, base.get(r[0]))
            scores.append(s)
            line += f"  {s:5.2f}"
        print(line)
    if base:
        print(f"mean score {np.mean(scores):.3f} (0 = as good as the first run, lower is better)")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", help="folder for navplan's files (default: a temporary one)")
    ap.add_argument("--navmap", help="default: LTBF_NAVMAP_FILE in build/CMakeCache.txt")
    ap.add_argument("--navplan", help="navplan.exe (default build/navplan.exe)")
    ap.add_argument("--vehicle", default="shion", help="a car in vehicles.json")
    ap.add_argument("--params", help="NAVPLAN_PARAMS for the run")
    ap.add_argument("--against-navplan", help="a second navplan.exe, scored against the first run")
    ap.add_argument("--against-params", help="NAVPLAN_PARAMS for the second run")
    ap.add_argument("--routes", type=int, default=0, help="helipad routes besides the benchmarks")
    ap.add_argument("--jobs", type=int, default=max(1, (os.cpu_count() or 4) - 2))
    args = ap.parse_args()

    navmap = args.navmap or measure.cmake_navmap()
    vehicle = measure.vehicle_spec(args.vehicle)
    rts = measure.routes(args.routes)
    out = args.out or tempfile.mkdtemp(prefix="bench_")
    measure.set_navplan(args.navplan)
    print(f"{measure.NAVPLAN} {args.params or ''}")
    first = measure.run_all(navmap, vehicle, rts, parse_params(args.params), os.path.join(out, "a"), args.jobs)
    table(rts, first)
    if args.against_navplan or args.against_params is not None:
        measure.set_navplan(args.against_navplan)
        print(f"\nagainst {measure.NAVPLAN} {args.against_params or ''}")
        second = measure.run_all(navmap, vehicle, rts, parse_params(args.against_params), os.path.join(out, "b"), args.jobs)
        table(rts, second, base=dict(zip([r[0] for r in rts], first)))
    print(f"\nfiles in {out}")


if __name__ == "__main__":
    main()
