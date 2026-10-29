"""Draw the planner's view over a recorded run: the nav map's blocked outlines along the racing
line, and the line itself, projected through the game camera frame by frame. What the planner
avoids, next to what the game shows: an outline in empty air is a hidden collider.

The run must log the camera (the test's --camera: LtbfBridge CameraPoseSampler writes
red4ext/logs/ltbf_camerapose_<time>.csv with a wall clock) and dump its line (LtbfBridge.RouteDump).
For every 2 m cell within --width of the line, the blocked space is sliced at the line's local height
and --offsets above and below it; each edge between a blocked and a free cell is drawn at that
height. The map holds car-centre space: walls sit the car's radius (3 m) inside the outlines.
The car's logged position is marked too: it lands on the car when the camera and video line up.
Outlines behind something solid are left out (--behind faint draws them faintly): a ray from the
camera to each segment, stopped --short of it, through a grid of the nav map (4 m across, 2 m up),
so buildings hide what is behind them and an outline in empty air still shows.

    python tools/navplan/navviz.py <navmap.bin> <route.csv> <camerapose.csv> <video.mp4> <wall start>
        -o out.mp4 [--audio take.wav --audio-offset 0.04] [--width 35] [--lag 0] [--fov-axis v]
"""

import argparse
import csv
import math
import os
import subprocess
import sys

import numpy as np
from PIL import Image, ImageDraw

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from plot import NavMap  # noqa: E402

FFMPEG = os.environ.get("FFMPEG", r"D:\cp2077_work\build\tools\ffmpeg\ffmpeg-9.0.2-essentials_build\bin\ffmpeg.exe")


def outlines(nav, line, width, offsets):
    """Segments (N, 2, 3) along the blocked space's edges near the line, sliced at the line's height."""
    cell = nav.cell
    near = {}
    r = int(math.ceil(width / cell))
    for p in line:
        cx, cy = nav.cell_of(p[0], p[1])
        for dy in range(-r, r + 1):
            for dx in range(-r, r + 1):
                if dx * dx + dy * dy <= r * r:
                    near.setdefault((cx + dx, cy + dy), p[2])

    def blocked(k, z):
        return not any(b <= z < t for b, t in nav.gaps(*k))

    segs = []
    for (cx, cy), zl in near.items():
        x0, y0 = nav.ox + cx * cell, nav.oy + cy * cell
        for off in offsets:
            z = zl + off
            if not blocked((cx, cy), z):
                continue
            for dx, dy, a, b in ((1, 0, (x0 + cell, y0), (x0 + cell, y0 + cell)),
                                 (-1, 0, (x0, y0), (x0, y0 + cell)),
                                 (0, 1, (x0, y0 + cell), (x0 + cell, y0 + cell)),
                                 (0, -1, (x0, y0), (x0 + cell, y0))):
                if not blocked((cx + dx, cy + dy), z):
                    segs.append(((a[0], a[1], z), (b[0], b[1], z)))
    return np.array(segs, dtype=np.float64).reshape(-1, 2, 3)


