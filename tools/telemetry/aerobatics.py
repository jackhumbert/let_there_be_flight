"""Summarise and plot the Autopilot's aerobatics telemetry ([AutopilotA] lines in the LTBF log).

Switch telemetry on (python ../cp2077_tools/bridge.py call LtbfBridge.Telemetry true), fly, then:

    python tools/telemetry/aerobatics.py [let_there_be_flight.log or an extract ...]
    python tools/telemetry/aerobatics.py before=run1.log after=run2.log --plot runs.png

Prints, for cruise and for each manoeuvre flown: duration, attitude error (mean / 95th / max,
degrees), lowest car up.z reached (-1 = fully inverted), height change, and how closely the body
rates followed the target's rates.

--plot draws one column per log (name=path names it) on shared scales: the bank angle about the
flight path (the car's against its target), the roll and yaw rates, and the speed, against time.
"""

import argparse
import math
import os
import re
import sys
from pathlib import Path

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))  # tools/, for common
from common import paths  # noqa: E402

DEFAULT_LOG = Path(paths.LTBF_LOG)
NAMES = {0: "cruise", 1: "aileron roll", 2: "barrel roll", 3: "loop", 4: "Immelmann", 5: "split-S"}


def parse(path):
    rows = []
    for line in Path(path).read_text(errors="replace").splitlines():
        m = re.search(r"\[AutopilotA\] (.*)$", line)
        if not m:
            continue
        f = m.group(1).split()
        if len(f) < 26:
            continue
        n = [float(x) for x in f[:25]]
        rows.append({
            "t": n[0], "p": n[1:4], "v": n[4:7], "err": n[7], "w": n[8:11], "wref": n[11:14],
            "up_t": n[14:17], "up": n[17:20], "a": n[20:23], "kind": int(n[23]), "mt": n[24],
            "status": " ".join(f[25:]),
        })
    return rows


def segments(rows):
    seg = []
    for r in rows:
        if seg and (r["kind"] != seg[-1]["kind"] or r["t"] < seg[-1]["t"]):
            yield seg
            seg = []
        seg.append(r)
    if seg:
        yield seg


def pct(xs, q):
    xs = sorted(xs)
    return xs[min(len(xs) - 1, int(q * len(xs)))]


def summary(rows):
    print(f"{len(rows)} frames")
    for seg in segments(rows):
        if len(seg) < 3:
            continue
        errs = [r["err"] for r in seg]
        rate_err = [math.dist(r["w"], r["wref"]) for r in seg]
        print(f"{NAMES.get(seg[0]['kind'], seg[0]['kind']):13s} {seg[-1]['t'] - seg[0]['t']:6.1f} s"
              f"  err mean {sum(errs) / len(errs):5.1f} p95 {pct(errs, 0.95):5.1f} max {max(errs):5.1f} deg"
              f"  up.z min {min(r['up'][2] for r in seg):5.2f} (target {min(r['up_t'][2] for r in seg):5.2f})"
              f"  dz {seg[-1]['p'][2] - seg[0]['p'][2]:+6.1f} m"
              f"  rate err mean {sum(rate_err) / len(rate_err):4.2f} rad/s"
              f"  speed {math.hypot(*seg[0]['v']):4.1f}->{math.hypot(*seg[-1]['v']):4.1f}")


def bank(up, v):
    """Bank (degrees, positive towards the right) of an up vector about the flight path v; None
    when slow or flying near vertical, where bank is not defined."""
    speed = math.sqrt(sum(c * c for c in v))
    if speed < 8.0:
        return None
    f = [c / speed for c in v]
    level = [-f[2] * f[0], -f[2] * f[1], 1.0 - f[2] * f[2]]  # world up, made perpendicular to f
    ln = math.sqrt(sum(c * c for c in level))
    if ln < 0.2:
        return None
    level = [c / ln for c in level]
    side = [f[1] * level[2] - f[2] * level[1], f[2] * level[0] - f[0] * level[2], f[0] * level[1] - f[1] * level[0]]
    return math.degrees(math.atan2(sum(a * b for a, b in zip(up, side)), sum(a * b for a, b in zip(up, level))))


