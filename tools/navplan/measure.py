"""Plan routes with navplan and measure the racing lines: shared by bench.py and tune.py.

navplan (tools/navplan/navplan.cpp) is the plugin's own planner and car limits
(src/red4ext/Navigation/Vehicle.hpp); each plan writes <prefix>_profile.csv and prints a
"metrics {json}" line, which measure() turns into the numbers the tools compare (score()).
"""

import concurrent.futures as cf
import csv
import json
import math
import os
import random
import subprocess

import numpy as np

from dips import dips

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
NAVPLAN = os.path.join(ROOT, "build", "navplan.exe")  # set_navplan(): a frozen copy, so a rebuild mid-run changes nothing


def set_navplan(path):
    global NAVPLAN
    if path:
        NAVPLAN = path


def benchmark_routes():
    """The benchmark routes (routes.json) as (name, start, goal)."""
    with open(os.path.join(HERE, "routes.json"), encoding="utf-8") as f:
        return [(r["name"], [float(v) for v in r["start"]], [float(v) for v in r["goal"]]) for r in json.load(f)["routes"]]


def cmake_navmap():
    with open(os.path.join(ROOT, "build", "CMakeCache.txt")) as f:
        for line in f:
            if line.startswith("LTBF_NAVMAP_FILE:"):
                return line.split("=", 1)[1].strip()
    raise SystemExit("no LTBF_NAVMAP_FILE in build/CMakeCache.txt: pass --navmap")


def vehicle_spec(name):
    with open(os.path.join(HERE, "vehicles.json")) as f:
        cars = json.load(f)
    if name not in cars:
        raise SystemExit(f"no car {name} in vehicles.json ({', '.join(k for k in cars if not k.startswith('_'))})")
    return ",".join(f"{k}={v}" for k, v in cars[name].items())


def routes(extra, seed=1):
    """The benchmark routes (routes.json) and `extra` helipad-to-helipad ones, 800 to 2500 m apart."""
    out = benchmark_routes()
    pads = []
    with open(os.path.join(ROOT, "src", "navigation", "helipads.tsv")) as f:
        for line in f:
            if line.strip() and not line.startswith("#"):
                c = line.split("\t")
                pads.append([float(c[1]), float(c[2]), float(c[3]) + 2.0])
    rng = random.Random(seed)
    tries = 0
    n = len(out)
    while len(out) < n + extra and tries < 10000:
        tries += 1
        a, b = rng.sample(pads, 2)
        if 800.0 <= math.dist(a[:2], b[:2]) <= 2500.0:
            out.append((f"pad{len(out)}", a, b))
    return out


def measure(prefix, metrics):
    """The line's numbers from navplan's profile and metrics line."""
    with open(prefix + "_profile.csv") as f:
        rows = list(csv.DictReader(f))
    if len(rows) < 10:  # a line too short to fly (the plan failed)
        return None
    d = np.array([float(r["d"]) for r in rows])
    v = np.array([float(r["v"]) for r in rows])
    p = np.array([[float(r["x"]), float(r["y"]), float(r["z"])] for r in rows])
    clear = np.array([float(r["clearance"]) for r in rows])
    why = [r["why"] for r in rows]
    lost = sum(t - lo for _, t, lo, _ in dips(d, v, why))
    # horizontal heading every 10 m: bend in deg/10 m and its change (jerk)
    k = 5
    h = np.diff(p[::k, :2], axis=0)
    heading = np.unwrap(np.arctan2(h[:, 1], h[:, 0]))
    bend = np.degrees(np.abs(np.diff(heading))) if len(heading) > 2 else np.zeros(1)
    jerk = np.abs(np.diff(bend)).mean() if len(bend) > 2 else 0.0
    return {
        "ok": metrics["racing_line"],
        "lost": lost,
        "jerk": float(jerk),
        "sharpest": float(np.percentile(bend, 99)) if len(bend) else 0.0,
        "tight": float(np.mean(clear < 3.0) * 100.0),
        "time": metrics["time"],
        "height": float(p[:, 2].mean()),
        "ms": metrics["ms"],
    }


def plan(navmap, vehicle, route, params, out):
    name, a, b = route
    prefix = os.path.join(out, name)
    env = dict(os.environ, NAVPLAN_RACE="1", NAVPLAN_VEHICLE=vehicle,
               NAVPLAN_PARAMS=",".join(f"{k}={v}" for k, v in params.items()))
    try:
        r = subprocess.run([NAVPLAN, navmap, *map(str, a), *map(str, b), prefix], env=env, capture_output=True, text=True,
                           timeout=120)
    except subprocess.TimeoutExpired:
        return None
    line = next((l for l in r.stdout.splitlines() if l.startswith("metrics ")), None)
    if not line or not os.path.exists(prefix + "_profile.csv"):
        return None
    return measure(prefix, json.loads(line[len("metrics "):]))


def run_all(navmap, vehicle, rts, params, out, jobs):
    os.makedirs(out, exist_ok=True)
    with cf.ThreadPoolExecutor(jobs) as pool:
        return list(pool.map(lambda r: plan(navmap, vehicle, r, params, out), rts))


def score(m, base):
    """One route's score against the defaults' line (0 = as good)."""
    lost_line = m is None or not m["ok"]
    if base is None or not base["ok"]:
        # the defaults found no racing line here: only gaining one counts
        return 0.0 if lost_line else -2.0
    if lost_line:
        return 10.0
    s = (m["lost"] - base["lost"]) / 10.0
    s += 3.0 * (m["jerk"] / max(base["jerk"], 1e-3) - 1.0)
    s += 1.0 * (m["sharpest"] / max(base["sharpest"], 1e-3) - 1.0)
    s += (m["tight"] - base["tight"]) / 2.0
    s += 3.0 * (m["time"] / max(base["time"], 1e-3) - 1.0)
    s += max(m["height"] - base["height"], 0.0) / 20.0
    # plan time only when it grows a lot: absolute times depend on how many plans run at once
    s += max(m["ms"] / max(base["ms"], 1.0) - 2.0, 0.0)
    return s
