"""Find invisible vehicle blockers in the streaming sectors, for removal with ArchiveXL.

    python blockers.py scan --sectors <dir with .streamingsector> -o <out dir> [--jobs 10]
    python blockers.py report <out dir> [-o blockers.md]

How 2.5 sectors hold collision: one worldCookedCollisionNode per sector, placed many times in
nodeData, once per collision group; the k-th placement of the node is the k-th entry of its
compiledData CollisionNodes (checked on exterior_-6_-12_0_2: 30 placements, 30 groups, same
order and positions). A group is a run of actors (1 to tens), each with shapes and a collision
preset per shape. ArchiveXL's nodeDeletions index is a nodeData index and expectedNodes the
nodeData count, so deleting one placement removes exactly one group.

A group is a candidate when its shapes use the invisible-blocker presets ("Vehicle Blocker",
"Block Player and Vehicles"): "pure" when every shape does, "foliage" when the rest is only
query-only presets such as "Foliage Crown" (both safe to delete whole), "mixed" when it also
holds real collision ("World Static", environment collision, tree trunks) that deleting the
group would remove too.

scan writes <out>/<sector>.json per sector with candidates, and <out>/scanned.json (done list).
"""
import argparse
import collections
import json
import math
import os
import re
import shutil
import subprocess
import tempfile
from concurrent.futures import ProcessPoolExecutor, as_completed

DEFAULT_WOLVENKIT = r"D:\cp2077_work\build\tools\wolvenkit-10.0.0-beta.1\WolvenKit.CLI.exe"
FIXED_POINT = 1.0 / 131072.0
BATCH = 80
BLOCKERS = {
    # name: hash (WolvenKit gives the name when it knows the CName, else the decimal hash)
    "Vehicle Blocker": "12102968184854455469",
    "Block Player and Vehicles": "7787476977785091711",
}
# presets that hold no collision at all (query only): deleting them with a blocker group is free
NO_COLLISION = {"Foliage Crown", "No Collision", "Sight Blocker", "Block PhotoMode Camera"}
PRESETS_TABLE = r"D:\cp2077_work\build\heightmap\presets_table.json"
_names = {h: n for n, h in BLOCKERS.items()}
if os.path.exists(PRESETS_TABLE):
    _names.update({p["hash"]: p["name"] for p in json.load(open(PRESETS_TABLE))})


def preset_name(v):
    v = str(v)
    return _names.get(v, v)


def classify(count):
    """pure: only blocker shapes; foliage: blockers plus query-only shapes (tree crowns);
    mixed: also real collision, which deleting the group would remove too."""
    other = {p for p in count if p not in BLOCKERS}
    if not other:
        return "pure"
    if other <= NO_COLLISION:
        return "foliage"
    return "mixed"


def shape_half(key):
    """Half extents of a primitive shape in its own frame; None for meshes."""
    t = key["$type"]
    s = key.get("size") or {}
    if t == "PhysicsGeometryBoxData":
        return (s["X"], s["Y"], s["Z"])
    if t == "PhysicsGeometryCapsuleData":  # size.X radius, size.Y half height, axis local Z
        r, hh = s["X"], s["Y"]
        return (r, r, hh + r)
    if t == "PhysicsGeometrySphereData":
        return (s["X"],) * 3
    return None


def group_summary(inner):
    """Presets and actors of one collision group; extent of its blocker actors."""
    shapes = inner["shapes"] or []
    presets = inner["presets"] or []
    positions = inner["shapePositions"] or []
    count = collections.Counter()
    blocker_actors = []
    for actor in inner["actors"] or []:
        p = actor["transform"]["Position"]
        pos = [p[c]["Bits"] * FIXED_POINT for c in "xyz"]
        first = actor["shapeStartIndex"]
        names, halves, kinds = [], [], []
        for shape in shapes[first : first + actor["shapeCount"]]:
            pi = shape["shapePresetIndex"]
            name = preset_name(presets[pi]["$value"]) if 0 <= pi < len(presets) else ""
            names.append(name)
            count[name] += 1
            key = shape.get("collisionSourceKey") or {}
            kinds.append(key.get("$type", "?").replace("PhysicsGeometry", "").replace("Data", ""))
            h = shape_half(key) if key else None
            if h is not None:
                off = positions[shape["shapePositionIndex"]] if shape["shapePositionIndex"] >= 0 else {"X": 0, "Y": 0, "Z": 0}
                halves.append((h, (off["X"], off["Y"], off["Z"])))
        if any(n in BLOCKERS for n in names):
            # size: largest primitive's full extents (meshes have no size here)
            size = None
            if halves:
                h = max(halves, key=lambda hv: hv[0][0] * hv[0][1] * hv[0][2])[0]
                size = [round(2 * x, 1) for x in h]
            blocker_actors.append({
                "position": [round(x, 1) for x in pos],
                # the actor's position as stored (WorldPosition fixed point): what the plugin
                # looks for in the cooked node's compiledData
                "bits": [p[c]["Bits"] for c in "xyz"],
                # every shape of the actor is a blocker or query-only: moving the whole actor
                # away removes nothing else
                "only_blockers": all(n in BLOCKERS or n in NO_COLLISION for n in names),
                "shapes": collections.Counter(kinds).most_common(),
                "presets": sorted(set(names)),
                "size": size,
            })
    return count, blocker_actors


