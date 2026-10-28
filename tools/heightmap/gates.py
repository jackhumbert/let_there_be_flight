"""Free roam gates from the planner's doorways: openings worth flying through.

The doorways baked into the nav map (doorways.py) are where a car can cross the edge of a 32 m
block. Each one is measured on the map at flight height: how wide the opening is along the block
edge, how high the walls rise on both sides of it, whether something is overhead, and whether the
car can fly straight through it for a stretch either side. Narrow, deep openings between buildings
(and under things) score best; one gate is kept per GATE_SPACING metres. The Autopilot's free
roam picks its legs through these (src/navigation/gates.tsv, FlightModeAutopilot).

    python tools/heightmap/gates.py NAVMAP.bin -o src/navigation/gates.tsv --navplan build/navplan.exe [--count 80] [--preview gates.png]

--navplan flies each candidate with the Autopilot's own planner (build/navplan.exe, cmake --build build
--target navplan) and keeps only those with enough ways in and out no steeper than 45 degrees.

NAVMAP.bin must carry the doorway section (doorways.py), as the bake the plugin embeds does.
"""

import argparse
import csv
import math
import os
import struct
import subprocess
import tempfile
import zlib
from concurrent.futures import ThreadPoolExecutor

import numpy as np

from pads import NavMap

TAG = b"LTBFDOOR"
BLOCK = 16  # columns per block edge (Planner.cpp kBlock)
FLY_ABOVE = 40.0  # m above the opening's floor the gate is flown at: the Race's cruising height
# (FlightModeAutopilot.raceRouteHeight). At 15 m the legs dived 80 degrees into the gates and climbed
# as steeply out of them: a leg's ends are fixed, the planner's 45 degree limit is between them.
FLY_UNDER = 6.0  # m under the opening's ceiling at most
MAX_HALF_WIDTH = 40  # columns either side measured along the edge
MIN_WIDTH = 6.0  # m of the map's free space across (the car's 3 m radius is already taken off)
MAX_WIDTH = 50.0
MIN_WALL = 20.0  # m both walls must rise above the gate
RUN = 80.0  # m the car must be able to fly straight either side of the gate (gateLead + 20)
GATE_SPACING = 150.0  # m between kept gates
# --navplan: the gate flown from and to points WAY_OUT m off in 8 directions with the Race's planner; kept
# with at least MIN_WAYS ways in and MIN_WAYS out no steeper than MAX_PITCH (FlightModeAutopilot.freeRoamMaxPitch).
# Gates deep among tall buildings passed every other test, and their legs came over the roofs and dropped
# into them at 80 degrees (2026-10-28).
GATE_LEAD = 60.0  # FlightModeAutopilot.gateLead
WAY_OUT = 800.0
MAX_PITCH = 45.0
MIN_WAYS = 3
MIN_GROUND = -2.0  # m, the opening's floor (open sea reads -13.5)


def read_doors(path):
    data = open(path, "rb").read()
    if data[-8:] != TAG:
        raise SystemExit(f"{path} has no doorway section (bake it in with doorways.py)")
    packed, raw_size = struct.unpack_from("<QQ", data, len(data) - 24)
    raw = zlib.decompress(data[len(data) - 24 - packed : len(data) - 24])
    assert len(raw) == raw_size
    at = 0
    version, = struct.unpack_from("<I", raw, at)
    at += 4
    assert version == 1, version
    at += 6 * 4  # StepCosts
    at += 4  # map width
    n, = struct.unpack_from("<I", raw, at)
    at += 4
    doors = []
    for _ in range(n):
        cx, cy, gap, links = struct.unpack_from("<iiBH", raw, at)
        at += 11 + links * 8
        doors.append((cx, cy, gap))
    return doors


class Map:
    def __init__(self, path):
        self.nav = NavMap(path)

    def world(self, cx, cy):
        return self.nav.ox + (cx + 0.5) * self.nav.cell, self.nav.oy + (cy + 0.5) * self.nav.cell

    def column(self, cx, cy):
        x, y = self.world(cx, cy)
        return self.nav.gaps(x, y)

    def free(self, cx, cy, z):
        return any(lo <= z <= hi for lo, hi in self.column(cx, cy))

    def free_at(self, x, y, z):
        return any(lo <= z <= hi for lo, hi in self.nav.gaps(x, y))

    def wall_top(self, cx, cy, z):
        """Top of the solid at (cx, cy) that blocks height z: the floor of the next gap above."""
        above = [lo for lo, hi in self.column(cx, cy) if lo > z]
        return min(above) if above else math.inf


