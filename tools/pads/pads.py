"""Find the AV landing pads (helipads) in the world sectors and check them against the nav map.

Background and probe results: docs/design/landing-pads.md.

  scan   Pick the raw .streamingsector files that reference a pad mesh (FNV-1a 64 hash of the
         depot path, or the plain path string for building proxies), serialize only those with
         WolvenKit, and write every pad mesh placement to <work>/placements.json.
  build  Group placements into pads, find each pad's deck on navmap.bin (flat floor, open sky
         above), apply helipads_overrides.json, and write helipads.json plus a preview PNG.

Game 2.5 cooked nodes (worldCookedStaticMeshNode, worldCookedGenericProxyMeshNode) bundle many
meshes in buffer.Data.InnerNodes; each nodeData entry picks one with instanceIndex and carries the
placement (transform, globalNodeId).

Usage:
  python pads.py scan --sectors <dir with exterior_*.streamingsector> -o W [--jobs 8]
  python pads.py build W --navmap navmap.bin -o helipads.json [--preview pads.png]
                       [--tsv ../../src/navigation/helipads.tsv]

helipads.json has every pad with its checks plus the rejects (for review); the .tsv is the
runtime list the plugin ships and loads (accepted pads only).
"""

import argparse
import collections
import glob
import json
import math
import os
import re
import shutil
import struct
import subprocess
import sys
import tempfile
import zlib
from concurrent.futures import ThreadPoolExecutor, as_completed

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))  # tools/, for common
from common import paths  # noqa: E402
from common import toolpath  # noqa: E402,F401 (modules from other tool folders)
from common.navmap import NavMap  # noqa: E402

from sectors import DEFAULT_WOLVENKIT, FIXED_POINT

BATCH = 40
HERE = os.path.dirname(os.path.abspath(__file__))
OVERRIDES = os.path.join(HERE, "helipads_overrides.json")

# Pad meshes by file name: (kind, height of the landing deck above the mesh origin at most, pad
# radius used for the flatness test). "prefab" pads give position and heading; "marking" is the
# painted H/box on the deck; "special" are big one-off structures whose origin is not the landing
# spot (kept for reference, placed by hand in the overrides).
PAD_MESHES = {
    "av_landing_pad_a_v1": ("prefab", 5.1, 6.0),
    "av_landing_pad_a_v2": ("prefab", 11.3, 6.0),
    "av_landing_pad_a_v3": ("prefab", 1.3, 6.0),
    "av_landing_pad_a_v3_small": ("prefab", 1.4, 4.0),
    "av_landing_pad_a_v4": ("prefab", 5.1, 6.0),
    "wbr_cha_av_landing_pad_v2": ("prefab", 1.6, 5.0),
    "landing_pad_a_01": ("marking", 0.0, 5.0),
    "landing_pad_b_01": ("marking", 0.0, 2.5),
    "hey_spr_building_av_landing_v1": ("special", 0.0, 0.0),
    "wbr_jpn_av_landing": ("special", 0.0, 0.0),
    "jt_building_top_garden_av_landing": ("special", 0.0, 0.0),
    "spaceport_landing_pad": ("special", 0.0, 0.0),
}

