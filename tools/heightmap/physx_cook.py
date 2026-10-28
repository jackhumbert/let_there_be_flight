"""Cook triangle meshes into PhysX 3.4 blobs with tools/physx_cook (cook.exe), in one process
call for many meshes. The blobs match the game's geometry cache format (NXS MESH v15, BV4).
"""

import os
import struct
import subprocess
import tempfile

import numpy as np

DEFAULT_COOK = r"D:\cp2077_work\build\tools\physx_cook\cook.exe"


def cook(meshes, exe=DEFAULT_COOK):
    """meshes: list of (verts float (n, 3), tris int (m, 3)). Returns a list of bytes (b"" if
    cooking failed)."""
    with tempfile.TemporaryDirectory() as tmp:
        src, dst = os.path.join(tmp, "in.bin"), os.path.join(tmp, "out.bin")
        with open(src, "wb") as f:
            f.write(struct.pack("<I", len(meshes)))
            for v, t in meshes:
                v = np.ascontiguousarray(v, "<f4")
                t = np.ascontiguousarray(t, "<u4")
                f.write(struct.pack("<II", len(v), len(t)))
                f.write(v.tobytes())
                f.write(t.tobytes())
        subprocess.run([exe, src, dst], check=False, stdout=subprocess.DEVNULL)
        data = open(dst, "rb").read()
    out, off = [], 4
    for _ in range(struct.unpack_from("<I", data)[0]):
        n = struct.unpack_from("<I", data, off)[0]
        out.append(data[off + 4 : off + 4 + n])
        off += 4 + n
    return out
