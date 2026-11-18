"""Checks that LTBF puts every flight thruster on a proper slot of every vanilla car and bike, without the game.

    python -I tools/flightsim/thruster_slots.py [--fixture <tsv>] [--report <md>] [-v]

The vehicles' facts come from vehicle_slots.tsv (vehicle_slots.py writes it from the game files). Everything about
LTBF comes from its own sources, never copied here:
- which configuration class a vehicle gets: FlightConfiguration.cpp, IFlightConfiguration::GetConfigurationClass
  (the six-wheel slot names, the class names it tries in order, the FlightConfiguration_<appearance> prefix), and
  which of those classes the redscript defines (commented-out code does not count);
- each configuration's thrusters: the `new <Thruster>().Create` lines of its class and the classes it extends;
- each thruster's bone, offset and the wheel it hides: its `AddSlot(n"<bone>", ..., new Vector3(...), ...)`,
  `parentSlotName`, `brakeSlotName`, isFront / isRight, and, where it overrides SetOGComponents, the slots and component names that
  one hides by (GetComponentsUsingSlot, FindComponentByName) in place of parentSlotName.

For every template and appearance group it works out the class, the thrusters and their positions, and fails, with
a list, when a configuration needs a bone the rig lacks or a wheel slot vehicle_slots lacks, when a sanity check is
out of tolerance, when a wheel, tyre or brake is left showing with no thruster over it, or when the thruster count differs
from what the survey measured in game. ALLOW holds what it lets pass, each with its reason: vanilla oddities that
are fine, and known LTBF gaps (an entry that matches nothing any more fails the check until it is removed).
--report writes the whole audit as markdown. ctest runs it (thruster_slots).
"""

import argparse
import math
import os
import re
import sys
from collections import Counter, defaultdict

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import rig_geometry as rg  # noqa: E402

REPO = os.path.dirname(os.path.dirname(HERE))
REDS = os.path.join(REPO, "src", "redscript", "Flight")
CPP = os.path.join(REPO, "src", "red4ext", "FlightConfiguration.cpp")
FIXTURE = os.path.join(HERE, "vehicle_slots.tsv")

# tolerances, in metres
WHEEL_DISTANCE = 0.25  # a car thruster from the centre of the wheel it replaces
MIRROR = 0.03  # a left thruster from its right one mirrored in X
BIKE_X = 0.03  # a bike thruster from the centre line

# What the check finds and lets pass: (template file or rig set, appearance group or "*", check) to the reason. An
# appearance group "" matches every group of the template. Checks: bone, slot, mirror, side, order, nowheel, wheel,
# bare, showing, survey, bike, class. Two kinds of entry: vanilla oddities that are fine, and LTBF gaps that are
# known and not fixed yet ("LTBF gap"); when a gap is fixed its entry matches nothing and the check fails until the
# entry is removed.
VANILLA = "vanilla oddity, fine"
GAP = "LTBF gap, known"
ALLOW = {
    # survey readings with thrusters missing: one record each, its tensor down in proportion to the count, while
    # the other records of the same car read 4 (thrusters are detachable)
    ("v_sport1_quadra_turbo__basic_01.ent", "*", "survey"): VANILLA + ": one Turbo-R record read 2 thrusters, "
                                                                     "tensor half the car's: two had come off",
    ("v_standard2_thorton_colby__basic_01.ent", "*", "survey"): VANILLA + ": one Vaquero record read 3, tensor 3/4 "
                                                                         "of the other five: one had come off",
    ("v_standard3_thorton_mackinaw_01__basic_01.ent", "*", "survey"): VANILLA + ": one Mackinaw record read 3: "
                                                                               "one had come off",
    # a quest gurney spawned as a car: no wheel meshes, tyre radius 0.1, its rig is the generic x4 base
    ("q101_wheelstretchers_vehicle.ent", "", "nowheel"): VANILLA + ": q101's wheeled stretcher, a car only by class",
    # the burnt Delamain wreck has lost its back wheels
    ("q005_delamain_cab_combat.ent", "villefort_cortes__basic_burnt_01", "nowheel"): VANILLA + ": burnt wreck "
                                                                                              "without back wheels",
}


