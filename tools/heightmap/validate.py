"""Check the baked span map against the live game's physics (through the CET bridge).

For random columns around a point (default: the player), cast rays straight down in game and
record every surface: after each hit the next ray starts just below it. The game only has
collision streamed in near the player, so keep --radius within a couple of hundred metres and
move the player (--teleport) to test other areas.

Compared per column, against the raw (uninflated) solid runs of solid.npz:
  top     first hit from above vs the top of the highest baked solid run below the ray start,
          once for the centre ray and once for the highest of a k x k ray grid over the cell
          (the bake keeps the highest solid anywhere in the 2 m cell, so the grid is the fair
          comparison). The bake also holds building proxies (visible geometry with no
          collision), which rays cannot see, so "bake above game" is expected near buildings;
          "bake below game" means the bake missed something.
  layers  each hit vs the nearest baked solid-run top (a surface the bake missed shows up as
          a large distance).

Usage:
  python validate.py SPANS_DIR [--center x,y] [--radius 120] [-n 300] [--top 600]
                     [--teleport x,y,z] [--group VehicleBlocker] [--json out.json]
"""

import argparse
import json
import os
import subprocess
import sys
import tempfile
import time

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
CET = os.path.join(HERE, "..", "cet.py")

LUA = r"""
local sqs = Game.GetSpatialQueriesSystem()
local group = CName.new("%(group)s")
local cols = {%(cols)s}
local offs = {%(offs)s}
local out = {}
for i, c in ipairs(cols) do
  local x, y = c[1], c[2]
  local z = %(top)f
  local hits = {}
  for k = 1, 12 do
    local ok, res = sqs:SyncRaycastByCollisionGroup(Vector4.new(x, y, z, 1), Vector4.new(x, y, %(bottom)f, 1), group, false, false)
    if not ok or res == nil or res.position == nil then break end
    local h = res.position.z
    if h >= z then break end
    table.insert(hits, string.format("%%.2f", h))
    z = h - %(step)f
  end
  local best = -9999
  for _, o in ipairs(offs) do
    local ok, res = sqs:SyncRaycastByCollisionGroup(Vector4.new(x + o[1], y + o[2], %(top)f, 1), Vector4.new(x + o[1], y + o[2], %(bottom)f, 1), group, false, false)
    if ok and res ~= nil and res.position ~= nil and res.position.z > best then best = res.position.z end
  end
  table.insert(out, "{\"hits\":[" .. table.concat(hits, ",") .. "],\"cellmax\":" .. string.format("%%.2f", best) .. "}")
end
return "[" .. table.concat(out, ",") .. "]"
"""


def cet(code_or_file, is_file=False, timeout=120):
    args = [sys.executable, CET, "run"]
    args += ["-f", code_or_file] if is_file else [code_or_file]
    r = subprocess.run(args, capture_output=True, text=True, timeout=timeout)
    if r.returncode != 0:
        raise RuntimeError(r.stdout + r.stderr)
    return r.stdout.strip()


def player_xy():
    s = cet("local p = Game.GetPlayer():GetWorldPosition(); return string.format('%.2f,%.2f,%.2f', p.x, p.y, p.z)")
    return tuple(float(v) for v in s.split(","))


def teleport(x, y, z):
    # move the mounted vehicle if there is one, else the player
    cet(
        "local p = Game.GetPlayer(); local v = p:GetMountedVehicle(); local e = v or p; "
        f"Game.GetTeleportationFacility():Teleport(e, Vector4.new({x}, {y}, {z}, 1), EulerAngles.new(0, 0, 0)); return 'ok'"
    )


