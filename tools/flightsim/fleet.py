"""Every surveyed vehicle through the flight simulator, summed up (docs/design/hover-fly-feel.md).

    python tools/flightsim/fleet.py [--vehicles tools/flightsim/vehicles.tsv] [--flightsim build/flightsim.exe]
                                    [--metric yaw_hold.yaw_rate_3s]... [--worst 8] [--csv out.csv]
                                    [--mode standard|hover|fly|drone|agdrone|droneassist] [--assist]

    python -P -E tools/flightsim/fleet.py --terrain [tools/flightsim/terrain_runs.tsv] [--only <line or tag part>]
                                          [--speeds 40,80,130] [--hover 0] [--mode ...] [--set ...]
                                          [--save runs.tsv | --from runs.tsv] [--worst 5]

--mode is flightsim's: the mode flown, and without scenario names that mode's scenarios (drone_* for the drone modes).

Records with the same physics (most of the 1400 car and bike records are paint and trim variants) are flown once.
For each metric: the spread over the distinct cars, and the cars at either end.

--terrain flies every distinct car (its underside counted in the physics) along the lines of a runs file over real
ground (flightsim --terrain, docs/design/hover-fly-terrain.md) and sums the runs up by car class (CLASSES), by kind
of ground (the lines' tags) and by speed: how often and how long the cars touch, the air over and sink under the
hover height, how late they follow, how much they rock, how many runs a wall stopped; then the cars that touch most
in each class. --save keeps the runs (a car's name is its index in the distinct cars), --from sums up a saved file
again without flying.
"""

import argparse
import csv
import io
import math
import os
import statistics
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
PHYSICS = ["type", "mass", "airResistance", "brakingFriction", "inertiaX", "inertiaY", "inertiaZ", "tensorX",
           "tensorY", "tensorZ", "wheelX", "wheelY", "wheelZ", "bottom"]


def distinct(path, keys=PHYSICS):
    """The table's rows, one per physics signature, each with the names that share it."""
    groups = {}
    with open(path, encoding="utf-8") as f:
        for row in csv.DictReader(f, delimiter="\t"):
            key = tuple(row.get(c, "") for c in keys)
            groups.setdefault(key, []).append(row)
    return [(rows[0], sorted({r["name"] for r in rows}), len(rows)) for rows in groups.values()]


# the car classes the terrain runs are summed up by, in order: a bound easy on a sedan can be impossible on a bus
CLASSES = ["bike", "sports", "sedan", "suv/pickup", "van/truck", "long", "8 thrusters", "12 thrusters"]
BODY = {"sport": "sports", "standard": "sedan", "suv": "suv/pickup", "truck": "van/truck", "bike": "bike"}


def car_class(row):
    """A car's class from its vehicles.tsv row: bikes by type, the 8 and 12 thruster cars by their count, a chassis
    over 7 m long (buses, the long trucks), then its body class (aero.py's, from the chassis shape's folder; by mass
    without one)."""
    if row["type"] == "bike":
        return "bike"
    thrusters = float(row["thrusters"] or 0)
    if thrusters >= 12:
        return "12 thrusters"
    if thrusters >= 8:
        return "8 thrusters"
    if row.get("boxMinY") and float(row["boxMaxY"]) - float(row["boxMinY"]) > 7.0:
        return "long"
    if row.get("body") in BODY:
        return BODY[row["body"]]
    return "van/truck" if float(row["mass"]) >= 2600 else "sedan"


def number(text):
    try:
        return float(text)
    except ValueError:
        return math.nan


def summary(runs):
    """One line's numbers over a set of runs (rows of flightsim --terrain), as (label, text) pairs."""
    f = {k: [number(r[k]) for r in runs] for k in runs[0]}
    finite = {k: [v for v in vs if math.isfinite(v)] for k, vs in f.items()}
    km = sum(finite["flown"]) / 1000.0

    def med(k):
        return statistics.median(finite[k]) if finite[k] else math.nan

    def share(k):
        return 100.0 * sum(v > 0 for v in finite[k]) / len(runs)

    seconds = sum(fl / max(sp, 0.1) for fl, sp in zip(finite["flown"], finite["speed"]))
    return [("runs", f"{len(runs):6d}"), ("touch%", f"{share('contacts'):6.0f}"),
            ("hits/km", f"{sum(finite['contacts']) / max(km, 1e-3):7.1f}"),
            ("touch t%", f"{100.0 * sum(finite['contact_time']) / max(seconds, 1e-3):8.1f}"),
            ("clr p05", f"{med('clear_p05'):7.2f}"), ("over95", f"{med('height_over_p95'):6.2f}"),
            ("under95", f"{med('height_under_p95'):7.2f}"), ("lag s", f"{med('lag'):5.2f}"),
            ("pitch", f"{med('pitch_wobble'):5.1f}"), ("roll", f"{med('roll_wobble'):5.2f}"),
            ("jerk", f"{med('jerk_p95'):5.0f}"), ("nose%", f"{share('nose_hits'):5.0f}"),
            ("wall%", f"{share('blocked'):5.0f}")]