# ---------------------------------------------------------------- LTBF's sources

def strip_comments(text):
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.S)
    return re.sub(r"//[^\n]*", "", text)


def class_bodies(text):
    """{class name: (base class, body)} of every class in a redscript source with its comments removed."""
    out = {}
    for m in re.finditer(r"\bclass\s+(\w+)\s+extends\s+(\w+)\s*\{", text):
        depth, i = 1, m.end()
        while depth and i < len(text):
            depth += {"{": 1, "}": -1}.get(text[i], 0)
            i += 1
        out[m.group(1)] = (m.group(2), text[m.end():i - 1])
    return out


class Thruster:
    def __init__(self, name, body):
        self.name = name
        add = re.search(r'AddSlot\(\s*n"(\w+)"\s*,[^,]*,\s*new\s+Vector3\(([^)]*)\)', body)
        self.bone = add.group(1) if add else None
        self.offset = tuple(float(v.strip().rstrip("fd")) for v in add.group(2).split(",")) if add else (0.0, 0.0, 0.0)
        parent = re.search(r'parentSlotName\s*=\s*n"(\w+)"', body)
        self.parent_slot = parent.group(1) if parent else None
        self.front = bool(re.search(r"isFront\s*=\s*true", body))
        self.right = bool(re.search(r"isRight\s*=\s*true", body))
        og = re.search(r"func\s+SetOGComponents\s*\(", body)
        if og:  # hides by its own list in place of parentSlotName
            rest = body[og.end():]
            self.hide_slots = re.findall(r'GetComponentsUsingSlot\(\s*n"(\w+)"', rest)
            self.hide_names = re.findall(r'FindComponentByName\(\s*n"(\w+)"', rest)
        else:
            brake = re.search(r'brakeSlotName\s*=\s*n"(\w+)"', body)
            self.hide_slots = [m.group(1) for m in (parent, brake) if m]
            self.hide_names = []


class Ltbf:
    """What LTBF's sources say about configurations and thrusters."""

    def __init__(self, reds=REDS, cpp=CPP):
        classes = {}
        for folder, _, files in os.walk(reds):
            for f in files:
                if f.endswith(".reds"):
                    with open(os.path.join(folder, f), encoding="utf-8") as fh:
                        classes.update(class_bodies(strip_comments(fh.read())))
        self.classes = classes
        self.thrusters = {n: Thruster(n, b) for n, (base, b) in classes.items() if self.extends(n, "IFlightThruster")}
        self.configurations = {n for n in classes if self.extends(n, "IFlightConfiguration")}

        with open(cpp, encoding="utf-8") as f:
            src = strip_comments(f.read())
        m = re.search(r"GetConfigurationClass\s*\([^)]*\)\s*\{", src)
        body = src[m.end():]
        body = body[:re.search(r"\n\}", body).start()]
        # the six-wheel flags: the slot names, in the order of the bits they set
        self.six_slots = [(s, int(b)) for s, b in
                          re.findall(r'slotName\s*==\s*"(\w+)"\s*\)\s*\{\s*isSixWheeler\s*\|=\s*(\d+)', body)]
        prefix = re.search(r'"(\w+_)%s"', body)
        self.override_prefix = prefix.group(1) if prefix else None
        # the branches in order: (condition, [class names tried in order])
        self.branches = []
        for cond, seg in re.findall(r"if\s*\(([^)]*)\)\s*\{(.*?)\n\s{4}\}", body[body.index("if (!configurationCls)") + 23:],
                                    flags=re.S):
            names = re.findall(r'GetClassByScriptName\("(\w+)"\)', seg)
            if names:
                self.branches.append((cond.strip(), names))

    def extends(self, name, base):
        seen = set()
        while name in self.classes and name not in seen:
            seen.add(name)
            name = self.classes[name][0]
            if name == base:
                return True
        return False

    def pick(self, kind, bound):
        """The configuration class FlightConfiguration.cpp picks, or None, and why."""
        bits = 0
        for slot, bit in self.six_slots:
            if slot in bound:
                bits |= bit
        flags = {"isCar": kind == "car", "isBike": kind == "bike"}
        for cond, names in self.branches:
            m = re.match(r"isSixWheeler\s*&\s*(\d+)", cond)
            exact = re.match(r"isSixWheeler\s*==\s*(\d+)", cond)
            if (m and bits & int(m.group(1))) or (exact and bits == int(exact.group(1))) or flags.get(cond):
                for n in names:
                    if n in self.configurations:
                        return n, cond
                return None, f"{cond}: none of {', '.join(names)} is defined"
        return None, "not a car or bike"

    def config_thrusters(self, cls):
        out = []
        chain = []
        while cls in self.classes:
            chain.append(cls)
            body = self.classes[cls][1]
            if not re.search(r"super\.OnSetup\(", body):
                break
            cls = self.classes[cls][0]
        for c in reversed(chain):
            out += [self.thrusters[n] for n in re.findall(r"new\s+(\w+)\(\)\.Create", self.classes[c][1])
                    if n in self.thrusters]
        return out

    def overrides(self):
        return sorted(n for n in self.configurations if self.override_prefix and n.startswith(self.override_prefix))


