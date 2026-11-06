"""A drag per vehicle from its chassis collision shape, and what it would do to top speeds (prototype; the research
note is docs/research/2026-11-04-aero-drag.md). Nothing here is used by the mod.

    python -I tools/flightsim/aero.py [--vehicles tools/flightsim/vehicles.tsv] [--dump <TweakDB dump>]
                                      [--rigs <dir>] [--phys <dir>] [-o <aero.tsv>] [--ref "<name>"]

Inputs, all already extracted under D:\\cp2077_work\\tmp\\flightsim:
- the fleet table (one row per physical car; its record maps to an entity template through the dump's vehicles.tsv
  and rig_geometry's templates.txt);
- each template's vehicleChassisComponent `Chassis`: its collisionResource, a physicsSystemResource (.phys), and its
  localTransform (the shapes are in the component's frame, which most templates lift 0.4 to 0.6 m);
- the .phys files serialized by WolvenKit under <phys>/<depot folder>/<name>.phys.json. Their shapes are inline:
  physicsColliderConvex with plain `vertices` (no cooked buffer on any vehicle), Box (halfExtents), Sphere (radius)
  and Capsule (radius, height; along the shape's own Z), each with a localToBody, in one body whose
  localToModel is the identity on every vehicle. Templates with no .phys fall back to the ChassisMesh boxes.

For each car, in the model frame (X right, Y forward, Z up): the silhouette area along a direction, both as the
convex hull of everything and as the union of the pieces (each piece's projected hull, rasterized on a 2 cm grid);
frontal (Y), side (X), top (Z), a yaw sweep, and how well A(d) = ((Ax|dx|)^p + (Ay|dy|)^p + (Az|dz|)^p)^(1/p)
fits the union over a hemisphere of directions (p 1 is exact for a box, p 2 for an ellipsoid). Then the drag F = 0.5 rho Cd A v^2 with a Cd per body class and axis, today's
drag F = 1.2 airResistanceFactor v^2 (FlightMode.ApplyPhysics) as a CdA, and the top speeds under four models:
  today   today's drag, surge 15 m/s2 as an acceleration (thrust proportional to mass)
  shape   shape drag with real Cd, surge 15 m/s2
  scaled  shape drag times one factor K for the fleet, chosen so --ref keeps today's top speed, surge 15 m/s2
  budget  shape drag times K, thrust = thrusters x one force per thruster, chosen so --ref keeps today's top speed
(budget with real drag and a smaller force per thruster gives the same speeds: only force / K matters.)
Speeds are uncapped and without TopSpeed's 0.97 (Navigation/Vehicle.cpp caps at speedCap, 120 m/s by default).
Finally a rough flat turn per named car under each drag model, with and without the yaw directionality redirect.
For the ground contact (docs/design/hover-fly-terrain.md), also the chassis box and its underside profile (the
lowest point per 0.5 m square, `underside()`), from the drive model's centre of mass as vehicles.tsv's wheel offsets
and bottom are; tweak_physics.py --aero carries them into vehicles.tsv.
Writes <out> (one row per fleet row) and prints the comparison.
"""

import argparse
import csv
import json
import math
import os
import statistics
import sys
from collections import Counter

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
from rig_geometry import NONE, Template, basename, compose, fnv1a64, load_json, qrot, read  # noqa: E402

WORK = r"D:\cp2077_work\tmp\flightsim"
RHO = 1.2
SURGE = 15.0     # m/s2, standardModeSurgeFactor
CELL = 0.02      # m, raster cell for the union of the pieces
# Cd by axis (front, side, top) for each body class, read from the chassis .phys folder. Rough textbook values:
# a sports car 0.3, a saloon 0.33, a pickup or SUV 0.42, a truck or bus 0.7 front; a car side-on about 1.0 (Hucho);
# top-down a curved roof over a flat floor, about 1.0. A motorbike's collider is the bike alone (0.4 m2 frontal): the
# rider is added as RIDER, a CdA by axis (side, front, top) for a seated rider leaning forward (a road bike with its
# rider measures a CdA of 0.5 to 0.7 m2 head-on).
CD = {"sport": (0.30, 1.0, 1.0), "standard": (0.33, 1.0, 1.0), "suv": (0.42, 1.0, 1.0), "truck": (0.70, 1.1, 1.1),
      "bike": (0.60, 1.0, 1.0), "other": (0.50, 1.0, 1.0)}
