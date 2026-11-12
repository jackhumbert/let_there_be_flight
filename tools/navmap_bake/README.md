# Nav map bake (tools/navmap_bake)

Builds `navmap.bin`, the city-wide free-space map the autopilot plans over
(`src/red4ext/Navigation/NavMap.hpp`), from the game's own files. Background, decisions and
validation results: `docs/design/navmap.md`.

What goes in: cooked collision of every exterior and interior streaming sector (boxes, capsules,
convex and triangle meshes from the geometry cache; presets that block vehicles only), terrain
height fields, and building proxy meshes (visible buildings often lack vehicle collision up high).
Output: per 2 m column, the vertical gaps a car's centre can occupy (solids grown by a 3 m radius,
1.5 m half-height cylinder), keeping only gaps reachable from open sky.

## Requirements

- WolvenKit.Console 10.0.0-beta.1 or later (9.x cannot read 2.5 sectors), at
  anywhere (the `WOLVENKIT` setting, `WOLVENKIT_CLI10`) with the game's `oo2ext_7_win64.dll`
  next to it. Its `appsettings.json` needs `"MeshExportArgs": { "WithMaterials": false }`
  (original kept as `appsettings.json.orig`), or mesh export fails asking for a depot path.
- Python with numpy, scipy, Pillow, matplotlib. About 25 GB free disk for a full run.
- The game install (archives are read directly): the `GAME` setting (`CP2077_GAME_DIR`), see
  `tools/common/paths.py`.

## Full run (about 3 hours on 16 threads)

`W` is a work folder (not in the repo). With Phantom Liberty installed, `ep1_1_nightcity.archive`
overrides 15,111 of the base game's 15,121 exterior sectors and adds Dogtown, so the ep1 sectors
are the source; the base-game geometry cache fills a few gaps.

```
CLI=<the WOLVENKIT setting: WolvenKit.CLI.exe 10.x>
PC="<the game folder>\archive\pc"

# 1. unbundle sectors + geometry caches + collision presets
$CLI unbundle $PC\ep1\ep1_1_nightcity.archive -o W\raw\ep1 -r "(_compiled.default.(exterior|interior)_.*\.streamingsector|\.geometry_cache)$"
$CLI unbundle $PC\content\basegame_3_nightcity.archive -o W\raw -r "03_night_city\.geometry_cache$"
$CLI unbundle $PC\content\basegame_1_engine.archive -o W\engine -r "engine.physics.*\.json$"
$CLI cr2w -s W\raw\ep1\base\worlds\03_night_city\sectors\_generated\collisions\03_night_city.geometry_cache -o W\json_ep1
$CLI cr2w -s W\raw\base\worlds\03_night_city\sectors\_generated\collisions\03_night_city.geometry_cache -o W\json   # ~500 MB json each
$CLI cr2w -s W\engine\engine\physics\collision_presets.json -o W\engine_json

# 2. decode meshes, preset table
python geometry_cache.py W\json_ep1\03_night_city.geometry_cache.json W\json\03_night_city.geometry_cache.json -o W\geometry.npz
python presets.py W\engine_json\collision_presets.json.json -o W\presets_table.json

# 3. sectors -> world triangles, terrain height fields, building proxy placements (~1.5 h)
python sectors.py --sectors W\raw\ep1\base\worlds\03_night_city\_compiled\default --geometry W\geometry.npz -o W\sectors --jobs 10 --match "^(exterior|interior)_.*\.streamingsector$"

# 4. building proxies: export meshes to glb, place them (~20 min)
python proxies.py export --sectors W\sectors --work W\proxywork --jobs 6
python proxies.py place --sectors W\sectors --work W\proxywork

# 5. terrain at 2 m, then the span map (~40 min)
python rasterize.py --sectors W\sectors --no-collision -o W\terrain2 --cell 2
python spans.py bin --sectors W\sectors --presets W\presets_table.json --terrain-dir W\terrain2 -o W\spans
python spans.py build W\spans --jobs 10
python spans.py merge W\spans
python spans.py reach W\spans

# 6. runtime file (~15 MB) -> copy into game_dir/red4ext/plugins/let_there_be_flight/ before cmake --install
python navmap.py W\spans -o W\navmap.bin
```

