"""Raw cooked collision blobs for a pad kit's mesh / convex shapes (docs/helipads.md, step 8).

A kit's collision actors reference their triangle / convex meshes by hash into the world's
geometry cache. An entity collider (physicsColliderMesh / physicsColliderConvex) carries the
same PhysX cooked data inline, so the blobs are copied as they are, no re-cooking.

  python pad_geometry.py kit.json --cache 03_night_city.geometry_cache.json [--cache more.json]
      -o kit_geometry.npz

Output: npz with h<hash> = uint8 blob and kind<hash> = 3 (convex) / 4 (triangle mesh).
See geometry_cache.py for the cache layout.
"""
import argparse
import base64
import json
import struct

import numpy as np


def kit_hashes(kit):
    out = set()
    for p in kit["parts"]:
        if p["type"] != "collisionActor":
            continue
        for s in p["shapes"]:
            key = s.get("key") or {}
            if key.get("hash"):
                out.add(int(key["hash"]))
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("kit")
    ap.add_argument("--cache", action="append", required=True)
    ap.add_argument("-o", "--out", required=True)
    args = ap.parse_args()
    want = kit_hashes(json.load(open(args.kit)))
    found = {}
    for path in args.cache:
        rc = json.load(open(path, encoding="utf-8"))["Data"]["RootChunk"]
        geos = rc["sectorGeometries"]
        ents = rc["sectorCacheEntries"]
        sectors = list(zip(rc["sectorEntries"], rc["bufferTableSectors"]))
        sectors.append((rc["alwaysLoadedSector"], rc["alwaysLoadedSectorDDB"]))
        index = {}
        for si, (sector, _) in enumerate(sectors):
            first = sector["entryOffset"]
            for gi in range(first, first + sector["entrySize"]):
                h = struct.unpack("<Q", bytes(geos[gi]["ta"]["Elements"][:8]))[0]
                if h in want and h not in found and h not in index:
                    index[h] = (si, gi)
        for h, (si, gi) in index.items():
            data = base64.b64decode(sectors[si][1]["Bytes"])
            e = ents[gi]
            found[h] = (np.frombuffer(data[e["entryOffset"]: e["entryOffset"] + e["entrySize"]], np.uint8).copy(),
                        geos[gi]["pe"])
    arrays = {}
    for h, (blob, pe) in found.items():
        arrays[f"h{h}"] = blob
        arrays[f"kind{h}"] = np.array(pe, np.uint8)
    np.savez(args.out, **arrays)
    missing = sorted(want - set(found))
    print(f"{len(found)} of {len(want)} blobs; missing {missing[:10]}")


if __name__ == "__main__":
    main()