RIDER = (0.6, 0.35, 0.3)
NAMED = [("Shion", "mizutani_shion"), ("Caliburn", "caliburn"), ("MaiMai", "maimai"), ("Colby", "thorton_colby"),
         ("Bratsk", "bratsk"), ("MT28 coach", "mt28"), ("Behemoth", "behemoth"), ("Kusanagi", "kusanagi"),
         ("Apollo", "apollo")]
YAWS = (0, 15, 30, 45, 60, 75, 90)


def body_class(phys):
    p = phys.lower()
    if "sportbike" in p:
        return "bike"
    if "\\sport" in p:
        return "sport"
    if "standard25" in p or "standard3" in p:
        return "suv"
    if "\\standard" in p:
        return "standard"
    if "\\utility" in p or "mt28" in p:
        return "truck"
    return "other"


def transform(t):
    p, q = t["position"], t["orientation"]
    return (p["X"], p["Y"], p["Z"]), (q["i"], q["j"], q["k"], q["r"])


def sphere_points(n=120):
    """n points spread over the unit sphere (Fibonacci)."""
    i = np.arange(n) + 0.5
    z = 1 - 2 * i / n
    r = np.sqrt(1 - z * z)
    a = math.pi * (1 + 5 ** 0.5) * i
    return np.stack([r * np.cos(a), r * np.sin(a), z], axis=1)


UNIT = sphere_points()


def shape_points(s):
    """A collider's points in its own frame, or None for a shape with no geometry here."""
    t = s["$type"]
    if t == "physicsColliderBox":
        h = s["halfExtents"]
        return np.array([[x * h["X"], y * h["Y"], z * h["Z"]] for x in (-1, 1) for y in (-1, 1) for z in (-1, 1)])
    if t == "physicsColliderConvex":
        v = s.get("vertices") or []
        return np.array([[p["X"], p["Y"], p["Z"]] for p in v]) if len(v) >= 4 else None
    if t == "physicsColliderSphere":
        return UNIT * s["radius"]
    if t == "physicsColliderCapsule":
        # CDPR's capsule lies along its own Z (unlike PhysX's X: the bikes' capsules are turned 75 to 90 degrees
        # about X, which lays a Z axis along the bike) and height is the distance between the two end spheres'
        # centres: the Kusanagi comes out 2.1 m long for a 1.84 m wheelbase that way (2.8 m with height as a half)
        h = s["height"] / 2
        b = UNIT * s["radius"]
        return np.concatenate([b + [0, 0, h], b - [0, 0, h]])
    return None


def place(points, *frames):
    """Points through a chain of (position, quaternion) frames, innermost first."""
    out = []
    for p in points:
        q = (tuple(p), (0.0, 0.0, 0.0, 1.0))
        for f in frames:
            q = compose(f, q)
        out.append(q[0])
    return np.array(out)


class Shapes:
    """The chassis collision pieces of one entity template, as point sets in the model frame."""

    def __init__(self, template, phys_dir):
        self.pieces, self.source, self.kinds = [], "", []
        chassis = next((c for c in template.components.values() if c["$type"] == "vehicleChassisComponent"), None)
        if chassis is None:
            self.source = "no chassis component"
            return
        lt = chassis.get("localTransform")
        frame = ((0.0, 0.0, 0.0), (0.0, 0.0, 0.0, 1.0))
        if lt:
            p, q = lt["Position"], lt["Orientation"]
            frame = ((p["x"]["Bits"] / 131072, p["y"]["Bits"] / 131072, p["z"]["Bits"] / 131072),
                     (q["i"], q["j"], q["k"], q["r"]))
        depot = chassis.get("collisionResource", {}).get("DepotPath", {}).get("$value", "0")
        if depot not in ("0", "", None):
            folder, name = depot.replace("/", "\\").rsplit("\\", 1)
            path = os.path.join(phys_dir, folder, name + ".json")
            if not os.path.exists(path):
                self.source = f"not extracted: {depot}"
                return
            self.source = depot
            for body in load_json(path)["Data"]["RootChunk"]["bodies"]:
                b = body["Data"]
                body_frame = transform(b["localToModel"])
                for s in b["collisionShapes"]:
                    s = s["Data"]
                    pts = shape_points(s)
                    if pts is not None:
                        self.pieces.append(place(pts, transform(s["localToBody"]), body_frame, frame))
                        self.kinds.append(s["$type"].replace("physicsCollider", ""))
            return
        # no .phys: the ChassisMesh collider component's own shapes
        mesh = template.components.get("ChassisMesh")
        for h in (mesh or {}).get("colliders", []):
            s = template.handles.get(h["HandleRefId"]) if "HandleRefId" in h else h.get("Data")
            pts = shape_points(s) if s else None
            if pts is not None:
                self.pieces.append(place(pts, transform(s["localToBody"]), frame))
                self.kinds.append(s["$type"].replace("physicsCollider", ""))
        self.source = "ChassisMesh" if self.pieces else "no collision shapes"

    def extent(self):
        a = np.concatenate(self.pieces)
        return a.min(axis=0), a.max(axis=0)


