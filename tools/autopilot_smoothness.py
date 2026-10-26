"""Smoothness numbers for an Autopilot flight from its per-frame telemetry.

Switch telemetry on in the game (CET), fly, then run this on the LTBF log:

    python tools/autopilot_smoothness.py [--log <let_there_be_flight.log>] [--run -1] [--csv out.csv]

Each telemetry line ("[AutopilotT] t x y z vx vy vz yawRate targetSpeed ax ay az yaw status")
is one frame. Runs are split where the log's wall clock has a gap of more than 2 s. Only frames
following the route above 5 m/s count. Acceleration and jerk come from finite differences of the
logged velocity (game frames are ~0.03 to 0.05 s), split into longitudinal (along the horizontal
velocity) and lateral parts.
"""
import argparse
import math
import re
from datetime import datetime

DEFAULT_LOG = r"D:\Games\SteamLibrary\steamapps\common\Cyberpunk 2077\red4ext\logs\let_there_be_flight.log"
LINE = re.compile(r"^\[(\d{4}-\d\d-\d\d \d\d:\d\d:\d\d\.\d+)\] \[info\] \[AutopilotT\] (.*)$")


def runs(path):
    out, cur, last = [], [], None
    with open(path, encoding="utf-8", errors="replace") as f:
        for line in f:
            m = LINE.match(line.strip())
            if not m:
                continue
            wall = datetime.strptime(m.group(1), "%Y-%m-%d %H:%M:%S.%f")
            parts = m.group(2).split()
            # t x y z vx vy vz yawRate target ax ay az yaw [bankTarget roll [speedCmd offRoute
            # blocked emergency [lookX lookY lookZ ahead]]] status; row: the first 13, status,
            # then the rest from index 14
            floats = [float(x) for x in parts[:-1]]
            if len(floats) < 23:
                floats += [0.0] * (23 - len(floats))
            row = floats[:13] + [parts[-1]] + floats[13:23]
            if last is not None and (wall - last).total_seconds() > 2.0 and cur:
                out.append(cur)
                cur = []
            cur.append(row)
            last = wall
    if cur:
        out.append(cur)
    return out


def pct(values, p):
    if not values:
        return float("nan")
    s = sorted(values)
    return s[min(len(s) - 1, int(p / 100.0 * len(s)))]


def rms(values):
    return math.sqrt(sum(v * v for v in values) / len(values)) if values else float("nan")


H = 0.15  # s, half width of the derivative window: frame-to-frame differences are mostly noise


def sample(rows, t, col):
    """Linear interpolation of column col at time t (rows sorted by time)."""
    lo, hi = 0, len(rows) - 1
    while hi - lo > 1:
        mid = (lo + hi) // 2
        if rows[mid][0] <= t:
            lo = mid
        else:
            hi = mid
    a, b = rows[lo], rows[hi]
    if b[0] - a[0] <= 1e-6:
        return a[col]
    f = min(max((t - a[0]) / (b[0] - a[0]), 0.0), 1.0)
    return a[col] + (b[col] - a[col]) * f


