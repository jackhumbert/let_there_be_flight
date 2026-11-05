"""Each vehicle's flight geometry worked out from its entity template and rig, nothing spawned (docs/design/hover-fly-feel.md).

    python -I tools/flightsim/rig_geometry.py <dump dir> [--rigs <dir>] [-o <dir>/geometry.tsv] [--survey <measured.tsv>]

<dump dir> is vehicle_tweaks.py's TweakDB dump (vehicles.tsv, drive_models.tsv, bike_drive_models.tsv). <rigs> holds
what WolvenKit made of the game files: templates.txt (the depot path of each entity template the dump's FNV-1a 64
hashes resolve to), json/<name>.ent.json (the templates; json/base and json/ep1 for the four names both have),
json/<name>.rig.json (every rig they name) and json/app/<name>.app.json (the appearances of the templates whose rig has
more than two axles). One row per entity template and drive model, since the centre of mass comes from the drive model:
template, driveModel, tensorX..Z, wheelX..Z, bottom, thrusters, source. tweak_physics.py --geometry reads it.

Where each number comes from (checked against the survey, vehicle_survey.py, 2026-11-04):
- centre of mass: the drive model's center_of_mass_offset, in the template's model space (GetCenterOfMass() is
  that point). The surveyed wheel heights put it there to the millimetre on every template.
- thrusters: the configuration FlightConfiguration.cpp picks from the wheel slots the appearance's components are
  bound to, and that configuration's thrusters with their bones and offsets, all read from LTBF's sources by
  thruster_slots.py's Ltbf (pick, config_thrusters): four on a car, six, eight or twelve on a car with more axles, two
  on a bike (0.5 m down its suspension bones; not surveyed). A record with no appearance gets one at random: the
  majority of the template's appearances is used (a burnt one has no third axle). A template whose default
  appearance is None loads none and has a plain car's. A template whose appearances were not serialized gets the
  configuration with the most thrusters its rig has every bone for.
- where a bone is: the rig's bind pose (boneTransforms composed up the parent chain), in the rig that the
  vehicle_slots component is bound to (deformation_rig on nearly every car), with the local transforms of the rig
  that drives it (vehicle_rig). The survey read the bind pose: a spawned car is not simulated until something moves
  it, so its bones stay put. Some readings (half the Colby pickup nomad's, the odd one elsewhere) have the four
  swingarms 0.15 to 0.3 m lower, the suspension hanging out: tensorZ the same, X and Y up 2 to 8%. A car in the air
  hangs its wheels like that too, so in flight the tensor may be nearer those readings than this one.
- thruster tensor: GetThrusterTensor's sum (not a mean) over the thrusters of sqrt(y²+z²), sqrt(x²+z²), sqrt(x²+y²)
  of each one's position minus the centre of mass.
- wheel offsets: the WheelAudioEmitter components (FL, FR, BL, BR; a bike's Front and Back count twice), placed
  through their parentTransform binding (to Chassis, whose localTransform lifts them 0.5 m on most cars); wheelX and
  wheelY are the mean |x| and |y| from the centre of mass, wheelZ the mean z.
- bottom: the ground under the parked car, from the centre of mass: GROUND minus the centre of mass's height. The
  survey's bottoms do not follow the tyre radius (0.23 to 0.8 m) at all: a spawned car stays where it is put, at V's
  height, so they read the spawn spot's ground. GROUND is their median over the surveyed templates.
--survey compares the rows with every plausible surveyed record (the stand-in physics body and a car without
thrusters left out) and prints the error spread and the worst templates.
"""

import argparse
import csv
import json
import math
import os
import statistics
import sys
from collections import Counter, defaultdict

HERE = os.path.dirname(os.path.abspath(__file__))
RIGS = r"D:\cp2077_work\tmp\flightsim\rigs"
NONE = ("", "0x0000000000000000", "None", "NULL")
COLUMNS = ["template", "driveModel", "tensorX", "tensorY", "tensorZ", "wheelX", "wheelY", "wheelZ", "bottom",
           "thrusters", "source"]
