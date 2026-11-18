"""The rig facts thruster_slots.py checks LTBF's thruster placement against, from the game files, nothing spawned.

    python -I tools/flightsim/vehicle_slots.py [<rigs dir>] [--dump <dir>] [--survey <measured.tsv>] [-o <tsv>]

<rigs dir> is what rig_geometry.py reads (templates.txt, json/<name>.ent.json, json/<name>.rig.json), plus
json/app_all/<base|ep1>/<name>.app.json: every appearance file a car or bike template names, serialized with
WolvenKit (the appearance's components decide the six-wheel class and which wheels LTBF can hide). --dump is
vehicle_tweaks.py's TweakDB dump (vehicles.tsv, wheel_setups.tsv, wheel_presets.tsv), --survey vehicle_survey.py's
measured.tsv. Rerun after a game patch, after unbundling and serializing the files again.

Writes tools/flightsim/vehicle_slots.tsv, one fact per line, columns template (FNV-1a 64 of the .ent path), kind,
name, data:
- T  <template file>  class, car|bike|none, its rig set (the B and S rows' key), records, tyre radius front and back
- B  <bone>           a swingarm or suspension bone's bind pose in model space, x y z [qi qj qk qr when it is
                       turned], in the rig vehicle_slots is bound to (rig_geometry.py)
  B and S rows are keyed by the rig's file name, not the template: templates with the same facts share them.
- S  <slot>           a vehicle_slots slot named like a wheel or axle: [its bone, when named otherwise] x y z;
                       S missing lists the slots whose bone the rig lacks (they sit on the rig's origin);
                       a wheel or axle bone no slot is named after is an S row too, its bone given as "bone"
- A  <appearances>    one group of appearances (comma separated; "None" is the entity with no appearance loaded)
                       with the same outcome: the wheel, axle and brake names its components are bound to (template and
                       appearance components, entHardTransformBinding slotName), for a bike its wheel parts
                       (component@slot), records, surveyed thruster counts (count x records). The usual outcome's
                       appearances are written as a count (30_appearances), "*" is the records spawned with no
                       appearance named (they get one of the others), "?" an appearance its .app does not have
The survey's display name, where one was surveyed, is the T row's last field.
"""

import argparse
import os
import re
import sys
from collections import Counter, defaultdict

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import rig_geometry as rg  # noqa: E402

FLIGHTSIM_WORK = os.path.join(rg.machine.WORK, "flightsim")  # the work folder's flightsim folder
RIGS = os.path.join(FLIGHTSIM_WORK, "rigs")
DUMP = os.path.join(FLIGHTSIM_WORK, "tweaks_vanilla")
SURVEY = os.path.join(FLIGHTSIM_WORK, "survey", "measured.tsv")
OUT = os.path.join(HERE, "vehicle_slots.tsv")
# the game's class tree, as far as LTBF's IsOfClass checks go (RED4ext.SDK: ArmedCarBaseObject : CarBaseObject)
KIND = {"vehicleCarBaseObject": "car", "vehicleArmedCarBaseObject": "car", "vehicleBikeBaseObject": "bike"}
# bones a thruster can be put on, and the slots and bones a wheel's components hang from
THRUSTER_BONE = re.compile(r"^(swingarm|suspension)")
WHEELISH = re.compile(r"^(wheel|axle|axel|tire|tyre)")
BRAKE = re.compile(r"^brake")  # a disc and caliper of their own
WHEEL_PART = re.compile(r"wheel|tire|tyre|axle|axel")
NOTHING = "-"


def num(values):
    return " ".join(f"{v:.4f}".rstrip("0").rstrip(".") if abs(v) >= 5e-5 else "0" for v in values)


def handles_of(root):
    out = {}

    def walk(o):
        if isinstance(o, dict):
            if "HandleId" in o:
                out[o["HandleId"]] = o.get("Data")
            for v in o.values():
                walk(v)
        elif isinstance(o, list):
            for v in o:
                walk(v)
    walk(root)
    return out


def bindings(components, handles):
    """(component, slot) for each component with an entHardTransformBinding: the slot or bone it hangs from, as
    GetComponentsUsingSlot compares them, "None" when it hangs from the parent component itself."""
    out = set()
    for c in components:
        c = c.get("Data", c) if isinstance(c, dict) and "HandleId" in c else c
        if not isinstance(c, dict) or "name" not in c:
            continue
        p = c.get("parentTransform")
        b = (handles.get(p["HandleRefId"]) if "HandleRefId" in p else p.get("Data")) if p else None
        if b and b.get("$type") == "entHardTransformBinding":
            out.add((c["name"]["$value"], b.get("slotName", {}).get("$value", "None")))
    return out


