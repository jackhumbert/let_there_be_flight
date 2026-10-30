"""Isolate a landing pad prefab's own parts by intersecting several of its placements
(docs/archive/helipads.md, step 8).

pad_donor.py lists everything in one pad's footprint, which includes the building around it.
Instances of the same prefab share their parts at the same local transforms while their
surroundings differ, so a part (mesh / decal / light / collision actor, local position and yaw
in the prefab's frame) present in at least --min of the given donor files is the prefab's.

  python pad_kit.py donor_a.json donor_b.json donor_c.json [--min 3] [--pos-tol 0.05]
      [--yaw-tol 1.0] -o kit.json

kit.json: {"parts": [...]} with one entry per part (the first donor's row plus "seen": count).
"""
import argparse
import json


def part_key(r):
    if r["type"] == "collisionActor":
        kinds = tuple(sorted(json.dumps([s.get("key"), s.get("preset")], sort_keys=True) for s in r["shapes"]))
        return ("actor", kinds)
    mesh = r.get("mesh") or (r.get("inner") or {}).get("mesh") or (r.get("inner") or {}).get("decal") \
        or r.get("resource") or r.get("debugName")
    return (r["type"], str(mesh))


def same_place(a, b, pos_tol, yaw_tol):
    if any(abs(x - y) > pos_tol for x, y in zip(a["local"], b["local"])):
        return False
    dy = abs((a["yaw"] - b["yaw"] + 180.0) % 360.0 - 180.0)
    return dy <= yaw_tol


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("donors", nargs="+")
    ap.add_argument("--min", type=int, default=None, help="donors a part must appear in (default: all)")
    ap.add_argument("--pos-tol", type=float, default=0.05)
    ap.add_argument("--yaw-tol", type=float, default=1.0)
    ap.add_argument("-o", "--out", required=True)
    args = ap.parse_args()
    donors = [json.load(open(f))["rows"] for f in args.donors]
    need = args.min or len(donors)
    parts = []
    for r in donors[0]:
        key = part_key(r)
        seen = 1
        for other in donors[1:]:
            if any(part_key(o) == key and same_place(r, o, args.pos_tol, args.yaw_tol) for o in other):
                seen += 1
        if seen >= need:
            parts.append(dict(r, seen=seen))
    json.dump({"donors": args.donors, "parts": parts}, open(args.out, "w"), indent=1)
    from collections import Counter
    print(len(parts), "parts:", dict(Counter(p["type"] for p in parts)))


if __name__ == "__main__":
    main()
