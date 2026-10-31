"""What the space round the car sounds like, read from the nav map: rays marched through the free
gaps, reduced to the few numbers the renderer needs (render.py).

The nav map's solids are grown by the car's shape (CarRadius sideways, CarHalfHeight up and down),
so a ray stops that far short of the real surface; hits are pushed back out along the ray by the
radius of that ellipsoid in the ray's direction. Columns with no data (outside the city) count as
open; an empty column inside the map is solid (inside a building)."""

import math

import numpy as np

C = 343.0  # speed of sound, m/s


class Solid:
    """Vectorised free / solid test of world points against a NavMap (tools/common/navmap.py)."""

    def __init__(self, nav):
        self.nav = nav

    def free(self, p):
        nav = self.nav
        p = np.asarray(p, np.float64).reshape(-1, 3)
        cx = np.floor((p[:, 0] - nav.ox) / nav.cell).astype(np.int64)
        cy = np.floor((p[:, 1] - nav.oy) / nav.cell).astype(np.int64)
        zz = (p[:, 2] - nav.z0) / nav.vz
        inside = (cx >= 0) & (cx < nav.w) & (cy >= 0) & (cy < nav.h)
        out = ~inside
        key = np.where(inside, (cy // nav.tile) * nav.tx + cx // nav.tile, -1)
        for k in np.unique(key[inside]):
            sel = np.nonzero(key == k)[0]
            t = nav.tile_data(int(k % nav.tx), int(k // nav.tx))
            if t is None:
                out[sel] = True
                continue
            counts, gaps, starts = t
            c = (cy[sel] % nav.tile) * nav.tile + cx[sel] % nav.tile
            n = counts[c].astype(np.int64)
            z = np.minimum(zz[sel], nav.nz - 1e-3)
            free = np.zeros(len(sel), bool)
            for j in range(int(n.max()) if len(n) else 0):
                ok = j < n
                g = gaps[np.where(ok, starts[c] + j, 0)]
                free |= ok & (g[:, 0] <= z) & (z < g[:, 1])
            out[sel] = free
        return out

    def march(self, origin, dirs, max_d=200.0, step=1.0):
        """Distance to the real surface along each unit direction (inf: nothing within max_d)."""
        t = np.arange(step, max_d + step, step)
        pts = origin[None, None, :] + dirs[:, None, :] * t[None, :, None]
        free = self.free(pts.reshape(-1, 3)).reshape(len(dirs), len(t))
        # a car on the ground starts inside the grown floor: a hit only counts once out of it
        hit = ~free & np.logical_or.accumulate(free, axis=1)
        first = np.argmax(hit, axis=1)
        d = np.where(hit.any(axis=1), t[first] - step / 2, np.inf)
        h = np.hypot(dirs[:, 0], dirs[:, 1])
        shell = 1.0 / np.sqrt((h / self.nav.radius) ** 2 + (dirs[:, 2] / self.nav.half) ** 2)
        return d + shell

    def floor_ceiling(self, p):
        """Distance down to the floor and up to the ceiling (inf: open sky) of the gap at p. A car
        centre closer to the ground than the grown solid (sitting on a road) takes the gap above."""
        nav = self.nav
        best = None
        for bottom, top in nav.gaps(p[0], p[1]):
            if bottom <= p[2] < top:
                best = (bottom, top)
                break
            if best is None and bottom > p[2] and bottom - p[2] < 2 * nav.half:
                best = (bottom, top)
        if best is None:
            return math.inf, math.inf
        bottom, top = best
        return max(0.3, p[2] - (bottom - nav.half)), (top + nav.half - p[2]) if math.isfinite(top) else math.inf


def fibonacci_sphere(n):
    i = np.arange(n) + 0.5
    z = 1 - 2 * i / n
    r = np.sqrt(1 - z * z)
    a = math.pi * (3 - math.sqrt(5)) * i
    return np.stack([r * np.cos(a), r * np.sin(a), z], axis=1)


def side_fan(right, yaws=(-30, -15, 0, 15, 30), pitches=(-10, 0, 10)):
    """Directions within a cone round the horizontal vector right."""
    base = math.atan2(right[1], right[0])
    out = []
    for p in pitches:
        for y in yaws:
            a, e = base + math.radians(y), math.radians(p)
            out.append((math.cos(e) * math.cos(a), math.cos(e) * math.sin(a), math.sin(e)))
    return np.array(out)


class Probe:
    """Per position and car frame: floor, ceiling, nearest wall each side and the room around."""

    def __init__(self, nav, rays=48, max_d=200.0):
        self.solid = Solid(nav)
        self.sphere = fibonacci_sphere(rays)
        self.max_d = max_d

    def __call__(self, pos, fwd, up):
        pos = np.asarray(pos, np.float64)
        right = np.cross(fwd, up)
        right[2] = 0
        n = np.linalg.norm(right)
        right = right / n if n > 1e-6 else np.array([1.0, 0.0, 0.0])
        floor, ceiling = self.solid.floor_ceiling(pos)
        fans = np.concatenate([side_fan(right), side_fan(-right)])
        d = self.solid.march(pos, np.concatenate([fans, self.sphere]), self.max_d)
        half = len(fans) // 2
        hits = d[len(fans):]
        hit = np.isfinite(hits)
        return {
            "floor": floor,
            "ceiling": ceiling,
            "right": float(d[:half].min()),
            "left": float(d[half:len(fans)].min()),
            "enclosure": float(hit.mean()),
            "mean_free_path": float(hits[hit].mean()) if hit.any() else math.inf,
            "right_dir": right,
        }