def hull2(p):
    """The 2D convex hull of points, counter-clockwise (monotone chain)."""
    p = sorted(set(map(tuple, np.round(p, 5))))
    if len(p) < 3:
        return np.array(p)

    def half(pts):
        h = []
        for q in pts:
            while len(h) >= 2 and (h[-1][0] - h[-2][0]) * (q[1] - h[-2][1]) - (h[-1][1] - h[-2][1]) * (q[0] - h[-2][0]) <= 0:
                h.pop()
            h.append(q)
        return h
    lower, upper = half(p), half(p[::-1])
    return np.array(lower[:-1] + upper[:-1])


def poly_area(h):
    if len(h) < 3:
        return 0.0
    x, y = h[:, 0], h[:, 1]
    return 0.5 * abs(np.dot(x, np.roll(y, -1)) - np.dot(y, np.roll(x, -1)))


def basis(d):
    d = np.asarray(d, float) / np.linalg.norm(d)
    a = np.array([0.0, 0.0, 1.0]) if abs(d[2]) < 0.9 else np.array([1.0, 0.0, 0.0])
    u = np.cross(a, d)
    u /= np.linalg.norm(u)
    return u, np.cross(d, u)


def areas(pieces, d):
    """(hull area, union area) of the pieces' silhouette seen along d, m2."""
    u, w = basis(d)
    hulls = [hull2(np.stack([p @ u, p @ w], axis=1)) for p in pieces]
    whole = hull2(np.concatenate(hulls))
    lo, hi = whole.min(axis=0), whole.max(axis=0)
    nx, ny = int(math.ceil((hi[0] - lo[0]) / CELL)) + 1, int(math.ceil((hi[1] - lo[1]) / CELL)) + 1
    cx = lo[0] + (np.arange(nx) + 0.5) * CELL
    cy = lo[1] + (np.arange(ny) + 0.5) * CELL
    mask = np.zeros((ny, nx), bool)
    for h in hulls:
        if len(h) < 3:
            continue
        i0, i1 = np.searchsorted(cx, h[:, 0].min()), np.searchsorted(cx, h[:, 0].max())
        j0, j1 = np.searchsorted(cy, h[:, 1].min()), np.searchsorted(cy, h[:, 1].max())
        if i1 <= i0 or j1 <= j0:
            continue
        X, Y = np.meshgrid(cx[i0:i1], cy[j0:j1])
        inside = np.ones(X.shape, bool)
        for a, b in zip(h, np.roll(h, -1, axis=0)):
            inside &= (b[0] - a[0]) * (Y - a[1]) - (b[1] - a[1]) * (X - a[0]) >= 0
        mask[j0:j1, i0:i1] |= inside
    return poly_area(whole), mask.sum() * CELL * CELL


def hemisphere(n=48):
    """n directions over the upper half of the unit sphere (A(d) = A(-d), so half is all of it)."""
    pts = sphere_points(2 * n)
    return pts[pts[:, 2] > 0]


PS = (1.0, 1.25, 1.5, 1.75, 2.0)


def pnorm(ax, ay, az, d, p):
    return ((ax * abs(d[0])) ** p + (ay * abs(d[1])) ** p + (az * abs(d[2])) ** p) ** (1 / p)


def fit(pieces, ax, ay, az):
    """{p: (worst, RMS) relative error} of the p-norm model against the union, over a hemisphere of directions."""
    dirs = hemisphere()
    union = np.array([areas(pieces, d)[1] for d in dirs])
    out = {}
    for p in PS:
        e = np.array([pnorm(ax, ay, az, d, p) for d in dirs]) / union - 1
        out[p] = (float(np.abs(e).max()), float(np.sqrt((e ** 2).mean())))
    return out