GROUND = -0.40  # m below the model origin; the survey's median over its templates (IQR -0.48 to -0.32)
CAR_EMITTERS = ["WheelAudioEmitterFL", "WheelAudioEmitterFR", "WheelAudioEmitterBL", "WheelAudioEmitterBR"]
BIKE_EMITTERS = ["WheelAudioEmitterFront", "WheelAudioEmitterFront", "WheelAudioEmitterBack", "WheelAudioEmitterBack"]
IDENTITY = ((0.0, 0.0, 0.0), (0.0, 0.0, 0.0, 1.0))


def read(path):
    if not os.path.exists(path):
        return {}
    with open(path, encoding="utf-8") as f:
        return {r["record"]: r for r in csv.DictReader(f, delimiter="\t")}


def fnv1a64(path):
    h = 0xcbf29ce484222325
    for b in path.lower().encode():
        h = ((h ^ b) * 0x100000001b3) & 0xffffffffffffffff
    return f"0x{h:016x}"


def basename(path):
    return path.replace("\\", "/").split("/")[-1]


def qmul(a, b):
    ax, ay, az, aw = a
    bx, by, bz, bw = b
    return (aw * bx + ax * bw + ay * bz - az * by, aw * by - ax * bz + ay * bw + az * bx,
            aw * bz + ax * by - ay * bx + az * bw, aw * bw - ax * bx - ay * by - az * bz)


def qrot(q, v):
    x, y, z, w = q
    tx, ty, tz = 2 * (y * v[2] - z * v[1]), 2 * (z * v[0] - x * v[2]), 2 * (x * v[1] - y * v[0])
    return (v[0] + w * tx + y * tz - z * ty, v[1] + w * ty + z * tx - x * tz, v[2] + w * tz + x * ty - y * tx)


def compose(parent, child):
    """A child transform (position, quaternion i j k r) in its parent's frame, to the parent's parent's frame."""
    r = qrot(parent[1], child[0])
    return tuple(p + c for p, c in zip(parent[0], r)), qmul(parent[1], child[1])


def load_json(path):
    with open(path, encoding="utf-8") as f:
        return json.load(f)


class Rigs:
    def __init__(self, folder):
        self.folder, self.cache = folder, {}

    def locals(self, depot_path):
        """[(bone name, parent index, local transform)] of a rig, or None."""
        if depot_path not in self.cache:
            path = os.path.join(self.folder, "json", basename(depot_path) + ".json")
            bones = None
            if os.path.exists(path):
                r = load_json(path)["Data"]["RootChunk"]
                bones = []
                for name, parent, t in zip(r["boneNames"], r["boneParentIndexes"], r["boneTransforms"]):
                    p, q = t["Translation"], t["Rotation"]
                    bones.append((name["$value"], parent, ((p["X"], p["Y"], p["Z"]), (q["i"], q["j"], q["k"], q["r"]))))
            self.cache[depot_path] = bones
        return self.cache[depot_path]

    def bones(self, depot_path, driver=None):
        """Bone name to bind pose in the rig's model space. A rig animated from another (deformation_rig's
        controlBinding to vehicle_rig) takes the driver's local transform for every bone the driver has: the Porsche's
        deformation rig puts its suspension 3 cm from its base rig's, and the survey read the base rig's."""
        own = self.locals(depot_path)
        if own is None:
            return None
        drive = {n: t for n, _, t in (self.locals(driver) or [])} if driver else {}
        bones, poses = {}, []
        for name, parent, local in own:
            local = drive.get(name, local)
            poses.append(compose(poses[parent], local) if parent >= 0 else local)
            bones[name] = poses[-1]
        return bones


