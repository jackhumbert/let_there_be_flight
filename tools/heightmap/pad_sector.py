"""Place our pad entities in the world (docs/helipads.md, step 8): rewrite our sector's pad nodes
from pad_sectors.json and fit its streaming block descriptor.

Works on the committed CR2W files in src/wolvenkit/source/archive: serializes the sector and the
block, keeps every node that is not one of our pad entities (the Watson car spawners), drops the
old pad nodes, adds one worldEntityNode per part entity of each listed pad at the pad's transform
(a copy of the first existing pad node, so the sector must hold one; the first was built from a
vanilla v68 exterior sector's worldEntityNode), sets the descriptor box to base_box joined with
each pad's position +- its streaming distance, and writes both files back.

  python pad_sector.py [--config pad_sectors.json] [--wolvenkit <WolvenKit 10 CLI>]
  then: python tools/pack_archive.py

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

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
SOURCE = os.path.join(REPO, "src", "wolvenkit", "source", "archive")
DEFAULT_WOLVENKIT = r"D:\cp2077_work\build\tools\wolvenkit-10.0.0-beta.1\WolvenKit.CLI.exe"
PAD_PREFIX = "user\\jackhumbert\\entities\\ltbf_landing_pad"


def cli(wk, *args):
    r = subprocess.run([wk, *args], capture_output=True, text=True)
    if "Error" in r.stdout or r.returncode not in (0,):
        raise SystemExit(f"WolvenKit {' '.join(args[:2])} failed:\n{r.stdout[-2000:]}{r.stderr[-1000:]}")


def is_pad(node):
    d = node["Data"]
    return d.get("$type") == "worldEntityNode" and \
        str(d.get("entityTemplate", {}).get("DepotPath", {}).get("$value", "")).startswith(PAD_PREFIX)


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
    if not pad_idx:
        raise SystemExit("the sector has no pad node to copy")
    template_node = copy.deepcopy(nodes[pad_idx[0]])
    template_data = copy.deepcopy(next(d for d in data if int(d["nodeIdx"]) == pad_idx[0]))
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
        h = math.radians(pad["yaw"]) / 2.0
        for j, path in enumerate(pad["entities"]):
            n_ = copy.deepcopy(template_node)
            n_["Data"]["entityTemplate"]["DepotPath"]["$value"] = path
            n_["Data"]["debugName"]["$value"] = path.rsplit("\\", 1)[-1][:-4]
            d = copy.deepcopy(template_data)
            d["nodeIdx"] = str(len(kept))
            kept.append(n_)
            d["globalNodeId"]["hash"] = str(pad["node_id"] + j)
            d["transform"]["position"] = {"$type": "Vector4", "W": 0, "X": x, "Y": y, "Z": z}
            d["transform"]["orientation"].update(i=0, j=0, k=math.sin(h), r=math.cos(h))
            for key in ("streamingRefPoint", "preboostStreamingRefPoint", "secondaryRefPointPosition"):
                d[key] = {"$type": "Vector3", "X": x, "Y": y, "Z": z}
            d["streamingDistance"] = float(pad["streaming_distance"])
            kept_data.append(d)
    rc["nodes"] = kept
    rc["nodeData"]["Data"] = kept_data
    rc["persistentNodes"] = copy.deepcopy(kept_data)
    renumber_handles(doc)
    return len(kept) - len(remap)


def fit_block(doc, cfg):
    (x0, y0, z0), (x1, y1, z1) = cfg["base_box"]
    for pad in cfg["pads"]:
        x, y, z = pad["pos"]
        r = pad["streaming_distance"]
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
        print("now run tools/pack_archive.py")
    finally:
        shutil.rmtree(tmp, ignore_errors=True)


if __name__ == "__main__":
    main()
