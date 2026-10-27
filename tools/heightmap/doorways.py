"""Bake the Autopilot planner's doorways into navmap.bin.

The planner searches long routes in two layers: first over doorways between 32 m blocks, then at
2 m inside the blocks that route passes (src/red4ext/Navigation/Planner.cpp, PlanParams.doorways).
A block's doorways are worked out from the map the first time a search reaches it; baked, a first
plan into new territory is as fast as a repeat one. navplan works them out over the area with the
Race's costs and writes them raw; this packs them with zlib and appends them to the map as a
section the plugin finds by its tag at the end of the file (an older plugin ignores it). A map that
already has a doorway section gets it replaced.

    python tools/heightmap/doorways.py NAVMAP.bin OUT.bin [--area x0,y0,x1,y1] [--navplan build/navplan.exe]

The Race's costs (height 40, climb 0.5, minimum passage 12) are fixed in navplan
(NAVPLAN_DOORWAY_COUNT); if the Autopilot's race route fields change (raceRouteHeight,
raceClimbCost, raceMinPassage), rebake: a set made with other costs is not used, and the plugin
falls back to working blocks out as it goes.
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
    """The map without a trailing section of this tag, if it has one."""
    if len(data) >= 24 and data[-8:] == tag:
        packed, _raw = struct.unpack_from("<QQ", data, len(data) - 24)
        return data[: len(data) - 24 - packed]
    return data


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("navmap")
    ap.add_argument("out")
    ap.add_argument("--area", default=CITY, help="x0,y0,x1,y1 in metres (default: the city)")
    ap.add_argument("--navplan", default=os.path.join(os.path.dirname(__file__), "..", "..", "build", "navplan.exe"))
    args = ap.parse_args()

    with tempfile.TemporaryDirectory() as tmp:
        prefix = os.path.join(tmp, "set")
        env = dict(os.environ, NAVPLAN_DOORWAY_COUNT=args.area)
        r = subprocess.run([args.navplan, args.navmap, "0", "0", "0", "0", "0", "0", prefix], env=env, capture_output=True, text=True)
        print(r.stdout.strip().splitlines()[-1] if r.stdout.strip() else r.stderr)
        raw = open(prefix + ".doorways", "rb").read()

    packed = zlib.compress(raw, 9)
    data = strip_section(open(args.navmap, "rb").read(), TAG)
    with open(args.out, "wb") as f:
        f.write(data)
        f.write(packed)
        f.write(struct.pack("<QQ", len(packed), len(raw)))
        f.write(TAG)
    print(f"wrote {args.out}: map {len(data) / 1e6:.1f} MB + doorways {len(packed) / 1e6:.1f} MB ({len(raw) / 1e6:.1f} MB raw)")


if __name__ == "__main__":
    main()