# Depot paths to look for in the raw sector bytes (from `WolvenKit.CLI archive -l` over the base
# and ep1 archives, game 2.5). Cooked nodes store FNV-1a 64 hashes of these; building proxies
# store a plain string, caught by STRING_NEEDLES.
PAD_PATHS = [
    r"base\worlds\03_night_city\sectors\_external\proxy\1323078461\av_landing_pad_a_v1.mesh",
    r"base\worlds\03_night_city\sectors\_external\proxy\3065888796\av_landing_pad_a_v2.mesh",
    r"base\worlds\03_night_city\sectors\_external\proxy\513731835\av_landing_pad_a_v3.mesh",
    r"base\worlds\03_night_city\sectors\_external\proxy\1209934131\av_landing_pad_a_v3_small.mesh",
    r"base\worlds\03_night_city\sectors\_external\proxy\2256542170\av_landing_pad_a_v4.mesh",
    r"base\worlds\03_night_city\sectors\_external\proxy\236306050\wbr_cha_av_landing_pad_v2.mesh",
    r"base\worlds\03_night_city\sectors\_external\proxy\2713699652\hey_spr_building_av_landing_v1.mesh",
    r"base\worlds\03_night_city\sectors\_external\proxy\839338013\wbr_jpn_av_landing.mesh",
    r"base\prefabs\environment\decoration\_decoration_set\building_deco\av_landing_pad\_proxyhelper\av_landing_pad_a_v1.mesh",
    r"base\prefabs\environment\decoration\_decoration_set\building_deco\av_landing_pad\_proxyhelper\av_landing_pad_a_v2_mproxy.mesh",
    r"base\prefabs\environment\decoration\_decoration_set\building_deco\av_landing_pad\_proxyhelper\av_landing_pad_a_v3.mesh",
    r"base\prefabs\environment\decoration\_decoration_set\building_deco\av_landing_pad\_proxyhelper\av_landing_pad_a_v4.mesh",
    r"base\prefabs\environment\architecture\westbrook\charter_hill\_proxyhelper\wbr_cha_av_landing_pad_v2_mproxy.mesh",
    r"base\prefabs\environment\architecture\heywood\wellsprings\_proxyhelper\hey_spr_building_av_landing_v1_mproxy.mesh",
    r"base\environment\architecture\_temp\tech_canopy_temp\jt_building_top_garden_av_landing.mesh",
    r"base\environment\decoration\public_utility\signages\landing_pad\landing_pad_a_01.mesh",
    r"base\environment\decoration\public_utility\signages\landing_pad\landing_pad_b_01.mesh",
    r"ep1\worlds\03_night_city\sectors\_external\proxy\3860382333\spaceport_landing_pad.mesh",
    r"ep1\environment\architecture\spaceport\spaceport_landing_pad.mesh",
]
STRING_NEEDLES = [b"av_landing_pad", b"av_landing", b"landing_pad_a_0", b"landing_pad_b_0"]

NAME_RE = re.compile(
    r"(?i)(?:^|[\\/])(" + "|".join(sorted(map(re.escape, PAD_MESHES), key=len, reverse=True))
    + r")(?:_mproxy)?(?:_\d+)?(?:_ProxyMesh)?(?:\.mesh)?$"
)


def fnv1a64(s):
    h = 0xCBF29CE484222325
    for b in s.lower().encode():
        h = ((h ^ b) * 0x100000001B3) & 0xFFFFFFFFFFFFFFFF
    return h


# ---------------------------------------------------------------------------------------- scan


def _strings(o):
    if isinstance(o, dict):
        for v in o.values():
            yield from _strings(v)
    elif isinstance(o, list):
        for v in o:
            yield from _strings(v)
    elif isinstance(o, str):
        yield o


def _pad_mesh(o):
    """The PAD_MESHES key named by any string in o (mesh path or proxy node name), or None."""
    for s in _strings(o):
        m = NAME_RE.search(s)
        if m:
            return m.group(1).lower()
    return None


def _placement(mesh, node, sector, pos, q, scale, node_id):
    return {
        "mesh": mesh,
        "node": node["$type"],
        "sector": sector,
        "pos": pos,
        "yaw": math.degrees(2.0 * math.atan2(q["k"], q["r"])),
        "tilt": math.degrees(2.0 * math.asin(min(1.0, math.hypot(q["i"], q["j"])))),
        "scale": [scale["X"], scale["Y"], scale["Z"]],
        "id": node_id,
    }