def table(title, groups):
    """Prints one summary line a group: {label: runs}."""
    print(f"\n{title}")
    header = None
    for label, runs in groups.items():
        if not runs:
            continue
        cells = summary(runs)
        if header is None:
            header = f"  {'':16}" + " ".join(f"{name:>{len(text)}}" for name, text in cells)
            print(header)
        print(f"  {label:16}" + " ".join(text for _, text in cells))


def terrain(args):
    cars = distinct(args.vehicles, PHYSICS + ["underside"])
    if args.from_:
        with open(args.from_, encoding="utf-8") as f:
            runs = list(csv.DictReader(f, delimiter="\t"))
    else:
        print(f"{sum(n for _, _, n in cars)} records, {len(cars)} distinct cars")
        with tempfile.NamedTemporaryFile("w", suffix=".tsv", delete=False, encoding="utf-8", newline="\n") as t:
            with open(args.vehicles, encoding="utf-8") as f:
                header = f.readline().rstrip("\n").split("\t")
            t.write("\t".join(header) + "\n")
            for i, (row, _, _) in enumerate(cars):
                t.write("\t".join(dict(row, name=f"{i}")[c] for c in header) + "\n")
        try:
            cmd = [args.flightsim, "--terrain", args.terrain, "--vehicles", t.name, "--speeds", args.speeds,
                   "--hover", str(args.hover)]
            cmd += (["--only", args.only] if args.only else []) + (["--mode", args.mode] if args.mode else [])
            cmd += [a for kv in args.set for a in ("--set", kv)]
            out = subprocess.run(cmd, capture_output=True, text=True, check=True).stdout
        finally:
            os.unlink(t.name)
        if args.save:
            with open(args.save, "w", encoding="utf-8", newline="\n") as f:
                f.write(out)
        runs = list(csv.DictReader(io.StringIO(out), delimiter="\t"))
    if not runs:
        sys.exit("no runs")
    for r in runs:
        row, names, _ = cars[int(r["car"])]
        r["class"], r["label"] = car_class(row), names[0] + (f" +{len(names) - 1}" if len(names) > 1 else "")
    speeds = sorted({r["kmh"] for r in runs}, key=float)
    print(f"{len(runs)} runs: {len({r['car'] for r in runs})} cars, {len({r['line'] for r in runs})} lines, "
          f"{', '.join(speeds)} km/h")
    print("touch%: runs with a contact; hits/km: touches a km flown; touch t%: the time in contact; clr p05, over95,"
          "\nunder95, lag, pitch, roll, jerk: medians over the runs of each run's number (m, s, deg, m/s3); nose%: runs"
          "\nwhose nose ran into rising ground; wall%: runs a wall stopped")
    table("by class, every line and speed", {c: [r for r in runs if r["class"] == c] for c in CLASSES})
    for kmh in speeds:
        table(f"by class at {kmh} km/h", {c: [r for r in runs if r["class"] == c and r["kmh"] == kmh] for c in CLASSES})
    tags = sorted({t for r in runs for t in r["tags"].split(",") if t})
    table("by kind of ground, every car and speed (a line has several)",
          {t: [r for r in runs if t in r["tags"].split(",")] for t in tags})
    print(f"\nthe cars in contact longest, by class (share of their flying time, every line and speed)")
    for c in CLASSES:
        by_car = {}
        for r in runs:
            if r["class"] == c:
                t, s = by_car.get(r["label"], (0.0, 0.0))
                by_car[r["label"]] = (t + number(r["contact_time"]),
                                      s + number(r["flown"]) / max(number(r["speed"]), 0.1))
        worst = sorted(by_car.items(), key=lambda kv: -kv[1][0] / max(kv[1][1], 1e-3))[:args.worst]
        if worst:
            print(f"  {c}: " + "; ".join(f"{name} {100 * t / max(s, 1e-3):.0f}%" for name, (t, s) in worst))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--vehicles", default=os.path.join(HERE, "vehicles.tsv"))
    ap.add_argument("--flightsim", default=os.path.join(REPO, "build", "flightsim.exe"))
    ap.add_argument("--metric", action="append", default=[], help="scenario.metric to report (default: all)")
    ap.add_argument("--worst", type=int, default=5, help="cars to name at each end")
    ap.add_argument("--csv", default="", help="write every distinct car's metrics here")
    ap.add_argument("--assist", action="store_true", help="fly Standard (flightsim --assist; Standard is the default)")
    ap.add_argument("--mode", default="", help="the mode flown (flightsim --mode)")
    ap.add_argument("--shape-drag", action="store_true", help="drag from each car's shape (flightsim --shape-drag)")
    ap.add_argument("--set", action="append", default=[], help="name=value, passed to flightsim --set")
    ap.add_argument("--terrain", nargs="?", const=os.path.join(HERE, "terrain_runs.tsv"), default="",
                    help="fly the lines of a runs file over real ground instead of the scenarios")
    ap.add_argument("--only", default="", help="--terrain: the lines whose name or tags hold this")
    ap.add_argument("--speeds", default="40,80,130", help="--terrain: km/h")
    ap.add_argument("--hover", type=float, default=0.0,
                    help="--terrain: the hover height, m; 0: each car's from flight on at rest (TerrainRuns.hpp)")
    ap.add_argument("--save", default="", help="--terrain: keep the runs here")
    ap.add_argument("--from", dest="from_", default="", help="--terrain: sum up runs --save kept, without flying")
    ap.add_argument("scenarios", nargs="*")
    args = ap.parse_args()
    if args.terrain or args.from_:
        return terrain(args)

    cars = distinct(args.vehicles)
    print(f"{sum(n for _, _, n in cars)} records, {len(cars)} distinct cars")
    # flightsim names a car by its table name: give each distinct car a unique one
    with tempfile.NamedTemporaryFile("w", suffix=".tsv", delete=False, encoding="utf-8", newline="\n") as t:
        with open(args.vehicles, encoding="utf-8") as f:
            header = f.readline().rstrip("\n").split("\t")
        t.write("\t".join(header) + "\n")
        for i, (row, names, _) in enumerate(cars):
            r = dict(row, name=f"{i}")
            t.write("\t".join(r[c] for c in header) + "\n")
    try:
        extra = ((["--assist"] if args.assist else []) + (["--mode", args.mode] if args.mode else [])
                 + (["--shape-drag"] if args.shape_drag else [])
                 + [a for kv in args.set for a in ("--set", kv)])
        out = subprocess.run([args.flightsim, "--vehicles", t.name, *extra, *args.scenarios], capture_output=True, text=True,
                             check=True).stdout
    finally:
        os.unlink(t.name)
    table = list(csv.DictReader(io.StringIO(out), delimiter="\t"))
    metrics = [m for m in table[0] if m != "car"] if table else []
    if args.metric:
        metrics = [m for m in metrics if m in args.metric]

    def label(i):
        row, names, n = cars[int(table[i]["car"])]
        more = f" +{len(names) - 1} more" if len(names) > 1 else ""
        return f"{names[0]}{more} ({row['type']}, {float(row['mass']):.0f} kg)"

    for m in metrics:
        values = [float(r[m]) for r in table]
        order = sorted(range(len(values)), key=lambda i: values[i])
        print(f"\n{m}: min {min(values):.2f}, median {statistics.median(values):.2f}, max {max(values):.2f}")
        for i in order[:args.worst]:
            print(f"   low  {values[i]:9.2f}  {label(i)}")
        for i in order[-args.worst:][::-1]:
            print(f"   high {values[i]:9.2f}  {label(i)}")

    if args.csv:
        with open(args.csv, "w", encoding="utf-8", newline="") as f:
            w = csv.writer(f)
            w.writerow(["names", "records", *PHYSICS, *metrics])
            for r in table:
                row, names, n = cars[int(r["car"])]
                w.writerow(["; ".join(names), n, *(row[c] for c in PHYSICS), *(r[m] for m in metrics)])


if __name__ == "__main__":
    sys.exit(main())