def process_json(path):
    rc = json.load(open(path, encoding="utf-8"))["Data"]["RootChunk"]
    nodes = rc["nodes"]
    data = rc["nodeData"]["Data"]
    refs = collections.defaultdict(list)
    for i, d in enumerate(data):
        refs[int(d["nodeIdx"])].append(i)
    groups = []
    for ni, handle in enumerate(nodes):
        node = handle["Data"]
        if node["$type"] not in ("worldCookedCollisionNode", "worldCollisionNode"):
            continue
        inner_list = node["compiledData"]["Data"]["CollisionNodes"]
        placements = refs.get(ni, [])
        for k, inner in enumerate(inner_list):
            count, blockers = group_summary(inner)
            if not blockers:
                continue
            groups.append({
                "node": ni,
                "group": k,
                # placement k of the node is group k (None if the counts ever disagree)
                "index": placements[k] if len(placements) == len(inner_list) else None,
                "actors": len(inner["actors"] or []),
                "blocker_actors": len(blockers),
                "kind": classify(count),
                "presets": dict(count),
                "blockers": blockers,
            })
    return {"nodeData": len(data), "groups": groups}


def run_batch(wolvenkit, files, out_dir):
    tmp = tempfile.mkdtemp(prefix="bl_")
    done = {}
    try:
        cmd = [wolvenkit, "cr2w", "-s", *files, "-o", tmp, "-v", "Minimal"]
        subprocess.run(cmd, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, check=False)
        for f in files:
            name = os.path.basename(f)
            jp = os.path.join(tmp, name + ".json")
            if not os.path.exists(jp):
                done[name] = "not serialized"
                continue
            try:
                result = process_json(jp)
            except Exception as err:  # keep going; report per sector
                done[name] = repr(err)
                continue
            finally:
                if os.path.exists(jp):
                    os.remove(jp)
            if result["groups"]:
                json.dump(result, open(os.path.join(out_dir, name + ".json"), "w"))
            done[name] = "ok"
        return done
    finally:
        shutil.rmtree(tmp, ignore_errors=True)


def scan(args):
    os.makedirs(args.out, exist_ok=True)
    done_path = os.path.join(args.out, "scanned.json")
    done = json.load(open(done_path)) if os.path.exists(done_path) else {}
    if args.rescan:
        have = {n[: -len(".json")] for n in os.listdir(args.out) if n.endswith(".streamingsector.json")}
        files = [os.path.join(args.sectors, n) for n in sorted(os.listdir(args.sectors)) if n in have]
    else:
        files = [
            os.path.join(args.sectors, n)
            for n in sorted(os.listdir(args.sectors))
            if re.match(args.match, n) and done.get(n) != "ok"
        ]
    print(f"{len(files)} sectors to scan")
    batches = [files[i : i + BATCH] for i in range(0, len(files), BATCH)]
    with ProcessPoolExecutor(args.jobs) as pool:
        futures = [pool.submit(run_batch, args.wolvenkit, b, args.out) for b in batches]
        for n, fut in enumerate(as_completed(futures), 1):
            done.update(fut.result())
            if n % 10 == 0 or n == len(futures):
                json.dump(done, open(done_path, "w"))
                bad = sum(1 for v in done.values() if v != "ok")
                print(f"  {n}/{len(futures)} batches, {len(done)} sectors, {bad} failed", flush=True)
    json.dump(done, open(done_path, "w"))