def outcome_of(pairs, bike):
    """An appearance's A row fields: the wheel, axle and brake slots something is bound to, and for a bike its wheel parts
    (name@slot: bound to a wheel or axle slot, or named like a wheel, tyre or axle; shadows left out), which
    BikeThrusters hides by name or by slot."""
    slots = sorted({s for _, s in pairs if WHEELISH.search(s) or BRAKE.search(s)})
    text = ",".join(slots) or NOTHING
    if bike:
        parts = sorted(f"{n}@{s}" for n, s in pairs if not n.endswith("_shadow")
                       and (WHEELISH.search(s) or WHEEL_PART.search(n)))
        text += " " + (",".join(parts) or NOTHING)
    return text


def appearance_file(rigs, depot_path):
    root = depot_path.replace("\\", "/").split("/")[0]
    return os.path.join(rigs, "json", "app_all", root, rg.basename(depot_path) + ".json")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("rigs", nargs="?", default=RIGS)
    ap.add_argument("--dump", default=DUMP)
    ap.add_argument("--survey", default=SURVEY)
    ap.add_argument("-o", "--out", default=OUT)
    args = ap.parse_args()

    vehicles = rg.read(os.path.join(args.dump, "vehicles.tsv"))
    setups = rg.read(os.path.join(args.dump, "wheel_setups.tsv"))
    presets = rg.read(os.path.join(args.dump, "wheel_presets.tsv"))
    survey = rg.read(args.survey) if os.path.exists(args.survey) else {}
    with open(os.path.join(args.rigs, "templates.txt"), encoding="utf-8") as f:
        paths = {rg.fnv1a64(line.strip()): line.strip() for line in f if line.strip()}
    rigs = rg.Rigs(args.rigs)

    records = defaultdict(list)
    for rid, v in vehicles.items():
        records[v["entityTemplatePath"]].append(rid)

    apps = {}

    def app_appearances(depot_path):
        if depot_path not in apps:
            path = appearance_file(args.rigs, depot_path)
            out = None
            if os.path.exists(path):
                out = {}
                for a in rg.load_json(path)["Data"]["RootChunk"]["appearances"]:
                    a = a.get("Data", a)
                    out[a["name"]["$value"]] = bindings(a.get("components", []), handles_of(a))
            apps[depot_path] = out
        return apps[depot_path]

    def radius(rids, which):
        values = Counter()
        for r in rids:
            s = setups.get(vehicles[r].get("vehWheelDimensionsSetup", ""))
            p = presets.get(s[which]) if s else None
            if p:
                values[round(float(p["tireRadius"]), 3)] += 1
        return values.most_common(1)[0][0] if values else 0.0

    lines, unresolved, rig_sets, rig_lines = [], 0, {}, []
    for tpl in sorted(records, key=lambda t: paths.get(t, "~" + t)):
        rids = records[tpl]
        path = paths.get(tpl)
        if tpl in rg.NONE or path is None:
            unresolved += 1
            lines.append([tpl, "T", "?", f"unresolved {NOTHING} {NOTHING} {len(rids)} 0 0"])
            continue
        root = path.replace("\\", "/").split("/")[0]
        js = os.path.join(args.rigs, "json", root, rg.basename(path) + ".json")
        js = js if os.path.exists(js) else os.path.join(args.rigs, "json", rg.basename(path) + ".json")
        raw = rg.load_json(js)
        cls = raw["Data"]["RootChunk"]["entity"]["Data"]["$type"]
        kind = KIND.get(cls, "none")
        names = Counter(survey[r]["name"] for r in rids if r in survey)
        display = names.most_common(1)[0][0] if names else ""
        if kind == "none":
            lines.append([tpl, "T", rg.basename(path), f"{cls} none {NOTHING} {len(rids)} 0 0" + (f" {display}" if display else "")])
            continue
        t = rg.Template(js, rigs)
        rig_name = t.slot_rig()
        bones = t.rig_of(rig_name) if rig_name else {}
        rows = []
        for b in sorted(n for n in (bones or {}) if THRUSTER_BONE.search(n)):
            p, q = t.bone(rig_name, b)
            rows.append(["B", b, num(p) + ("" if max(abs(v) for v in q[:3]) < 5e-5 else " " + num(q))])
        missing = []
        for s in t.components.get("vehicle_slots", {}).get("slots", []):
            name, bone = s["slotName"]["$value"], s["boneName"]["$value"]
            if not WHEELISH.search(name):
                continue
            if bone not in (bones or {}):
                missing.append(name)
                continue
            rows.append(["S", name, ("" if bone == name else bone + " ") + num(t.bone("vehicle_slots", name)[0])])
        if missing:
            rows.append(["S", "missing", ",".join(missing)])
        # components can hang from a rig bone by name too (the Aras has no wheel slots): those bones, where no slot
        # has the name, as "bone" rows
        named = {r[1] for r in rows if r[0] == "S"} | set(missing)
        for b in sorted(n for n in (bones or {}) if WHEELISH.search(n) and n not in named):
            rows.append(["S", b, "bone " + num(t.bone(rig_name, b)[0])])
        # templates with the same rig facts (a quest copy of a car) share one set of B and S rows, named after the rig
        sig = tuple(map(tuple, rows))
        if sig not in rig_sets:
            label = rg.basename(t.components[rig_name]["rig"]["DepotPath"]["$value"]) if rig_name else NOTHING
            while label in rig_sets.values():
                label += "+"
            rig_sets[sig] = label
            rig_lines.extend([label] + r for r in rows)
        lines.append([tpl, "T", rg.basename(path), f"{cls} {kind} {rig_sets[sig]} {len(rids)} "
                      f"{radius(rids, 'frontPreset'):g} {radius(rids, 'backPreset'):g}" + (f" {display}" if display else "")])

        # the components every appearance has (the template's), then each appearance's own
        own = bindings(raw["Data"]["RootChunk"]["components"], t.handles)
        outcome = {}
        for name, (app, app_name) in t.appearances.items():
            depot = next(a["appearanceResource"]["DepotPath"]["$value"] for a in raw["Data"]["RootChunk"]["appearances"]
                         if a["name"]["$value"] == name)
            found = app_appearances(depot)
            got = None if found is None else found.get(app_name)
            outcome[name] = "?" if got is None else outcome_of(own | got, kind == "bike")
        if t.default_appearance in rg.NONE or not t.appearances:
            outcome["None"] = outcome_of(own, kind == "bike")
        counts, surveyed = Counter(), defaultdict(Counter)
        for r in rids:
            a = vehicles[r].get("appearanceName", "None")
            # spawned with no appearance named, the car gets one of its appearances (rig_geometry.py)
            a = a if a in outcome else "None" if "None" in outcome else "*"
            counts[a] += 1
            if r in survey and rg.plausible(survey[r]):
                surveyed[a][int(float(survey[r]["thrusters"]))] += 1
        groups = defaultdict(list)
        for name, bound in outcome.items():
            groups[bound].append(name)
        for i, (bound, names) in enumerate(sorted(groups.items(), key=lambda kv: (-len(kv[1]), kv[0]))):
            n = sum(counts[a] for a in names)
            seen = Counter()
            for a in names:
                seen.update(surveyed[a])
            seen_text = ",".join(f"{k}x{c}" for k, c in sorted(seen.items())) or NOTHING
            # the usual outcome is written as a count, the appearances that differ from it by name
            label = f"{len(names)}_appearances" if i == 0 and len(names) > 3 else ",".join(sorted(names))
            lines.append([tpl, "A", label, f"{bound} {n} {seen_text}"])
        if counts["*"]:
            seen_text = ",".join(f"{k}x{c}" for k, c in sorted(surveyed["*"].items())) or NOTHING
            lines.append([tpl, "A", "*", f"* {counts['*']} {seen_text}"])

    with open(args.out, "w", encoding="utf-8", newline="\n") as f:
        f.write("# generated by tools/flightsim/vehicle_slots.py from the game's vehicle templates, rigs and appearances;"
                " columns in its docstring\n")
        for row in [["template", "kind", "name", "data"]] + rig_lines + lines:
            f.write("\t".join(row) + "\n")
    kinds = Counter(l[3].split()[1] for l in lines if l[1] == "T")
    print(f"{len(records)} templates ({dict(kinds)}, {unresolved} not resolved to a path), {len(rig_sets)} rig sets, "
          f"{sum(l[1] == 'A' for l in lines)} appearance groups, {len(lines) + len(rig_lines)} lines to {args.out}")


if __name__ == "__main__":
    main()