def measure(m, cx, cy, gap):
    col = m.column(cx, cy)
    if gap >= len(col):
        return None
    lo, hi = col[gap]
    if lo < MIN_GROUND:
        return None
    z = lo + FLY_ABOVE
    if math.isfinite(hi):
        z = min(z, hi - FLY_UNDER)
    if z < lo + 2.0:
        return None
    lx, ly = cx % BLOCK, cy % BLOCK
    best = None
    # the crossing is over an east/west edge (along x) or a north/south one (along y)
    for axis, on_edge in ((0, lx in (0, BLOCK - 1)), (1, ly in (0, BLOCK - 1))):
        if not on_edge:
            continue
        ex, ey = (0, 1) if axis == 0 else (1, 0)  # along the edge
        ends = []
        for sign in (1, -1):
            k = 1
            while k <= MAX_HALF_WIDTH and m.free(cx + sign * k * ex, cy + sign * k * ey, z):
                k += 1
            ends.append(k)
        if max(ends) > MAX_HALF_WIDTH:
            continue  # open on one side: not an opening
        width = (ends[0] + ends[1] - 1) * m.nav.cell
        walls = [m.wall_top(cx + s * k * ex, cy + s * k * ey, z) - z for s, k in ((1, ends[0]), (-1, ends[1]))]
        # straight through, both ways along the crossing
        x, y = m.world(cx, cy)
        dx, dy = (1.0, 0.0) if axis == 0 else (0.0, 1.0)
        clear = all(m.free_at(x + dx * d, y + dy * d, z) for d in np.arange(-RUN, RUN + 0.1, 2.0))
        if not clear:
            continue
        cand = dict(x=x, y=y, z=z, yaw=0.0 if axis == 0 else 90.0, width=width,
                    wall=min(walls), ceiling=(hi - z) if math.isfinite(hi) else math.inf, floor=lo)
        if best is None or cand["width"] < best["width"]:
            best = cand
    if best is None or not (MIN_WIDTH <= best["width"] <= MAX_WIDTH) or best["wall"] < MIN_WALL:
        return None
    # deep and narrow; something overhead is a bonus
    best["score"] = min(best["wall"], 120.0) / (best["width"] + 10.0) + (1.0 if best["ceiling"] < 40.0 else 0.0)
    return best


def steepest_pitch(points, span=10.0):
    """FlightModeAutopilot.SteepestPitch: steepest climb or descent, each over span m of horizontal travel."""
    steepest, i, j, n = 0.0, 0, 1, len(points)
    while j < n:
        flat = math.hypot(points[j][0] - points[i][0], points[j][1] - points[i][1])
        rise = abs(points[j][2] - points[i][2])
        if flat >= span:
            steepest = max(steepest, math.degrees(math.atan2(rise, flat)))
            i += 1
        else:
            if rise > span:
                steepest = max(steepest, math.degrees(math.atan2(rise, max(flat, 0.01))))
            j += 1
    return steepest


def plan_line(navmap, navplan, start, goal, prefix):
    """The Race's racing line from start to goal (navplan with FlightModeAutopilot's race fields), or None."""
    env = dict(os.environ, NAVPLAN_MIN_PASSAGE="12", NAVPLAN_HEURISTIC="1.2", NAVPLAN_TURN_COST="5")
    r = subprocess.run([navplan, navmap, *(f"{v:.1f}" for v in start), *(f"{v:.1f}" for v in goal), prefix, "40", "0.5"],
                       env=env, capture_output=True, text=True)
    if r.returncode != 0 or not os.path.exists(prefix + "_line.csv"):
        return None
    with open(prefix + "_line.csv") as f:
        return [tuple(float(row[k]) for k in "xyz") for row in csv.DictReader(f)]