UNDER_CELL = 0.5   # m, the underside profile's cell
UNDER_STEP = 0.1   # m, the samples inside a cell


def underside(pieces, com):
    """The chassis box and the underside profile, from the centre of mass (body frame: X right, Y forward, Z up).
    The profile is the lowest point of the pieces in each UNDER_CELL square of the footprint, sampled every UNDER_STEP:
    "cell,x0,y0,nx,ny:z,z,..." row by row along Y from the back, x0/y0 the first cell's corner, an empty z where no
    piece is over the square. Each piece is convex: its lowest point over (x, y) is the highest of its downward
    facing facets' planes there, inside when that is under the lowest upward one."""
    from scipy.spatial import ConvexHull, QhullError
    com = np.asarray(com, float)
    pts = [p - com for p in pieces]
    a = np.concatenate(pts)
    lo, hi = a.min(axis=0), a.max(axis=0)
    nx, ny = int(math.ceil((hi[0] - lo[0]) / UNDER_CELL - 1e-6)), int(math.ceil((hi[1] - lo[1]) / UNDER_CELL - 1e-6))
    sub = int(round(UNDER_CELL / UNDER_STEP))
    xs = lo[0] + (np.arange(nx * sub + 1)) * (UNDER_CELL / sub)
    ys = lo[1] + (np.arange(ny * sub + 1)) * (UNDER_CELL / sub)
    X, Y = np.meshgrid(np.minimum(xs, hi[0]), np.minimum(ys, hi[1]))
    low = np.full(X.shape, np.inf)
    for p in pts:
        try:
            eq = ConvexHull(p).equations
        except QhullError:
            eq = ConvexHull(p, qhull_options="QJ").equations
        zlo, zhi, inside = np.full(X.shape, -np.inf), np.full(X.shape, np.inf), np.ones(X.shape, bool)
        for nxf, nyf, nzf, d in eq:
            plane = nxf * X + nyf * Y + d
            if nzf < -1e-6:
                zlo = np.maximum(zlo, -plane / nzf)
            elif nzf > 1e-6:
                zhi = np.minimum(zhi, -plane / nzf)
            else:
                inside &= plane <= 1e-6
        inside &= zlo <= zhi + 1e-6
        low = np.where(inside, np.minimum(low, zlo), low)
    cells = []
    for j in range(ny):
        for i in range(nx):
            z = low[j * sub:(j + 1) * sub + 1, i * sub:(i + 1) * sub + 1].min()
            cells.append(f"{z:.3f}" if np.isfinite(z) else "")
    profile = f"{UNDER_CELL:g},{lo[0]:.3f},{lo[1]:.3f},{nx},{ny}:" + ",".join(cells)
    return {"boxMinX": lo[0], "boxMaxX": hi[0], "boxMinY": lo[1], "boxMaxY": hi[1], "boxMinZ": lo[2],
            "boxMaxZ": hi[2], "underside": profile}


def label(row, template_path):
    n = row["name"]
    if not n or n.startswith("0x") or n.startswith("LocKey"):
        return basename(template_path).replace(".ent", "")
    return n


def turn(m, cda_body, mode, v0=50.0, rate=60.0, angle=90.0, hold=2.0, redirect=0.0, thrust=None, dt=1 / 60):
    """A flat turn: full surge along the nose, nose yawing at `rate` deg/s through `angle`, then `hold` s straight.
    cda_body: (CdA side, CdA front) for 'axis' (per-axis drag, F_i = -0.5 rho CdA_i |v| v_i) or one CdA for
    'iso' (today's: along -v). redirect: ApplyPhysics' generalYawDirectionalityFactor (5 by default; 0 = off).
    Returns (largest slip deg, slip at the end deg, speed at the end / v0)."""
    a = SURGE if thrust is None else thrust / m
    psi, v = 0.0, np.array([0.0, v0])
    t, slip_max, t_end = 0.0, 0.0, angle / rate + hold
    while t < t_end:
        if t < angle / rate:
            psi += math.radians(rate) * dt
        c, s = math.cos(psi), math.sin(psi)
        fwd, right = np.array([-s, c]), np.array([c, s])
        vb = np.array([v @ right, v @ fwd])           # body frame: x right, y forward
        speed = float(np.linalg.norm(v))
        if mode == "iso":
            drag_b = -0.5 * RHO * cda_body * speed * vb / m
        else:
            drag_b = -0.5 * RHO * np.array(cda_body) * speed * vb / m
        acc_b = drag_b + np.array([0.0, a])
        if redirect and speed > 0.1:
            # Flight::ApplyPhysics: (forward - direction) scaled by sideY * yawDir * |aeroFactor|
            dirb = vb / speed
            side = abs((np.array([0.0, 1.0]) - dirb)[0])
            k = side * (speed / 100.0) * redirect * abs(dirb[1])
            acc_b = acc_b + k * (np.array([0.0, 1.0]) - dirb)
        v = v + (acc_b[0] * right + acc_b[1] * fwd) * dt
        slip = math.degrees(math.atan2(abs(v @ right), v @ fwd))
        slip_max = max(slip_max, slip)
        t += dt
    return slip_max, slip, float(np.linalg.norm(v)) / v0


