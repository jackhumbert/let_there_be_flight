"""Place our pad entities in the world (docs/archive/helipads.md, step 8): rewrite our sector's pad nodes
from pad_sectors.json and fit its streaming block descriptor.

Works on the committed CR2W files in src/wolvenkit/source/archive: serializes the sector and the
block, keeps every node that is not one of our pad entities (the Watson car spawners), drops the
old pad nodes, adds one worldEntityNode per part entity of each listed pad at the pad's transform
(a copy of the first existing pad node, so the sector must hold one; the first was built from a
vanilla v68 exterior sector's worldEntityNode), sets the descriptor box to base_box joined with
each pad's position +- its streaming distance, and writes both files back.

  python pad_sector.py [--config pad_sectors.json] [--wolvenkit <WolvenKit 10 CLI>]
  then: python tools/packaging/pack_archive.py

"decals" (optional): {"material": .mi, "pos", "quat" [i, j, k, r], "scale" (the decal box),
"node_id", settings of the cooked decal it was copied from}, as worldStaticDecalNodes with
debugName "[LTBF decal] <material>" (hangar_build.py fills them from the hangar's vignettes).

"spawners" (optional): vehicle spawners, {"record": "Vehicle.<id>", "pos", "yaw", "node_id",
"streaming_distance"}, copies of the sector's first spawner (a Watson pad car) with debugName
"[LTBF spawner] <record>"; like that one they give a fresh car on every load.

"colliders" (optional): a tiles file from proxy_colliders.py build (relative to this folder), one
collider entity per tile (collision for proxy-only buildings), placed unrotated at its "pos".

"quest_nodes" (optional): named nodes a quest or scene addresses by NodeRef (docs/design/hangar.md,
"Quest nodes"), each {"ref": "$/mod/let_there_be_flight/<place>/#<name>", "kind", "pad": the
entry whose frame "local" [x, y, z] is in (+Y the pad's forward, z up from its origin), "yaw" (in
that frame) or "aim" (a point in that frame the node faces), "streaming_distance"}. Kinds are the
vanilla shapes: "scene_marker" (worldStaticMarkerNode + scnSceneMarker, what a scene's
sceneLocation names), "spawn_marker" (worldStaticMarkerNode + worldSpawnPointMarker, a spot for
an actor or V), "camera" (worldEntityNode of engine/scenesystem/camera.ent, what a scene's
scneventsCameraEvent cuts to). globalNodeId is FNV1a64 of the ref with '#' removed, as the game
hashes a NodeRef; the refs also go into the sector's nodeRefs, as vanilla registers its names.

Lessons (2026-10-27): a second sector of our own with its own descriptor in the same block never
loaded (not even at a spot where the same nodes worked), so all pads share this sector; the
descriptor box gates loading, so it must cover every node.
"""
import argparse
import copy
import json
import math
import os
import shutil
import subprocess
import tempfile

import sys

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))  # tools/, for common
from common import paths  # noqa: E402

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
SOURCE = os.path.join(REPO, "src", "wolvenkit", "source", "archive")
DEFAULT_WOLVENKIT = paths.WOLVENKIT
PAD_PREFIX = "user\\jackhumbert\\entities\\ltbf_"  # every entity of ours (pads, hangar)


def cli(wk, *args):
    r = subprocess.run([wk, *args], capture_output=True, text=True)
    if "Error" in r.stdout or r.returncode not in (0,):
        raise SystemExit(f"WolvenKit {' '.join(args[:2])} failed:\n{r.stdout[-2000:]}{r.stderr[-1000:]}")


SPAWNER_PREFIX = "[LTBF spawner] "  # debugName of the vehicle spawners this tool adds
DECAL_PREFIX = "[LTBF decal] "      # and of its decals
QUEST_PREFIX = "[LTBF quest] "      # and of its quest nodes
CAMERA_ENT = r"engine\scenesystem\camera.ent"
MARKER_DATA = {"scene_marker": {"$type": "scnSceneMarker", "markers": [], "workspotMarkers": []},
               "spawn_marker": {"$type": "worldSpawnPointMarker", "type": 0}}