def placements_in(rc, sector, match=None):
    """Every placement in a serialized sector of a mesh that match(o) names (default: pad meshes)."""
    match = match or _pad_mesh
    by_key = collections.defaultdict(list)
    for k, d in enumerate(rc["nodeData"]["Data"]):
        d = {**d, "_k": k}  # several entries can share (nodeIdx, instanceIndex); k is their own slot
        by_key[(int(d["nodeIdx"]), int(d["instanceIndex"]))].append(d)
        by_key[(int(d["nodeIdx"]), None)].append(d)
    out = []
    for i, handle in enumerate(rc["nodes"]):
        node = handle["Data"]
        buf = node.get("buffer")
        data = buf.get("Data") if isinstance(buf, dict) else None
        inner = data.get("InnerNodes") if isinstance(data, dict) else None
        if node["$type"] == "worldInstancedMeshNode" and inner:
            # instances live in cookedWorldTransformsBuffer, TransformRanges[j] per inner mesh;
            # the node's nodeData entries are all at the origin
            transforms = node["cookedWorldTransformsBuffer"]["Data"]["Transforms"]
            for j, n in enumerate(inner):
                mesh = match(n)
                if not mesh:
                    continue
                r = data["TransformRanges"][j]
                for k in range(r["transformIndex"], r["transformIndex"] + r["transformCount"]):
                    t = transforms[k]
                    wp = t["transform"]["Position"]
                    pos = [wp[a]["Bits"] * FIXED_POINT for a in ("x", "y", "z")]
                    out.append(_placement(mesh, node, sector, pos, t["transform"]["Orientation"], t["scale"],
                                          f"{sector}/{i}/{k}"))
            continue
        hits = []
        if inner:
            for j, n in enumerate(inner):
                mesh = match(n)
                if mesh:
                    hits.append((mesh, by_key.get((i, j), [])))
        else:
            mesh = match(node)
            if mesh:
                hits.append((mesh, by_key.get((i, None), [])))
        for mesh, ds in hits:
            for d in ds:
                p = d["transform"]["position"]
                # cooked static meshes (the markings) have no global node id: use their nodeData slot
                gid = str(d["globalNodeId"]["hash"])
                node_id = gid if gid != "0" else f"{sector}/{d['_k']}"
                out.append(_placement(mesh, node, sector, [p["X"], p["Y"], p["Z"]], d["transform"]["orientation"],
                                      d["scale"], node_id))
    return out


def serialize_batch(wolvenkit, files, match=None):
    tmp = tempfile.mkdtemp(prefix="pads_")
    try:
        cmd = [wolvenkit, "cr2w", "-s", *files, "-o", tmp, "-v", "Minimal"]
        subprocess.run(cmd, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, check=False)
        found, errors = [], []
        for f in files:
            name = os.path.basename(f)
            jp = os.path.join(tmp, name + ".json")
            if not os.path.exists(jp):
                errors.append(name)
                continue
            try:
                rc = json.load(open(jp, encoding="utf-8"))["Data"]["RootChunk"]
            except ValueError:  # WolvenKit left an empty or cut-off json
                errors.append(name)
                continue
            found += placements_in(rc, name[: -len(".streamingsector")], match)
            os.remove(jp)
        return found, errors
    finally:
        shutil.rmtree(tmp, ignore_errors=True)


def scan(args):
    needles = [struct.pack("<Q", fnv1a64(p)) for p in PAD_PATHS] + STRING_NEEDLES
    files = sorted(glob.glob(os.path.join(args.sectors, "exterior_*.streamingsector")))
    hits = []
    for f in files:
        data = open(f, "rb").read()
        if any(n in data for n in needles):
            hits.append(f)
    print(f"{len(hits)} of {len(files)} sectors reference a pad mesh", file=sys.stderr)

    os.makedirs(args.out, exist_ok=True)
    placements, errors = [], []
    batches = [hits[i : i + BATCH] for i in range(0, len(hits), BATCH)]
    with ThreadPoolExecutor(args.jobs) as pool:
        futures = [pool.submit(serialize_batch, args.wolvenkit, b) for b in batches]
        for n, fut in enumerate(as_completed(futures), 1):
            p, e = fut.result()
            placements += p
            errors += e
            print(f"{n}/{len(batches)} batches, {len(placements)} placements", file=sys.stderr)
    placements.sort(key=lambda p: (p["pos"][0], p["pos"][1], p["pos"][2], p["mesh"]))
    json.dump({"placements": placements, "errors": sorted(errors)},
              open(os.path.join(args.out, "placements.json"), "w"), indent=0)
    print(f"{len(placements)} placements, {len(errors)} sectors failed to serialize", file=sys.stderr)


# --------------------------------------------------------------------------------------- build


def group(placements, reach=8.0, rise=14.0):
    """Placements within reach m horizontally and rise m vertically belong to one pad."""
    groups = []
    for p in placements:
        x, y, z = p["pos"]
        for g in groups:
            gx, gy, gz = g["anchor"]
            if math.hypot(x - gx, y - gy) <= reach and abs(z - gz) <= rise:
                g["parts"].append(p)
                break
        else:
            groups.append({"anchor": (x, y, z), "parts": [p]})
    return groups


