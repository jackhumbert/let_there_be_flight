"""The simulator's car table from TweakDB, grouped by physics (docs/design/hover-fly-feel.md).

    python tools/flightsim/tweak_physics.py <dump dir> [--survey <measured.tsv>] [--geometry <geometry.tsv>]
                                            [-o tools/flightsim/vehicles.tsv] [--todo <records to survey.txt>]

<dump dir> is vehicle_tweaks.py's output (cp2077_tools/mods/let_there_be_flight): every car and bike record's drive
model, wheel setup and entity template, and each drive model's mass, inertia and friction. Records sharing all three
are one physical car (most records are paint and trim variants) and become one row of the table.

Where each number comes from:
- mass, air resistance, braking friction: the drive model, always (they matched the game on 905 of 907 surveyed
  cars, 2026-11-04; the other two were read from a stand-in physics body);
- inertia: the drive model's momentOfInertia, except where it is 0,0,0 (28 of 109 car drive models: trucks, vans,
  buses), where the game works it out from the collision shape; then a survey of a car with that drive model;
- thruster tensor, wheel emitter offsets, body bottom, thruster count: they depend on where the wheels sit on the
  model, which TweakDB does not hold; a survey (vehicle_survey.py) of any record with the same entity template, else
  --geometry: rig_geometry.py's row for the entity template and drive model, worked out from the template and its rig
  (tensor within 2% of the survey on 163 of 176 surveyed templates, wheel offsets within 1 mm on all of them). A
  survey reading whose thruster count is not the rig row's (read before LTBF gave the car more thrusters, or with
  some broken off) gives only its inertia.
--todo lists one record for each physical car still missing a measurement, for vehicle_survey.py --records.
Records on CDPR's stand-in drive model (PLACEHOLDER_DRIVE_MODELS: AVs, drones, props) are left out.
--aero adds, from aero.py's aero.tsv, each car's drag area per body axis (cdaX side, cdaY front, cdaZ top, m2) and
its chassis box and underside profile from the centre of mass (boxMinX .. boxMaxZ, m; underside, the lowest point per
square, as aero.underside() writes it: the ground contact, docs/design/hover-fly-terrain.md); empty for the three cars
with no chassis shape.
"""

import argparse
import csv
import os
import statistics
from collections import defaultdict

HERE = os.path.dirname(os.path.abspath(__file__))
COLUMNS = ["record", "name", "type", "mass", "airResistance", "brakingFriction", "inertiaX", "inertiaY", "inertiaZ",
           "tensorX", "tensorY", "tensorZ", "wheelX", "wheelY", "wheelZ", "bottom", "thrusters", "records",
           "cdaX", "cdaY", "cdaZ", "boxMinX", "boxMaxX", "boxMinY", "boxMaxY", "boxMinZ", "boxMaxZ", "underside"]
# from aero.py's table (--aero), empty without: the chassis shape's drag area per body axis (m2: side, front, top), its
# box and its underside
AERO = ["cdaX", "cdaY", "cdaZ", "boxMinX", "boxMaxX", "boxMinY", "boxMaxY", "boxMinZ", "boxMaxZ", "underside"]
GEOMETRY = ["tensorX", "tensorY", "tensorZ", "wheelX", "wheelY", "wheelZ", "bottom", "thrusters"]
NONE = ("", "0x0000000000000000", "None", "NULL")
# The stand-in drive model CDPR gives vehicles that are never driven: 1300 kg, inertia 0,0,0, air resistance 0.35
# (AVs, drones, trains, barges, q101's stretchers, quest wrecks: 164 records, 67 physical cars, none named).
# Not something a player flies, and its low drag put every one of them at the Autopilot's speed cap: left out
# (Jack, 2026-11-04).
PLACEHOLDER_DRIVE_MODELS = {"0x00000027714d3107"}


def read(path):
    if not os.path.exists(path):
        return {}
    with open(path, encoding="utf-8") as f:
        return {r["record"]: r for r in csv.DictReader(f, delimiter="\t")}


def vec(text):
    try:
        return [float(v) for v in text.split(",")[:3]]
    except ValueError:
        return None


