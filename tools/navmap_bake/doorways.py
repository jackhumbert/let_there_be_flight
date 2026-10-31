"""Bake the Autopilot planner's doorways into navmap.bin.

The planner searches long routes in two layers: first over doorways between 32 m blocks, then at
2 m inside the blocks that route passes (src/red4ext/Navigation/Planner.cpp, PlanParams.doorways).
A block's doorways are worked out from the map the first time a search reaches it; baked, a first
plan into new territory is as fast as a repeat one. navplan works them out over the area with the
Race's costs and writes them raw; this packs them with zlib and appends them to the map as
sections the plugin finds by their tag at the end of the file (an older plugin ignores them). A
map's doorway sections are replaced.

    python tools/navmap_bake/doorways.py NAVMAP.bin OUT.bin [--area x0,y0,x1,y1] [--navplan build/navplan.exe]
        [--sets "",minPassage=0]

One set per --sets entry: the Race's costs (height 40, climb 0.5 growing with the rise at scale 30,
minimum passage 12, fixed in navplan's NAVPLAN_DOORWAY_COUNT) with that entry's NAVPLAN_PARAMS
changes. The default bakes the Race's and the Race's without a minimum passage, which
PlanWithRetries falls back to when a goal is reached only through a lower gap (a rooftop north of
the city centre: 11.7 s to plan without it, the retry working out about 40 blocks on the spot).
If the Autopilot's race route fields change (raceRouteHeight, raceClimbCost, raceMinPassage, or
climbScale in FlightNavMap::PlanRaceAsync), rebake: a set made with other costs is not used, and
the plugin falls back to working blocks out as it goes.
"""

import argparse
import os
import struct
import subprocess
import tempfile
import zlib

TAG = b"LTBFDOOR"
# the built-up city and the edge of the Badlands (tools/navplan/city.py's CITY)
CITY = "-3400,-3600,3600,2600"


def strip_section(data, tag):
    """The map without its trailing sections of this tag."""
    while len(data) >= 24 and data[-8:] == tag:
        packed, _raw = struct.unpack_from("<QQ", data, len(data) - 24)
        data = data[: len(data) - 24 - packed]
    return data


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("navmap")
    ap.add_argument("out")
    ap.add_argument("--area", default=CITY, help="x0,y0,x1,y1 in metres (default: the city)")
    ap.add_argument("--navplan", default=os.path.join(os.path.dirname(__file__), "..", "..", "build", "navplan.exe"))
    ap.add_argument("--sets", default=",minPassage=0", help="comma-separated NAVPLAN_PARAMS changes, one set each ('' = the Race's)")
    args = ap.parse_args()

    sections = []
    with tempfile.TemporaryDirectory() as tmp:
        for k, change in enumerate(args.sets.split(",")):
            prefix = os.path.join(tmp, f"set{k}")
            env = dict(os.environ, NAVPLAN_DOORWAY_COUNT=args.area)
            env.pop("NAVPLAN_PARAMS", None)
            if change:
                env["NAVPLAN_PARAMS"] = change
            r = subprocess.run([args.navplan, args.navmap, "0", "0", "0", "0", "0", "0", prefix], env=env, capture_output=True, text=True)
            print(f"set '{change}':", r.stdout.strip().splitlines()[-1] if r.stdout.strip() else r.stderr)
            raw = open(prefix + ".doorways", "rb").read()
            sections.append((zlib.compress(raw, 9), len(raw)))

    data = strip_section(open(args.navmap, "rb").read(), TAG)
    with open(args.out, "wb") as f:
        f.write(data)
        for packed, raw in sections:
            f.write(packed)
            f.write(struct.pack("<QQ", len(packed), raw))
            f.write(TAG)
    print(f"wrote {args.out}: map {len(data) / 1e6:.1f} MB + {len(sections)} doorway set(s) {sum(len(p) for p, _ in sections) / 1e6:.1f} MB ({sum(r for _, r in sections) / 1e6:.1f} MB raw)")


if __name__ == "__main__":
    main()