def unwrapped(angles):
    """Degrees for plotting: None as a gap, and a gap where the angle wraps past +-180 (otherwise a
    line is drawn across the whole axis)."""
    out, prev = [], None
    for a in angles:
        if a is None:
            out.append(float("nan"))
        elif prev is not None and abs(a - prev) > 180.0:
            out.append(float("nan"))
        else:
            out.append(a)
        prev = a
    return out


def plot(runs, out):
    import matplotlib

    matplotlib.use("Agg")
    import matplotlib.pyplot as plt

    # reference palette (light surface), categorical slots 1 to 3, validated together
    surface, ink, ink2, grid = "#fcfcfb", "#0b0b0b", "#52514e", "#e6e5e1"
    actual, target, third = "#2a78d6", "#eb6834", "#1baf7a"
    plt.rcParams.update({
        "figure.facecolor": surface, "axes.facecolor": surface, "axes.edgecolor": grid,
        "axes.labelcolor": ink2, "xtick.color": ink2, "ytick.color": ink2, "text.color": ink,
        "axes.grid": True, "grid.color": grid, "grid.linewidth": 0.6, "font.size": 9,
        "axes.spines.top": False, "axes.spines.right": False,
    })
    cols = len(runs)
    fig, axes = plt.subplots(3, cols, figsize=(5.2 * cols, 8.2), sharex="col", sharey="row", squeeze=False)
    for c, (name, rows) in enumerate(runs):
        t = [r["t"] - rows[0]["t"] for r in rows]
        b_act = [bank(r["up"], r["v"]) for r in rows]
        b_tgt = [bank(r["up_t"], r["v"]) for r in rows]
        ax = axes[0][c]
        ax.plot(t, unwrapped(b_tgt), color=target, lw=1.4, label="target")
        ax.plot(t, unwrapped(b_act), color=actual, lw=1.4, label="car")
        ax.set_title(name, loc="left", fontsize=10, color=ink)
        ax = axes[1][c]
        ax.plot(t, [r["w"][1] for r in rows], color=actual, lw=1.2, label="roll rate")
        ax.plot(t, [r["w"][2] for r in rows], color=target, lw=1.2, label="yaw rate")
        ax.plot(t, [r["w"][0] for r in rows], color=third, lw=1.2, label="pitch rate")
        ax = axes[2][c]
        ax.plot(t, [math.sqrt(sum(x * x for x in r["v"])) for r in rows], color=actual, lw=1.4)
        axes[2][c].set_xlabel("time (s)")
    axes[0][0].set_ylabel("bank about the flight path (deg)")
    axes[1][0].set_ylabel("body rate (rad/s)")
    axes[2][0].set_ylabel("speed (m/s)")
    axes[0][0].set_ylim(-180, 180)
    axes[0][0].set_yticks([-180, -90, 0, 90, 180])
    axes[1][0].set_ylim(-3, 3)
    for row in range(2):
        axes[row][cols - 1].legend(loc="upper right", frameon=False, fontsize=8)
    fig.suptitle("Autopilot telemetry", x=0.01, ha="left", fontsize=12, color=ink)
    fig.tight_layout()
    fig.savefig(out)
    print(f"wrote {out}")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("logs", nargs="*", help="LTBF logs or extracts; name=path names a run in the plot")
    ap.add_argument("--plot", help="write a PNG with one column per log")
    args = ap.parse_args()
    runs = []
    for item in args.logs or [str(DEFAULT_LOG)]:
        name, _, path = item.rpartition("=") if "=" in item and not Path(item).exists() else ("", "", item)
        rows = parse(path)
        if not rows:
            print(f"{path}: no [AutopilotA] lines")
            continue
        runs.append((name or Path(path).name, rows))
        print(f"== {runs[-1][0]}")
        summary(rows)
    if args.plot and runs:
        plot(runs, args.plot)


if __name__ == "__main__":
    main()
