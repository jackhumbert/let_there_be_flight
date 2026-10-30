"""Summarise an Autopilot telemetry file (red4ext/logs/ltbf_autopilot_<time>.csv, written by
FlightModeAutopilot.WriteTelemetry through FlightTelemetry with the mode's `telemetry` on).

How the car's orientation follows the plan, while following a route outside manoeuvres:
plan_cmd is the angle between the route's direction and the nose the controller asks for (the
controller's choices: drift into bends, the lookahead), cmd_car the angle between that and the
car's nose, up_cmd_car the same for the up vectors (bank): both the car's dynamics. Then the
stretches flown steeper than --steep degrees, each with what the plan, the command and the car
were doing there, and the collisions (collision_timer resetting).

--attitude splits the orientation into heading, pitch and bank (degrees) for the plan (the route
segment's direction; a route has no bank), the command (targetForward / targetUp) and the car, and
reports each difference by phase: level and straight, turning (the plan's heading turning faster
than 8 deg/s), climbing or diving (the plan steeper than 10 deg). --plot PNG draws them over time,
with the body tilt and the thrusters' angle off the car's top for logs that have them (drone style).

    python tools/telemetry/autopilot.py LOG.csv [--steep 45] [--worst 10] [--attitude] [--plot out.png]
"""

import argparse
import csv
import math


def heading(x, y):
    return math.degrees(math.atan2(y, x))


def pitch(x, y, z):
    n = math.sqrt(x * x + y * y + z * z)
    return math.degrees(math.asin(max(-1.0, min(1.0, z / n)))) if n > 1e-6 else 0.0


def bank(fx, fy, fz, ux, uy, uz):
    """Roll about the nose, degrees, positive with the right side down."""
    # right = forward x up
    rz = fx * uy - fy * ux
    return math.degrees(math.asin(max(-1.0, min(1.0, -rz))))


def wrap(d):
    return (d + 180.0) % 360.0 - 180.0


def attitude(rows):
    """Per row: plan / command / car heading, pitch, bank, and the plan's heading rate (deg/s)."""
    out = []
    for r in rows:
        a = {"t": r["t"], "following": r["following"] > 0 and r["maneuver"] == 0 and r["plan_cmd"] >= 0}
        a["plan_h"], a["plan_p"] = heading(r["plan_x"], r["plan_y"]), pitch(r["plan_x"], r["plan_y"], r["plan_z"])
        a["cmd_h"], a["cmd_p"] = heading(r["cmd_fwd_x"], r["cmd_fwd_y"]), pitch(r["cmd_fwd_x"], r["cmd_fwd_y"], r["cmd_fwd_z"])
        a["car_h"], a["car_p"] = heading(r["fwd_x"], r["fwd_y"]), pitch(r["fwd_x"], r["fwd_y"], r["fwd_z"])
        a["cmd_b"] = bank(r["cmd_fwd_x"], r["cmd_fwd_y"], r["cmd_fwd_z"], r["cmd_up_x"], r["cmd_up_y"], r["cmd_up_z"])
        a["car_b"] = bank(r["fwd_x"], r["fwd_y"], r["fwd_z"], r["up_x"], r["up_y"], r["up_z"])
        out.append(a)
    # the plan's heading rate, over 0.5 s either side (route segments are short and noisy frame to frame)
    j0 = j1 = 0
    for i, a in enumerate(out):
        while out[j0]["t"] < a["t"] - 0.5:
            j0 += 1
        while j1 + 1 < len(out) and out[j1 + 1]["t"] <= a["t"] + 0.5:
            j1 += 1
        dt = out[j1]["t"] - out[j0]["t"]
        a["plan_rate"] = wrap(out[j1]["plan_h"] - out[j0]["plan_h"]) / dt if dt > 0.1 else 0.0
    return out


