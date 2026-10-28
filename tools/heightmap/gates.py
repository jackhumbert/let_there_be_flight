"""Free roam gates from the planner's doorways: openings worth flying through.

The doorways baked into the nav map (doorways.py) are where a car can cross the edge of a 32 m
block. Each one is measured on the map at flight height: how wide the opening is along the block
edge, how high the walls rise on both sides of it, whether something is overhead, and whether the
car can fly straight through it for a stretch either side. Narrow, deep openings between buildings
(and under things) score best; one gate is kept per GATE_SPACING metres. The Autopilot's free
roam picks its legs through these (src/navigation/gates.tsv, FlightModeAutopilot).

    python tools/heightmap/gates.py NAVMAP.bin -o src/navigation/gates.tsv [--count 80] [--preview gates.png]

NAVMAP.bin must carry the doorway section (doorways.py), as the bake the plugin embeds does.
"""

import argparse
import math
import struct
import zlib

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


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("navmap")
    ap.add_argument("-o", "--out", required=True)
    ap.add_argument("--count", type=int, default=80)
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
    kept = []
    for g in gates:
        if all(math.hypot(g["x"] - k["x"], g["y"] - k["y"]) >= GATE_SPACING for k in kept):
            kept.append(g)
        if len(kept) >= args.count:
            break
    with open(args.out, "w", newline="\n") as f:
        f.write("# Free roam gates: openings to fly through, generated by tools/heightmap/gates.py from the nav map's doorways.\n")
        f.write("# x y z: the gate's centre at flight height; yaw: the way through (degrees, either direction);\n")
        f.write("# width: free space across on the map (m, the car's radius already off); wall: lower wall above the gate (m);\n")
        f.write("# ceiling: free height above the gate (m, inf for open sky)\n")
        f.write("# x\ty\tz\tyaw\twidth\twall\tceiling\tscore\n")
        for g in kept:
            ceiling = "inf" if math.isinf(g["ceiling"]) else f"{g['ceiling']:.0f}"
            f.write(f"{g['x']:.1f}\t{g['y']:.1f}\t{g['z']:.1f}\t{g['yaw']:.0f}\t{g['width']:.0f}\t{g['wall']:.0f}\t"
                    f"{ceiling}\t{g['score']:.2f}\n")
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