def is_pad(node):
    d = node["Data"]
    if d.get("$type") == "worldPopulationSpawnerNode":
        return str(d.get("debugName", {}).get("$value", "")).startswith(SPAWNER_PREFIX)
    if d.get("$type") == "worldStaticDecalNode":
        return str(d.get("debugName", {}).get("$value", "")).startswith(DECAL_PREFIX)
    if str(d.get("debugName", {}).get("$value", "")).startswith(QUEST_PREFIX):
        return True
    return d.get("$type") == "worldEntityNode" and \
        str(d.get("entityTemplate", {}).get("DepotPath", {}).get("$value", "")).startswith(PAD_PREFIX)


def place(data, x, y, z, yaw, node_id, streaming_distance, index):
    d = copy.deepcopy(data)
    h = math.radians(yaw) / 2.0
    d["nodeIdx"] = str(index)
    d["globalNodeId"]["hash"] = str(node_id)
    d["transform"]["position"] = {"$type": "Vector4", "W": 0, "X": x, "Y": y, "Z": z}
    d["transform"]["orientation"].update(i=0, j=0, k=math.sin(h), r=math.cos(h))
    for key in ("streamingRefPoint", "preboostStreamingRefPoint", "secondaryRefPointPosition"):
        d[key] = {"$type": "Vector3", "X": x, "Y": y, "Z": z}
    d["streamingDistance"] = float(streaming_distance)
    return d


def decal_node(dc):
    """A worldStaticDecalNode (the uncooked decal node sector mods use) from a decal entry: the
    material and the cooked decal's settings; WolvenKit fills the other fields with defaults."""
    node = {"$type": "worldStaticDecalNode",
            "debugName": {"$type": "CName", "$storage": "string", "$value": DECAL_PREFIX + dc["material"].rsplit("\\", 1)[-1]},
            "material": {"DepotPath": {"$type": "ResourcePath", "$storage": "string", "$value": dc["material"]}, "Flags": "Default"},
            # the default hides a decal 20 m away, less than the hangar is long
            "autoHideDistance": float(dc.get("auto_hide", 60.0))}
    for key in ("alpha", "normalThreshold", "roughnessScale"):
        if key in dc:
            node[key] = dc[key]
    if "normalsBlendingMode" in dc:
        node["normalsBlendingMode"] = dc["normalsBlendingMode"]
    if "surfaceType" in dc:
        node["surfaceType"] = dc["surfaceType"]
    return node


def fnv1a64(text):
    h = 14695981039346656037
    for b in text.encode():
        h = ((h ^ b) * 1099511628211) % (1 << 64)
    return h


def node_id(ref):
    """worldGlobalNodeID of a NodeRef: FNV1a64 of it with '#' removed (questkit community.node_id)."""
    return fnv1a64(ref.replace("#", ""))


def quest_nodes(cfg):
    """The "quest_nodes" entries in world space: (entry, (x, y, z), quaternion (i, j, k, r))."""
    pads = {p["pad"]: p for p in cfg["pads"]}
    out = []
    for q in cfg.get("quest_nodes", []):
        pad = pads[q["pad"]]
        c, s = math.cos(math.radians(pad["yaw"])), math.sin(math.radians(pad["yaw"]))

        def world(p):
            return (pad["pos"][0] + p[0] * c - p[1] * s, pad["pos"][1] + p[0] * s + p[1] * c, pad["pos"][2] + p[2])
        pos = world(q["local"])
        if "aim" in q:
            # entity forward is +Y: turn about Z to face the point, then pitch about X
            t = world(q["aim"])
            dx, dy, dz = (t[k] - pos[k] for k in range(3))
            yaw, pitch = math.atan2(-dx, dy), math.atan2(dz, math.hypot(dx, dy))
        else:
            yaw, pitch = math.radians(pad["yaw"] + q.get("yaw", 0.0)), 0.0
        cz, sz, cx, sx = math.cos(yaw / 2), math.sin(yaw / 2), math.cos(pitch / 2), math.sin(pitch / 2)
        out.append((q, pos, (cz * sx, sz * sx, sz * cx, cz * cx)))
    return out