# ---------------------------------------------------------------- the fixture

class Group:
    def __init__(self, label, data, bike):
        parts = data.split()
        self.label = label
        self.bound = set() if parts[0] in ("-", "?", "*") else set(parts[0].split(","))
        self.missing_app = parts[0] == "?"
        self.random = parts[0] == "*"
        self.parts = []
        if bike and not self.missing_app and not self.random:
            self.parts = [] if parts[1] == "-" else [p.rsplit("@", 1) for p in parts[1].split(",")]
            parts = parts[1:]
        self.records = int(parts[1])
        self.surveyed = Counter() if parts[2] == "-" else Counter(
            {int(k): int(c) for k, c in (x.split("x") for x in parts[2].split(","))})


class Vehicle:
    def __init__(self, tpl, name, data):
        f = data.split(" ", 6)
        self.tpl, self.file, self.cls, self.kind, self.rig = tpl, name, f[0], f[1], f[2]
        self.records = int(f[3])
        self.radius = (float(f[4]), float(f[5]))
        self.display = f[6] if len(f) > 6 else ""
        self.groups = []

    def title(self):
        return f"{self.file}" + (f" ({self.display})" if self.display else "")


def load_fixture(path):
    bones, slots, missing, vehicles = defaultdict(dict), defaultdict(dict), defaultdict(set), {}
    with open(path, encoding="utf-8") as f:
        for line in f:
            if line.startswith("#") or line.startswith("template\t"):
                continue
            key, kind, name, data = line.rstrip("\n").split("\t")
            if kind == "B":
                v = [float(x) for x in data.split()]
                bones[key][name] = (tuple(v[:3]), tuple(v[3:]) if len(v) == 7 else (0.0, 0.0, 0.0, 1.0))
            elif kind == "S":
                if name == "missing":
                    missing[key].update(data.split(","))
                    continue
                v = data.split()
                slots[key][name] = tuple(float(x) for x in v[-3:])
            elif kind == "T":
                vehicles[key] = Vehicle(key, name, data)
            elif kind == "A":
                vehicles[key].groups.append(Group(name, data, vehicles[key].kind == "bike"))
    return bones, slots, missing, vehicles


# ---------------------------------------------------------------- the audit

def fmt(p):
    return "(" + ", ".join(f"{v:+.2f}" for v in p) + ")"


