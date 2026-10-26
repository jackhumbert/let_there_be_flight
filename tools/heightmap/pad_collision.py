"""Which landing pads have no collision under their deck (visual only, e.g. a lone building proxy)?

The nav map bakes building proxies as solid, so it cannot tell. This reads the cooked collision
alone: sectors.py output (<sector>.npz world triangles) for the sectors around each pad, then
casts vertical rays over the pad's disc (13 columns of 2 m within 4 m, as pads.py) and reports,
per column, the vehicle-blocking surface nearest the deck height.

  python pad_collision.py sectors W\\pads\\helipads.json -o W\\padcol\\sectors.txt
      sector names (levels 0 to 2, the pad's cell and its neighbours) for sectors.py --match
  python sectors.py --sectors <raw ep1 _compiled\\default> --geometry W\\geometry.npz
      -o W\\padcol\\tris --match "<regex printed by the step above>"
  python pad_collision.py check W\\pads\\helipads.json --tris W\\padcol\\tris
      --presets W\\presets_table.json -o W\\padcol\\report.json

A pad passes when at least 10 of its 13 columns hit vehicle-blocking collision within 1.5 m of
the deck. Others are listed with what the rays did hit (nothing, or a surface lower down).
"""
import argparse
import glob
import json
import math
import os
import re

import numpy as np

LEVELS = (0, 1, 2)
DECK_TOL = 1.5      # m: a hit this close to the deck counts as the deck
PASS_COLUMNS = 10   # of 13
DISC = [(dx, dy) for dx in (-4, -2, 0, 2, 4) for dy in (-4, -2, 0, 2, 4) if dx * dx + dy * dy <= 16]


def load_pads(path):
    d = json.load(open(path, encoding="utf-8"))
    pads = [dict(p, status="accepted") for p in d["pads"]]
    pads += [dict(p, status="rejected") for p in d.get("rejected", [])]
    return pads


def sector_names(x, y, z):
    names = set()
    for level in LEVELS:
        size = 64.0 * (1 << level)
        cx, cy, cz = (int(round(v / size)) for v in (x, y, z))
        for dx in (-1, 0, 1):
            for dy in (-1, 0, 1):
                for dz in (-1, 0, 1):
                    names.add(f"exterior_{cx + dx}_{cy + dy}_{cz + dz}_{level}")
    return names


def cmd_sectors(args):
    names = set()
    for p in load_pads(args.helipads):
        names |= sector_names(*p["pos"])
    os.makedirs(os.path.dirname(os.path.abspath(args.out)), exist_ok=True)
    open(args.out, "w", newline="\n").write("\n".join(sorted(names)) + "\n")
    regex = "^(" + "|".join(re.escape(n) for n in sorted(names)) + r")\.streamingsector$"
    open(args.out + ".regex", "w").write(regex)
    print(f"{len(names)} sectors; regex in {args.out}.regex")


def blocking_presets(path):
    table = json.load(open(path, encoding="utf-8"))
    out = set()
    for row in table:
        if row.get("blocks_vehicles"):
            out.add(row["name"])
            out.add(str(row["hash"]))
    return out


def load_tris(tris_dir, names, blocking):
    parts = []
    for n in names:
        f = os.path.join(tris_dir, n + ".npz")
        if not os.path.exists(f):
            continue
        d = np.load(f)
        keep = np.array([str(s) in blocking for s in d["preset_names"]])
        mask = keep[d["preset"]]
        if mask.any():
            parts.append(d["tris"][mask])
    return np.concatenate(parts) if parts else np.zeros((0, 3, 3), np.float32)


def column_hits(tris, x, y):
    """z of every triangle under the vertical line through (x, y)."""
    if not len(tris):
        return np.zeros(0)
    a, b, c = tris[:, 0], tris[:, 1], tris[:, 2]
    v0 = b[:, :2] - a[:, :2]
    v1 = c[:, :2] - a[:, :2]
    v2 = np.array([x, y]) - a[:, :2]
    den = v0[:, 0] * v1[:, 1] - v1[:, 0] * v0[:, 1]
    ok = np.abs(den) > 1e-9
    den = np.where(ok, den, 1.0)
    u = (v2[:, 0] * v1[:, 1] - v1[:, 0] * v2[:, 1]) / den
    v = (v0[:, 0] * v2[:, 1] - v2[:, 0] * v0[:, 1]) / den
    inside = ok & (u >= -1e-4) & (v >= -1e-4) & (u + v <= 1 + 1e-4)
    z = a[:, 2] + u * (b[:, 2] - a[:, 2]) + v * (c[:, 2] - a[:, 2])
    return z[inside]


def cmd_check(args):
    blocking = blocking_presets(args.presets)
    report = []
    for p in load_pads(args.helipads):
        x, y, deck = p["pos"]
        tris = load_tris(args.tris, sector_names(x, y, deck), blocking)
        if len(tris):  # only triangles near the pad
            lo = tris.min(axis=1)
            hi = tris.max(axis=1)
            near = (hi[:, 0] >= x - 6) & (lo[:, 0] <= x + 6) & (hi[:, 1] >= y - 6) & (lo[:, 1] <= y + 6)
            tris = tris[near]
        on_deck, below, none = 0, [], 0
        for dx, dy in DISC:
            zs = column_hits(tris, x + dx, y + dy)
            if np.any(np.abs(zs - deck) <= args.tol):
                on_deck += 1
                continue
            under = zs[zs < deck - args.tol]
            if len(under):
                below.append(float(deck - under.max()))
            else:
                none += 1
        report.append({
            "id": p["id"], "status": p["status"], "pos": [round(v, 1) for v in p["pos"]],
            "sources": p.get("sources"), "on_deck": on_deck, "columns": len(DISC),
            "drop_below": round(min(below), 1) if below else None, "no_hit": none,
            "ok": on_deck >= PASS_COLUMNS,
        })
    bad = [r for r in report if not r["ok"]]
    json.dump(report, open(args.out, "w"), indent=1)
    print(f"{len(report)} pads, {len(bad)} without deck collision")
    for r in bad:
        print(f"  {r['status']:8} {r['id']:24} {r['pos']} on_deck {r['on_deck']}/{r['columns']} "
              f"drop {r['drop_below']} no_hit {r['no_hit']} {r['sources']}")


def main():
    ap = argparse.ArgumentParser()
    sub = ap.add_subparsers(dest="cmd", required=True)
    s = sub.add_parser("sectors")
    s.add_argument("helipads")
    s.add_argument("-o", "--out", required=True)
    c = sub.add_parser("check")
    c.add_argument("helipads")
    c.add_argument("--tris", required=True)
    c.add_argument("--presets", required=True)
    c.add_argument("-o", "--out", required=True)
    c.add_argument("--tol", type=float, default=DECK_TOL, help="m from the deck that counts as the deck")
    args = ap.parse_args()
    {"sectors": cmd_sectors, "check": cmd_check}[args.cmd](args)


if __name__ == "__main__":
    main()