class Solid:
    """The nav map as a solid/free grid around the line (CELL m across, LAYER m up), for occlusion."""
    CELL, LAYER = 4.0, 2.0

    def __init__(self, nav, line, margin=250.0):
        self.x0, self.y0 = line[:, 0].min() - margin, line[:, 1].min() - margin
        self.z0 = max(line[:, 2].min() - 60.0, -50.0)
        nx = int((line[:, 0].max() + margin - self.x0) / self.CELL) + 1
        ny = int((line[:, 1].max() + margin - self.y0) / self.CELL) + 1
        nz = int((line[:, 2].max() + 200.0 - self.z0) / self.LAYER) + 1
        self.solid = np.ones((nx, ny, nz), dtype=bool)
        zs = self.z0 + (np.arange(nz) + 0.5) * self.LAYER
        for i in range(nx):
            for j in range(ny):
                x, y = self.x0 + (i + 0.5) * self.CELL, self.y0 + (j + 0.5) * self.CELL
                col = self.solid[i, j]
                for b, t in nav.gaps(*nav.cell_of(x, y)):
                    col[(zs >= b) & (zs < t)] = False

    def blocked(self, pts):
        i = ((pts[..., 0] - self.x0) / self.CELL).astype(int)
        j = ((pts[..., 1] - self.y0) / self.CELL).astype(int)
        k = ((pts[..., 2] - self.z0) / self.LAYER).astype(int)
        inside = (i >= 0) & (j >= 0) & (k >= 0) & (i < self.solid.shape[0]) & (j < self.solid.shape[1]) & (k < self.solid.shape[2])
        out = np.zeros(pts.shape[:-1], dtype=bool)
        out[inside] = self.solid[i[inside], j[inside], k[inside]]
        return out

    def hidden(self, eye, targets, short=4.0, step=2.0):
        """Whether something solid lies between eye and each target (stopping short of it)."""
        d = targets - eye
        dist = np.linalg.norm(d, axis=1)
        reach = np.maximum(dist - short, 0.0)
        steps = np.arange(step, max(reach.max(), step) + step, step)
        pts = eye + d[:, None, :] / np.maximum(dist, 1e-6)[:, None, None] * steps[None, :, None]
        hit = self.blocked(pts) & (steps[None, :] < reach[:, None])
        return hit.any(axis=1)


