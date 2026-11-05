"""Sweep plan ahead (docs/design/trajectory.md step 7) offline: for each first-leg horizon (metres
along the doorway route) and margin (seconds added to the expected plan time before the join),
navplan plans every route the way the Autopilot would (NAVPLAN_AHEAD: first leg, the rest from
where the car will be, spliced) and against one full plan from rest. Per setting: the time until
the car moves, how often the rest came later than the join (the car would be braking at the first
leg's end), the arrival against one full plan (both counting the planning wait), the speed lost in
dips against one full plan, and failures.

    python tools/navplan/ahead_sweep.py [--horizons 150,300,600,1000] [--margins 0.5,1,2]
        [--routes 20] [--jobs 6] [--out <dir>] [--navmap <navmap.bin>] [--vehicle shion]

Plan times are measured with --jobs plans at once: keep it below the cores, or they read high.
"""

import argparse
import collections
import concurrent.futures as cf
import json
import os
import statistics
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import measure  # noqa: E402

sys.path.insert(0, os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "common"))
import paths  # noqa: E402
from measure import cmake_navmap, routes, vehicle_spec  # noqa: E402


def run(navmap, vehicle, route, horizon, margin, out):
    name, a, b = route
    prefix = os.path.join(out, f"{name}_{horizon:g}_{margin:g}")
    env = dict(os.environ, NAVPLAN_RACE="1", NAVPLAN_VEHICLE=vehicle, NAVPLAN_AHEAD=f"{horizon},{margin}")
    try:
        r = subprocess.run([measure.NAVPLAN, navmap, *map(str, a), *map(str, b), prefix], env=env, capture_output=True,
                           text=True, timeout=300)
    except subprocess.TimeoutExpired:
        return None
    line = next((l for l in r.stdout.splitlines() if l.startswith("ahead ")), None)
    return json.loads(line[len("ahead "):]) if line else None


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--horizons", default="150,300,600,1000")
    ap.add_argument("--margins", default="0.5,1,2")
    ap.add_argument("--routes", type=int, default=20, help="helipad routes on top of the benchmark ones")
    ap.add_argument("--jobs", type=int, default=6)
    ap.add_argument("--out", help="folder for navplan's files and sweep.json (default: a new one in paths.TMP, D:/cp2077_work/tmp)")
    ap.add_argument("--navmap")
    ap.add_argument("--vehicle", default="shion")
    args = ap.parse_args()
    navmap = args.navmap or cmake_navmap()
    vehicle = vehicle_spec(args.vehicle)
    out = args.out or paths.scratch("ahead_")
    os.makedirs(out, exist_ok=True)
    rts = routes(args.routes)
    settings = [(float(h), float(m)) for h in args.horizons.split(",") for m in args.margins.split(",")]
    results = collections.defaultdict(dict)
    with cf.ThreadPoolExecutor(args.jobs) as pool:
        futs = {pool.submit(run, navmap, vehicle, r, h, m, out): (r[0], h, m) for r in rts for h, m in settings}
        for f in cf.as_completed(futs):
            name, h, m = futs[f]
            results[(h, m)][name] = f.result()
    print(f"{len(rts)} routes; times with {args.jobs} plans at once")
    print(f"{'horizon':>7s} {'margin':>6s} {'move s':>7s} {'late':>5s} {'short':>5s} {'fail':>4s} {'arrive':>8s} {'dips':>7s}  (arrive and dips: "
          "against one full plan, mean)")
    table = []
    for h, m in settings:
        rs = results[(h, m)]
        ok = [x for x in rs.values() if x and x["ok"]]
        fail = len(rs) - len(ok)
        if not ok:
            print(f"{h:7.0f} {m:6.1f}  all failed")
            continue
        move = statistics.median(x["move_s"] for x in ok)
        late = sum(x["late"] for x in ok)
        short = sum(x["short"] for x in ok)
        arrive = statistics.mean(x["arrive_s"] - x["full_arrive_s"] for x in ok)
        dips = statistics.mean(x["dips"] - x["full_dips"] for x in ok)
        print(f"{h:7.0f} {m:6.1f} {move:7.2f} {late:5d} {short:5d} {fail:4d} {arrive:+8.2f} {dips:+7.1f}")
        table.append(dict(horizon=h, margin=m, move_s=move, late=late, short=short, fail=fail, arrive=arrive, dips=dips))
    json.dump(dict(table=table, runs={f"{h}_{m}": v for (h, m), v in results.items()}), open(os.path.join(out, "sweep.json"), "w"),
              indent=1)
    print("files in", out)


if __name__ == "__main__":
    main()
