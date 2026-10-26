# Nav map bake (tools/heightmap)

Builds `navmap.bin`, the city-wide free-space map the autopilot plans over
(`src/red4ext/Navigation/NavMap.hpp`), from the game's own files. Background, decisions and
validation results: `docs/autodrive-flight-plan.md`.

What goes in: cooked collision of every exterior and interior streaming sector (boxes, capsules,
convex and triangle meshes from the geometry cache; presets that block vehicles only), terrain
height fields, and building proxy meshes (visible buildings often lack vehicle collision up high).
Output: per 2 m column, the vertical gaps a car's centre can occupy (solids grown by a 3 m radius,
1.5 m half-height cylinder), keeping only gaps reachable from open sky.

## Requirements

- WolvenKit.Console 10.0.0-beta.1 or later (9.x cannot read 2.5 sectors), at
  `D:\cp2077_work\build\tools\wolvenkit-10.0.0-beta.1\` with the game's `oo2ext_7_win64.dll`
  next to it. Its `appsettings.json` needs `"MeshExportArgs": { "WithMaterials": false }`
  (original kept as `appsettings.json.orig`), or mesh export fails asking for a depot path.
- Python with numpy, scipy, Pillow, matplotlib. About 25 GB free disk for a full run.
- The game install (archives are read directly). Paths are set for this PC (game on
  `D:\Games\SteamLibrary\...`); check `ARCHIVES` in `proxies.py` and `DEFAULT_WOLVENKIT` in
  `sectors.py` on another machine.

## Full run (about 3 hours on kevin, 16 threads)

`W` is a work folder (not in the repo). With Phantom Liberty installed, `ep1_1_nightcity.archive`
overrides 15,111 of the base game's 15,121 exterior sectors and adds Dogtown, so the ep1 sectors
are the source; the base-game geometry cache fills a few gaps.

```
CLI=D:\cp2077_work\build\tools\wolvenkit-10.0.0-beta.1\WolvenKit.CLI.exe
PC="D:\Games\SteamLibrary\steamapps\common\Cyberpunk 2077\archive\pc"

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

Previews and checks:

- `rasterize.py --presets ... -o W\city4 --cell 4` + `preview.py W\city4 -o top.png`: shaded
  top-surface map. `spans.py preview W\spans -o gaps.png --bounds x0,y0,x1,y1`: gaps per column.
- `validate.py W\spans [--teleport x,y,z]`: compares the bake with live raycasts through the CET
  bridge around the player (collision streams in only ~60 m around a parked car, so keep
  `--radius` near 55). `check_navmap.py W\spans`: compares the plugin's `FlightNavMap.GetGaps`
  with the bake (needs navmap.bin installed and the game running).

Last full bake (2026-10-26, game 2.5): outputs kept in `D:\cp2077_work\build\heightmap\`
(`navmap.bin`, `spans\free.npz`, `spans\solid.npz`, `geometry.npz`, `presets_table.json`,
`nodes.json`). The raw sectors and per-sector triangles (~20 GB) were not kept.

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
