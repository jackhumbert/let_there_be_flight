"""Summarise an Autopilot telemetry file (red4ext/logs/ltbf_autopilot_<time>.csv, written by
FlightModeAutopilot.WriteTelemetry through FlightTelemetry with the mode's `telemetry` on).

How the car's orientation follows the plan, while following a route outside manoeuvres:
plan_cmd is the angle between the route's direction and the nose the controller asks for (the
controller's choices: drift into bends, the lookahead), cmd_car the angle between that and the
car's nose, up_cmd_car the same for the up vectors (bank): both the car's dynamics. Then the
stretches flown steeper than --steep degrees, each with what the plan, the command and the car
were doing there, and the collisions (collision_timer resetting).

    python tools/autopilot_telemetry.py LOG.csv [--steep 45] [--worst 10]
"""

import argparse
import csv
import math


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
    args = ap.parse_args()
    rows = [{k: float(v) for k, v in r.items()} for r in csv.DictReader(open(args.log))]
    if not rows:
        raise SystemExit("no rows")
    span = rows[-1]["t"] - rows[0]["t"]
    print(f"{len(rows)} rows over {span:.0f} s ({len(rows) / max(span, 1e-6):.0f} a second)")

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


if __name__ == "__main__":
    main()
