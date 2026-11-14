"""Reader for navmap.bin, the Autopilot's nav map (format: tools/navmap_bake/navmap.py, read in the
plugin by src/red4ext/Navigation/NavMap.hpp). Sections appended after the tiles (the baked doorways,
tag LTBFDOOR) are ignored. As the plugin does, an open-sky gap reaching below the sea's surface (water:
the bake knows none) starts at the surface plus the car's half height (NavMap.hpp, kSeaLevel)."""

import math
import struct
import zlib

import numpy as np

MAGIC = b"LTBFNAV1"
VERSION = 1
HEADER = struct.Struct("<8sII5fIIIIII2f")
INDEX = struct.Struct("<QII")
SEA_LEVEL = 0.0  # NavMap.hpp kSeaLevel: the ocean's water patches are placed at z 0


class NavMap:
    def __init__(self, path):
        with open(path, "rb") as f:
            self.data = f.read()
        (magic, version, flags, self.ox, self.oy, self.cell, self.z0, self.vz, self.nz, self.w, self.h,
         self.tile, self.tx, self.ty, self.radius, self.half) = HEADER.unpack_from(self.data, 0)
        assert magic == MAGIC and version == VERSION, f"{path}: not a navmap.bin"
        self._tiles = {}

    def tile_data(self, tx, ty):
        """(counts, gaps, starts) of tile (tx, ty): gaps per column, (bottom, top) voxel pairs, and
        each column's first gap; None where the tile has no data."""
        key = ty * self.tx + tx
        if key not in self._tiles:
            off, packed, _ = INDEX.unpack_from(self.data, HEADER.size + key * INDEX.size)
            if not off:
                self._tiles[key] = None
            else:
                buf = zlib.decompress(self.data[off:off + packed])
                n = self.tile * self.tile
                counts = np.frombuffer(buf, np.uint8, n)
                gaps = np.frombuffer(buf, "<u2", offset=n).reshape(-1, 2)
                starts = np.concatenate([[0], np.cumsum(counts, dtype=np.int64)])
                self._tiles[key] = (counts, gaps, starts)
        return self._tiles[key]

    def cell_of(self, x, y):
        return int(math.floor((x - self.ox) / self.cell)), int(math.floor((y - self.oy) / self.cell))

    def column(self, cx, cy):
        """Gaps (bottom, top) in metres of cell (cx, cy), bottom up; top is inf for open sky."""
        if not (0 <= cx < self.w and 0 <= cy < self.h):
            return []
        t = self.tile_data(cx // self.tile, cy // self.tile)
        if t is None:
            return []
        counts, gaps, starts = t
        c = (cy % self.tile) * self.tile + cx % self.tile
        out = [(self.z0 + b * self.vz, math.inf if top >= self.nz else self.z0 + top * self.vz)
               for b, top in gaps[starts[c]:starts[c] + counts[c]]]
        sea = self.z0 + math.ceil((SEA_LEVEL + self.half - self.z0) / self.vz - 1e-4) * self.vz
        if out and math.isinf(out[-1][1]) and out[-1][0] < sea:
            out[-1] = (sea, math.inf)
        return out

    def gaps(self, x, y):
        """Gaps (floor, ceiling) a car's centre can occupy at world (x, y); ceiling inf = open sky."""
        return self.column(*self.cell_of(x, y))