def renumber_handles(doc):
    n = [0]

    def walk(o):
        if isinstance(o, dict):
            if "HandleId" in o:
                o["HandleId"] = str(n[0])
                n[0] += 1
            for v in o.values():
                walk(v)
        elif isinstance(o, list):
            for v in o:
                walk(v)
    walk(doc)


def build_sector(doc, cfg):
    rc = doc["Data"]["RootChunk"]
    nodes, data = rc["nodes"], rc["nodeData"]["Data"]
    pad_idx = [i for i, n in enumerate(nodes) if is_pad(n)]
    entity_idx = [i for i in pad_idx if nodes[i]["Data"]["$type"] == "worldEntityNode"]
    if not entity_idx:
        raise SystemExit("the sector has no pad node to copy")
    template_node = copy.deepcopy(nodes[entity_idx[0]])
    template_data = copy.deepcopy(next(d for d in data if int(d["nodeIdx"]) == entity_idx[0]))
    # vehicle spawners copy the first spawner node (the Watson pad's, which this tool never touches)
    spawner_idx = next(i for i, n in enumerate(nodes)
                       if n["Data"]["$type"] == "worldPopulationSpawnerNode" and i not in pad_idx)
    spawner_node = copy.deepcopy(nodes[spawner_idx])
    spawner_data = copy.deepcopy(next(d for d in data if int(d["nodeIdx"]) == spawner_idx))
    # keep the other nodes, renumbered
    remap, kept = {}, []
    for i, n in enumerate(nodes):
        if i not in pad_idx:
            remap[i] = len(kept)
            kept.append(n)
    kept_data = []
    for d in data:
        if int(d["nodeIdx"]) in remap:
            d["nodeIdx"] = str(remap[int(d["nodeIdx"])])
            kept_data.append(d)
    for pad in cfg["pads"]:
        x, y, z = pad["pos"]
        for j, path in enumerate(pad["entities"]):
            n_ = copy.deepcopy(template_node)
            n_["Data"]["entityTemplate"]["DepotPath"]["$value"] = path
            n_["Data"]["debugName"]["$value"] = path.rsplit("\\", 1)[-1][:-4]
            kept_data.append(place(template_data, x, y, z, pad["yaw"], pad["node_id"] + j,
                                   pad["streaming_distance"], len(kept)))
            kept.append(n_)
    for c in colliders(cfg):
        n_ = copy.deepcopy(template_node)
        n_["Data"]["entityTemplate"]["DepotPath"]["$value"] = c["entity"]
        n_["Data"]["debugName"]["$value"] = c["entity"].rsplit("\\", 1)[-1][:-4]
        kept_data.append(place(template_data, *c["pos"], 0.0, c["node_id"], c["streaming_distance"], len(kept)))
        kept.append(n_)
    for sp in cfg.get("spawners", []):
        n_ = copy.deepcopy(spawner_node)
        n_["Data"]["objectRecordId"]["$value"] = sp["record"]
        n_["Data"]["debugName"]["$value"] = SPAWNER_PREFIX + sp["record"]
        x, y, z = sp["pos"]
        kept_data.append(place(spawner_data, x, y, z, sp["yaw"], sp["node_id"],
                               sp.get("streaming_distance", spawner_data["streamingDistance"]), len(kept)))
        kept.append(n_)
    for dc in cfg.get("decals", []):
        n_ = {"HandleId": "0", "Data": decal_node(dc)}
        d = place(template_data, *dc["pos"], 0.0, dc["node_id"], dc.get("streaming_distance", 80.0), len(kept))
        i, j, k, r = dc["quat"]
        d["transform"]["orientation"].update(i=i, j=j, k=k, r=r)
        d["scale"] = {"$type": "Vector3", "X": dc["scale"][0], "Y": dc["scale"][1], "Z": dc["scale"][2]}
        kept_data.append(d)
        kept.append(n_)
    refs = []
    for q, pos, quat in quest_nodes(cfg):
        name = q["ref"].rsplit("#", 1)[-1]
        if q["kind"] == "camera":
            n_ = copy.deepcopy(template_node)
            n_["Data"]["entityTemplate"]["DepotPath"]["$value"] = CAMERA_ENT
            n_["Data"]["debugName"]["$value"] = QUEST_PREFIX + name
        else:
            n_ = {"HandleId": "0", "Data": {
                "$type": "worldStaticMarkerNode",
                "data": {"HandleId": "0", "Data": copy.deepcopy(MARKER_DATA[q["kind"]])},
                "debugName": {"$type": "CName", "$storage": "string", "$value": QUEST_PREFIX + name},
                "isEnabled": 1, "isHostOnly": 0, "isVisibleInGame": 1, "proxyScale": None,
                "sourcePrefabHash": "0", "tag": "None", "tagExt": "None",
                "tags": {"$type": "redTagList", "tags": []}}}
        d = place(template_data, *pos, 0.0, node_id(q["ref"]), q["streaming_distance"], len(kept))
        d["transform"]["orientation"].update(i=quat[0], j=quat[1], k=quat[2], r=quat[3])
        kept_data.append(d)
        kept.append(n_)
        refs.append({"$type": "NodeRef", "$storage": "string", "$value": q["ref"]})
    rc["nodeRefs"] = refs
    rc["nodes"] = kept
    rc["nodeData"]["Data"] = kept_data
    rc["persistentNodes"] = copy.deepcopy(kept_data)
    renumber_handles(doc)
    return len(kept) - len(remap)