def pearson(x, y):
    mx, my = statistics.mean(x), statistics.mean(y)
    sx = math.sqrt(sum((a - mx) ** 2 for a in x))
    sy = math.sqrt(sum((b - my) ** 2 for b in y))
    return sum((a - mx) * (b - my) for a, b in zip(x, y)) / (sx * sy)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--vehicles", default=os.path.join(HERE, "vehicles.tsv"))
    ap.add_argument("--dump", default=os.path.join(WORK, "tweaks_vanilla"))
    ap.add_argument("--rigs", default=os.path.join(WORK, "rigs"))
    ap.add_argument("--phys", default=os.path.join(WORK, "aero", "phys"))
    ap.add_argument("-o", "--out", default=os.path.join(WORK, "aero", "aero.tsv"))
    ap.add_argument("--ref", default="Cortes V5000 Valor", help="the mid-size car whose top speed is kept")
    args = ap.parse_args()

    tweak = read(os.path.join(args.dump, "vehicles.tsv"))
    with open(os.path.join(args.rigs, "templates.txt"), encoding="utf-8") as f:
        paths = {fnv1a64(line.strip()): line.strip() for line in f if line.strip()}
    with open(args.vehicles, encoding="utf-8") as f:
        fleet = list(csv.DictReader(f, delimiter="\t"))

    drive = {**read(os.path.join(args.dump, "drive_models.tsv")), **read(os.path.join(args.dump, "bike_drive_models.tsv"))}
    shapes, measured, unders, rows, skipped = {}, {}, {}, [], {}
    for row in fleet:
        v = tweak.get(row["record"])
        path = paths.get(v["entityTemplatePath"]) if v else None
        if not path:
            skipped[row["record"]] = "template not resolved"
            continue
        if path not in shapes:
            root = path.replace("\\", "/").split("/")[0]
            js = os.path.join(args.rigs, "json", root, basename(path) + ".json")
            js = js if os.path.exists(js) else os.path.join(args.rigs, "json", basename(path) + ".json")
            shapes[path] = Shapes(Template(js, None), args.phys) if os.path.exists(js) else None
        sh = shapes[path]
        if sh is None or not sh.pieces:
            skipped[row["record"]] = f"{basename(path)}: {sh.source if sh else 'template not serialized'}"
            continue
        key = (sh.source, tuple(np.round(np.concatenate(sh.pieces).sum(axis=0), 3)))
        if key not in measured:
            m = {}
            for axis, d in (("X", (1, 0, 0)), ("Y", (0, 1, 0)), ("Z", (0, 0, 1))):
                m["hull" + axis], m["area" + axis] = areas(sh.pieces, d)
            for yaw in YAWS:
                r = math.radians(yaw)
                m[f"yaw{yaw}"] = areas(sh.pieces, (math.sin(r), math.cos(r), 0))[1]
            f = fit(sh.pieces, m["areaX"], m["areaY"], m["areaZ"])
            for p, (worst, rms) in f.items():
                m[f"fitMax{p:g}"], m[f"fitRms{p:g}"] = worst, rms
            m["fitMax"], m["fitRms"] = f[1.0]
            m["bestP"] = min(f, key=lambda p: f[p][1])
            lo, hi = sh.extent()
            m["width"], m["length"], m["height"] = (hi - lo).tolist()
            measured[key] = m
        m = measured[key]
        dm = v.get("vehDriveModelData", "")
        dm = dm if dm not in NONE else v.get("bikeDriveModelData", "")
        com = [float(c) for c in drive[dm]["center_of_mass_offset"].split(",")[:3]] if dm in drive else [0.0] * 3
        ukey = key + (tuple(com),)
        if ukey not in unders:
            unders[ukey] = underside(sh.pieces, com)
        cls = body_class(sh.source)
        cdf, cds, cdt = CD[cls]
        mass, af = float(row["mass"]), float(row["airResistance"])
        r = {"record": row["record"], "name": label(row, path), "type": row["type"], "template": basename(path),
             "phys": basename(sh.source) if "\\" in sh.source else sh.source, "class": cls,
             "pieces": len(sh.pieces), "mass": mass, "airResistance": af,
             "thrusters": int(float(row["thrusters"])), "records": int(row["records"]), **m, **unders[ukey],
             "wheelX": float(row["wheelX"]), "wheelY": float(row["wheelY"]), "bottom": float(row["bottom"]),
             "cdFront": cdf, "cdSide": cds, "cdTop": cdt}
        r["cdaX"], r["cdaY"], r["cdaZ"] = cds * m["areaX"], cdf * m["areaY"], cdt * m["areaZ"]
        if cls == "bike":
            r["cdaX"], r["cdaY"], r["cdaZ"] = r["cdaX"] + RIDER[0], r["cdaY"] + RIDER[1], r["cdaZ"] + RIDER[2]
        r["cdaToday"] = 2 * af                   # 1.2 af v^2 = 0.5 rho CdA v^2
        r["cdImplied"] = r["cdaToday"] / m["areaY"]
        rows.append(r)

    def top(force, cda):
        return math.sqrt(2 * force / (RHO * cda))

    ref = next((r for r in rows if r["name"] == args.ref), None)
    if ref is None:
        sys.exit(f"--ref {args.ref!r} is not in the fleet")
    ref_today = top(SURGE * ref["mass"], ref["cdaToday"])
    K = (top(SURGE * ref["mass"], ref["cdaY"]) / ref_today) ** 2          # drag factor keeping ref's top speed
    per_thruster = 0.5 * RHO * K * ref["cdaY"] * ref_today ** 2 / ref["thrusters"]   # N, budget model
    for r in rows:
        r["vToday"] = top(SURGE * r["mass"], r["cdaToday"])
        r["vShape"] = top(SURGE * r["mass"], r["cdaY"])
        r["vScaled"] = top(SURGE * r["mass"], K * r["cdaY"])
        r["vBudget"] = top(r["thrusters"] * per_thruster, K * r["cdaY"])
        r["accelBudget"] = r["thrusters"] * per_thruster / r["mass"]

    cols = ["record", "name", "type", "class", "template", "phys", "pieces", "records", "mass", "airResistance",
            "thrusters", "width", "length", "height", "wheelX", "wheelY", "hullX", "areaX", "hullY", "areaY", "hullZ",
            "areaZ"] + [f"yaw{y}" for y in YAWS] + [
            "fitMax", "fitRms", "fitRms1.5", "fitMax1.5", "bestP", "cdFront", "cdSide", "cdTop", "cdaX", "cdaY", "cdaZ",
            "cdaToday", "cdImplied",
            "vToday", "vShape", "vScaled", "vBudget", "accelBudget",
            "boxMinX", "boxMaxX", "boxMinY", "boxMaxY", "boxMinZ", "boxMaxZ", "underside"]
    os.makedirs(os.path.dirname(args.out), exist_ok=True)
    with open(args.out, "w", encoding="utf-8", newline="\n") as f:
        f.write("\t".join(cols) + "\n")
        for r in rows:
            f.write("\t".join(f"{r[c]:.4g}" if isinstance(r[c], float) else str(r[c]) for c in cols) + "\n")

    # ---- report ----
    print(f"{len(fleet)} fleet rows: {len(rows)} measured ({len(measured)} distinct shapes), {len(skipped)} skipped")
    for rec, why in skipped.items():
        print(f"  skipped {rec}: {why}")
    print(f"written {args.out}")

    def pick(sub):
        c = [r for r in rows if sub in r["template"].lower() or sub in r["phys"].lower()]
        c.sort(key=lambda r: (-r["records"], r["name"]))
        return c[0] if c else None

    named = [(n, pick(s)) for n, s in NAMED]
    named = [(n, r) for n, r in named if r]
    print("\nunderside, m from the centre of mass: the chassis's lowest point against the tyres' (bottom); the")
    print("overhangs past the wheel emitters, front and back; the lowest point between the axles")
    print(f"  {'car':24} {'chassis':>7} {'tyres':>6} {'clear':>6} {'front':>6} {'back':>6} {'mid':>6}")

    def mid_low(r):
        cell, x0, y0, nx, ny = r["underside"].split(":")[0].split(",")
        zs = r["underside"].split(":")[1].split(",")
        cell, y0, nx, ny = float(cell), float(y0), int(nx), int(ny)
        z = [float(zs[j * nx + i]) for j in range(ny) for i in range(nx)
             if zs[j * nx + i] and abs(y0 + (j + 0.5) * cell) < r["wheelY"]]
        return min(z) if z else float("nan")

    for n, r in named:
        print(f"  {n:24} {r['boxMinZ']:7.2f} {r['bottom']:6.2f} {r['boxMinZ'] - r['bottom']:6.2f} "
              f"{r['boxMaxY'] - r['wheelY']:6.2f} {-r['boxMinY'] - r['wheelY']:6.2f} {mid_low(r):6.2f}")
    clear = [r["boxMinZ"] - r["bottom"] for r in rows]
    print(f"  chassis over the tyres' bottom, fleet: min {min(clear):.2f}, median {statistics.median(clear):.2f}, "
          f"max {max(clear):.2f}; under the tyres on {sum(c < 0 for c in clear)} of {len(rows)}")
    print("\nchecks: collider width against the wheel track, length against the wheelbase")
    print(f"  {'car':24} {'width':>6} {'track':>6} {'length':>6} {'wbase':>6} {'height':>6}")
    for n, r in named:
        print(f"  {n:24} {r['width']:6.2f} {2 * r['wheelX']:6.2f} {r['length']:6.2f} {2 * r['wheelY']:6.2f} "
              f"{r['height']:6.2f}")

    print("\nareas (m2; union of the pieces, hull in brackets), CdA (m2) and today's implied Cd")
    print(f"  {'car':24} {'class':8} {'front':>13} {'side':>13} {'top':>13} {'CdA f':>6} {'CdA s':>6} "
          f"{'today':>6} {'Cd now':>6} {'fit':>9}")
    for n, r in named:
        print(f"  {n:24} {r['class']:8} {r['areaY']:5.2f} ({r['hullY']:5.2f}) {r['areaX']:5.2f} ({r['hullX']:5.2f}) "
              f"{r['areaZ']:5.2f} ({r['hullZ']:5.2f}) {r['cdaY']:6.2f} {r['cdaX']:6.2f} {r['cdaToday']:6.2f} "
              f"{r['cdImplied']:6.2f} {r['fitRms1.5'] * 100:3.0f}/{r['fitMax1.5'] * 100:3.0f}%")
    print("  (fit: RMS / worst error of the p 1.5 model against the union over 48 directions)")
    print("\nsilhouette (union) by yaw of the velocity off the nose, as a multiple of the frontal area")
    print(f"  {'car':14} " + " ".join(f"{y:>5}" for y in YAWS))
    for n, r in named:
        print(f"  {n:14} " + " ".join(f"{r[f'yaw{y}'] / r['areaY']:5.2f}" for y in YAWS))

    cars = [r for r in rows if r["type"] == "car"]
    hull_over = [r["hullY"] / r["areaY"] - 1 for r in rows]
    side_over = [r["hullX"] / r["areaX"] - 1 for r in rows]
    print(f"\nhull over union: front median {statistics.median(hull_over) * 100:.1f}% (max {max(hull_over) * 100:.0f}%), "
          f"side median {statistics.median(side_over) * 100:.1f}% (max {max(side_over) * 100:.0f}%)")
    for p in PS:
        fits = [r[f"fitRms{p:g}"] for r in rows]
        print(f"p {p:g} model over the fleet: RMS error median {statistics.median(fits) * 100:.1f}%, worst car "
              f"{max(fits) * 100:.1f}%; worst single direction {max(r[f'fitMax{p:g}'] for r in rows) * 100:.0f}%")
    print(f"best p per car: {dict(sorted(Counter(r['bestP'] for r in rows).items()))}")
    ratio = [r["areaX"] / r["areaY"] for r in cars]
    print(f"side / front area (cars): median {statistics.median(ratio):.2f}, {min(ratio):.2f} to {max(ratio):.2f}")
    implied = [r["cdImplied"] for r in cars]
    print(f"today's implied Cd (cars): median {statistics.median(implied):.2f}, {min(implied):.2f} to {max(implied):.2f}"
          f"; today's CdA / shape CdA front: median {statistics.median(r['cdaToday'] / r['cdaY'] for r in cars):.1f}, "
          f"side {statistics.median(r['cdaToday'] / r['cdaX'] for r in cars):.2f}")
    print(f"airResistanceFactor against: frontal area r = {pearson([r['airResistance'] for r in rows], [r['areaY'] for r in rows]):.2f}, "
          f"shape CdA r = {pearson([r['airResistance'] for r in rows], [r['cdaY'] for r in rows]):.2f}, "
          f"mass r = {pearson([r['airResistance'] for r in rows], [r['mass'] for r in rows]):.2f} (all rows)")
    print(f"  cars only: frontal area r = {pearson([r['airResistance'] for r in cars], [r['areaY'] for r in cars]):.2f}, "
          f"mass r = {pearson([r['airResistance'] for r in cars], [r['mass'] for r in cars]):.2f}")

    print(f"\ntop speeds, m/s (uncapped). reference {ref['name']}: today {ref_today:.0f}; K = {K:.2f} "
          f"(shape drag x K keeps it); budget {per_thruster:.0f} N per thruster")
    hdr = f"  {'car':34} {'kg':>6} {'thr':>3} {'today':>6} {'shape':>6} {'scaled':>6} {'budget':>6}"

    def line(n, r):
        return (f"  {n[:34]:34} {r['mass']:6.0f} {r['thrusters']:3d} {r['vToday']:6.0f} {r['vShape']:6.0f} "
                f"{r['vScaled']:6.0f} {r['vBudget']:6.0f}")

    def spread(rs, k):
        vals = [r[k] for r in rs]
        return f"{min(vals):.0f} / {statistics.median(vals):.0f} / {max(vals):.0f}"

    print(hdr)
    print("  named")
    for n, r in named:
        print(line(n + " (" + r["name"] + ")", r))
    by_mass = sorted(cars, key=lambda r: r["mass"])
    print("  lightest cars")
    for r in by_mass[:4]:
        print(line(r["name"], r))
    print("  heaviest cars")
    for r in by_mass[-5:]:
        print(line(r["name"], r))
    print("  bikes")
    for r in sorted((r for r in rows if r["type"] == "bike"), key=lambda r: r["mass"]):
        print(line(r["name"], r))
    for k in ("vToday", "vShape", "vScaled", "vBudget"):
        print(f"  {k:8} cars min / median / max {spread(cars, k)}; bikes {spread([r for r in rows if r['type'] == 'bike'], k)}")

    print("\nflat turn at 50 m/s, nose yawing 60 deg/s through 90 deg then held 2 s, full surge (15 m/s2):")
    print("  largest slip / slip 2 s after / speed kept, for each drag model; +r = with ApplyPhysics' redirect (5),")
    print("  +A = with the Automatic mode's (yawDirectionality 300, the same form)")
    print(f"  {'car':14} {'today':>16} {'today+r':>16} {'today+A':>16} {'shape':>16} {'scaled':>16} {'scaled+r':>16}")
    for n, r in named:
        cells = []
        for mode, cda, red in (("iso", r["cdaToday"], 0), ("iso", r["cdaToday"], 5), ("iso", r["cdaToday"], 300),
                               ("axis", (r["cdaX"], r["cdaY"]), 0),
                               ("axis", (K * r["cdaX"], K * r["cdaY"]), 0), ("axis", (K * r["cdaX"], K * r["cdaY"]), 5)):
            a, b, c = turn(r["mass"], cda, mode, redirect=red)
            cells.append(f"{a:4.0f} {b:4.0f} {c:5.2f}")
        print(f"  {n:14} " + " ".join(f"{c:>16}" for c in cells))
    print("\nlateral deceleration at 50 m/s and 20 deg slip, m/s2: side drag (scaled) against the redirect's sideways part")
    for n, r in named:
        vs = 50 * math.sin(math.radians(20))
        side = 0.5 * RHO * K * r["cdaX"] * 50 * vs / r["mass"]
        today_side = 0.5 * RHO * r["cdaToday"] * 50 * vs / r["mass"]
        b = math.radians(20)
        redirect = 5 * 0.5 * math.sin(b) ** 2 * math.cos(b)   # yawDir sin^2 cos, yawDir = 5 * v / 100
        print(f"  {n:14} side drag {side:5.2f}  (today's isotropic {today_side:5.2f}), redirect {redirect:5.2f}")


if __name__ == "__main__":
    main()
