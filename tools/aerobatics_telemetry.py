"""Summarise the Autopilot's aerobatics telemetry ([AutopilotA] lines in the LTBF log).

Switch telemetry on from CET (tools/cet/aerobatics.lua with SETUP = true does it), fly, then:

    python tools/aerobatics_telemetry.py [path to let_there_be_flight.log or an extract]

Prints, for cruise and for each manoeuvre flown: duration, attitude error (mean / 95th / max,
degrees), lowest car up.z reached (-1 = fully inverted), height change, and how closely the body
rates followed the target's rates.
"""

import math
import re
import sys
from pathlib import Path

DEFAULT_LOG = Path(r"D:\Games\SteamLibrary\steamapps\common\Cyberpunk 2077\red4ext\logs\let_there_be_flight.log")
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


def main():
    rows = parse(sys.argv[1] if len(sys.argv) > 1 else DEFAULT_LOG)
    if not rows:
        print("no [AutopilotA] lines")
        return
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


if __name__ == "__main__":
    main()