def report_attitude(att):
    phases = {
        "level, straight": lambda a: abs(a["plan_rate"]) <= 8.0 and abs(a["plan_p"]) <= 10.0,
        "turning": lambda a: abs(a["plan_rate"]) > 8.0 and abs(a["plan_p"]) <= 10.0,
        "climbing or diving": lambda a: abs(a["plan_p"]) > 10.0,
    }
    rows = [a for a in att if a["following"]]
    print(f"\nattitude, following a route ({len(rows)} rows); median / p90 of the absolute difference, degrees")
    print("                       share | heading plan>cmd cmd>car | pitch plan>cmd cmd>car | bank cmd>car")
    for name, test in phases.items():
        sel = [a for a in rows if test(a)]
        if not sel:
            continue
        def col(f):
            v = [abs(f(a)) for a in sel]
            return f"{pct(v, 0.5):5.1f}/{pct(v, 0.9):5.1f}"
        print(f"{name:>20} {len(sel) / len(rows) * 100:5.0f}% | {col(lambda a: wrap(a['cmd_h'] - a['plan_h']))} "
              f"{col(lambda a: wrap(a['car_h'] - a['cmd_h']))} | {col(lambda a: a['cmd_p'] - a['plan_p'])} "
              f"{col(lambda a: a['car_p'] - a['cmd_p'])} | {col(lambda a: a['car_b'] - a['cmd_b'])}")
    # signed: does the car lag the command (turning less) or lead it?
    turning = [a for a in rows if abs(a["plan_rate"]) > 8.0]
    if turning:
        lag = [wrap(a["car_h"] - a["cmd_h"]) * (1 if a["plan_rate"] > 0 else -1) for a in turning]
        cut = [wrap(a["cmd_h"] - a["plan_h"]) * (1 if a["plan_rate"] > 0 else -1) for a in turning]
        print(f"in turns, signed with the turn (+ = further into it): command vs plan median {pct(cut, 0.5):+.1f}, "
              f"car vs command median {pct(lag, 0.5):+.1f} degrees")


def plot_attitude(att, rows, path):
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    t = [a["t"] for a in att]
    # drone-era logs (2026-10-28) also have the body tilt and the commanded thrust in the car's frame
    thrust = "thrust_up" in rows[0]
    fig, ax = plt.subplots(6 if thrust else 5, 1, figsize=(14, 18 if thrust else 15), sharex=True)
    ax[0].plot(t, [r["speed"] for r in rows], label="speed")
    ax[0].plot(t, [r["target"] for r in rows], label="target", alpha=0.7)
    ax[0].set_ylabel("m/s")
    unwrap = lambda key: _unwrap([a[key] for a in att])
    for key, label in (("plan_h", "plan"), ("cmd_h", "command"), ("car_h", "car")):
        ax[1].plot(t, unwrap(key), label=label, lw=1)
    ax[1].set_ylabel("heading, deg")
    for key, label in (("plan_p", "plan"), ("cmd_p", "command"), ("car_p", "car")):
        ax[2].plot(t, [a[key] for a in att], label=label, lw=1)
    ax[2].set_ylabel("pitch, deg")
    ax[3].plot(t, [a["cmd_b"] for a in att], label="command", lw=1)
    ax[3].plot(t, [a["car_b"] for a in att], label="car", lw=1)
    ax[3].set_ylabel("bank, deg")
    ax[4].plot(t, [r["plan_cmd"] for r in rows], label="plan vs command", lw=1)
    ax[4].plot(t, [r["cmd_car"] for r in rows], label="command vs car (nose)", lw=1)
    ax[4].plot(t, [r["up_cmd_car"] for r in rows], label="command vs car (up)", lw=1)
    ax[4].set_ylabel("angle, deg")
    if thrust:
        ax[5].plot(t, [r["cmd_tilt"] for r in rows], label="body tilt, command", lw=1)
        ax[5].plot(t, [r["car_tilt"] for r in rows], label="body tilt, car", lw=1)
        ax[5].plot(t, [math.degrees(math.atan2(math.hypot(r["thrust_right"], r["thrust_fwd"]), r["thrust_up"])) for r in rows],
                   label="thrust off the car's top", lw=1)
        ax[5].set_ylabel("deg from vertical / top")
    ax[-1].set_xlabel("s")
    for a in ax:
        a.legend(loc="upper right", fontsize=8)
        a.grid(alpha=0.3)
        # shade manoeuvres and time off a route
        for r0, r1 in _spans(t, [not x["following"] for x in att]):
            a.axvspan(r0, r1, color="0.85", lw=0)
    fig.tight_layout()
    fig.savefig(path, dpi=90)
    print(f"plot -> {path}")


def _unwrap(degrees):
    out, offset = [], 0.0
    for i, d in enumerate(degrees):
        if i and d + offset - out[-1] > 180:
            offset -= 360
        elif i and d + offset - out[-1] < -180:
            offset += 360
        out.append(d + offset)
    return out


def _spans(t, flags):
    spans, start = [], None
    for ti, f in zip(t, flags):
        if f and start is None:
            start = ti
        elif not f and start is not None:
            spans.append((start, ti))
            start = None
    if start is not None:
        spans.append((start, t[-1]))
    return spans