def plausible(s):
    """A survey row read from the real physics body (the stand-in one has the same inertia on every axis) with its
    thrusters on (a car read without them has a zero tensor)."""
    try:
        i = [float(s[f"inertia{a}"]) for a in "XYZ"]
        t = [float(s[f"tensor{a}"]) for a in "XYZ"]
    except (KeyError, ValueError):
        return False
    return not (abs(i[0] - i[1]) < 0.01 and abs(i[1] - i[2]) < 0.01) and min(t) > 0.0


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("dump")
    ap.add_argument("--survey", default="")
    ap.add_argument("--geometry", default="")
    ap.add_argument("-o", "--out", default=os.path.join(HERE, "vehicles.tsv"))
    ap.add_argument("--todo", default="")
    ap.add_argument("--aero", default="", help="aero.py's aero.tsv: each car's drag area per body axis, box and underside")
    args = ap.parse_args()

    vehicles = read(os.path.join(args.dump, "vehicles.tsv"))
    drive = {**read(os.path.join(args.dump, "drive_models.tsv")), **read(os.path.join(args.dump, "bike_drive_models.tsv"))}
    survey = {k: s for k, s in read(args.survey).items() if plausible(s)} if args.survey else {}
    derived = {}
    if args.geometry:
        with open(args.geometry, encoding="utf-8") as f:
            derived = {(r["template"], r["driveModel"]): r for r in csv.DictReader(f, delimiter="\t")}

    def model(v):
        dm = v.get("vehDriveModelData", "")
        return dm if dm not in NONE else v.get("bikeDriveModelData", "")

    groups = defaultdict(list)
    placeholder = 0
    for rid, v in vehicles.items():
        if model(v) in PLACEHOLDER_DRIVE_MODELS:
            placeholder += 1
            continue
        groups[(model(v), v["vehWheelDimensionsSetup"], v["entityTemplatePath"])].append(rid)
    print(f"{placeholder} records on the stand-in drive model left out")
    print(f"{len(vehicles)} records, {len(groups)} physical cars, {len({k[2] for k in groups})} entity templates, "
          f"{len({k[0] for k in groups})} drive models")

    # measurements per entity template (geometry) and per drive model (inertia the game works out)
    by_template, by_model = defaultdict(list), defaultdict(list)
    for rid, s in survey.items():
        v = vehicles.get(rid)
        if v:
            by_template[v["entityTemplatePath"]].append(s)
            by_model[model(v)].append(s)
    spreads = [max(float(r[c]) for r in rows) - min(float(r[c]) for r in rows)
               for rows in by_template.values() if len(rows) > 1 for c in ("tensorX", "tensorY", "tensorZ")]
    if spreads:
        print(f"thruster tensor within an entity template ({len(spreads) // 3} templates surveyed more than once): "
              f"largest spread {max(spreads):.4f}")

    rows, todo, unknown, from_rig = [], [], [], 0
    for (dm, wheels, tpl), rids in sorted(groups.items(), key=lambda g: -len(g[1])):
        d = drive.get(dm)
        if not d:
            unknown.append(rids[0])
            continue
        moi = vec(d.get("momentOfInertia", ""))
        if moi == [0.0, 0.0, 0.0]:
            measured = by_model.get(dm)
            moi = [float(measured[0][f"inertia{a}"]) for a in "XYZ"] if measured else None
        geometry = by_template.get(tpl) if tpl not in NONE else None
        if geometry and (tpl, dm) in derived:
            # a reading with another thruster count than LTBF gives the car now was read under an older configuration
            # (or with thrusters broken off): its inertia stands, its geometry does not
            count = int(float(derived[(tpl, dm)]["thrusters"]))
            geometry = [r for r in geometry if int(float(r["thrusters"])) == count]
        rig = not geometry and (tpl, dm) in derived
        if rig:
            geometry = [derived[(tpl, dm)]]
        if not moi or not geometry:
            todo.append(rids[0])
            continue
        from_rig += rig
        # the median of the template's readings: the bottom reads 0.2 to 0.55 m on one car (uneven ground where it
        # was spawned), and the odd reading is off altogether
        g = {c: statistics.median(float(r[c]) for r in geometry) for c in GEOMETRY}
        names = sorted({survey[r]["name"] for r in rids if r in survey})
        rows.append({"record": rids[0], "name": names[0] if names else rids[0],
                     "type": "bike" if dm in read(os.path.join(args.dump, "bike_drive_models.tsv")) else "car",
                     "mass": d["total_mass"], "airResistance": d["airResistanceFactor"],
                     "brakingFriction": d["brakingFrictionFactor"], "inertiaX": moi[0], "inertiaY": moi[1],
                     "inertiaZ": moi[2], **{c: g[c] for c in GEOMETRY}, "records": len(rids)})

    aero = read(args.aero) if args.aero else {}
    for r in rows:
        a = aero.get(r["record"], {})
        r.update({c: a.get(c, "") for c in AERO})
    with open(args.out, "w", encoding="utf-8", newline="\n") as f:
        f.write("\t".join(COLUMNS) + "\n")
        for r in rows:
            f.write("\t".join(str(r[c]) for c in COLUMNS) + "\n")
    print(f"{len(rows)} physical cars ({sum(r['records'] for r in rows)} records) to {args.out}"
          + (f", {from_rig} of them with geometry from the rig" if args.geometry else ""))
    print(f"{len(todo)} physical cars still need a survey of one record; {len(unknown)} have no drive model record")
    if args.todo:
        with open(args.todo, "w", encoding="utf-8", newline="\n") as f:
            f.write("\n".join(todo) + "\n")


if __name__ == "__main__":
    main()