def rotate(q, v):
    """v rotated by quaternion q = (i, j, k, r); v is (..., 3)."""
    u, w = np.array(q[:3]), q[3]
    t = 2.0 * np.cross(u, v)
    return v + w * t + np.cross(u, t)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("navmap")
    ap.add_argument("route")
    ap.add_argument("camera")
    ap.add_argument("video")
    ap.add_argument("wall", type=float, help="the video's start, seconds since midnight UTC (gfx_take.wall)")
    ap.add_argument("-o", "--out", required=True)
    ap.add_argument("--audio")
    ap.add_argument("--audio-offset", type=float, default=0.0)
    ap.add_argument("--width", type=float, default=35.0)
    ap.add_argument("--offsets", default="-8,0,8")
    ap.add_argument("--lag", type=float, default=0.0, help="s added to the video's clock (capture start latency)")
    ap.add_argument("--fov-axis", choices=("v", "h"), default="v")
    ap.add_argument("--size", default="1280x720")
    ap.add_argument("--seconds", type=float, default=0.0, help="only this much of the video (0: all)")
    ap.add_argument("--behind", choices=("hide", "faint", "show"), default="hide", help="outlines behind something solid")
    ap.add_argument("--short", type=float, default=4.0, help="m short of an outline its occlusion ray stops")
    args = ap.parse_args()

    nav = NavMap(args.navmap)
    with open(args.route) as f:
        line = np.array([[float(r["x"]), float(r["y"]), float(r["z"])] for r in csv.DictReader(f)])
    with open(args.camera) as f:
        cam = np.array([[float(r[k]) for k in ("wall", "x", "y", "z", "qi", "qj", "qk", "qr", "fov", "carx", "cary", "carz")]
                        for r in csv.DictReader(f)])
    segs = outlines(nav, line, args.width, [float(v) for v in args.offsets.split(",")])
    lines = np.stack([line[:-1], line[1:]], axis=1)
    solid = Solid(nav, line) if args.behind != "show" else None
    print(f"{len(segs)} outline segments, {len(line)} line points, {len(cam)} camera frames")

    w, h = (int(v) for v in args.size.split("x"))
    decode = [FFMPEG, "-hide_banner", "-loglevel", "error", "-i", args.video]
    if args.seconds:
        decode += ["-t", str(args.seconds)]
    decode += ["-vf", f"scale={w}:{h},fps=30", "-f", "rawvideo", "-pix_fmt", "rgb24", "-"]
    encode = [FFMPEG, "-hide_banner", "-loglevel", "error", "-y", "-f", "rawvideo", "-pix_fmt", "rgb24",
              "-s", f"{w}x{h}", "-r", "30", "-i", "-"]
    if args.audio:
        encode += ["-itsoffset", str(args.audio_offset), "-i", args.audio, "-map", "0:v", "-map", "1:a",
                   "-c:a", "aac", "-b:a", "160k", "-shortest"]
    encode += ["-c:v", "libx264", "-preset", "medium", "-crf", "24", "-pix_fmt", "yuv420p", "-movflags", "+faststart", args.out]
    src = subprocess.Popen(decode, stdout=subprocess.PIPE)
    dst = subprocess.Popen(encode, stdin=subprocess.PIPE)

    frame = 0
    size = w * h * 3
    while True:
        raw = src.stdout.read(size)
        if len(raw) < size:
            break
        img = Image.frombuffer("RGB", (w, h), raw, "raw", "RGB", 0, 1).copy()
        wall = args.wall + frame / 30.0 + args.lag
        i = int(np.searchsorted(cam[:, 0], wall))
        if 0 < i < len(cam):
            a, b = cam[i - 1], cam[i]
            t = (wall - a[0]) / max(b[0] - a[0], 1e-6)
            row = a + (b - a) * min(max(t, 0.0), 1.0)
            q = row[4:8] / np.linalg.norm(row[4:8])
            pos = row[1:4]
            right, fwd, up = rotate(q, np.array([[1.0, 0, 0], [0, 1.0, 0], [0, 0, 1.0]]))
            half = math.radians(row[8]) / 2
            focal = (h / 2 if args.fov_axis == "v" else w / 2) / math.tan(half)
            draw = ImageDraw.Draw(img, "RGBA")

            def project(points):
                d = points - pos
                return d @ right, d @ fwd, d @ up

            near = 0.5
            for seg_set, color, thick, occlude in ((segs, (255, 60, 230, 210), 2, True), (lines, (60, 200, 255, 230), 3, False)):
                if not len(seg_set):
                    continue
                xa, ya, za = project(seg_set[:, 0])
                xb, yb, zb = project(seg_set[:, 1])
                keep = ((ya > near) | (yb > near)) & (np.minimum(ya, yb) < 600)
                behind = np.zeros(len(seg_set), dtype=bool)
                if occlude and solid is not None and keep.any():
                    behind[keep] = solid.hidden(pos, seg_set[keep].mean(axis=1), args.short)
                for k in np.nonzero(keep)[0]:
                    if behind[k] and args.behind == "hide":
                        continue
                    x1, y1, z1, x2, y2, z2 = xa[k], ya[k], za[k], xb[k], yb[k], zb[k]
                    if y1 < near:
                        s = (near - y1) / (y2 - y1)
                        x1, y1, z1 = x1 + (x2 - x1) * s, near, z1 + (z2 - z1) * s
                    elif y2 < near:
                        s = (near - y2) / (y1 - y2)
                        x2, y2, z2 = x2 + (x1 - x2) * s, near, z2 + (z1 - z2) * s
                    draw.line((w / 2 + focal * x1 / y1, h / 2 - focal * z1 / y1, w / 2 + focal * x2 / y2, h / 2 - focal * z2 / y2),
                              fill=color[:3] + (60,) if behind[k] else color, width=1 if behind[k] else thick)
            cx, cy_, cz = project(row[9:12][None, :])  # the car
            if cy_[0] > near:
                sx, sy = w / 2 + focal * cx[0] / cy_[0], h / 2 - focal * cz[0] / cy_[0]
                draw.ellipse((sx - 6, sy - 6, sx + 6, sy + 6), outline=(255, 230, 0, 255), width=2)
        dst.stdin.write(img.tobytes())
        frame += 1
    dst.stdin.close()
    dst.wait()
    print(f"{frame} frames -> {args.out}")


if __name__ == "__main__":
    main()
