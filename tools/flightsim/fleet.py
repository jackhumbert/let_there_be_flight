"""Every surveyed vehicle through the flight simulator, summed up (docs/design/hover-fly-feel.md).

    python tools/flightsim/fleet.py [--vehicles tools/flightsim/vehicles.tsv] [--flightsim build/flightsim.exe]
                                    [--metric yaw_hold.yaw_rate_3s]... [--worst 8] [--csv out.csv]
                                    [--mode hoverfly|assist|drone|agdrone|droneassist] [--assist]

--mode is flightsim's: the mode flown, and without scenario names that mode's scenarios (drone_* for the drone modes).

Records with the same physics (most of the 1400 car and bike records are paint and trim variants) are flown once.
For each metric: the spread over the distinct cars, and the cars at either end.
"""

import argparse
import csv
import io
import os
import statistics
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
PHYSICS = ["type", "mass", "airResistance", "brakingFriction", "inertiaX", "inertiaY", "inertiaZ", "tensorX",
           "tensorY", "tensorZ", "wheelX", "wheelY", "wheelZ", "bottom"]


def distinct(path):
    """The table's rows, one per physics signature, each with the names that share it."""
    groups = {}
    with open(path, encoding="utf-8") as f:
        for row in csv.DictReader(f, delimiter="\t"):
            key = tuple(row[c] for c in PHYSICS)
            groups.setdefault(key, []).append(row)
    return [(rows[0], sorted({r["name"] for r in rows}), len(rows)) for rows in groups.values()]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--vehicles", default=os.path.join(HERE, "vehicles.tsv"))
    ap.add_argument("--flightsim", default=os.path.join(REPO, "build", "flightsim.exe"))
    ap.add_argument("--metric", action="append", default=[], help="scenario.metric to report (default: all)")
    ap.add_argument("--worst", type=int, default=5, help="cars to name at each end")
    ap.add_argument("--csv", default="", help="write every distinct car's metrics here")
    ap.add_argument("--assist", action="store_true", help="fly the redesigned controller (flightsim --assist)")
    ap.add_argument("--mode", default="", help="the mode flown (flightsim --mode)")
    ap.add_argument("--shape-drag", action="store_true", help="drag from each car's shape (flightsim --shape-drag)")
    ap.add_argument("--set", action="append", default=[], help="name=value, passed to flightsim --set")
    ap.add_argument("scenarios", nargs="*")
    args = ap.parse_args()

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