def pct(values, q):
    if not values:
        return float("nan")
    s = sorted(values)
    return s[min(len(s) - 1, int(q * len(s)))]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("log")
    ap.add_argument("--steep", type=float, default=45.0)
    ap.add_argument("--worst", type=int, default=10)
    ap.add_argument("--window", type=float, default=0.5, help="s over which the flown climb angle is taken")
    ap.add_argument("--attitude", action="store_true", help="heading, pitch and bank: plan vs command vs car, by phase")
    ap.add_argument("--plot", help="draw speed, heading, pitch, bank and the angles over time into this PNG")
    args = ap.parse_args()
    rows = [{k: float(v) for k, v in r.items()} for r in csv.DictReader(open(args.log))]
    if not rows:
        raise SystemExit("no rows")
    span = rows[-1]["t"] - rows[0]["t"]
    print(f"{len(rows)} rows over {span:.0f} s ({len(rows) / max(span, 1e-6):.0f} a second)")
    # the flight as a whole (what the CET route timer and free roam log reported before 2026-10-28)
    flown = sum(math.dist((a["x"], a["y"], a["z"]), (b["x"], b["y"], b["z"])) for a, b in zip(rows, rows[1:])
                if math.dist((a["x"], a["y"], a["z"]), (b["x"], b["y"], b["z"])) < 50.0)  # not a reset's jump
    moving = [r for r in rows if r["following"]]
    print(f"flown {flown:.0f} m, top speed {max(r['speed'] for r in rows):.1f} m/s, mean {flown / max(span, 1e-6):.1f} m/s,"
          f" following a route {len(moving) / len(rows) * 100:.0f}% of the time,"
          f" slowest while following {min((r['speed'] for r in moving), default=0.0):.1f} m/s")

    flying = [r for r in rows if r["following"] and not r["maneuver"] and r["plan_cmd"] >= 0]
    print(f"\nfollowing a route, no manoeuvre: {len(flying)} rows")
    print("            median    p90    p99    max  (degrees)")
    for key in ("plan_cmd", "cmd_car", "up_cmd_car", "attitude_error"):
        v = [r[key] for r in flying if r[key] >= 0]
        print(f"{key:>14} {pct(v, 0.5):6.1f} {pct(v, 0.9):6.1f} {pct(v, 0.99):6.1f} {max(v):6.1f}")

    # flown climb angle over a window, against the plan's and the command's pitch there
    steep, i = [], 0
    for j, r in enumerate(rows):
        while rows[i]["t"] < r["t"] - args.window:
            i += 1
        a = rows[i]
        dh = math.hypot(r["x"] - a["x"], r["y"] - a["y"])
        dz = r["z"] - a["z"]
        if r["t"] - a["t"] > 0.5 * args.window and abs(dz) > 2.0:
            flown = math.degrees(math.atan2(dz, dh))
            if abs(flown) > args.steep:
                steep.append((r, flown))
    print(f"\nflown steeper than {args.steep:.0f} degrees: {len(steep)} rows ({len(steep) / max(len(rows), 1) * 100:.1f}%)")
    # one line per stretch: the steepest row of each run of steep rows less than a second apart
    stretches, last_t = [], None
    for r, flown in steep:
        if last_t is None or r["t"] - last_t > 1.0:
            stretches.append([r, flown, r["t"]])
        elif abs(flown) > abs(stretches[-1][1]):
            stretches[-1][0], stretches[-1][1] = r, flown
        last_t = r["t"]
    print("     t  flown  plan_pitch  cmd_pitch  car_pitch  speed2d  target  off  maneuver  at")
    for r, flown, t0 in stretches[: args.worst * 3]:
        print(f"{t0:6.1f} {flown:6.0f} {r['plan_pitch']:10.0f} {r['cmd_pitch']:10.0f} {r['car_pitch']:10.0f}"
              f" {r['speed2d']:8.1f} {r['target']:7.1f} {r['off']:4.1f} {int(r['maneuver']):9d}  ({r['x']:.0f}, {r['y']:.0f}, {r['z']:.0f})")

    hits = [r for k, r in enumerate(rows[1:], 1) if r["collision_timer"] < rows[k - 1]["collision_timer"] - 0.001]
    print(f"\ncollisions: {len(hits)}")
    for r in hits[: args.worst]:
        print(f"  t {r['t']:.1f} at ({r['x']:.0f}, {r['y']:.0f}, {r['z']:.0f}) speed {r['speed']:.1f} off {r['off']:.1f}")

    worst = sorted(flying, key=lambda r: -r["cmd_car"])[: args.worst]
    print("\nworst nose tracking (cmd_car):")
    for r in worst:
        print(f"  t {r['t']:.1f} cmd_car {r['cmd_car']:.0f} plan_cmd {r['plan_cmd']:.0f} speed {r['speed']:.1f} at ({r['x']:.0f}, {r['y']:.0f}, {r['z']:.0f})")

    if args.attitude or args.plot:
        att = attitude(rows)
        report_attitude(att)
        if args.plot:
            plot_attitude(att, rows, args.plot)


if __name__ == "__main__":
    main()
