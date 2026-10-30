"""Collision preset table from engine/physics/collision_presets.json (WolvenKit JSON).

Shapes in sectors name their preset by CName; WolvenKit prints the string when it knows it and
the FNV-1a 64 hash otherwise. This writes {name, hash, collision_type, query_detect,
blocks_vehicles} per preset so rasterize.py can keep only what a vehicle collides with
(LTBF's ground probe queries the VehicleBlocker group; see FlightComponent.FindGround).

Usage: python presets.py <collision_presets.json.json> -o presets_table.json
"""

import argparse
import json


def fnv1a64(s):
    h = 0xCBF29CE484222325
    for b in s.encode("utf-8"):
        h ^= b
        h = (h * 0x100000001B3) & 0xFFFFFFFFFFFFFFFF
    return h


def names(lst):
    return [str(x["$value"]) for x in lst or []]


def build(path):
    root = json.load(open(path, encoding="utf-8"))["Data"]["RootChunk"]["root"]["Data"]
    table = []
    for p in root["presets"]:
        name = str(p["Name"]["$value"])
        ctype = names(p["CollisionType"])
        detect = names(p["QueryDetect"])
        table.append(
            {
                "name": name,
                "hash": str(fnv1a64(name)),
                "collision_type": ctype,
                "query_detect": detect,
                "blocks_vehicles": "VehicleBlocker" in detect or "VehicleBlocker" in ctype,
            }
        )
    return table


def load_lookup(path):
    """name-or-hash string -> preset entry"""
    lookup = {}
    for p in json.load(open(path)):
        lookup[p["name"]] = p
        lookup[p["hash"]] = p
    return lookup


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("presets_json")
    ap.add_argument("-o", "--out", required=True)
    args = ap.parse_args()
    table = build(args.presets_json)
    json.dump(table, open(args.out, "w"), indent=1)
    for p in table:
        print(f"{'V' if p['blocks_vehicles'] else ' '} {p['name']}")


if __name__ == "__main__":
    main()