class Audit:
    def __init__(self, ltbf, fixture):
        self.ltbf = ltbf
        self.bones, self.slots, self.missing, self.vehicles = load_fixture(fixture)
        self.problems = []  # (vehicle, group label, check, text)
        self.allowed = []  # (problem, reason)
        self.rows = []  # one per vehicle and group, for the report
        self.used_allow = set()
        self.notes = []  # facts worth reading that are not problems

    def problem(self, v, g, check, text):
        for key in ((v.file, g, check), (v.file, "", check), (v.rig, "", check)):
            if key in ALLOW:
                self.used_allow.add(key)
                self.allowed.append(((v, g, check, text), ALLOW[key]))
                return
        self.problems.append((v, g, check, text))

    def run(self):
        for v in self.vehicles.values():
            if v.kind not in ("car", "bike"):
                continue
            outcomes = {}
            for g in v.groups:
                if g.missing_app:
                    self.notes.append(f"{v.title()}: appearances {g.label} are not in their .app file")
                if g.random or g.missing_app:
                    continue
                outcomes[g.label] = self.group(v, g)
            for g in v.groups:
                if g.random and g.surveyed:
                    possible = {n for n in outcomes.values() if n is not None}
                    if not possible:
                        self.notes.append(f"{v.title()}: {sum(g.surveyed.values())} surveyed records with no "
                                          "appearance named and no appearance of the template to compare them with")
                        continue
                    for count, c in g.surveyed.items():
                        if count not in possible:
                            self.problem(v, "*", "survey", f"{c} records spawned with no appearance named measured "
                                         f"{count} thrusters, the appearances give {sorted(possible)}")

    def group(self, v, g):
        bones, slots = self.bones[v.rig], self.slots[v.rig]
        cls, why = self.ltbf.pick(v.kind, g.bound)
        override = [self.ltbf.override_prefix + a for a in g.label.split(",")
                    if self.ltbf.override_prefix and self.ltbf.override_prefix + a in self.ltbf.configurations]
        if override:
            cls, why = override[0], "appearance override"
        row = {"vehicle": v, "group": g, "class": cls, "why": why, "thrusters": [], "notes": []}
        self.rows.append(row)
        if cls is None:
            self.problem(v, g.label, "class", f"no configuration: {why}")
            return None
        thrusters = self.ltbf.config_thrusters(cls)
        placed = []
        for t in thrusters:
            if t.bone not in bones:
                self.problem(v, g.label, "bone", f"{cls}: {t.name} needs bone {t.bone}, the rig has none"
                             + (" (vehicle_slots has a slot of that name with no bone)" if t.bone in self.missing[v.rig]
                                else ""))
                row["thrusters"].append((t, None, None))
                continue
            p, q = bones[t.bone]
            pos = tuple(a + b for a, b in zip(p, rg.qrot(q, t.offset)))
            # a bike thruster hides its wheel by its own list (SetOGComponents), never by parentSlotName: its
            # reference is the first of those slots the rig has (axle_back is the swingarm's pivot, not the wheel)
            wheel_slots = t.hide_slots if v.kind == "bike" else [t.parent_slot] + t.hide_slots
            ref = next((s for s in wheel_slots if s in slots), None)
            if v.kind == "car":
                if t.parent_slot not in slots:
                    self.problem(v, g.label, "slot", f"{t.name}: wheel slot {t.parent_slot} "
                                 + ("has no bone in the rig" if t.parent_slot in self.missing[v.rig] else "is not in vehicle_slots"))
                elif t.parent_slot not in g.bound:
                    self.problem(v, g.label, "nowheel", f"{t.name}: nothing is bound to {t.parent_slot}: no wheel "
                                 "under it to hide")
            if ref is None:  # bikes: the nearest wheel slot at the same end
                cands = [(math.dist(pos, s), n) for n, s in slots.items() if n.startswith("wheel")
                         and (s[1] > 0) == t.front]
                ref = min(cands)[1] if cands else None
            row["thrusters"].append((t, pos, ref))
            placed.append((t, pos, ref))
        self.sanity(v, g, placed, slots)
        if v.kind == "car":
            self.bare_car(v, g, thrusters)
        else:
            self.bare_bike(v, g, thrusters, row)
        count = len(placed)
        for n, c in g.surveyed.items():
            if n != count:
                self.problem(v, g.label, "survey", f"{c} surveyed records measured {n} thrusters, {cls} gives {count}")
        return count

    def sanity(self, v, g, placed, slots):
        by_bone = {t.bone: (t, p) for t, p, _ in placed}
        for t, p, ref in placed:
            w = slots.get(ref)
            if v.kind == "car":
                if w and math.dist(p, w) > WHEEL_DISTANCE:
                    self.problem(v, g.label, "wheel", f"{t.name} at {fmt(p)} is {math.dist(p, w):.2f} m from {ref} "
                                 f"{fmt(w)}")
                if (p[0] > 0) != t.right:
                    self.problem(v, g.label, "side", f"{t.name} (isRight {t.right}) at x {p[0]:+.2f}")
                if "left" in t.bone and t.bone.replace("left", "right") in by_bone:
                    _, r = by_bone[t.bone.replace("left", "right")]
                    d = max(abs(p[0] + r[0]), abs(p[1] - r[1]), abs(p[2] - r[2]))
                    if d > MIRROR:
                        self.problem(v, g.label, "mirror", f"{t.bone} {fmt(p)} and its right {fmt(r)} differ by "
                                     f"{d:.3f} m mirrored")
            else:
                radius = v.radius[0 if t.front else 1] or 0.3
                if abs(p[0]) > BIKE_X:
                    self.problem(v, g.label, "bike", f"{t.name} at x {p[0]:+.3f}, off the centre line")
                if w is None:
                    self.problem(v, g.label, "bike", f"{t.name}: no wheel slot to check it against")
                    continue
                dz = p[2] - w[2]
                if not (-radius < dz < 0.0):
                    self.problem(v, g.label, "bike", f"{t.name} at {fmt(p)} is {dz:+.2f} m from the axle ({ref}, "
                                 f"{fmt(w)}): not between the axle and the ground (tyre radius {radius:.2f})")
                if math.dist(p, w) > radius:
                    self.problem(v, g.label, "bike", f"{t.name} at {fmt(p)} is {math.dist(p, w):.2f} m from the axle "
                                 f"({ref}), outside the tyre (radius {radius:.2f})")
        fronts = [p[1] for t, p, _ in placed if t.front]
        backs = [p[1] for t, p, _ in placed if not t.front]
        if fronts and backs and min(fronts) <= max(backs):
            self.problem(v, g.label, "order", f"a front thruster (y {min(fronts):+.2f}) is not ahead of a back one "
                         f"(y {max(backs):+.2f})")

    def bare_car(self, v, g, thrusters):
        hidden = {s for t in thrusters for s in t.hide_slots}
        for s in sorted(g.bound):
            if s.startswith("wheel") and s not in hidden and s in self.slots[v.rig]:
                self.problem(v, g.label, "bare", f"a wheel is bound to {s} {fmt(self.slots[v.rig][s])} and no thruster "
                             "replaces it")
            elif s.startswith("brake") and s not in hidden:
                self.problem(v, g.label, "bare", f"a brake is bound to {s} and no thruster hides it")

    def bare_bike(self, v, g, thrusters, row):
        names = {n for t in thrusters for n in t.hide_names}
        slots = {s for t in thrusters for s in t.hide_slots}
        for name, slot in g.parts:
            if name in names or slot in slots:
                continue
            if re.search(r"wheel|tire|tyre", name):
                self.problem(v, g.label, "showing", f"{name} (on {slot}) is not hidden: the wheel shows beside the "
                             "thruster")
            else:
                row["notes"].append(f"{name} (on {slot}) stays")


