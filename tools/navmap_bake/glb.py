"""Minimal glTF binary (.glb) reader: triangles of every mesh primitive, in game coordinates.

WolvenKit exports meshes Y-up (glTF convention); the game is Z-up. WolvenKit maps game
(x, y, z) to glTF (x, z, -y), so we map back with (x, -z, y). Node transforms in the file are
applied (WolvenKit writes meshes at the origin, but be safe).
"""

import json
import struct

import numpy as np

_COMPONENT = {5120: np.int8, 5121: np.uint8, 5122: np.int16, 5123: np.uint16, 5125: np.uint32, 5126: np.float32}
_COUNT = {"SCALAR": 1, "VEC2": 2, "VEC3": 3, "VEC4": 4, "MAT4": 16}


def _accessor(doc, binary, index):
    acc = doc["accessors"][index]
    view = doc["bufferViews"][acc["bufferView"]]
    dtype = np.dtype(_COMPONENT[acc["componentType"]])
    n = _COUNT[acc["type"]]
    start = view.get("byteOffset", 0) + acc.get("byteOffset", 0)
    stride = view.get("byteStride", 0)
    count = acc["count"]
    if stride and stride != dtype.itemsize * n:
        raw = np.frombuffer(binary, np.uint8, stride * (count - 1) + dtype.itemsize * n, start)
        rows = np.lib.stride_tricks.as_strided(raw, (count, dtype.itemsize * n), (stride, 1))
        out = rows.copy().view(dtype).reshape(count, n)
    else:
        out = np.frombuffer(binary, dtype, count * n, start).reshape(count, n)
    if acc.get("normalized"):
        info = np.iinfo(dtype)
        out = np.maximum(out.astype(np.float32) / info.max, -1.0)
    return out


def _node_matrix(node):
    if "matrix" in node:
        return np.array(node["matrix"], np.float64).reshape(4, 4).T
    m = np.eye(4)
    if "scale" in node:
        m = np.diag([*node["scale"], 1.0]) @ m
    if "rotation" in node:
        x, y, z, w = node["rotation"]
        r = np.array(
            [
                [1 - 2 * (y * y + z * z), 2 * (x * y - z * w), 2 * (x * z + y * w)],
                [2 * (x * y + z * w), 1 - 2 * (x * x + z * z), 2 * (y * z - x * w)],
                [2 * (x * z - y * w), 2 * (y * z + x * w), 1 - 2 * (x * x + y * y)],
            ]
        )
        rm = np.eye(4)
        rm[:3, :3] = r
        m = rm @ m
    if "translation" in node:
        t = np.eye(4)
        t[:3, 3] = node["translation"]
        m = t @ m
    return m


def read_glb_triangles(path, lod0_only=True):
    """(n, 3, 3) float32 triangles in game space (Z-up). With lod0_only, meshes whose name
    marks a lower LOD (WolvenKit appends _LOD_2 etc.) are skipped."""
    data = open(path, "rb").read()
    magic, _version, _length = struct.unpack_from("<4sII", data, 0)
    if magic != b"glTF":
        raise ValueError("not a glb file")
    off = 12
    doc = binary = None
    while off < len(data):
        clen, ctype = struct.unpack_from("<I4s", data, off)
        chunk = data[off + 8 : off + 8 + clen]
        if ctype == b"JSON":
            doc = json.loads(chunk)
        elif ctype == b"BIN\x00":
            binary = chunk
        off += 8 + clen
    tris = []

    def visit(ni, parent):
        node = doc["nodes"][ni]
        m = parent @ _node_matrix(node)
        if "mesh" in node:
            mesh = doc["meshes"][node["mesh"]]
            name = mesh.get("name", "")
            if not (lod0_only and "_LOD_" in name and not name.endswith("_LOD_1")):
                for prim in mesh["primitives"]:
                    if prim.get("mode", 4) != 4:
                        continue
                    pos = _accessor(doc, binary, prim["attributes"]["POSITION"]).astype(np.float64)
                    pos = pos @ m[:3, :3].T + m[:3, 3]
                    if "indices" in prim:
                        idx = _accessor(doc, binary, prim["indices"]).reshape(-1, 3).astype(np.int64)
                    else:
                        idx = np.arange(len(pos)).reshape(-1, 3)
                    t = pos[idx]
                    tris.append(np.stack([t[..., 0], -t[..., 2], t[..., 1]], -1).astype(np.float32))
        for c in node.get("children", []):
            visit(c, m)

    scene = doc["scenes"][doc.get("scene", 0)]
    for ni in scene["nodes"]:
        visit(ni, np.eye(4))
    return np.concatenate(tris) if tris else np.zeros((0, 3, 3), np.float32)
