"""Fit the car's surge share (Navigation::VehicleSpec::surgeShare) and drag from Autopilot telemetry
(red4ext/logs/ltbf_autopilot_<time>.csv, or folders with a telemetry.path as fly_route.py leaves them).

Frames while following with the surge held full (surge_held, or thrust_fwd at 95% of the surge or
more), in level flight (the velocity within --level degrees of level: climbing, the lift pushes along
the path too), not within --hit-gap s of a hit and over --min-speed: the acceleration along the velocity, with
gravity's share along it taken out, is fitted as a = S - k v^2 (least squares). S over the surge factor
is the share the car gets (the drone's forward tilt included: that is how it flies at full surge), k the
drag (modelled 1.2 x airResistance / mass, 16.0e-4 for the Shion). Top speed is where they meet.

    python tools/telemetry/fit_surge.py RUN [RUN ...] [--surge 15] [--plot out.png]
"""

import argparse
import csv
import os

import numpy as np

G = 9.81


def load(path):
    if os.path.isdir(path):
        with open(os.path.join(path, "telemetry.path")) as f:
            path = f.read().strip()
    with open(path) as f:
        return list(csv.DictReader(f))


def frames(rows, surge, span, min_speed, hit_gap, level):
    """(speed, acceleration less gravity) of the frames with the surge full, from one run."""
    col = lambda k: np.array([float(r.get(k) or 0.0) for r in rows])
    t, v, vz = col("t"), col("speed"), col("vz")
    fol = col("following") > 0.5
    full = (col("surge_held") > 0.5) | (col("thrust_fwd") >= 0.95 * surge)
    hit = np.diff(col("collision_timer"), prepend=0.0) < -0.2
    near_hit = np.zeros(len(t), bool)
    for th in t[hit]:
        near_hit |= np.abs(t - th) < hit_gap
    # dv/dt over +-span s (frame to frame the speed is noisy)
    a = np.full(len(t), np.nan)
    j0 = j1 = 0
    for i in range(len(t)):
        while t[j0] < t[i] - span:
            j0 += 1
        while j1 + 1 < len(t) and t[j1 + 1] <= t[i] + span:
            j1 += 1
        if t[j1] - t[j0] > span:
            a[i] = (v[j1] - v[j0]) / (t[j1] - t[j0])
    # gravity along the velocity pulls back climbing: what the thrust gave is a + g vz / v
    a += G * vz / np.maximum(v, 0.1)
    # the window must be all full surge too
    flat = np.abs(np.degrees(np.arcsin(np.clip(vz / np.maximum(v, 0.1), -1.0, 1.0)))) < level
    keep = fol & full & flat & ~near_hit & (v > min_speed) & np.isfinite(a)
    steady = np.convolve(keep.astype(float), np.ones(9) / 9.0, mode="same") > 0.99
    return v[steady], a[steady]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("runs", nargs="+")
    ap.add_argument("--surge", type=float, default=15.0, help="m/s2, standardModeSurgeFactor")
    ap.add_argument("--span", type=float, default=0.25, help="s either side the acceleration is taken over")
    ap.add_argument("--min-speed", type=float, default=10.0, help="m/s: below, take-off and the lift muddy it")
    ap.add_argument("--level", type=float, default=5.0, help="deg off level the velocity may be")
    ap.add_argument("--hit-gap", type=float, default=2.0, help="s either side of a hit left out")
    ap.add_argument("--plot", help="PNG: acceleration against speed with the fit")
    args = ap.parse_args()
    vs, as_ = [], []
    for run in args.runs:
        v, a = frames(load(run), args.surge, args.span, args.min_speed, args.hit_gap, args.level)
        print(f"{os.path.basename(os.path.normpath(run)):24s} {len(v):5d} frames, {v.min() if len(v) else 0:5.1f} to {v.max() if len(v) else 0:5.1f} m/s")
        vs.append(v)
        as_.append(a)
    v, a = np.concatenate(vs), np.concatenate(as_)
    if len(v) < 50:
        raise SystemExit("too few full-surge frames to fit")
    X = np.column_stack([np.ones_like(v), -v * v])
    (S, k), *_ = np.linalg.lstsq(X, a, rcond=None)
    resid = a - X @ np.array([S, k])
    top = np.sqrt(S / k) if k > 0 else float("inf")
    # with the drag held at the model's, the surge alone
    S_model_drag = float(np.mean(a + 16.0e-4 * v * v))
    print(f"{len(v)} frames: surge {S:.2f} m/s2 (share {S / args.surge:.3f}), drag {k * 1e4:.2f}e-4 v^2, top {top:.1f} m/s, "
          f"residual sd {resid.std():.2f} m/s2")
    print(f"with the drag at the model's 16.0e-4: surge {S_model_drag:.2f} m/s2 (share {S_model_drag / args.surge:.3f}), "
          f"top {np.sqrt(S_model_drag / 16.0e-4):.1f} m/s")
    for lo in range(10, 100, 20):
        m = (v >= lo) & (v < lo + 20)
        if m.sum():
            print(f"  {lo:3d}-{lo + 20:3d} m/s: {m.sum():5d} frames, a {np.median(a[m]):5.2f}, fit {S - k * np.median(v[m]) ** 2:5.2f}, "
                  f"surge with the model's drag {np.median(a[m] + 16.0e-4 * v[m] ** 2):5.2f}")
    if args.plot:
        import matplotlib
        matplotlib.use("Agg")
        import matplotlib.pyplot as plt
        fig, ax = plt.subplots(figsize=(8, 5))
        ax.scatter(v, a, s=2, alpha=0.3, label="frames at full surge")
        x = np.linspace(0, max(v.max(), top), 100)
        ax.plot(x, S - k * x * x, "r", label=f"fit: {S:.2f} - {k * 1e4:.2f}e-4 v^2")
        ax.set_xlabel("speed m/s")
        ax.set_ylabel("acceleration less gravity m/s2")
        ax.legend()
        fig.savefig(args.plot, dpi=110)


if __name__ == "__main__":
    main()