BAND = 1.0  # a column is on the deck when its floor is within this many m of the deck height
CLEARANCE = 15.0  # free height above the deck for a car to come down onto it
MIN_SHARE = 0.75  # share of the test disc's columns that must be on the deck and clear
TEST_RADIUS = 4.0  # the car's centre must be able to rest anywhere within this of the pad centre


def _disc(nav, x, y, radius):
    """(x, y, deck-height gaps) of the 2 m columns within radius of (x, y)."""
    out = []
    for dx in np.arange(-radius, radius + 0.01, nav.cell):
        for dy in np.arange(-radius, radius + 0.01, nav.cell):
            if dx * dx + dy * dy <= radius * radius + 1e-6:
                out.append((x + dx, y + dy, [(b - nav.half, t) for b, t in nav.gaps(x + dx, y + dy)]))
    return out


def _on_deck(col, deck):
    """(floor, free height above it) of the column's gap on the deck, or None."""
    near = [g for g in col if abs(g[0] - deck) <= BAND]
    if not near:
        return None
    f, t = min(near, key=lambda g: abs(g[0] - deck))
    return f, t - f  # the gap's ceiling minus its floor is the free height for the car's centre


def find_deck(nav, x, y, zlo, zhi, search, test, want, recenter):
    """Find a pad's deck: a flat floor between zlo and zhi (deck heights, not car centres).

    The deck height is the floor with the most columns on it and clear above within search m of
    (x, y). With recenter the pad centre moves to the middle of those columns (pad prefab origins
    can sit off the deck). Then the test disc of test m is checked. The bake grew solids by the
    car's radius, so a column on the deck already has room for the whole car.

    Returns a dict (deck z, centre, columns on the deck / sampled / with CLEARANCE above in the
    test disc, floor spread, lowest finite free height or None for open sky), or None.
    """
    wide = _disc(nav, x, y, search)
    test2 = test * test + 1e-6
    candidates = sorted({round(f, 1) for _, _, col in wide for f, _ in col if zlo <= f <= zhi})
    best = None
    for deck in candidates:
        hits = [(cx, cy, g) for cx, cy, col in wide if (g := _on_deck(col, deck))]
        weight = [1 + (room >= CLEARANCE) for _, _, (_, room) in hits]
        # columns near the given centre first (a raised rim around the deck has more columns),
        # then the whole search disc (an origin at the pad's corner sees little of the deck)
        centre = sum(w for (cx, cy, _), w in zip(hits, weight) if (cx - x) ** 2 + (cy - y) ** 2 <= test2)
        score = (centre, sum(weight), -abs(deck - want))
        if best is None or score > best[0]:
            best = (score, deck, hits)
    if best is None:
        return None
    _, deck, hits = best
    if recenter:
        clear = [(cx, cy) for cx, cy, (_, room) in hits if room >= CLEARANCE] or [(cx, cy) for cx, cy, _ in hits]
        x, y = float(np.mean([c[0] for c in clear])), float(np.mean([c[1] for c in clear]))
    disc = _disc(nav, x, y, test)
    on = [g for _, _, col in disc if (g := _on_deck(col, deck))]
    if not on:
        return None
    floors = [f for f, _ in on]
    middle = _on_deck(_disc(nav, x, y, 0.0)[0][2], deck)
    return {
        "deck": float(np.median(floors)),
        "x": x,
        "y": y,
        "on": len(on),
        "n": len(disc),
        "clear": sum(room >= CLEARANCE for _, room in on),
        "spread": max(floors) - min(floors),
        # free height above the pad centre (None: open sky, or the centre column is off the deck)
        "clearance": middle[1] if middle and math.isfinite(middle[1]) else None,
    }