# ---------------------------------------------------------------- output

def report(audit, path):
    L = audit.ltbf
    vs = list(audit.vehicles.values())
    out = ["# Thruster slot audit", "",
           "Written by `tools/flightsim/thruster_slots.py --report` from `tools/flightsim/vehicle_slots.tsv` and LTBF's "
           "sources. Positions are model space (x right, y forward, z up, metres), bind pose.", ""]
    kinds = Counter(v.kind for v in vs)
    groups = sum(len(v.groups) for v in vs if v.kind in ("car", "bike"))
    out += [f"- {len(vs)} templates named by vanilla vehicle records: {kinds['car']} cars, {kinds['bike']} bikes, "
            f"{kinds['none']} with no configuration (AVs, drones, trains, props), {kinds['-']} whose path did not "
            "resolve.",
            f"- {len(audit.rows)} appearance groups checked on the cars and bikes (appearances with the same wheel "
            f"bindings grouped), of {groups}: the rest are records spawned with no appearance named (checked against "
            "the survey only) and appearances missing from their .app (notes).",
            f"- Six-wheel slots (FlightConfiguration.cpp): {', '.join(f'{s} (bit {b})' for s, b in L.six_slots)}.",
            f"- Configuration classes defined: {', '.join(sorted(L.configurations))}. Overrides "
            f"`{L.override_prefix}<appearance>`: {', '.join(L.overrides()) or 'none'}.",
            f"- Problems: {len(audit.problems)}; allowed: {sum(r.startswith(VANILLA) for _, r in audit.allowed)} "
            f"vanilla oddities, {sum(r.startswith(GAP) for _, r in audit.allowed)} known LTBF gaps.", ""]
    out += ["## Thrusters per configuration", ""]
    for cls in sorted({r["class"] for r in audit.rows if r["class"]}):
        out.append(f"- `{cls}`: " + "; ".join(
            f"{t.name} on `{t.bone}` {fmt(t.offset)}, hides "
            + ", ".join([f"slot `{s}`" for s in t.hide_slots] + [f"`{n}`" for n in t.hide_names])
            for t in L.config_thrusters(cls)))
    out.append("")

    by_class = defaultdict(lambda: [0, 0, 0, 0, set()])
    bad = {(p[0].tpl, p[1]) for p in audit.problems}
    gap = {(a[0][0].tpl, a[0][1]) for a in audit.allowed if a[1].startswith(GAP)}
    odd = {(a[0][0].tpl, a[0][1]) for a in audit.allowed if a[1].startswith(VANILLA)}
    for r in audit.rows:
        k, key = r["class"] or "none", (r["vehicle"].tpl, r["group"].label)
        by_class[k][1 if key in bad else 2 if key in gap else 3 if key in odd else 0] += 1
        by_class[k][4].add(r["vehicle"].tpl)
    out += ["## Per configuration", "",
            "| configuration | templates | groups clean | with problems | with a known LTBF gap | with a vanilla oddity |",
            "|---|---|---|---|---|---|"]
    for k, (ok, nok, g, o, t) in sorted(by_class.items()):
        out.append(f"| {k} | {len(t)} | {ok} | {nok} | {g} | {o} |")
    out.append("")
    for cls in ("SixWheelCarFlightConfiguration", "SixWheelRearCarFlightConfiguration"):
        names = sorted({r["vehicle"].title() for r in audit.rows if r["class"] == cls})
        out.append(f"- `{cls}`: {', '.join(names) or 'none'}.")
    out += ["", "## Bike setups", "",
            "Each bike rig: the wheel and axle slots and bones, where the thrusters land, and the wheel parts of its "
            "usual appearance with how BikeThrusters finds them (by name, by slot, or not at all).", ""]
    seen = set()
    for r in sorted(audit.rows, key=lambda r: (r["vehicle"].rig, -len(r["group"].parts))):
        v, g = r["vehicle"], r["group"]
        if v.kind != "bike" or v.rig in seen or not g.parts:
            continue
        seen.add(v.rig)
        ths = [t for t, _, _ in r["thrusters"]]
        names = {n for t in ths for n in t.hide_names}
        slots = {s for t in ths for s in t.hide_slots}
        parts = ", ".join(f"`{n}`@`{sl}` " + ("(name)" if n in names else "(slot)" if sl in slots else "**(not hidden)**")
                          for n, sl in g.parts)
        wheel = ", ".join(f"`{n}` {fmt(p)}" for n, p in sorted(audit.slots[v.rig].items()))
        pos = "; ".join(f"{t.name} {fmt(p)} ~{ref}" for t, p, ref in r["thrusters"] if p)
        out.append(f"- `{v.rig}` (e.g. {v.title()} [{g.label[:60]}]): slots and bones {wheel}. Thrusters: {pos}. "
                   f"Parts: {parts}")
    out.append("")

    out += ["## Problems", ""]
    if not audit.problems:
        out.append("None.")
    for v, g, check, text in sorted(audit.problems, key=lambda p: (p[2], p[0].file)):
        out.append(f"- **{check}** {v.title()} [{g}]: {text}")
    out += ["", "## Allowed oddities", ""]
    for (v, g, check, text), reason in sorted(audit.allowed, key=lambda a: (a[0][2], a[0][0].file)):
        out.append(f"- **{check}** {v.title()} [{g}]: {text}. Allowed: {reason}")
    if not audit.allowed:
        out.append("None.")

    out += ["", "## Notes", ""] + [f"- {n}" for n in audit.notes] + (["None."] if not audit.notes else [])
    out += ["", "## Every car and bike", "",
            "One line per template and appearance group: the class, then each thruster's position and the wheel slot "
            "it was checked against. A group named `N_appearances` is the template's usual outcome; `*` is records "
            "spawned with no appearance named.", ""]
    for r in sorted(audit.rows, key=lambda r: (r["vehicle"].kind, r["vehicle"].file, r["group"].label)):
        v, g = r["vehicle"], r["group"]
        ths = "; ".join(f"{t.name} {fmt(p) if p else 'MISSING ' + str(t.bone)}" + (f" ~{ref}" if ref else "")
                        for t, p, ref in r["thrusters"])
        sv = (" surveyed " + ",".join(f"{k}x{c}" for k, c in sorted(g.surveyed.items()))) if g.surveyed else ""
        out.append(f"- {v.title()} `{v.rig}` [{g.label[:120]}] {g.records} records{sv}: **{r['class'] or r['why']}** "
                   f"{ths}" + (f". Kept: {'; '.join(r['notes'])}" if r["notes"] else ""))
    out += ["", "## No configuration", ""]
    for v in sorted(vs, key=lambda v: (v.kind, v.cls, v.file)):
        if v.kind not in ("car", "bike"):
            out.append(f"- {v.title() if v.file != '?' else v.tpl + ' (path not resolved)'}: {v.cls}, {v.records} records")
    with open(path, "w", encoding="utf-8", newline="\n") as f:
        f.write("\n".join(out) + "\n")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--fixture", default=FIXTURE)
    ap.add_argument("--report", default="")
    ap.add_argument("-v", "--verbose", action="store_true")
    args = ap.parse_args()

    ltbf = Ltbf()
    errors = []
    if not ltbf.six_slots or len(ltbf.branches) < 4 or not ltbf.thrusters:
        errors.append(f"could not read LTBF's sources: six-wheel slots {ltbf.six_slots}, branches {ltbf.branches}, "
                      f"{len(ltbf.thrusters)} thruster classes")
    for t in ltbf.thrusters.values():
        if t.bone is None:
            errors.append(f"{t.name}: no AddSlot line found")
    if errors:
        print("\n".join(errors))
        return 2
    audit = Audit(ltbf, args.fixture)
    audit.run()
    if args.report:
        report(audit, args.report)
    stale = sorted(set(ALLOW) - audit.used_allow)
    cars = [v for v in audit.vehicles.values() if v.kind in ("car", "bike")]
    print(f"thruster_slots: {len(cars)} car and bike templates, {len(audit.rows)} appearance groups, "
          f"{len(audit.problems)} problems, {len(audit.allowed)} allowed")
    if args.verbose:
        for (v, g, check, text), reason in audit.allowed:
            print(f"  allowed {check} {v.title()} [{g}]: {text} ({reason})")
    for v, g, check, text in audit.problems:
        print(f"  {check} {v.title()} [{g[:80]}]: {text}")
    for k in stale:
        print(f"  allow-list entry matched nothing (remove it): {k}")
    if ltbf.overrides():
        print(f"  note: {', '.join(ltbf.overrides())} replace the configuration for one appearance; their bones are "
              "checked only where the fixture names that appearance")
    return 1 if audit.problems or stale else 0


if __name__ == "__main__":
    sys.exit(main())
