"""Plan and fly the benchmark routes (routes.json) with every distinct car in the fleet: the Autopilot across
the vanilla cars and bikes (docs/design/autopilot.md, "Across the fleet").

The fleet is tools/flightsim/vehicles.tsv, one row per physical car. The Autopilot reads three numbers of the
car (mass, airResistance, brakingFriction: FlightModeAutopilot.RefreshVehicle); the rest of its VehicleSpec is
the flight and race tuning, the same on every car. Cars with the same three numbers plan and fly the same, so
each distinct triple is run once, with the Autopilot's defaults for the rest (drone style). Prints one row per
triple (top speed, the routes' planned and flown time, the worst offset from the line, the mean speed error,
runs off the line, failures), then the spread over the fleet per route.

    python tools/navplan/fleet.py [--out <dir>] [--routes 0] [--only 300/0.65,9380/5.5] [--navmap <file>]

navplan builds as cmake --build build --target navplan. A run of all 70 triples over the 5 benchmark routes
takes about 6 minutes (16 threads). Not part of ltbf_tests (it needs navmap.bin); tests/autopilot_fleet_tests.cpp is the
offline check on synthetic routes.
"""

import argparse
import csv
import os
import sys
from collections import Counter

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import measure  # noqa: E402

sys.path.insert(0, os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "common"))
import paths  # noqa: E402

FLEET = os.path.join(measure.ROOT, "tools", "flightsim", "vehicles.tsv")


def fleet(only=None, character=None):
    """[(mass, airResistance, brakingFriction, cars, example name, types, extra spec)] for each distinct car. With
    character (a thrust strength), each car's shape drag and thrust too (cdaFront, cdaTop, thrusters: VehicleSpec)."""
    with open(FLEET, encoding="utf-8") as f:
        rows = list(csv.DictReader(f, delimiter="\t", quoting=csv.QUOTE_NONE))  # names hold quotes
    key = lambda r: ((float(r["mass"]), round(float(r["airResistance"]), 3), round(float(r["brakingFriction"]), 3))
                     + ((float(r["cdaY"] or 0), float(r["thrusters"] or 0), float(r["cdaZ"] or 0))
                        if character is not None else ()))
    count = Counter(key(r) for r in rows)
    out = []
    for k in sorted(count):
        if only and f"{k[0]:g}/{k[1]:g}" not in only:
            continue
        cars = [r for r in rows if key(r) == k]
        name = next((r["name"] for r in cars if r["name"] and not r["name"].startswith("0x")), cars[0]["record"])
        extra = (f",cdaFront={k[3]:g},cdaTop={k[5]:g},thrusters={k[4]:g},thrustStrength={character:g}"
                 if character is not None else "")
        out.append((*k[:3], count[k], name, "/".join(sorted({r["type"] for r in cars})), extra))
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", help="folder for navplan's files (default: a new one in paths.TMP, D:/cp2077_work/tmp)")
    ap.add_argument("--navmap", help="default: LTBF_NAVMAP_FILE in build/CMakeCache.txt")
    ap.add_argument("--navplan", help="navplan.exe (default build/navplan.exe)")
    ap.add_argument("--routes", type=int, default=0, help="helipad routes besides the benchmarks")
    ap.add_argument("--only", help="mass/airResistance pairs to run, comma separated")
    ap.add_argument("--character", type=float, default=None,
                    help="each car's shape drag and thrust at this strength (0 to 1; docs/research/2026-11-04-aero-drag.md)")
    ap.add_argument("--jobs", type=int, default=max(1, (os.cpu_count() or 4) - 2))
    args = ap.parse_args()

    navmap = args.navmap or measure.cmake_navmap()
    measure.set_navplan(args.navplan)
    rts = measure.routes(args.routes)
    out = args.out or paths.scratch("fleet_")
    cars = fleet(set(args.only.split(",")) if args.only else None, args.character)

    print(f"{'mass':>6s} {'air':>5s} {'brake':>5s} {'cars':>4s} {'type':8s} {'top':>5s} {'plan s':>7s} {'flown s':>7s}"
          f" {'off95':>5s} {'off99':>5s} {'spd':>4s} {'out':>3s} {'fail':>4s}  example")
    per_route = {r[0]: [] for r in rts}
    for i, (mass, air, brake, n, name, kind, extra) in enumerate(cars):
        vehicle = f"mass={mass:g},airResistance={air:g},brakingFriction={brake:g}{extra}"
        res = measure.run_all(navmap, vehicle, rts, {}, os.path.join(out, f"{i}_{mass:g}_{air:g}"), args.jobs, follow=True)
        plan_t = flown_t = 0.0
        off95 = off99 = 0.0
        spd = []
        lost = fail = 0
        top = 0.0
        for r, m in zip(rts, res):
            f = m.get("follow") if m and m["ok"] else None
            if f is None or not (f["arrived"] or f["ended"]):
                fail += 1
                per_route[r[0]].append(None)
                continue
            plan_t += m["time"]
            flown_t += f["time"]
            off95 = max(off95, f["off_p95"])
            off99 = max(off99, f["off_p99"])
            spd.append(f["speed_err_mean"])
            lost += f["lost"]
            top = max(top, m.get("top", 0.0))
            per_route[r[0]].append((mass, air, m["time"], f["time"], f["off_p95"], f["speed_err_mean"], f["lost"]))
        print(f"{mass:6g} {air:5g} {brake:5g} {n:4d} {kind:8s} {top:5.1f} {plan_t:7.1f} {flown_t:7.1f}"
              f" {off95:5.1f} {off99:5.1f} {np.mean(spd) if spd else 0:4.1f} {lost:3d} {fail:4d}  {name}")

    print("\nper route over the fleet: min / median / max (car mass/airResistance at the ends)")
    for r, rows in per_route.items():
        ok = [x for x in rows if x]
        if not ok:
            print(f"{r:10s} no car flew it")
            continue
        for label, i in (("plan s", 2), ("flown s", 3), ("off95 m", 4), ("spd m/s", 5)):
            vals = sorted(ok, key=lambda x: x[i])
            lo, hi = vals[0], vals[-1]
            print(f"{r:10s} {label:8s} {lo[i]:6.1f} ({lo[0]:g}/{lo[1]:g})  {np.median([x[i] for x in ok]):6.1f}"
                  f"  {hi[i]:6.1f} ({hi[0]:g}/{hi[1]:g})")
        failed = len(rows) - len(ok)
        if failed:
            print(f"{r:10s} failed on {failed} of {len(rows)} cars")
    print(f"\nfiles in {out}")


if __name__ == "__main__":
    main()