Landing pads (`pads.py`, minutes; needs only the raw exterior sectors and `navmap.bin`, see
`docs/design/landing-pads.md`):

```
$CLI unbundle $PC\ep1\ep1_1_nightcity.archive -o W\raw\ep1 -r "_compiled.default.exterior_.*\.streamingsector$"
python pads.py scan --sectors W\raw\ep1\base\worlds\03_night_city\_compiled\default -o W\pads
python pads.py build W\pads --navmap W\navmap.bin -o W\pads\helipads.json --preview W\pads\pads.png
python flat_spots.py W\navmap.bin -6400 -400 -2300 1200 -o spaceport.png --radius 12   # spots for helipads_overrides.json
```

Visual-only pads (`docs/design/landing-pads.md`, "Recipe"): `pad_collision.py` finds them (unbundle
the level-3 sectors too: `-r "_compiled.default.exterior_-?\d+_-?\d+_-?\d+_3\.streamingsector$"`,
517 files, and run `sectors.py` on them); `pad_donor.py` / `pad_kit.py` / `pad_geometry.py` /
`pad_appearances.py` / `pad_entity.py` turn a complete prefab into our pad entity;
`pad_sector.py` places the entities listed in `pad_sectors.json` in our sector and fits its
streaming block descriptor. Hide the proxy in `src/navigation/hidden_nodes.txt`, list the pad
with `own_deck` in `helipads_overrides.json`.

Invisible blockers (`blockers.py`, ~20 min with 10 jobs): collision groups using the presets
"Vehicle Blocker" / "Block Player and Vehicles", for removal with ArchiveXL `nodeDeletions` in
`src/archive_xl/let_there_be_flight.archive.json` (index = nodeData index, expectedNodes = the
nodeData count; one deletion removes one collision group). Groups are `pure` (only blockers),
`foliage` (blockers plus query-only tree crowns) or `mixed` (also real walls / trunks):

```
$CLI unbundle $PC\ep1\ep1_1_nightcity.archive -o W\raw\ep1 -r "_compiled.default.(exterior|interior)_.*\.streamingsector$"
python blockers.py scan --sectors W\raw\ep1\base\worlds\03_night_city\_compiled\default -o W\blockers --jobs 10
python blockers.py report W\blockers -o W\blockers.md
```

Collision for proxy-only buildings (`proxy_colliders.py`, 2026-10-28): parts of the city exist only
as proxy meshes and have no collision at all, so flying cars pass through them (the raised complex
at (-1600, -680), 150 to 350 m up, is the biggest; 80 clusters, 8.5 km2 of surface in the city).
The tool finds proxy surface with no vehicle-blocking collision or terrain within 4 m, cooks it
into PhysX triangle meshes (`tools/physx_cook`, see its README) and writes one collider-only
entity per 128 m tile, placed by `pad_sector.py` from `proxy_collider_tiles.json` ("colliders" in
`pad_sectors.json`). The proxies stay the visual. Needs the full run's `sectors` (collision
triangles) and `terrain2`. About an hour:

```
python proxy_colliders.py scan --sectors W\raw\ep1\base\worlds\03_night_city\_compiled\default -o P --match "^exterior_.*_[1-6]\.streamingsector$"
python proxy_colliders.py export P --raw W\proxywork\raw
python proxy_colliders.py solid P --tris W\sectors --presets W\presets_table.json
python proxy_colliders.py audit P --terrain W\terrain2 --city=-3000,-3500,1600,3800
python proxy_colliders.py force P          # tools/world/proxy_collider_force.json: buildings audit missed
python proxy_colliders.py detail P --sectors W\raw\ep1\base\worlds\03_night_city\_compiled\default
python proxy_colliders.py drawn P --raw W\proxywork\raw   # nothing drawn up close where a proxy hides
python proxy_colliders.py build P --entities P\ents --force --drawn
$CLI cr2w -d P\ents\*.ent.json -o ..\..\src\wolvenkit\source\archive\user\jackhumbert\entities   # in batches of ~40
python pad_sector.py
python ..\pack_archive.py
```