def analyse(rows):
    # centred differences over +-H, on a fixed 0.05 s grid
    t0, t1 = rows[0][0] + 2 * H, rows[-1][0] - 2 * H
    status = {}
    for r in rows:
        status[r[0]] = r[13]
    times = [r[0] for r in rows]

    def status_at(t):
        i = min(range(len(times)), key=lambda k: abs(times[k] - t)) if len(times) < 50 else None
        if i is None:
            lo, hi = 0, len(times) - 1
            while hi - lo > 1:
                mid = (lo + hi) // 2
                if times[mid] <= t:
                    lo = mid
                else:
                    hi = mid
            i = lo
        return rows[i][13]

    def d(col, t):
        return (sample(rows, t + H, col) - sample(rows, t - H, col)) / (2 * H)

    acc = []  # (t, along, lateral, speed, following, yawrate, target)
    t = t0
    while t <= t1:
        vx, vy = sample(rows, t, 4), sample(rows, t, 5)
        sp = math.hypot(vx, vy)
        ax, ay = d(4, t), d(5, t)
        if sp > 0.1:
            ux, uy = vx / sp, vy / sp
            along, lat = ax * ux + ay * uy, -ax * uy + ay * ux
        else:
            along, lat = 0.0, 0.0
        ok = status_at(t) == "following" and sp > 5.0
        acc.append((t, along, lat, sp, ok, sample(rows, t, 7), sample(rows, t, 8), sample(rows, t, 15), d(15, t), sample(rows, t, 14)))
        t += 0.05
    step = max(1, int(round(H / 0.05)))
    jl, jt, ya, al, at, ts = [], [], [], [], [], []
    for i in range(step, len(acc) - step):
        a, b, c = acc[i - step], acc[i], acc[i + step]
        if not (a[4] and b[4] and c[4]):
            continue
        dt = c[0] - a[0]
        jl.append((c[1] - a[1]) / dt)
        jt.append((c[2] - a[2]) / dt)
        ya.append((c[5] - a[5]) / dt)
        al.append(b[1])
        at.append(b[2])
        ts.append((c[6] - a[6]) / dt)
    # roll: rate, and how often it reverses direction by more than 2 deg (rocking)
    rr = [x[8] for x in acc if x[4]]
    reversals, last_extreme, direction = 0, None, 0
    for x in acc:
        if not x[4]:
            continue
        roll = x[7]
        if last_extreme is None:
            last_extreme = roll
            continue
        if direction >= 0 and roll < last_extreme - 2.0:
            reversals += direction > 0
            direction, last_extreme = -1, roll
        elif direction <= 0 and roll > last_extreme + 2.0:
            reversals += direction < 0
            direction, last_extreme = 1, roll
        elif (direction > 0 and roll > last_extreme) or (direction < 0 and roll < last_extreme):
            last_extreme = roll
    # yaw wobble: reversals of the yaw rate by more than 0.05 rad/s
    yaw_rev, ylast, ydir = 0, None, 0
    for x in acc:
        if not x[4]:
            continue
        y = x[5]
        if ylast is None:
            ylast = y
            continue
        if ydir >= 0 and y < ylast - 0.05:
            yaw_rev += ydir > 0
            ydir, ylast = -1, y
        elif ydir <= 0 and y > ylast + 0.05:
            yaw_rev += ydir < 0
            ydir, ylast = 1, y
        elif (ydir > 0 and y > ylast) or (ydir < 0 and y < ylast):
            ylast = y
    frames = sum(1 for x in acc if x[4])
    dur = rows[-1][0] - rows[0][0]
    return {
        "duration s": dur,
        "frames following": frames,
        "accel along p95 |m/s2|": pct([abs(x) for x in al], 95),
        "accel lateral p95 |m/s2|": pct([abs(x) for x in at], 95),
        "jerk along rms m/s3": rms(jl),
        "jerk along p95 |m/s3|": pct([abs(x) for x in jl], 95),
        "jerk lateral rms m/s3": rms(jt),
        "jerk lateral p95 |m/s3|": pct([abs(x) for x in jt], 95),
        "yaw accel rms rad/s2": rms(ya),
        "yaw accel p95 |rad/s2|": pct([abs(x) for x in ya], 95),
        "target speed change p95 |m/s2|": pct([abs(x) for x in ts], 95),
        "yaw reversals > 0.05 rad/s per min": yaw_rev / max(frames * 0.05 / 60.0, 1e-6),
        "roll rate rms deg/s": rms(rr),
        "roll rate p95 |deg/s|": pct([abs(x) for x in rr], 95),
        "roll reversals > 2 deg per min": reversals / max(frames * 0.05 / 60.0, 1e-6),
    }, acc


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--log", default=DEFAULT_LOG)
    ap.add_argument("--run", type=int, default=-1, help="which run (default: the last)")
    ap.add_argument("--csv", help="write the per-frame derived values here")
    args = ap.parse_args()
    all_runs = runs(args.log)
    if not all_runs:
        raise SystemExit("no [AutopilotT] lines in the log")
    print(f"{len(all_runs)} run(s); using {args.run}")
    stats, acc = analyse(all_runs[args.run])
    for k, v in stats.items():
        print(f"  {k:32s} {v:9.2f}" if isinstance(v, float) else f"  {k:32s} {v:9d}")
    if args.csv:
        with open(args.csv, "w") as f:
            f.write("t,along,lateral,speed,following,yawrate,target,roll,rollrate,bank\n")
            for row in acc:
                f.write(",".join(str(round(x, 4)) if isinstance(x, float) else str(int(x)) for x in row) + "\n")


if __name__ == "__main__":
    main()