def approaches(nav, navmap, navplan, g):
    """(ways in, ways out, directions tried): of points WAY_OUT m off in 8 directions, how many the
    planner reaches the gate from and goes on to with nothing steeper than MAX_PITCH, its straight
    run (GATE_LEAD either side, as the Autopilot flies it) included."""
    yaw = math.radians(g["yaw"])
    ins = outs = tried = 0
    with tempfile.TemporaryDirectory() as tmp:
        for k in range(8):
            a = 2 * math.pi * k / 8
            fx, fy = g["x"] + WAY_OUT * math.cos(a), g["y"] + WAY_OUT * math.sin(a)
            col = nav.gaps(fx, fy)
            if not col or col[-1][0] < MIN_GROUND:
                continue
            far = (fx, fy, col[-1][0] + FLY_ABOVE)
            # through the gate the way that faces away from this point
            sign = 1.0 if math.cos(a) * math.cos(yaw) + math.sin(a) * math.sin(yaw) < 0 else -1.0
            way = (sign * math.cos(yaw), sign * math.sin(yaw))
            entry = (g["x"] - way[0] * GATE_LEAD, g["y"] - way[1] * GATE_LEAD, g["z"])
            run = [(entry[0] + way[0] * d, entry[1] + way[1] * d, g["z"]) for d in np.arange(0.0, 2 * GATE_LEAD + 0.1, 5.0)]
            tried += 1
            line = plan_line(navmap, navplan, far, entry, os.path.join(tmp, f"in{k}"))
            if line and steepest_pitch(line + run[1:]) <= MAX_PITCH:
                ins += 1
            # and out: from the run's end on to the point on the other side
            back = (g["x"] - (fx - g["x"]), g["y"] - (fy - g["y"]))
            col = nav.gaps(*back)
            if col and col[-1][0] >= MIN_GROUND:
                line = plan_line(navmap, navplan, run[-1], (back[0], back[1], col[-1][0] + FLY_ABOVE), os.path.join(tmp, f"out{k}"))
                if line and steepest_pitch(run + line[1:]) <= MAX_PITCH:
                    outs += 1
    return ins, outs, tried


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("navmap")
    ap.add_argument("-o", "--out", required=True)
    ap.add_argument("--count", type=int, default=80)
    ap.add_argument("--navplan", help="build/navplan.exe: keep only gates the planner can fly in and out of within MAX_PITCH")
    ap.add_argument("--jobs", type=int, default=8)
    ap.add_argument("--area", default="-3400,-3600,3600,2600", help="x0,y0,x1,y1 (default: the city)")
    ap.add_argument("--preview")
    args = ap.parse_args()
    x0, y0, x1, y1 = map(float, args.area.split(","))

    m = Map(args.navmap)
    doors = read_doors(args.navmap)
    gates = []
    for i, (cx, cy, gap) in enumerate(doors):
        x, y = m.world(cx, cy)
        if not (x0 <= x <= x1 and y0 <= y <= y1):
            continue
        g = measure(m, cx, cy, gap)
        if g:
            gates.append(g)
    print(f"{len(doors)} doorways, {len(gates)} openings")
    gates.sort(key=lambda g: -g["score"])
    pool = []
    for g in gates:
        if all(math.hypot(g["x"] - k["x"], g["y"] - k["y"]) >= GATE_SPACING for k in pool):
            pool.append(g)
    if args.navplan:
        args.navplan = os.path.abspath(args.navplan)
        # fly each one as the Autopilot would plan it: kept if enough ways in and out stay within MAX_PITCH
        with ThreadPoolExecutor(args.jobs) as ex:
            results = list(ex.map(lambda g: approaches(m.nav, args.navmap, args.navplan, g), pool))
        for g, (ins, outs, tried) in zip(pool, results):
            g["ins"], g["outs"], g["tried"] = ins, outs, tried
        print(f"{len(pool)} candidates flown; {sum(1 for g in pool if g['ins'] >= MIN_WAYS and g['outs'] >= MIN_WAYS)} pass")
        pool = [g for g in pool if g["ins"] >= MIN_WAYS and g["outs"] >= MIN_WAYS]
    kept = pool[: args.count]
    with open(args.out, "w", newline="\n") as f:
        f.write("# Free roam gates: openings to fly through, generated by tools/heightmap/gates.py from the nav map's doorways.\n")
        f.write("# x y z: the gate's centre at flight height; yaw: the way through (degrees, either direction);\n")
        f.write("# width: free space across on the map (m, the car's radius already off); wall: lower wall above the gate (m);\n")
        f.write("# ceiling: free height above the gate (m, inf for open sky)\n")
        f.write("# ways_in, ways_out: of 8 directions, how many the planner flies in and out of within 45 degrees (--navplan)\n")
        f.write("# x\ty\tz\tyaw\twidth\twall\tceiling\tscore\tways_in\tways_out\n")
        for g in kept:
            ceiling = "inf" if math.isinf(g["ceiling"]) else f"{g['ceiling']:.0f}"
            f.write(f"{g['x']:.1f}\t{g['y']:.1f}\t{g['z']:.1f}\t{g['yaw']:.0f}\t{g['width']:.0f}\t{g['wall']:.0f}\t"
                    f"{ceiling}\t{g['score']:.2f}\t{g.get('ins', -1)}\t{g.get('outs', -1)}\n")
    print(f"kept {len(kept)} gates -> {args.out}")
    if args.preview:
        import matplotlib
        matplotlib.use("Agg")
        import matplotlib.pyplot as plt
        fig, ax = plt.subplots(figsize=(10, 10))
        ax.scatter([g["x"] for g in gates], [g["y"] for g in gates], s=2, c="0.75")
        ax.scatter([g["x"] for g in kept], [g["y"] for g in kept], s=30, c=[g["score"] for g in kept], cmap="viridis")
        for i, g in enumerate(kept):
            ax.annotate(str(i), (g["x"], g["y"]), fontsize=7)
        ax.set_aspect("equal")
        fig.savefig(args.preview, dpi=120)


if __name__ == "__main__":
    main()
