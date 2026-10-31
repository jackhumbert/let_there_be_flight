"""Compare two features.csv files row by row: flyby.py's (the reference) against the plugin code's
(the acoustics target, tools/acoustics/acoustics.cpp). Per column: the largest and the 99th
percentile difference, and how many rows disagree on whether there is a hit at all (inf).

    python tools/acoustics/compare_features.py REFERENCE.csv OTHER.csv [--tol 1.0]

Exit 1 if a distance's 99th percentile difference passes --tol metres or enclosure's passes 0.02."""

import argparse
import csv
import math
import sys

import numpy as np

DISTANCES = ("floor", "ceiling", "left", "right", "mean_free_path")


def read(path):
    with open(path) as f:
        rows = list(csv.DictReader(f))
    return {k: np.array([float(r[k]) for r in rows]) for k in rows[0]}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("reference")
    ap.add_argument("other")
    ap.add_argument("--tol", type=float, default=1.0)
    args = ap.parse_args()
    a, b = read(args.reference), read(args.other)
    n = min(len(a["t"]), len(b["t"]))
    if len(a["t"]) != len(b["t"]):
        print(f"rows: {len(a['t'])} against {len(b['t'])}, comparing the first {n}")
    dt = np.abs(a["t"][:n] - b["t"][:n]).max()
    print(f"t: largest difference {dt:.4f} s")
    ok = dt < 1e-3
    for k in DISTANCES + ("enclosure", "speed"):
        x, y = a[k][:n], b[k][:n]
        fx, fy = np.isfinite(x), np.isfinite(y)
        both = fx & fy
        x, y = np.where(both, x, 0), np.where(both, y, 0)
        d = np.abs(x[both] - y[both])
        p99 = float(np.percentile(d, 99)) if len(d) else 0.0
        top = float(d.max()) if len(d) else 0.0
        worst = int(np.argmax(np.where(both, np.abs(x - y), -1))) if len(d) else 0
        tol = 0.02 if k == "enclosure" else args.tol if k in DISTANCES else math.inf
        flag = "" if p99 <= tol else "  <- over"
        ok &= p99 <= tol
        print(f"{k:15s} p99 {p99:7.3f}  max {top:7.3f} (t {a['t'][worst]:.2f})  hit/miss disagree {int((fx != fy).sum())}{flag}")
    sys.exit(0 if ok else 1)


if __name__ == "__main__":
    main()