class Template:
    """An entity template's components, placed in model space through their parentTransform bindings."""

    def __init__(self, path, rigs):
        self.rigs = rigs
        root = load_json(path)
        handles = {}

        def walk(o):
            if isinstance(o, dict):
                if "HandleId" in o:
                    handles[o["HandleId"]] = o.get("Data")
                for v in o.values():
                    walk(v)
            elif isinstance(o, list):
                for v in o:
                    walk(v)
        walk(root)
        self.handles = handles
        r = root["Data"]["RootChunk"]
        self.components = {c["name"]["$value"]: c for c in r["components"] if "name" in c}
        self.appearances = {a["name"]["$value"]: (basename(a["appearanceResource"]["DepotPath"]["$value"]),
                                                  a["appearanceName"]["$value"]) for a in r.get("appearances", [])}
        self.default_appearance = r.get("defaultAppearance", {}).get("$value", "None")
        self.placed, self.posed = {}, {}

    def binding(self, c):
        p = c.get("parentTransform")
        if not p:
            return None
        b = self.handles.get(p["HandleRefId"]) if "HandleRefId" in p else p.get("Data")
        return b if b and b.get("$type") == "entHardTransformBinding" else None

    def rig_of(self, name):
        if name in self.posed:
            return self.posed[name]
        c = self.components.get(name, {})
        bones = None
        if "rig" in c:
            driver = None
            cb = c.get("controlBinding")
            cb = (self.handles.get(cb["HandleRefId"]) if "HandleRefId" in cb else cb.get("Data")) if cb else None
            if cb and cb.get("$type") == "entAnimationControlBinding":
                d = self.components.get(cb["bindName"]["$value"], {})
                driver = d["rig"]["DepotPath"]["$value"] if "rig" in d and d is not c else None
            bones = self.rigs.bones(c["rig"]["DepotPath"]["$value"], driver)
        self.posed[name] = bones
        return bones

    def slot_rig(self):
        """The rig vehicle_slots (where AddSlot puts the thrusters) is bound to."""
        vs = self.components.get("vehicle_slots")
        b = self.binding(vs) if vs else None
        for name in ([b["bindName"]["$value"]] if b else []) + ["deformation_rig", "vehicle_rig"]:
            if self.rig_of(name):
                return name
        return None

    def bone(self, component, bone):
        """A component's bone or slot, in model space."""
        c = self.components.get(component, {})
        if c.get("$type") == "entSlotComponent":
            for s in c["slots"]:
                if s["slotName"]["$value"] == bone:
                    b = self.binding(c)
                    rig_name = b["bindName"]["$value"] if b else None
                    pose = (self.rig_of(rig_name) or {}).get(s["boneName"]["$value"], IDENTITY)
                    p, q = s["relativePosition"], s["relativeRotation"]
                    return compose(compose(self.place(rig_name), pose),
                                   ((p["X"], p["Y"], p["Z"]), (q["i"], q["j"], q["k"], q["r"])))
            return None
        pose = (self.rig_of(component) or {}).get(bone)
        return compose(self.place(component), pose) if pose else None

    def place(self, name, depth=0):
        if name in self.placed:
            return self.placed[name]
        c = self.components.get(name)
        if c is None or depth > 20:
            return IDENTITY
        parent = IDENTITY
        b = self.binding(c)
        if b:
            parent_name, slot = b["bindName"]["$value"], b.get("slotName", {}).get("$value", "None")
            parent = self.place(parent_name, depth + 1)
            if slot not in NONE:
                parent = self.bone(parent_name, slot) or parent
        lt = c.get("localTransform")
        local = IDENTITY
        if lt:
            p, q = lt["Position"], lt["Orientation"]
            local = ((p["x"]["Bits"] / 131072, p["y"]["Bits"] / 131072, p["z"]["Bits"] / 131072),
                     (q["i"], q["j"], q["k"], q["r"]))
        self.placed[name] = compose(parent, local)
        return self.placed[name]


def bound_slots(folder, wheel_slots):
    """(.app name, appearance) to the wheel_slots (FlightConfiguration.cpp's) its components are bound to."""
    out = {}
    app_dir = os.path.join(folder, "json", "app")
    for f in sorted(os.listdir(app_dir)) if os.path.isdir(app_dir) else []:
        for a in load_json(os.path.join(app_dir, f))["Data"]["RootChunk"]["appearances"]:
            a = a.get("Data", a)
            text = json.dumps(a)
            out[(f[:-5], a["name"]["$value"])] = frozenset(s for s in wheel_slots if f'"{s}"' in text)
    return out


def configuration(ltbf, template, appearance, kind, bound, bones):
    """The configuration class one record gets, as FlightConfiguration.cpp picks it, or None."""
    if appearance in NONE:
        if template.default_appearance in NONE:
            return ltbf.pick(kind, frozenset())[0]
        picks = [ltbf.pick(kind, bound[a])[0] for a in template.appearances.values() if a in bound]
        if picks:
            # spawned without one, the car gets one of its appearances at random: the usual one wins, ties to the
            # one with more thrusters
            count = Counter(picks)
            return max(count, key=lambda c: (count[c], len(ltbf.config_thrusters(c)) if c else 0))
    elif template.appearances.get(appearance) in bound:
        return ltbf.pick(kind, bound[template.appearances[appearance]])[0]
    # no appearance serialized: the configuration with the most thrusters whose bones the rig has
    slots = [s for s, _ in ltbf.six_slots]
    fits = []
    for mask in range(1 << len(slots)):
        cls = ltbf.pick(kind, frozenset(s for i, s in enumerate(slots) if mask >> i & 1))[0]
        if cls and all(t.bone in bones for t in ltbf.config_thrusters(cls)):
            fits.append((len(ltbf.config_thrusters(cls)), -mask, cls))
    return max(fits)[2] if fits else None