def proxy_pad(nav, lead, parts, markings, radius, top, removed, names):
    """A pad whose prefab is a building proxy (v1): deck from the markings, clearance from the map.

    Above the platform's hull (origin + mesh top) the nav map is right again, so a column is clear
    when its gap reaching above the hull leaves CLEARANCE over the deck.
    """
    pad = {
        "id": lead["id"],
        "mesh": lead["mesh"],
        "yaw": round(lead["yaw"] % 360.0, 1),
        "radius": round(radius, 1),
        "sources": sorted({p["mesh"] for p in parts}),
        "sector": lead["sector"],
    }
    if not markings:
        x, y, z = lead["pos"]
        pad.update(pos=[round(x, 2), round(y, 2), round(z, 2)], ok=False,
                   reason="platform without painted markings: no deck collision")
        return pad
    m = max(markings, key=lambda p: PAD_MESHES[p["mesh"]][2])
    x, y, deck = m["pos"]
    pad["pos"] = [round(x, 2), round(y, 2), round(deck, 2)]
    above = lead["pos"][2] + top + nav.half + 0.5  # a car centre just over the hull
    disc = _disc(nav, x, y, min(radius, TEST_RADIUS))
    clear, rooms = 0, []
    for _, _, col in disc:
        gap = next((g for g in col if g[0] <= above - nav.half < g[1] or g[0] >= above - nav.half), None)
        if gap and gap[1] - deck >= CLEARANCE:
            clear += 1
            if math.isfinite(gap[1]):
                rooms.append(gap[1] - deck)
    pad["check"] = {"on_deck": "markings", "clear": f"{clear}/{len(disc)}", "spread": None}
    pad["clearance"] = round(min(rooms), 1) if rooms else None
    pad["ok"] = clear >= MIN_SHARE * len(disc)
    if not pad["ok"]:
        pad["reason"] = f"covered (under {CLEARANCE:.0f} m free above)"
    if pad["id"] in removed:
        pad.update(ok=False, reason="removed in overrides")
    if pad["id"] in names:
        pad["name"] = names[pad["id"]]
    return pad


