"""Rebuild hud_flight.inkhud from the game's own HUD entries.

The plugin copies this file's "hud flight" entry into every HUD entries resource as it loads
(src/red4ext/Hooks/ResourceSerializer_OnDependenciesReady.cpp). The whole file is the HUD only on
the developer fallback that puts a FlightHUD component on each vehicle
(src/red4ext/Hooks/Entity_Initialize.cpp, attachToPlayer off). For that it has to be the game's
base\\gameplay\\gui\\prototype_hud.inkhud with the "hud flight" entry added. Anything the game adds
or changes in a patch (entries, the contexts an entry shows in) is missing until this is run again.

Steps (WolvenKit.Console 9.0.1, the WOLVENKIT_PACK setting; the game, the GAME setting):
  1. prototype_hud.inkhud is unbundled from the game's content archives and serialized.
  2. The current hud_flight.inkhud is serialized and its "hud flight" entry taken.
  3. The game's entries with "hud flight" first (where it has always been) are written back to
     src/wolvenkit/source/archive/user/jackhumbert/widgets/hud_flight.inkhud.
  4. The written file is serialized again and must hold exactly what step 3 wrote.
Pack it with tools/packaging/pack_archive.py. Run after every game patch.

Not WolvenKit 10.0.0-beta.1: it reads the contextVisibility bitfield with the wrong names
(the game's "SceneDefault, SceneTier1" comes out as "SceneTier4, SceneTier5") and writes those
names back as those bits, so every game entry's scene tiers change in the round trip.

Usage: python hud_flight.py --work <scratch dir> [--check]
  --check: exit 1 if the archive's file differs from what this would write (nothing is written).
"""

import argparse
import json
import os
import shutil
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))  # tools/, for common
from common import paths  # noqa: E402

REPO = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
OURS = os.path.join(REPO, r"src\wolvenkit\source\archive\user\jackhumbert\widgets\hud_flight.inkhud")
VANILLA = r"base\gameplay\gui\prototype_hud.inkhud"
FLIGHT_ENTRY = "hud flight"


def wk(*args):
    r = subprocess.run([paths.WOLVENKIT_PACK, *args], capture_output=True, text=True)
    if r.returncode != 0:
        raise SystemExit(f"WolvenKit {args[0]} failed ({r.returncode}):\n{r.stdout[-2000:]}\n{r.stderr[-2000:]}")
    return r.stdout


def serialize(cr2w, out_dir):
    os.makedirs(out_dir, exist_ok=True)
    wk("cr2w", "-s", cr2w, "-o", out_dir)
    with open(os.path.join(out_dir, os.path.basename(cr2w) + ".json"), encoding="utf-8") as f:
        return json.load(f)


def entry_name(entry):
    return entry["hudEntryName"]["$value"]


def contents(doc):
    """What the file holds, without the export header (dates, paths)."""
    return json.dumps(doc["Data"], sort_keys=True)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--work", required=True, help="scratch folder (emptied first)")
    ap.add_argument("--check", action="store_true")
    args = ap.parse_args()

    work = os.path.abspath(args.work)
    shutil.rmtree(work, ignore_errors=True)
    os.makedirs(work)

    content = os.path.join(paths.GAME, "archive", "pc", "content")
    wk("unbundle", content, "-o", os.path.join(work, "unbundled"), "-r", "^" + VANILLA.replace("\\", "\\\\") + "$")
    vanilla_file = os.path.join(work, "unbundled", VANILLA)
    if not os.path.exists(vanilla_file):
        raise SystemExit(f"{VANILLA} not found in {content}")
    vanilla = serialize(vanilla_file, os.path.join(work, "vanilla"))
    ours = serialize(OURS, os.path.join(work, "ours"))

    flight = [e for e in ours["Data"]["RootChunk"]["entries"] if entry_name(e) == FLIGHT_ENTRY]
    if len(flight) != 1:
        raise SystemExit(f"{OURS} has {len(flight)} '{FLIGHT_ENTRY}' entries, expected 1")
    entries = [e for e in vanilla["Data"]["RootChunk"]["entries"] if entry_name(e) != FLIGHT_ENTRY]

    built = vanilla
    built["Data"]["RootChunk"]["entries"] = flight + entries
    built["Header"]["ArchiveFileName"] = ""

    old = {entry_name(e) for e in ours["Data"]["RootChunk"]["entries"]}
    new = {entry_name(e) for e in built["Data"]["RootChunk"]["entries"]}
    print(f"{len(entries)} game entries + '{FLIGHT_ENTRY}'; added {sorted(new - old)}, removed {sorted(old - new)}")
    same = contents(built) == contents(ours)
    if args.check:
        print("up to date" if same else "out of date: run without --check")
        sys.exit(0 if same else 1)
    if same:
        print("already up to date")
        return

    json_dir = os.path.join(work, "built")
    os.makedirs(json_dir)
    json_file = os.path.join(json_dir, "hud_flight.inkhud.json")
    with open(json_file, "w", encoding="utf-8") as f:
        json.dump(built, f, indent=2)
    out_dir = os.path.join(work, "out")
    os.makedirs(out_dir)
    wk("cr2w", "-d", json_file, "-o", out_dir)
    reread = serialize(os.path.join(out_dir, "hud_flight.inkhud"), os.path.join(work, "reread"))
    if contents(reread) != contents(built):
        raise SystemExit(f"the written file does not read back as built: compare {json_file} with {os.path.join(work, 'reread')}")
    shutil.copyfile(os.path.join(out_dir, "hud_flight.inkhud"), OURS)
    print(f"wrote {OURS}")


if __name__ == "__main__":
    main()