def geometry(ltbf, template, com, bike, classes):
    """tensor, wheel offsets, bottom, thruster count and source for one template and centre of mass, or None."""
    rig_name = template.slot_rig()
    bones = template.rig_of(rig_name) if rig_name else None
    if not bones:
        return None, "no rig"
    cls = Counter(classes).most_common(1)[0][0] if classes else None
    if cls is None:
        return None, "no configuration"
    thrusters = ltbf.config_thrusters(cls)
    if not all(t.bone in bones for t in thrusters):
        return None, f"rig without {cls}'s bones"
    # AddSlot's offset is in the bone's frame
    points = [compose(template.bone(rig_name, t.bone), (t.offset, (0.0, 0.0, 0.0, 1.0)))[0] for t in thrusters]
    if bike:
        emitters, source = BIKE_EMITTERS, f"rig, {cls} (no survey to check against)"
    else:
        emitters, source = CAR_EMITTERS, f"rig, {cls}"
    tensor = [0.0, 0.0, 0.0]
    for p in points:
        v = [p[i] - com[i] for i in range(3)]
        tensor[0] += math.hypot(v[1], v[2])
        tensor[1] += math.hypot(v[0], v[2])
        tensor[2] += math.hypot(v[0], v[1])
    wheels = [template.place(n)[0] for n in emitters if n in template.components]
    wheel, bottom = [0.0, 0.0, 0.0], 0.0
    if wheels:
        wheel = [statistics.mean(abs(w[0] - com[0]) for w in wheels), statistics.mean(abs(w[1] - com[1]) for w in wheels),
                 statistics.mean(w[2] - com[2] for w in wheels)]
        bottom = GROUND - com[2]
    return {"tensorX": tensor[0], "tensorY": tensor[1], "tensorZ": tensor[2], "wheelX": wheel[0], "wheelY": wheel[1],
            "wheelZ": wheel[2], "bottom": bottom, "thrusters": len(points)}, source


def plausible(s):
    try:
        i = [float(s[f"inertia{a}"]) for a in "XYZ"]
        t = [float(s[f"tensor{a}"]) for a in "XYZ"]
    except (KeyError, ValueError):
        return False
    return not (abs(i[0] - i[1]) < 0.01 and abs(i[1] - i[2]) < 0.01) and min(t) > 0.0