- Finest proxy first: a proxy surface counts as covered by real collision, by terrain, by being
  under the top real surface of its column (undersides below streets), or by an already kept
  finer proxy. Partly covered triangles are cut down to 6 m and only the uncovered pieces kept.
- The "under the top real surface" rule also drops the walls of a building with collision only near
  its top (the Dogtown combat tower: collision to 117 m and from about 312 m, walls between drawn
  only). Such buildings are listed by hand in `tools/world/proxy_collider_force.json` and `force`
  gives their placements' triangles inside each entry's box collision wherever no real collision
  is within 4 m. The list came from the space the bake's old hollow fill closed
  (`tools/world/opened_patches.py`); the fill is gone.
- The ocean patches (`global_ocean_patch_*`, 87 km2) are left out (`--exclude`), and so are terrain
  proxies.
- 548 tiles, 1.69 M triangles, 50 MB cooked (27 MB packed archive, from 7 MB). Each tile streams
  within its reach + 300 m.
- The nav map bake takes the same triangles: `spans.py bin --extra P\colliders.npz` (written by
  `build`), so the Autopilot does not plan through the new walls.
- Checked in game with rays: the five reported points inside the
  raised complex all hit the proxy's surfaces above and below (e.g. (-1200, -689): 294.9 and
  208.4, the proxy's floor slabs at 295 and 208).

Previews and checks:

- `rasterize.py --presets ... -o W\city4 --cell 4` + `preview.py W\city4 -o top.png`: shaded
  top-surface map. `spans.py preview W\spans -o gaps.png --bounds x0,y0,x1,y1`: gaps per column.
- To check the bake in game: compare it with live raycasts around the player (collision streams in only
  ~60 m around a parked car, so keep the radius near 55), and the plugin's `FlightNavMap.GetGaps` with the
  bake (needs navmap.bin installed).

Last full bake (2026-10-26 evening, game 2.5, with `spans.py` fill_hollow): everything in
`heightmap/bake_2026-10-26/` under the work folder (raw sectors, per-sector triangles, proxies,
spans, `navmap.bin`), 36 minutes with 10 jobs, reusing `geometry.npz` and
`presets_table.json` from the morning bake in `heightmap/` (whose
`navmap.bin` is the previous map). Against the morning bake about 2% of columns lost free space
(towers whose collision has a hole, now solid, and some low arcades under skyscrapers), none
gained. 3 sectors failed to parse (JSONDecodeError): exterior_-17_21_0_0, exterior_-21_-1_0_0,
exterior_-23_1_4_0.

## Format notes worth knowing

- 2.5 node names: `worldCookedCollisionNode` (was `worldCollisionNode`). Actor positions are
  absolute `WorldPosition` fixed point (17 fractional bits); `scaleIndex -1` means unscaled.
- Capsules: `size.X` radius, `size.Y` half height, axis along local Z (checked in game).
- Geometry cache: shape hash = first 8 bytes of `sectorGeometries[i].ta`; the run after the last
  sector lives in `alwaysLoadedSector` / `alwaysLoadedSectorDDB`. Blobs are PhysX 3.4 cooked
  `MESH` v15 / `CVXM` v13.
- Terrain: `worldTerrainCollisionNode` holds an inline PhysX `HFHF` v1 height field (264 x 264
  int16 samples at 0.5 m, material 127 = hole) placed by `actorTransform`; sector levels 2 to 6
  hold the same terrain at falling detail.
- Building proxies: `worldBuildingProxyMeshNode` meshes under `sectors\_external\proxy\` (base
  archive), placed by `nodeData` (`nodeIdx` is a string there).
