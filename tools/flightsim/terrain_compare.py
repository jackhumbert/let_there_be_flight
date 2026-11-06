"""Every in-game terrain recording of a folder against the offline collision and simulator (docs/design/hover-fly-terrain.md,
step 6): flightsim --ground-check (the game's ground rays cast again through the region) and flightsim --replay
--region (the flight in 3 s windows over the region), summed up a recording a line.

    python -P -E tools/flightsim/terrain_compare.py <dir> [--flightsim build/flightsim.exe]

<dir> holds cp2077_tools' terrain_check.py output: <line>@<kmh>_<record>.csv, a TerrainDriver recording each. The
line's region comes from terrain_runs.tsv, the car from vehicles.tsv by its record. A recording whose game found no
ground on most frames is left out (the car fell through the world), and one is cut where the game's car stood still
at full surge (it ran into something the bake does not hold). For each: the rays both hit and
how far apart (p95), the rays one alone hit, the simulator's height off the game's (mean: + the simulator higher; and
RMS), its ground distance against the game's (medians), the game's car's lowest clearance over the region (the
simulator's contact points at the recorded pose) and the touches of the game's car and of the simulator's.
"""

import argparse
import csv
import glob
import os
import re
import statistics
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))


def run(cmd):
    return subprocess.run(cmd, capture_output=True, text=True, check=True).stdout


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("dir")
    ap.add_argument("--flightsim", default=os.path.join(REPO, "build", "flightsim.exe"))
    ap.add_argument("--set", action="append", default=[], help="name=value, passed to flightsim --set for the replay")
    args = ap.parse_args()

    with open(os.path.join(HERE, "terrain_runs.tsv"), encoding="utf-8") as f:
        regions = {r[0]: r[1] for r in (line.rstrip("\n").split("\t") for line in f if line.strip() and line[0] != "#")}
    with open(os.path.join(HERE, "vehicles.tsv"), encoding="utf-8") as f:
        names = {r["record"]: r["name"] for r in csv.DictReader(f, delimiter="\t")}
    vehicles = os.path.join(HERE, "vehicles.tsv")
    sets = [a for kv in args.set for a in ("--set", kv)]

    rows = []
    for path in sorted(glob.glob(os.path.join(args.dir, "*@*_*.csv"))):
        m = re.match(r"(.+)@(\d+)_(.+)\.csv$", os.path.basename(path))
        if not m or m.group(1) not in regions:
            continue
        line, kmh, record = m.groups()
        region = regions[line]
        with open(path, encoding="utf-8") as f:
            header, *lines = f.read().splitlines()
        recorded = list(csv.DictReader([header, *lines]))
        # the game's ground found (all four rays hit) on under half the frames: the car was not over the line's ground
        # (placed before the ground had streamed in, it fell through)
        found = sum(all(r[f"r{i}hit"] == "1" for i in range(4)) for r in recorded)
        if found < len(recorded) / 2:
            print(f"left out {os.path.basename(path)}: the game found ground on {found} of {len(recorded)} frames")
            continue
        # cut where the game's car stood still at full surge for a second: it ran into something the bake does not
        # hold (traffic, a parked car), and the simulator flies on through it
        still, cut = 0.0, len(recorded)
        for i, r in enumerate(recorded):
            stuck = abs(float(r["surge"])) > 0.9 and (float(r["vx"]) ** 2 + float(r["vy"]) ** 2) ** 0.5 < 1.0
            still = still + float(r["dt"]) if stuck else 0.0
            if still > 1.0:
                cut = i
                break
        note = ""
        if cut < len(recorded):
            note = f" (cut at {float(recorded[cut]['t']):.1f} s: the game's car stopped against something)"
            tmp_csv = tempfile.NamedTemporaryFile("w", suffix=".csv", delete=False, encoding="utf-8", newline="\n")
            tmp_csv.write("\n".join([header, *lines[:cut]]) + "\n")
            tmp_csv.close()
            path = tmp_csv.name
        g = run([args.flightsim, "--ground-check", path, "--region", region, "--vehicles", vehicles, "--match", record])
        counts = re.search(r"(\d+) rays: both hit (\d+), the game only (\d+), the region only (\d+)", g)
        p95 = re.search(r"p95 ([\d.]+) m, max ([\d.]+) m", g)
        game = re.search(r"lowest clearance (-?[\d.]+) m, (\d+) touches", g)
        with tempfile.TemporaryDirectory() as tmp:
            run([args.flightsim, "--replay", path, "--region", region, "--vehicles", vehicles, "--match", record,
                 "--csv", tmp, *sets])
            with open(os.path.join(tmp, "terrain_replay.csv"), encoding="utf-8") as f:
                trace = list(csv.DictReader(f))
        dz = [float(t["sim_z"]) - float(t["game_z"]) for t in trace]
        gd = [float(t["game_distance"]) for t in trace if float(t["game_distance"]) >= 0]
        sd = [float(t["sim_distance"]) for t in trace if float(t["sim_distance"]) >= 0]
        touches = sum(1 for a, b in zip(trace, trace[1:]) if b["sim_contact"] == "1" and a["sim_contact"] == "0")
        rows.append({
            "car": names.get(record, record)[:22], "line": line, "kmh": kmh,
            "rays": int(counts.group(1)), "both": int(counts.group(2)),
            "only": int(counts.group(3)) + int(counts.group(4)), "dz95": float(p95.group(1)),
            "dzmax": float(p95.group(2)),
            "bias": statistics.mean(dz) if dz else float("nan"),
            "rms": statistics.mean(d * d for d in dz) ** 0.5 if dz else float("nan"),
            "game_d": statistics.median(gd) if gd else float("nan"),
            "sim_d": statistics.median(sd) if sd else float("nan"), "touches": touches,
            "game_clear": float(game.group(1)), "game_touches": int(game.group(2)), "note": note})
        if note:
            os.unlink(path)
    if not rows:
        sys.exit(f"no recordings in {args.dir}")

    print(f"{'car':22} {'line':18} {'kmh':>3} | {'rays':>5} {'only':>4} {'dz95':>5} {'dzmax':>5} | "
          f"{'bias':>6} {'rms':>5} {'game d':>6} {'sim d':>6} | {'clear':>5} {'touches g/s':>11}")
    for r in rows:
        print(f"{r['car']:22} {r['line']:18} {r['kmh']:>3} | {r['rays']:5d} {r['only']:4d} {r['dz95']:5.2f} "
              f"{r['dzmax']:5.2f} | {r['bias']:+6.2f} {r['rms']:5.2f} {r['game_d']:6.2f} {r['sim_d']:6.2f} | "
              f"{r['game_clear']:5.2f} {r['game_touches']:5d}/{r['touches']:<5d}{r['note']}")
    bias = [r["bias"] for r in rows]
    print(f"\n{len(rows)} recordings; rays one alone hit {sum(r['only'] for r in rows)} of {sum(r['rays'] for r in rows)};"
          f" the simulator's height off the game's: mean {statistics.mean(bias):+.3f} m, from {min(bias):+.2f} to "
          f"{max(bias):+.2f}")


if __name__ == "__main__":
    main()