def colliders(cfg):
    """Proxy collider tiles (proxy_colliders.py build): {"entity", "pos", "streaming_distance",
    "node_id"} rows from the tiles file that "colliders" names (relative to this folder)."""
    if not cfg.get("colliders"):
        return []
    return json.load(open(os.path.join(HERE, cfg["colliders"]), encoding="utf-8"))["tiles"]


def fit_block(doc, cfg):
    (x0, y0, z0), (x1, y1, z1) = cfg["base_box"]
    spots = [{"pos": pos, "streaming_distance": q["streaming_distance"]} for q, pos, _ in quest_nodes(cfg)]
    for pad in cfg["pads"] + cfg.get("spawners", []) + colliders(cfg) + spots:
        x, y, z = pad["pos"]
        r = pad.get("streaming_distance", 100.0)
        x0, y0, z0 = min(x0, x - r), min(y0, y - r), min(z0, z - r)
        x1, y1, z1 = max(x1, x + r), max(y1, y + r), max(z1, z + r)
    for desc in doc["Data"]["RootChunk"]["descriptors"]:
        if desc["data"]["DepotPath"]["$value"] == cfg["sector"]:
            desc["streamingBox"]["Min"].update(X=x0, Y=y0, Z=z0)
            desc["streamingBox"]["Max"].update(X=x1, Y=y1, Z=z1)
            return (x0, y0, z0), (x1, y1, z1)
    raise SystemExit(f"no descriptor for {cfg['sector']} in the block")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--config", default=os.path.join(HERE, "pad_sectors.json"))
    ap.add_argument("--wolvenkit", default=DEFAULT_WOLVENKIT)
    args = ap.parse_args()
    cfg = json.load(open(args.config, encoding="utf-8"))
    tmp = tempfile.mkdtemp(prefix="pad_sector_")
    try:
        files = {}
        for key in ("sector", "block"):
            src = os.path.join(SOURCE, cfg[key])
            cli(args.wolvenkit, "cr2w", "-s", src, "-o", tmp)
            files[key] = (src, os.path.join(tmp, os.path.basename(src) + ".json"))
        sector = json.load(open(files["sector"][1], encoding="utf-8"))
        added = build_sector(sector, cfg)
        block = json.load(open(files["block"][1], encoding="utf-8"))
        box = fit_block(block, cfg)
        out = os.path.join(tmp, "out")
        os.makedirs(out)
        for key, doc in (("sector", sector), ("block", block)):
            path = files[key][1]
            json.dump(doc, open(path, "w", encoding="utf-8"), indent=1)
            cli(args.wolvenkit, "cr2w", "-d", path, "-o", out)
            shutil.copy(os.path.join(out, os.path.basename(files[key][0])), files[key][0])
        print(f"{added} pad node(s) in {cfg['sector']}; descriptor box {box}")
        print("now run tools/packaging/pack_archive.py")
    finally:
        shutil.rmtree(tmp, ignore_errors=True)


if __name__ == "__main__":
    main()