def build(args):
    data = json.load(open(os.path.join(args.work, "placements.json")))
    nav = NavMap(args.navmap)
    overrides = json.load(open(args.overrides)) if os.path.exists(args.overrides) else {}
    dropped = set(overrides.get("remove", []))
    # a scanned pad with the id of a hand-placed one is replaced by it (e.g. a deck from our sector)
    removed = dropped | {a["id"] for a in overrides.get("add", [])}
    names = overrides.get("names", {})

    pads, specials = [], []
    usable = [p for p in data["placements"] if p["tilt"] < 10.0]
    for g in group([p for p in usable if PAD_MESHES[p["mesh"]][0] != "special"]):
        parts = g["parts"]
        prefabs = [p for p in parts if PAD_MESHES[p["mesh"]][0] == "prefab"]
        markings = [p for p in parts if PAD_MESHES[p["mesh"]][0] == "marking"]
        lead = prefabs[0] if prefabs else max(markings, key=lambda p: PAD_MESHES[p["mesh"]][2])
        x, y, z0 = lead["pos"]
        kind, top, radius = PAD_MESHES[lead["mesh"]]
        top *= lead["scale"][2]
        radius *= min(lead["scale"][0], lead["scale"][1])
        if markings:  # the painting is on the deck itself: trust it for height and centre
            m = max(markings, key=lambda p: PAD_MESHES[p["mesh"]][2])
            x, y = m["pos"][0], m["pos"][1]
            # the bake's floor can sit up to ~2 m above the painting (collision thicker than the
            # visual deck)
            want, zlo, zhi = m["pos"][2], m["pos"][2] - 1.0, m["pos"][2] + 2.5
            if not prefabs:
                radius = max(PAD_MESHES[p["mesh"]][2] * min(p["scale"][:2]) for p in markings)
        else:
            want, zlo, zhi = z0 + top, z0 - 1.0, z0 + top + 3.0
        radius = max(radius, 2.0)
        if lead["node"] == "worldBuildingProxyMeshNode":
            # v1 platforms (4 m on pillars) are building proxies, which the bake treats as solid, so
            # the nav map cannot show their deck. In game (2026-10-26): with painted markings the
            # deck is at the markings and has collision; without, the top is visual only and a car
            # sinks to the platform's base (-1841, -808).
            pads.append(proxy_pad(nav, lead, parts, markings, radius, top, removed, names))
            continue
        deck = find_deck(nav, x, y, zlo, zhi, radius + 4.0, min(radius, TEST_RADIUS), want,
                         recenter=not markings)
        pad = {
            "id": lead["id"],
            "mesh": lead["mesh"],
            "pos": [round(x, 2), round(y, 2), None],
            "yaw": round(lead["yaw"] % 360.0, 1),
            "radius": round(radius, 1),
            "sources": sorted({p["mesh"] for p in parts}),
            "sector": lead["sector"],
        }
        if deck is None:
            # seen on pads with markings but no vehicle collision under them: a car falls through
            pad.update(ok=False, reason="no vehicle collision at the deck height")
            pad["pos"][2] = round(want, 2)
        else:
            n = deck["n"]
            shift = math.hypot(deck["x"] - x, deck["y"] - y)
            if shift > 0.5:
                pad["moved"] = round(shift, 1)  # re-centred from the prefab origin onto the deck
            pad["pos"] = [round(deck["x"], 2), round(deck["y"], 2), round(deck["deck"], 2)]
            pad["clearance"] = None if deck["clearance"] is None else round(deck["clearance"], 1)
            pad["check"] = {"on_deck": f"{deck['on']}/{n}", "clear": f"{deck['clear']}/{n}",
                            "spread": round(deck["spread"], 2)}
            reasons = []
            if deck["on"] < MIN_SHARE * n:
                reasons.append("deck too small or uneven")
            if deck["clear"] < MIN_SHARE * n:
                reasons.append(f"covered (under {CLEARANCE:.0f} m free above)")
            pad["ok"] = not reasons
            if reasons:
                pad["reason"] = ", ".join(reasons)
        if pad["id"] in removed:
            pad.update(ok=False, reason="removed in overrides")
        if pad["id"] in names:
            pad["name"] = names[pad["id"]]
        pads.append(pad)
    for p in usable:
        if PAD_MESHES[p["mesh"]][0] == "special":
            specials.append({"id": p["id"], "mesh": p["mesh"], "origin": [round(v, 2) for v in p["pos"]],
                             "yaw": round(p["yaw"] % 360.0, 1), "sector": p["sector"]})
    for a in overrides.get("add", []):
        # hand-placed pads get the same deck check (no re-centring), so a typo shows up as a reject
        x, y, z = a["pos"]
        radius = a.get("radius", 6.0)
        pad = {**a, "sources": ["overrides"]}
        if a.get("own_deck"):
            # the deck is ours (an entity in our sector, docs/design/landing-pads.md step 8): the nav map has
            # only what is under it, so trust the measured height
            pad.update(clearance=None, check={"on_deck": "own deck", "clear": None, "spread": None}, ok=True)
            if pad["id"] in dropped:
                pad.update(ok=False, reason="removed in overrides")
            pads.append(pad)
            continue
        deck = find_deck(nav, x, y, z - 1.5, z + 1.5, radius + 4.0, min(radius, TEST_RADIUS), z, recenter=False)
        if deck is None:
            pad.update(ok=False, reason="no vehicle collision at the deck height")
        else:
            n = deck["n"]
            pad["pos"] = [round(x, 2), round(y, 2), round(deck["deck"], 2)]
            pad["clearance"] = None if deck["clearance"] is None else round(deck["clearance"], 1)
            pad["check"] = {"on_deck": f"{deck['on']}/{n}", "clear": f"{deck['clear']}/{n}",
                            "spread": round(deck["spread"], 2)}
            pad["ok"] = bool(deck["on"] >= MIN_SHARE * n and deck["clear"] >= MIN_SHARE * n)
            if not pad["ok"]:
                pad["reason"] = "hand-placed pad fails the deck check"
        if pad["id"] in dropped:
            pad.update(ok=False, reason="removed in overrides")
        pads.append(pad)

    pads.sort(key=lambda p: (not p["ok"], p["pos"][0], p["pos"][1]))
    out = {
        "comment": "Generated by tools/pads/pads.py from the world sectors and navmap.bin; "
                   "edit tools/pads/helipads_overrides.json instead. pos = deck centre "
                   "(the surface a car rests on), yaw in degrees.",
        "pads": [p for p in pads if p["ok"]],
        "rejected": [p for p in pads if not p["ok"]],
        "special": specials,
    }
    with open(args.out, "w") as f:
        json.dump(out, f, indent=1)
    if args.tsv:
        write_tsv(out["pads"], args.tsv)
    reasons = collections.Counter(p.get("reason") for p in out["rejected"])
    print(f"{len(out['pads'])} pads, {len(out['rejected'])} rejected {dict(reasons)}, "
          f"{len(specials)} special placements", file=sys.stderr)
    if args.preview:
        preview(nav, out, args.preview)


