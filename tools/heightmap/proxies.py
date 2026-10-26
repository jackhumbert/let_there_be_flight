"""Building proxy meshes as solid: visible buildings often lack vehicle collision up high.

Input: <sector>.proxies.json from sectors.py (mesh depot path, position, quaternion, scale per
placed worldBuildingProxyMeshNode). Steps:

  export   Unbundle every referenced proxy mesh from the base and ep1 archives and export it to
           .glb with WolvenKit (needs MeshExportArgs.WithMaterials = false in the CLI's
           appsettings.json; with materials on it fails asking for a depot path). WolvenKit
           names the .glb after the mesh file and different proxy folders reuse names, so each
           mesh is exported into a folder named after its parent folder id.
  place    Read the .glb files, place every instance, and write <sector>.proxy.npz next to the
           collision files (tris + preset "Building Proxy"), which spans.py bin picks up.

Usage:
  python proxies.py export --sectors S --work W [--jobs 6]
  python proxies.py place --sectors S --work W
"""

import argparse
import collections
import glob
import json
import os
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor

import numpy as np

from glb import read_glb_triangles
from sectors import DEFAULT_WOLVENKIT, quat_matrix

GAME = r"D:\Games\SteamLibrary\steamapps\common\Cyberpunk 2077"
ARCHIVES = [
    os.path.join(GAME, r"archive\pc\content\basegame_3_nightcity.archive"),
    os.path.join(GAME, r"archive\pc\ep1\ep1_1_nightcity.archive"),
]
PRESET = "Building Proxy"
BATCH = 60


def all_instances(sectors):
    for path in glob.glob(os.path.join(sectors, "*.proxies.json")):
        stem = os.path.basename(path)[: -len(".proxies.json")]
        yield stem, json.load(open(path))


def glb_path(work, depot):
    parts = depot.replace("/", "\\").split("\\")
    return os.path.join(work, "glb", parts[-2], parts[-1][: -len(".mesh")] + ".glb")


def cmd_export(args):
    depots = sorted({inst[0] for _, lst in all_instances(args.sectors) for inst in lst})
    print(f"{len(depots)} proxy meshes referenced", file=sys.stderr)
    raw = os.path.join(args.work, "raw")
    for a in ARCHIVES:
        subprocess.run(
            [args.wolvenkit, "unbundle", a, "-o", raw, "-r", r"_external.proxy.*\.mesh$", "-v", "Minimal"],
            stdout=subprocess.DEVNULL,
            check=False,
        )
    todo = collections.defaultdict(list)  # parent folder id -> source files
    missing = 0
    for d in depots:
        src = os.path.join(raw, d)
        if not os.path.exists(src):
            missing += 1
            continue
        if not os.path.exists(glb_path(args.work, d)):
            todo[d.replace("/", "\\").split("\\")[-2]].append(src)
    print(f"{missing} not found in the archives, {sum(map(len, todo.values()))} to export", file=sys.stderr)

    # one WolvenKit call per batch of folders; names only clash across folders, so a batch
    # never holds two files with the same name
    jobs, batch, names = [], [], set()
    for folder, files in sorted(todo.items()):
        base = {os.path.basename(f) for f in files}
        if batch and (len(batch) >= BATCH or names & base):
            jobs.append(batch)
            batch, names = [], set()
        batch.append((folder, files))
        names |= base

    if batch:
        jobs.append(batch)

    def run(batch):
        tmp = os.path.join(args.work, "tmp", batch[0][0])
        os.makedirs(tmp, exist_ok=True)
        files = [f for _, fs in batch for f in fs]
        subprocess.run(
            [args.wolvenkit, "export", *files, "-o", tmp, "-v", "Minimal"],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
            check=False,
        )
        for folder, fs in batch:
            out = os.path.join(args.work, "glb", folder)
            os.makedirs(out, exist_ok=True)
            for f in fs:
                g = os.path.join(tmp, os.path.basename(f)[: -len(".mesh")] + ".glb")
                if os.path.exists(g):
                    os.replace(g, os.path.join(out, os.path.basename(g)))
        return len(files)

    done = 0
    with ThreadPoolExecutor(args.jobs) as pool:
        for n in pool.map(run, jobs):
            done += n
            print(f"{done} exported", file=sys.stderr)


def cmd_place(args):
    cache = {}
    bad = collections.Counter()
    total = 0
    for stem, lst in all_instances(args.sectors):
        tris = []
        for depot, pos, quat, scale in lst:
            g = glb_path(args.work, depot)
            if g not in cache:
                try:
                    cache[g] = read_glb_triangles(g) if os.path.exists(g) else None
                except Exception as err:
                    cache[g] = None
                    bad[repr(err)[:60]] += 1
            t = cache[g]
            if t is None or not len(t):
                bad["missing glb"] += 1
                continue
            rot = quat_matrix(dict(zip(("i", "j", "k", "r"), quat)))
            w = (t.astype(np.float64) * np.asarray(scale)) @ rot.T + np.asarray(pos)
            tris.append(w.astype(np.float32))
        if tris:
            t = np.concatenate(tris)
            np.savez(
                os.path.join(args.sectors, stem + ".proxy.npz"),
                tris=t,
                preset=np.zeros(len(t), np.uint16),
                preset_names=np.array([PRESET]),
            )
            total += len(t)
    print(f"{total} proxy triangles, problems {dict(bad)}", file=sys.stderr)


def main():
    ap = argparse.ArgumentParser()
    sub = ap.add_subparsers(dest="cmd", required=True)
    for name in ("export", "place"):
        p = sub.add_parser(name)
        p.add_argument("--sectors", required=True)
        p.add_argument("--work", required=True)
        p.add_argument("--wolvenkit", default=DEFAULT_WOLVENKIT)
        p.add_argument("--jobs", type=int, default=6)
    args = ap.parse_args()
    {"export": cmd_export, "place": cmd_place}[args.cmd](args)


if __name__ == "__main__":
    main()