def compare(rows, vehicles, model, survey_path, names):
    """Each plausible surveyed record against its row; per template the median of its records' errors."""
    by_template = defaultdict(list)
    for rid, s in read(survey_path).items():
        v = vehicles.get(rid)
        if not v or not plausible(s):
            continue
        g = rows.get((v["entityTemplatePath"], model(v)))
        if g is None:
            continue
        by_template[v["entityTemplatePath"]].append({
            "tensor": max(abs(g[f"tensor{a}"] / float(s[f"tensor{a}"]) - 1) for a in "XYZ"),
            "wheel": max(abs(g[f"wheel{a}"] - float(s[f"wheel{a}"])) for a in "XYZ"),
            "bottom": g["bottom"] - float(s["bottom"]),
            "thrusters": g["thrusters"] == int(float(s["thrusters"]))})
    print(f"against the survey: {sum(len(r) for r in by_template.values())} records of {len(by_template)} templates")
    med = {t: {k: statistics.median(float(r[k]) for r in rs) for k in ("tensor", "wheel", "bottom")} |
           {"thrusters": sum(r["thrusters"] for r in rs) / len(rs)} for t, rs in by_template.items()}
    n = len(med)
    for key, unit, scale, limits in (("tensor", "%", 100, (0.001, 0.005, 0.02)), ("wheel", "m", 1, (0.001, 0.01, 0.03))):
        vals = sorted(m[key] for m in med.values())
        within = ", ".join(f"{sum(v <= lim for v in vals)} within {lim * scale:g}{unit}" for lim in limits)
        print(f"  {key}: median {statistics.median(vals) * scale:.3g}{unit}, 90th {vals[int(0.9 * n)] * scale:.3g}{unit}, "
              f"worst {vals[-1] * scale:.3g}{unit}; {within} (of {n})")
    b = [m["bottom"] for m in med.values()]
    q = statistics.quantiles(b, n=4)
    print(f"  bottom (predicted minus surveyed): median {statistics.median(b):+.3f} m, IQR {q[0]:+.3f} to {q[2]:+.3f} m")
    print(f"  thruster count right on {sum(m['thrusters'] == 1 for m in med.values())} of {n} templates")
    for t, m in sorted(med.items(), key=lambda kv: -kv[1]["tensor"])[:10]:
        print(f"    {names.get(t, t)}: tensor {m['tensor'] * 100:.1f}%, wheel {m['wheel']:.3f} m, "
              f"thrusters right {m['thrusters']:.0%} ({len(by_template[t])} records)")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("dump")
    ap.add_argument("--rigs", default=RIGS)
    ap.add_argument("-o", "--out", default="")
    ap.add_argument("--survey", default="")
    args = ap.parse_args()
    out = args.out or os.path.join(args.rigs, "geometry.tsv")

    vehicles = read(os.path.join(args.dump, "vehicles.tsv"))
    bike_models = read(os.path.join(args.dump, "bike_drive_models.tsv"))
    drive = {**read(os.path.join(args.dump, "drive_models.tsv")), **bike_models}
    with open(os.path.join(args.rigs, "templates.txt"), encoding="utf-8") as f:
        paths = {fnv1a64(line.strip()): line.strip() for line in f if line.strip()}
    # imported here: thruster_slots imports this module (python -I leaves the script's folder off the path)
    sys.path.insert(0, HERE)
    from thruster_slots import Ltbf
    ltbf = Ltbf()
    rigs, bound = Rigs(args.rigs), bound_slots(args.rigs, [s for s, _ in ltbf.six_slots])

    def model(v):
        dm = v.get("vehDriveModelData", "")
        return dm if dm not in NONE else v.get("bikeDriveModelData", "")

    groups = defaultdict(list)
    for rid, v in vehicles.items():
        groups[(v["entityTemplatePath"], model(v))].append(rid)

    templates, rows, skipped = {}, {}, Counter()
    for (tpl, dm), rids in sorted(groups.items()):
        path = paths.get(tpl)
        if tpl in NONE or path is None:
            skipped["template not resolved"] += 1
            continue
        if tpl not in templates:
            root = path.replace("\\", "/").split("/")[0]
            js = os.path.join(args.rigs, "json", root, basename(path) + ".json")
            js = js if os.path.exists(js) else os.path.join(args.rigs, "json", basename(path) + ".json")
            templates[tpl] = Template(js, rigs) if os.path.exists(js) else None
        template = templates[tpl]
        if template is None or dm not in drive:
            skipped["template not serialized" if template is None else "no drive model"] += 1
            continue
        com = [float(x) for x in drive[dm]["center_of_mass_offset"].split(",")[:3]]
        rig_name = template.slot_rig()
        bones = template.rig_of(rig_name) if rig_name else {}
        kind = "bike" if dm in bike_models else "car"
        classes = [configuration(ltbf, template, vehicles[r].get("appearanceName", "None"), kind, bound, bones or {})
                   for r in rids]
        g, source = geometry(ltbf, template, com, kind == "bike", classes)
        if g is None:
            skipped[source] += 1
            continue
        rows[(tpl, dm)] = {**g, "source": source}

    with open(out, "w", encoding="utf-8", newline="\n") as f:
        f.write("\t".join(COLUMNS) + "\n")
        for (tpl, dm), g in rows.items():
            f.write("\t".join([tpl, dm] + [f"{g[c]:.4f}" for c in COLUMNS[2:9]] + [str(g["thrusters"]), g["source"]])
                    + "\n")
    print(f"{len(groups)} template and drive model pairs: {len(rows)} to {out}; skipped {dict(skipped)}")
    if args.survey:
        compare(rows, vehicles, model, args.survey, {t: basename(p) for t, p in paths.items()})


if __name__ == "__main__":
    main()
