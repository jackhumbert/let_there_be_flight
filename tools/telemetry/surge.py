"""How much an Autopilot flight held the surge, from its telemetry (red4ext/logs/ltbf_autopilot_<time>.csv,
or a folder with a telemetry.path as fly_route.py leaves it). While following a route: seconds following,
the share of that time with the surge at 90% or more (thrust_fwd against the surge factor), the mean
surge while thrusting forward, the share held full by AutopilotSurge (surge_held), the speed against
the trajectory's (median and p95 of flown minus planned), off the line p95, top speed and the largest
drift; and the nose's pitch (median, degrees, down < 0) and the surge while speeding up and while
braking (the plan's rate over 3 m/s2 either way), on level stretches only (the route within 8 degrees
of level: on a dive a level nose reads as pulled up against the path). One row per file, so several runs read as a
table.

    python tools/telemetry/surge.py RUN [RUN ...] [--surge 15]
"""

import argparse
import csv
import os

import numpy as np


def load(path):
    if os.path.isdir(path):
        with open(os.path.join(path, "telemetry.path")) as f:
            path = f.read().strip()
    with open(path) as f:
        return list(csv.DictReader(f))


def summary(rows, surge):
    fol = np.array([r["following"] in ("1", "1.0", "1.000000") or float(r["following"]) > 0.5 for r in rows])
    t = np.array([float(r["t"]) for r in rows])
    if not fol.any():
        return None
    dt = np.diff(t, append=t[-1])
    fwd = np.array([float(r["thrust_fwd"]) for r in rows]) / surge
    held = np.array([float(r.get("surge_held") or 0.0) for r in rows])
    speed = np.array([float(r["speed"]) for r in rows])
    target = np.array([float(r["target"]) for r in rows])
    off = np.array([float(r["off"]) for r in rows])
    drift = np.array([abs(float(r.get("drift") or 0.0)) for r in rows])
    rate = np.array([float(r["rate"]) for r in rows])
    pitch = np.array([float(r["car_pitch"]) for r in rows])
    level = np.array([abs(float(r["plan_pitch"])) < 8.0 for r in rows])
    up, down = fol & level & (rate > 3.0), fol & level & (rate < -3.0)
    w = dt * fol
    total = w.sum()
    push = fol & (fwd > 0.05)
    err = (speed - target)[fol]
    return {
        "following": total,
        "full": (w * (fwd >= 0.9)).sum() / total,
        "mean": float(np.average(fwd[push], weights=dt[push])) if push.any() else 0.0,
        "held": (w * (held > 0.5)).sum() / total,
        "err50": float(np.median(err)),
        "err95": float(np.percentile(np.abs(err), 95)),
        "off95": float(np.percentile(off[fol], 95)),
        "top": float(speed[fol].max()),
        "drift": float(drift[fol].max()),
        "pitchUp": float(np.median(pitch[up])) if up.any() else 0.0,
        "surgeUp": float(np.median(fwd[up])) if up.any() else 0.0,
        "pitchDown": float(np.median(pitch[down])) if down.any() else 0.0,
        "surgeDown": float(np.median(fwd[down])) if down.any() else 0.0,
    }


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("runs", nargs="+")
    ap.add_argument("--surge", type=float, default=15.0, help="m/s2, standardModeSurgeFactor")
    args = ap.parse_args()
    print(f"{'run':28s} {'follow s':>8s} {'surge>=90%':>10s} {'mean':>5s} {'held':>5s} {'v-plan p50':>10s} {'|v-plan| p95':>12s} {'off p95':>7s} {'top':>5s} {'drift':>5s}  {'accel pitch/surge':>17s}  {'brake pitch/surge':>17s}")
    for run in args.runs:
        m = summary(load(run), args.surge)
        name = os.path.basename(os.path.normpath(run))[:28]
        if not m:
            print(f"{name:28s} (never followed)")
            continue
        print(f"{name:28s} {m['following']:8.1f} {m['full'] * 100:9.0f}% {m['mean']:5.2f} {m['held'] * 100:4.0f}% {m['err50']:10.1f} {m['err95']:12.1f} "
              f"{m['off95']:7.2f} {m['top']:5.1f} {m['drift']:5.1f}  {m['pitchUp']:8.1f} / {m['surgeUp']:5.2f}  {m['pitchDown']:8.1f} / {m['surgeDown']:5.2f}")


if __name__ == "__main__":
    main()