def report(args):
    rows = []
    for n in sorted(os.listdir(args.out)):
        if not n.endswith(".streamingsector.json"):
            continue
        r = json.load(open(os.path.join(args.out, n)))
        for g in r["groups"]:
            rows.append((n[: -len(".json")], r["nodeData"], g))
    kinds = collections.Counter(x[2]["kind"] for x in rows)
    actors = sum(x[2]["blocker_actors"] for x in rows)
    lines = [
        "# Invisible vehicle blockers",
        "",
        f"{len(rows)} collision groups with blocker presets in {len({x[0] for x in rows})} sectors, "
        f"{actors} blocker actors: {kinds['pure']} only blockers and {kinds['foliage']} blockers plus "
        f"tree crowns (query only) are safe to delete whole; {kinds['mixed']} also hold real "
        "collision (walls, trunks) that deleting the group would remove.",
        "",
        "Index = ArchiveXL nodeDeletions index (nodeData); nodeData = expectedNodes.",
        "",
        "| sector | nodeData | index | kind | actors | blockers | first blocker at | size (m) | presets |",
        "|---|---|---|---|---|---|---|---|---|",
    ]
    order = {"pure": 0, "foliage": 1, "mixed": 2}
    for sector, nd, g in sorted(rows, key=lambda x: (order[x[2]["kind"]], x[0], x[2]["group"])):
        b = g["blockers"][0]
        presets = ", ".join(f"{k} x{v}" for k, v in sorted(g["presets"].items(), key=lambda kv: -kv[1]))
        lines.append(
            f"| {sector} | {nd} | {g['index']} | {g['kind']} | {g['actors']} | {g['blocker_actors']} | "
            f"{tuple(b['position'])} | {b['size']} | {presets} |"
        )
    open(args.output, "w", encoding="utf-8").write("\n".join(lines) + "\n")
    print(lines[2])
    print("wrote", args.output)


def export(args):
    """Write the plugin's table (src/red4ext/Navigation/Blockers): per sector, the stored
    positions of the blocker actors to move away. Only actors whose every shape is a blocker or
    query-only, so real walls in the same group stay; minus the --exclude boxes (map edges)."""
    boxes = [tuple(float(v) for v in b.split(",")) for b in args.exclude]
    per_sector = collections.defaultdict(list)
    skipped = collections.Counter()
    for n in sorted(os.listdir(args.out)):
        if not n.endswith(".streamingsector.json"):
            continue
        sector = n[: -len(".json")]
        for g in json.load(open(os.path.join(args.out, n)))["groups"]:
            for b in g["blockers"]:
                x, y = b["position"][0], b["position"][1]
                if any(x0 <= x <= x1 and y0 <= y <= y1 for x0, y0, x1, y1 in boxes):
                    skipped["excluded area"] += 1
                elif not b.get("only_blockers"):
                    skipped["actor also has real collision"] += 1
                elif "bits" not in b:
                    skipped["no position bits (rescan)"] += 1
                else:
                    per_sector[sector].append(b["bits"])
    lines = [
        "# Let There Be Flight: invisible vehicle blockers to move away when their sector loads.",
        "# Written by tools/heightmap/blockers.py export; S = sector path, A = actor position as",
        "# stored (WorldPosition fixed point x y z), found in the sector's cooked collision data.",
    ]
    for sector in sorted(per_sector):
        lines.append(r"S base\worlds\03_night_city\_compiled\default" + "\\" + sector)
        lines += [f"A {x} {y} {z}" for x, y, z in per_sector[sector]]
    open(args.output, "w", newline="\n").write("\n".join(lines) + "\n")
    print(f"{sum(len(v) for v in per_sector.values())} actors in {len(per_sector)} sectors -> {args.output}; skipped {dict(skipped)}")


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest="cmd", required=True)
    e = sub.add_parser("export")
    e.add_argument("out")
    e.add_argument("-o", "--output", required=True)
    e.add_argument("--exclude", action="append", default=[], help="x0,y0,x1,y1 box to leave alone (repeatable)")
    s = sub.add_parser("scan")
    s.add_argument("--sectors", required=True)
    s.add_argument("-o", "--out", required=True)
    s.add_argument("--wolvenkit", default=DEFAULT_WOLVENKIT)
    s.add_argument("--jobs", type=int, default=max(1, (os.cpu_count() or 2) // 2))
    s.add_argument("--match", default=r"^(exterior|interior)_.*\.streamingsector$")
    s.add_argument("--rescan", action="store_true", help="redo the sectors that have results in -o")
    r = sub.add_parser("report")
    r.add_argument("out")
    r.add_argument("-o", "--output", default="blockers.md")
    args = ap.parse_args()
    {"scan": scan, "report": report, "export": export}[args.cmd](args)


if __name__ == "__main__":
    main()