def write_tsv(pads, path):
    """The runtime list the plugin loads (src/red4ext/Navigation/Helipads.hpp): accepted pads only."""
    with open(path, "w", encoding="utf-8", newline="\n") as f:
        f.write("# Landing pads for the autopilot, generated by tools/pads/pads.py (docs/design/landing-pads.md).\n")
        f.write("# id\tx\ty\tz\tyaw\tradius\tclearance\tname\n")
        f.write("# z = deck the car rests on; yaw in degrees; clearance = free height above the deck, inf = open sky\n")
        for p in sorted(pads, key=lambda p: p["id"]):
            x, y, z = p["pos"]
            clearance = "inf" if p.get("clearance") is None else f"{p['clearance']:.1f}"
            name = p.get("name", "").replace("\t", " ")
            f.write(f"{p['id']}\t{x:.2f}\t{y:.2f}\t{z:.2f}\t{p.get('yaw', 0.0):.1f}\t{p.get('radius', 6.0):.1f}\t{clearance}\t{name}\n")
    print(f"wrote {path} ({len(pads)} pads)", file=sys.stderr)


def preview(nav, out, path, px_cell=8.0):
    """Top-floor shading of the whole map from navmap.bin with the pads drawn on it."""
    import matplotlib

    matplotlib.use("Agg")
    import matplotlib.pyplot as plt

    step = int(px_cell / nav.cell)
    tw = nav.tile // step
    img = np.full((nav.ty * tw, nav.tx * tw), np.nan, np.float32)
    for ty in range(nav.ty):
        for tx in range(nav.tx):
            t = nav.tile_data(tx, ty)
            if t is None:
                continue
            counts, gaps, starts = t
            top = np.full(counts.shape, np.nan, np.float32)
            has = counts > 0
            top[has] = nav.z0 + gaps[starts[1:][has] - 1, 0] * nav.vz  # floor of the highest gap
            top = top.reshape(nav.tile, nav.tile)[::step, ::step]
            img[ty * tw : (ty + 1) * tw, tx * tw : (tx + 1) * tw] = top
    extent = [nav.ox, nav.ox + img.shape[1] * px_cell, nav.oy, nav.oy + img.shape[0] * px_cell]
    fig, ax = plt.subplots(figsize=(16, 16 * img.shape[0] / img.shape[1]), dpi=125)
    ax.imshow(np.clip(img, -20, 300), origin="lower", extent=extent, cmap="gray", interpolation="nearest")
    for key, color, label in (("pads", "lime", "pad"), ("rejected", "red", "rejected")):
        pts = np.array([p["pos"][:2] for p in out[key]]) if out[key] else np.zeros((0, 2))
        ax.scatter(pts[:, 0], pts[:, 1], s=14, c=color, edgecolors="black", linewidths=0.4,
                   label=f"{label} ({len(pts)})")
    if out["special"]:
        pts = np.array([p["origin"][:2] for p in out["special"]])
        ax.scatter(pts[:, 0], pts[:, 1], s=30, marker="s", facecolors="none", edgecolors="orange",
                   label=f"special ({len(pts)})")
    every = np.array([p["pos"][:2] for p in out["pads"] + out["rejected"]] + [p["origin"][:2] for p in out["special"]])
    if len(every):
        lo, hi = every.min(axis=0) - 400.0, every.max(axis=0) + 400.0
        ax.set_xlim(lo[0], hi[0])
        ax.set_ylim(lo[1], hi[1])
    ax.legend(loc="upper right")
    ax.set_title("Landing pads (pads.py)")
    fig.savefig(path, bbox_inches="tight")
    print(f"wrote {path}", file=sys.stderr)


def main():
    ap = argparse.ArgumentParser()
    sub = ap.add_subparsers(dest="cmd", required=True)
    s = sub.add_parser("scan")
    s.add_argument("--sectors", required=True)
    s.add_argument("-o", "--out", required=True)
    s.add_argument("--wolvenkit", default=DEFAULT_WOLVENKIT)
    s.add_argument("--jobs", type=int, default=max(1, (os.cpu_count() or 2) // 2))
    b = sub.add_parser("build")
    b.add_argument("work")
    b.add_argument("--navmap", required=True)
    b.add_argument("-o", "--out", required=True)
    b.add_argument("--preview")
    b.add_argument("--tsv", help="runtime list for the plugin (src/navigation/helipads.tsv)")
    b.add_argument("--overrides", default=OVERRIDES)
    args = ap.parse_args()
    scan(args) if args.cmd == "scan" else build(args)


if __name__ == "__main__":
    main()
