"""Decode a physicsGeometryCache (WolvenKit JSON) into triangle soups.

The world's cooked collision shapes (worldCookedCollisionShape with
PhysicsGeometryTriangleMeshData / PhysicsGeometryConvexMeshData) reference their geometry by a
64-bit hash. The geometry lives in <world>.geometry_cache:

- sectorGeometries[i].ta: first 8 bytes = that hash (little endian), pe = 4 triangle mesh, 3 convex
- sectorEntries[s]: a run [entryOffset, entryOffset + entrySize) of geometry indices, whose blobs
  are in bufferTableSectors[s]
- sectorCacheEntries[i]: (entryOffset, entrySize) of geometry i inside its sector's buffer
- alwaysLoadedSector / alwaysLoadedSectorDDB: one more run + buffer, after the last sector

Blobs are PhysX 3.4 cooked data: "NXS\\x01MESH" (v15) or "NXS\\x01CVXM" (v13). Only vertices and
triangles are read; midphase trees, materials and remaps are skipped. Convex hulls are
re-triangulated with scipy from their hull vertices.

Usage: python geometry_cache.py <geometry_cache.json> [...more caches] -o geometry.npz
"""

import argparse
import base64
import json
import struct
import sys

import numpy as np
from scipy.spatial import ConvexHull, QhullError

PE_CONVEX = 3
PE_MESH = 4

MESH_8BIT_INDICES = 1 << 2
MESH_16BIT_INDICES = 1 << 3


def parse_mesh(blob):
    if blob[:8] != b"NXS\x01MESH":
        raise ValueError("not a cooked triangle mesh")
    version, _midphase, flags, nb_verts, nb_tris = struct.unpack_from("<5I", blob, 8)
    if version != 15:
        raise ValueError(f"triangle mesh version {version}")
    off = 28
    verts = np.frombuffer(blob, "<f4", nb_verts * 3, off).reshape(-1, 3)
    off += nb_verts * 12
    if flags & MESH_8BIT_INDICES:
        dtype, size = "u1", 1
    elif flags & MESH_16BIT_INDICES:
        dtype, size = "<u2", 2
    else:
        dtype, size = "<u4", 4
    tris = np.frombuffer(blob, dtype, nb_tris * 3, off).reshape(-1, 3).astype(np.int32)
    return verts, tris


def parse_convex(blob):
    if blob[:8] != b"NXS\x01CVXM":
        raise ValueError("not a cooked convex mesh")
    # NXS CVXM v13, serial flags, ICE CLHL v8, ICE CVHL v8, then the hull header
    off = blob.index(b"ICE\x01CVHL") + 12
    nb_verts = struct.unpack_from("<I", blob, off)[0]
    verts = np.frombuffer(blob, "<f4", nb_verts * 3, off + 16).reshape(-1, 3)
    try:
        tris = ConvexHull(verts).simplices.astype(np.int32)
    except QhullError:
        tris = np.zeros((0, 3), np.int32)  # flat or degenerate hull
    return verts, tris


def decode(cache_json_paths):
    hashes, kinds, vstart, tstart = [], [], [], []
    all_verts, all_tris = [], []
    nv = nt = 0
    seen = set()
    bad = 0
    for path in cache_json_paths:
        print(f"loading {path}", file=sys.stderr)
        rc = json.load(open(path, encoding="utf-8"))["Data"]["RootChunk"]
        geos = rc["sectorGeometries"]
        ents = rc["sectorCacheEntries"]
        sectors = list(zip(rc["sectorEntries"], rc["bufferTableSectors"]))
        # the run after the last sector (big shared pieces) lives in its own buffer
        sectors.append((rc["alwaysLoadedSector"], rc["alwaysLoadedSectorDDB"]))
        for sector, buf in sectors:
            data = base64.b64decode(buf["Bytes"])
            first = sector["entryOffset"]
            for gi in range(first, first + sector["entrySize"]):
                h = struct.unpack("<Q", bytes(geos[gi]["ta"]["Elements"][:8]))[0]
                if h in seen:
                    continue
                e = ents[gi]
                blob = data[e["entryOffset"] : e["entryOffset"] + e["entrySize"]]
                pe = geos[gi]["pe"]
                try:
                    verts, tris = parse_mesh(blob) if pe == PE_MESH else parse_convex(blob)
                except ValueError as err:
                    bad += 1
                    if bad <= 10:
                        print(f"skip geometry {h}: {err}", file=sys.stderr)
                    continue
                seen.add(h)
                hashes.append(h)
                kinds.append(pe)
                vstart.append(nv)
                tstart.append(nt)
                all_verts.append(verts)
                all_tris.append(tris)
                nv += len(verts)
                nt += len(tris)
    vstart.append(nv)
    tstart.append(nt)
    print(f"{len(hashes)} geometries, {nv} vertices, {nt} triangles, {bad} skipped", file=sys.stderr)
    return {
        "hash": np.array(hashes, np.uint64),
        "kind": np.array(kinds, np.uint8),
        "vstart": np.array(vstart, np.int64),
        "tstart": np.array(tstart, np.int64),
        "verts": np.concatenate(all_verts).astype(np.float32),
        "tris": np.concatenate(all_tris).astype(np.int32),
    }


class GeometryLibrary:
    """Look up decoded geometry by hash (output of decode / geometry.npz)."""

    def __init__(self, path):
        d = np.load(path)
        self.verts = d["verts"]
        self.tris = d["tris"]
        self.vstart = d["vstart"]
        self.tstart = d["tstart"]
        self.index = {int(h): i for i, h in enumerate(d["hash"])}

    def get(self, h):
        i = self.index.get(h)
        if i is None:
            return None
        v = self.verts[self.vstart[i] : self.vstart[i + 1]]
        t = self.tris[self.tstart[i] : self.tstart[i + 1]]
        return v, t


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("caches", nargs="+")
    ap.add_argument("-o", "--out", required=True)
    args = ap.parse_args()
    np.savez(args.out, **decode(args.caches))


if __name__ == "__main__":
    main()
