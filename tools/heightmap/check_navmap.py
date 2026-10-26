"""Check the plugin's NavMap (FlightNavMap natives, through the CET bridge) against the bake.

Queries GetGaps for random columns in the running game and compares them with the reachable
gaps in spans/free.npz, then times the calls. Needs the game running with navmap.bin
installed.

Usage: python check_navmap.py SPANS_DIR [-n 2000] [--bounds xmin,ymin,xmax,ymax]
"""

import argparse
import json
import os
import subprocess
import sys
import tempfile

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
CET = os.path.join(HERE, "..", "cet.py")

LUA = r"""
local pts = {%s}
local out = {}
local t0 = os.clock()
for _, p in ipairs(pts) do
  local gaps = FlightNavMap.GetGaps(Vector4.new(p[1], p[2], 0, 1))
  local s = {}
  for _, g in ipairs(gaps) do table.insert(s, string.format("[%%.2f,%%.2f]", g.X, g.Y)) end
  table.insert(out, "[" .. table.concat(s, ",") .. "]")
end
local t1 = os.clock()
return "{\"gaps\":[" .. table.concat(out, ",") .. "],\"seconds\":" .. string.format("%%.4f", t1 - t0) .. "}"
"""


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("spans")
    ap.add_argument("-n", type=int, default=2000)
    ap.add_argument("--bounds", default="-3300,-3300,1500,1300")
    ap.add_argument("--seed", type=int, default=0)
    args = ap.parse_args()

    cfg = json.load(open(os.path.join(args.spans, "config.json")))
    x0, y0, cell, z0, vz = cfg["origin"][0], cfg["origin"][1], cfg["cell"], cfg["z0"], cfg["vz"]
    nz = int(round((cfg["z1"] - z0) / vz))
    d = np.load(os.path.join(args.spans, "free.npz"))
    counts, runs, reach = d["counts"], d["runs"], d["reachable"]
    offsets = np.concatenate([[0], np.cumsum(counts.ravel(), dtype=np.int64)])
    w = counts.shape[1]

    bx0, by0, bx1, by1 = map(float, args.bounds.split(","))
    rng = np.random.default_rng(args.seed)
    pts = np.stack([rng.uniform(bx0, bx1, args.n), rng.uniform(by0, by1, args.n)], 1)

    lua = LUA % ",".join(f"{{{x:.3f},{y:.3f}}}" for x, y in pts)
    with tempfile.NamedTemporaryFile("w", suffix=".lua", delete=False) as f:
        f.write(lua)
        path = f.name
    try:
        r = subprocess.run([sys.executable, CET, "run", "-f", path], capture_output=True, text=True, timeout=600)
    finally:
        os.remove(path)
    if r.returncode != 0:
        sys.exit(r.stdout + r.stderr)
    got = json.loads(r.stdout)

    bad = 0
    for (x, y), g in zip(pts, got["gaps"]):
        i = int((y - y0) // cell) * w + int((x - x0) // cell)
        rr = runs[offsets[i] : offsets[i + 1]][reach[offsets[i] : offsets[i + 1]]]
        want = [[z0 + a * vz, 1e9 if b >= nz else z0 + b * vz] for a, b in rr]
        if len(want) != len(g) or any(abs(a - c) > 0.01 or abs(b - e) > 0.01 for (a, b), (c, e) in zip(want, g)):
            bad += 1
            if bad <= 5:
                print(f"mismatch at ({x:.1f}, {y:.1f}): game {g} bake {want}")
    print(f"{len(pts)} columns, {bad} mismatches, {1e6 * got['seconds'] / len(pts):.1f} us per GetGaps call from Lua")


if __name__ == "__main__":
    main()
