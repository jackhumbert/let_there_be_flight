"""Plan the benchmark routes (routes.json) with navplan and print one table: the line's speed lost
in dips, bend jerk, sharpest bend, share within 3 m of blocked space, planned time and plan time per
route, and the whole trip flown (navplan NAVPLAN_TRIP: the Autopilot's own C++ from rest at the start
to landed at the goal, on flightsim's rigid body with the force tail; tools/flightsim/Trip.hpp): landed,
the end status when not, flown time, off the line p95 and p99 in m, mean speed error in m/s, landing
error in m, touchdown speed in m/s, replans, rays at most in a frame, and the trip checks broken
(CheckTrip; their text follows the table). With --against, a second run (another navplan build, or other
parameters) is scored against the first, as tune.py scores its trials (0 = as good, lower is better).
The measures and the score are measure.py's, shared with tune.py.

    python tools/navplan/bench.py [--out <dir>] [--vehicle shion] [--params a=1,b=2] [--seed 1]
        [--knock remaining,sx,sy,sz,vx,vy,vz] [--against-navplan <exe>] [--against-params a=1,b=2] [--routes 0]

The navplan builds as cmake --build build --target navplan; copy build/navplan.exe somewhere first
to compare a change against the build before it.
"""

import argparse
import os
import sys

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import measure  # noqa: E402

sys.path.insert(0, os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "common"))
import paths  # noqa: E402


def parse_params(text):
    out = {}
    for item in filter(None, (text or "").split(",")):
        k, v = item.split("=", 1)
        out[k] = v
    return out


def trip_columns(t):
    """The trip's columns (bench and fleet): landed (or the end status), flown s, off95, off99, speed error, landing
    error, touchdown, replans, rays at most in a frame, checks broken."""
    if not t:
        return f" {'-':>9s} {'-':>6s} {'-':>5s} {'-':>5s} {'-':>4s} {'-':>5s} {'-':>4s} {'-':>3s} {'-':>4s} {'-':>3s}"
    end = "landed" if t["landed"] else t["status"].replace(" ", "_")[:9]
    land = f"{t['landing_distance']:5.2f}" if t["landed"] else f"{'-':>5s}"
    touch = f"{t['touchdown_speed']:4.1f}" if t["touchdown_speed"] >= 0 else f"{'-':>4s}"
    return (f" {end:>9s} {t['time']:6.1f} {t['off_p95']:5.1f} {t['off_p99']:5.1f} {t['speed_err_mean']:4.1f} {land} {touch}"
            f" {t['replans']:3d} {t['rays_max_frame']:4d} {len(t['broken']):3d}")


TRIP_HEADER = (f" {'trip':>9s} {'flown':>6s} {'off95':>5s} {'off99':>5s} {'spd':>4s} {'land':>5s} {'td':>4s} {'rp':>3s}"
               f" {'rays':>4s} {'brk':>3s}")


def table(rts, res, base=None):
    print(f"{'route':10s} {'lost':>6s} {'jerk':>6s} {'sharp':>6s} {'tight%':>6s} {'time':>6s} {'ms':>6s} {'fixed':>5s}"
          + TRIP_HEADER + ("  score" if base else ""))
    scores = []
    broken = []
    for r, m in zip(rts, res):
        if not m or not m["ok"]:
            print(f"{r[0]:10s}  (not planned)")
            scores.append(10.0)
            continue
        line = (f"{r[0]:10s} {m['lost']:6.0f} {m['jerk']:6.2f} {m['sharpest']:6.1f} {m['tight']:6.1f} {m['time']:6.1f} "
                f"{m['ms']:6.0f} {m['repaired']:5d}")
        t = m.get("trip")
        line += trip_columns(t)
        broken += [f"{r[0]}: {b}" for b in (t["broken"] if t else [])]
        if base:
            s = measure.score(m, base.get(r[0]))
            scores.append(s)
            line += f"  {s:5.2f}"
        print(line)
    for b in broken:
        print(f"  {b}")
    if base:
        print(f"mean score {np.mean(scores):.3f} (0 = as good as the first run, lower is better)")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", help="folder for navplan's files (default: a new one in paths.TMP)")
    ap.add_argument("--navmap", help="default: LTBF_NAVMAP_FILE in build/CMakeCache.txt")
    ap.add_argument("--navplan", help="navplan.exe (default build/navplan.exe)")
    ap.add_argument("--vehicle", default="shion", help="a car in vehicles.json")
    ap.add_argument("--params", help="NAVPLAN_PARAMS for the run")
    ap.add_argument("--against-navplan", help="a second navplan.exe, scored against the first run")
    ap.add_argument("--against-params", help="NAVPLAN_PARAMS for the second run")
    ap.add_argument("--routes", type=int, default=0, help="helipad routes besides the benchmarks")
    ap.add_argument("--seed", help="NAVPLAN_TRIP_SEED")
    ap.add_argument("--knock", help="NAVPLAN_TRIP_KNOCK: a knock once this much route is left (remaining,spin x,y,z,velocity x,y,z)")
    ap.add_argument("--jobs", type=int, default=max(1, (os.cpu_count() or 4) - 2))
    args = ap.parse_args()

    navmap = args.navmap or measure.cmake_navmap()
    vehicle = measure.vehicle_spec(args.vehicle)
    rts = measure.routes(args.routes)
    out = args.out or paths.scratch("bench_")
    measure.set_navplan(args.navplan)
    print(f"{measure.NAVPLAN} {args.params or ''}")
    extra = {k: v for k, v in (("NAVPLAN_TRIP_SEED", args.seed), ("NAVPLAN_TRIP_KNOCK", args.knock)) if v}
    first = measure.run_all(navmap, vehicle, rts, parse_params(args.params), os.path.join(out, "a"), args.jobs,
                            trip=True, env_extra=extra)
    table(rts, first)
    if args.against_navplan or args.against_params is not None:
        measure.set_navplan(args.against_navplan)
        print(f"\nagainst {measure.NAVPLAN} {args.against_params or ''}")
        second = measure.run_all(navmap, vehicle, rts, parse_params(args.against_params), os.path.join(out, "b"),
                                 args.jobs, trip=True, env_extra=extra)
        table(rts, second, base=dict(zip([r[0] for r in rts], first)))
    print(f"\nfiles in {out}")


if __name__ == "__main__":
    main()
