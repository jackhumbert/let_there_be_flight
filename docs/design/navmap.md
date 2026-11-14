# Nav map

`navmap.bin`: per 2 m column, the vertical gaps a car's centre can occupy (solids grown by a 3 m
radius, 1.5 m half-height cylinder), only gaps reachable from open sky, in zlib tiles of 128 x 128
columns. The Race's doorways are appended (tag `LTBFDOOR`). Read by `src/red4ext/Navigation/NavMap.hpp`
and `tools/common/navmap.py`; written by `tools/navmap_bake/navmap.py` (format in its docstring).

The bake knows no water: over the sea, the bay and the canals the open-sky gap reaches down to the
seabed. As it loads a tile, the plugin (and `tools/common/navmap.py`) raises such a gap's bottom to the
sea's surface (`NavMap::kSeaLevel`, z 0, where the ocean's water patches are placed) plus the car's half
height, so the planner keeps its floor clearance above the water. Without it a free roam leg over the
bay flew the line down to 4 m above the water at 60 m/s, and a car flying into the sea kills V.

It is baked offline from the game's own files by `tools/navmap_bake/` (about 3 hours; the steps,
requirements and format notes are in `tools/navmap_bake/README.md`): cooked collision of every
streaming sector, terrain height fields, building proxy meshes (visible buildings often lack
collision up high), our proxy colliders (`docs/design/city-collision.md`), the street furniture
placed as entities or destructible meshes (lamps, traffic lights, sign gantries, railings:
`tools/navmap_bake/entities.py`; intact destructibles count as solid), less the blockers the
plugin removes.

Shipping: too big for git. The bakes are release assets of the public repo
[cp2077_navmap](https://github.com/jackhumbert/cp2077_navmap), one release per bake, tagged
`<game version>-<first 12 hex of the SHA-256>`, so other mods can use them too. CMake downloads the
one pinned in the root `CMakeLists.txt` (URL and SHA-256) or takes a local bake
(`-DLTBF_NAVMAP_FILE=...`) and embeds it in the DLL as resource `NAVMAP`. A new bake for players:
`python tools/navmap_bake/publish.py <navmap.bin> [--game <version>]` uploads it, checks the served
hash and updates the URL and hash; commit `CMakeLists.txt`.

Checks against the live game: `tools/ingame/validate_bake.py` (rays) and
`tools/ingame/check_navmap.py` (the plugin's `FlightNavMap.GetGaps`).