class Spans:
    def __init__(self, work):
        cfg = json.load(open(os.path.join(work, "config.json")))
        self.x0, self.y0 = cfg["origin"]
        self.cell, self.vz, self.z0 = cfg["cell"], cfg["vz"], cfg["z0"]
        d = np.load(os.path.join(work, "solid.npz"))
        self.counts, self.runs = d["counts"], d["runs"]
        self.offsets = np.concatenate([[0], np.cumsum(self.counts.ravel(), dtype=np.int64)])

    def solid(self, x, y):
        cx = int((x - self.x0) // self.cell)
        cy = int((y - self.y0) // self.cell)
        i = cy * self.counts.shape[1] + cx
        r = self.runs[self.offsets[i] : self.offsets[i + 1]].astype(np.float64)
        return self.z0 + r * self.vz  # (n, 2) [bottom, top) in metres

    def centre(self, x, y):
        cx = (x - self.x0) // self.cell
        cy = (y - self.y0) // self.cell
        return self.x0 + (cx + 0.5) * self.cell, self.y0 + (cy + 0.5) * self.cell


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("spans")
    ap.add_argument("--center")
    ap.add_argument("--radius", type=float, default=120.0)
    ap.add_argument("-n", type=int, default=300)
    ap.add_argument("--top", type=float, default=600.0, help="ray start height, m")
    ap.add_argument("--bottom", type=float, default=-150.0)
    ap.add_argument("--step", type=float, default=0.6, help="restart this far below each hit")
    ap.add_argument("--teleport", help="x,y,z: move there first and wait for streaming")
    ap.add_argument("--wait", type=float, default=20.0)
    ap.add_argument("--group", default="VehicleBlocker")
    ap.add_argument("--seed", type=int, default=0)
    ap.add_argument(
        "--cell-rays",
        type=int,
        default=5,
        help="k: also cast a k x k grid of rays over each cell and compare the highest hit "
        "(the bake keeps the highest solid anywhere in the cell)",
    )
    ap.add_argument("--json")
    args = ap.parse_args()

    spans = Spans(args.spans)
    if args.teleport:
        teleport(*map(float, args.teleport.split(",")))
        time.sleep(args.wait)
    cx, cy = map(float, args.center.split(",")) if args.center else player_xy()[:2]

    rng = np.random.default_rng(args.seed)
    ang = rng.uniform(0, 2 * np.pi, args.n)
    rad = args.radius * np.sqrt(rng.uniform(0, 1, args.n))
    cols = [spans.centre(cx + r * np.cos(a), cy + r * np.sin(a)) for a, r in zip(ang, rad)]

    k = args.cell_rays
    half = 0.45 * spans.cell
    grid = [0.0] if k <= 1 else [-half + 2 * half * i / (k - 1) for i in range(k)]
    lua = LUA % {
        "group": args.group,
        "cols": ",".join(f"{{{x:.2f},{y:.2f}}}" for x, y in cols),
        "top": args.top,
        "bottom": args.bottom,
        "step": args.step,
        "offs": ",".join(f"{{{a:.3f},{b:.3f}}}" for a in grid for b in grid),
    }
    with tempfile.NamedTemporaryFile("w", suffix=".lua", delete=False) as f:
        f.write(lua)
        path = f.name
    try:
        hits = json.loads(cet(path, is_file=True, timeout=300))
    finally:
        os.remove(path)

    top_diff, cell_diff, layer_diff, rows = [], [], [], []
    no_hit = no_bake = 0
    for (x, y), r in zip(cols, hits):
        h, cellmax = r["hits"], r["cellmax"]
        s = spans.solid(x, y)
        s = s[s[:, 0] < args.top]
        if not h:
            no_hit += 1
        if not len(s):
            no_bake += 1
        if h and len(s):
            top = min(s[-1, 1], args.top)
            top_diff.append(top - h[0])
            if cellmax > -9999:
                cell_diff.append(top - cellmax)
            tops = s[:, 1]
            layer_diff += [float(np.min(np.abs(tops - z))) for z in h]
        rows.append({"x": x, "y": y, "hits": h, "cellmax": cellmax, "solid": s.round(2).tolist()})

    ld = np.array(layer_diff)
    print(f"centre ({cx:.0f}, {cy:.0f}) r {args.radius:.0f} m, {len(cols)} columns, group {args.group}")
    print(f"  no ray hit {no_hit}, no baked solid {no_bake}")
    for label, d in (("top, centre ray", np.array(top_diff)), (f"top, {k}x{k} cell max", np.array(cell_diff))):
        if not len(d):
            continue
        q = np.percentile(d, [5, 50, 95])
        print(
            f"  {label} (bake - game): median {q[1]:+.2f} m, 5%..95% {q[0]:+.2f}..{q[2]:+.2f}, "
            f"|d|<1 m {np.mean(np.abs(d) < 1):.1%}, bake >3 m above {np.mean(d > 3):.1%}, "
            f"bake >2 m below {np.mean(d < -2):.1%}"
        )
    if len(ld):
        print(
            f"  layers: {len(ld)} hits, within 1 m of a baked top {np.mean(ld < 1):.1%}, "
            f"within 3 m {np.mean(ld < 3):.1%}"
        )
    if args.json:
        json.dump(rows, open(args.json, "w"), indent=0)


if __name__ == "__main__":
    main()
