# Autopilot (flying autodrive) - plan

Status (2026-10-26): working end to end in game 2.5, not released. Offline bake in
`tools/heightmap/` (README there), C++ map + planner in `src/red4ext/Navigation/`, the Autopilot
flight mode in `src/redscript/Flight/FlightModeAutopilot.reds`, engaged by the vanilla autodrive
hold while flying. What is left: "Hand-off: remaining work" at the end. Research notes:
`docs/autopilot-research.md`. Videos of the test route: `D:\cp2077_work\build\heightmap\video\`.

## Goal

With flight active, a tracked destination is reachable hands-off. The car flies above traffic,
can use gaps under structures (overpasses, megabuilding underpasses) when they are worth it, and
looks flown: it banks into turns, pitches into climbs, carries momentum and has a turning radius
that grows with speed. The same machinery can fly NPC vehicles later.

## Decisions so far

- Not vanilla autodrive. Native autodrive only follows road lanes and drops out once the car is
  off the lane, so it cannot fly above traffic. (Earlier idea "approach A" is dropped.)
- Offline, city-wide collision map baked from the game files, shipped with the mod. Runtime
  raycasts only see what has streamed in (a few hundred metres), too short for planning at speed.
- Multi-level: per 2 m cell a sorted list of solid height intervals (Recast-style spans), so a
  cell can have several free gaps (road under an overpass, open sky above it).
- Path search, trajectory and map queries in C++ in the plugin. No redscript scale limits.
- The autopilot is its own flight mode (a `FlightMode` subclass next to Hover & Fly), not a
  change to Hover & Fly.

## Offline map pipeline (`tools/heightmap/`)

Source data (game 2.5, Phantom Liberty installed; `ep1_1_nightcity.archive` overrides 15,111
of the base game's 15,121 exterior sectors and adds Dogtown):

- `base/worlds/03_night_city/_compiled/default/exterior_X_Y_Z_L.streamingsector`: 16,199
  sectors. Level 0 is a 64 m grid; higher levels are coarser.
  - `worldCookedCollisionNode` (2.5 name; `worldCollisionNode` before): actors with a world
    transform, optional uniform scale, and shapes (box, capsule, convex, triangle mesh), each
    with optional local position/rotation and a collision preset.
  - `worldTerrainCollisionNode` (sector levels 2 to 6, same terrain at falling detail): an
    inline PhysX height field (264 x 264 samples at 0.5 m), placed by an actor transform.
- `.../_generated/collisions/03_night_city.geometry_cache`: the convex and triangle meshes the
  shapes reference by 64-bit hash, as PhysX 3.4 cooked blobs. 146,404 entries, 50,788 unique.
- `engine/physics/collision_presets.json`: preset names and the groups they block. Only presets
  that block vehicles are kept ("Simple Environment Collision" is the vehicle version of the
  environment; "Complex Environment Collision" is the detailed version for people).

Tools: WolvenKit.Console 10.0.0-beta.1 (first version that reads 2.5 sectors; 9.0.1 fails on a
new node class), Python with numpy/scipy/PIL.

Steps:

1. `geometry_cache.py`: decode cooked meshes and convex hulls into `geometry.npz`.
2. `sectors.py`: serialize sectors with WolvenKit, turn collision into world-space triangles
   per sector (`<sector>.npz`) and terrain height fields (`<sector>.terrain.npz`).
3. `presets.py`: preset table with "blocks vehicles" per preset.
4. `rasterize.py`: today a top-surface map (highest solid per cell) plus terrain; next, solid
   intervals per cell (see below).
5. `preview.py`: shaded PNG for eyeballing.

Numbers from the first full run: 11,707 sectors with collision, about 314 million triangles,
826 sectors with terrain; the 4 m merged map covers 99.97% of cells.

Multi-level span map (`spans.py`, done 2026-10-26): exterior and interior sectors (13,787
collision nodes), building proxies, plus terrain, binned into 256 m tiles, voxelized at 2 m x 0.5 m (z -200 to
1000 m), solid below each column's lowest surface and below terrain, inflated by a car
cylinder (3 m radius, 1.5 m half height), free runs per column, then `reach` keeps gaps
connected to open sky (enclosed building interiors drop out).

- Result: 40.6M columns, 44.2M free gaps (96% reachable), 2.1M columns with two or more
  reachable gaps (freeways, bridges, megabuilding plazas, overhangs). `free.npz` is 23 MB.
- Runtime format estimate: int16 floor per column (81 MB raw, compresses well) plus about 12 MB
  of extra (floor, ceiling) gaps for multi-level columns.
- Known gaps: 0.15% of city columns have no data (no collision in any exterior/interior sector
  and a terrain hole); they are treated as blocked, so the planner also cannot fly over them.
  3 sectors fail to serialize in WolvenKit 10 beta (`exterior_-17_21_0_0`, `exterior_-21_-1_0_0`,
  `exterior_-23_1_4_0`). Quest sectors are not included.
- Building proxies (`proxies.py`, 2026-10-26): visible buildings often have no vehicle
  collision up high (a 312 m Watson tower has none above 20 m; others stop at 128 m of 191 m).
  The 987 `worldBuildingProxyMeshNode` meshes are exported with WolvenKit (CLI
  `appsettings.json` needs `MeshExportArgs.WithMaterials = false`; original kept as
  `appsettings.json.orig`), placed by nodeData, and baked as solid (1.76M triangles; 212
  instances have no exported mesh yet).
- Capsules: axis is local Z (size.X = radius, size.Y = half height), not PhysX's X. Found by
  mapping a leaning tree and a lamp arm with dense in-game rays; X and Y both put them wrong.

Validation against live physics (`validate.py`, 2026-10-26, game 2.5, CET bridge). Rays only
see collision streamed in near the player (about 60 m in 3D for a parked car), so each site
samples 300 to 400 columns within 55 m. "Cell max" compares the bake's top with the highest of
5 x 5 rays over the 2 m cell.

| Site | Top within 1 m (cell max) | Bake > 2 m below game | Bake > 3 m above game |
|---|---|---|---|
| City centre (-1842, -891) | 89.0% | 2.0% (parked/passing cars answer the query too) | 6.2% |
| Elevated freeway (-1322, 47) | 96.0% | 0.0% | 3.3% |
| Badlands (2700, -1712) | 99.7% | 0.3% | 0.0% |
| Dogtown (-1777, -2446) | 89.7% | 0.0% | 2.7% |
| Watson megablock (-740, 972), below 110 m, car at 60 m | 81.7% | 0.4% | 14.9% |

"Above" is expected near buildings (proxies are visible-only; rays cannot see them) and above
the streaming range; the bake practically never misses solid the game has. The car resting on
the freeway deck came to 16.41 m against a baked 16.5 m, and inside the Watson structure it
stopped against a ceiling at 59.7 m against a baked 59.5 m.

Open: the 3 m inflation radius is conservative (the Shion sat in a Watson gap the bake closes);
consider 2 to 2.5 m or a per-vehicle bake. Upper megablock levels are not validated yet (needs
the car flown up there; teleporting high just falls because nothing is streamed in).

## Runtime (C++ in the plugin, driven from a flight mode)

- Map service (done 2026-10-26): `tools/heightmap/navmap.py` writes `navmap.bin` (format in its
  docstring: 68-byte header, tile index, 128 x 128 column tiles as zlib streams of per-column
  counts plus reachable gaps as u16 voxel pairs). 15 MB for the whole map (211 MB inflated).
  `src/red4ext/Navigation/NavMap.{hpp,cpp}` reads it whole at plugin load from the plugin
  folder, inflates tiles on demand (vendored miniz inflate in `ThirdParty/miniz`, 256-tile LRU,
  mutex) and answers GetGaps / FindGap / IsFree / SegmentClear (half-cell steps).
  `FlightNavMap` (`Navigation/FlightNavMap.hpp`, `src/redscript/Navigation/FlightNavMap.reds`)
  exposes them to scripts. `tools/heightmap/check_navmap.py` compared 4,000 random columns in
  the running game with the bake: 0 mismatches, 60 to 100 us per call from Lua.
  Not decided: how navmap.bin ships (15 MB binary; not committed). For local testing it is
  copied into `game_dir/red4ext/plugins/let_there_be_flight/` before `cmake --install`.
- Next for the map service: "highest solid along a segment" and a faster segment walk (DDA over
  columns instead of fixed steps), once the planner shows what it needs.
- Planner (done 2026-10-26, `Navigation/Planner.{hpp,cpp}`): optional direct leg at cruise
  altitude; else weighted A* (1.5) over (column, gap) nodes, 8-connected, gaps linked when they
  overlap by a voxel; cost = distance x (1 + 0.5 under a ceiling) + 3 x rise in preferred
  altitude (floor + 25 m, or mid-gap under a ceiling). Then a height profile inside the joined
  gaps (raised ahead of climbs, max gradient 1), and string pulling to corners. Scripts:
  `FlightNavMap.Plan` (sync), `PlanAsync` / `IsPlanReady` / `TakePlan` (worker thread),
  `SetPlanParameters`, `GetLastPlanInfo`. From the city centre: Watson 2.3 km in 1.0 s
  (2.3M expansions), badlands 4.8 km in 0.32 s (direct: 0.9 ms), Dogtown 1.6 km in 0.09 s,
  under the freeway 1.2 km in 0.28 s; routes ride about 25 m over whatever is below and pass
  under overhangs. Next: cut expansions (the climb term weakens the distance heuristic; try a
  coarse 16 m layer or JPS-style pruning), and let a running request be cancelled.
- Planner: straight line at cruise altitude first (line-of-sight test against the map). If
  blocked, A* over (cell, gap-band) nodes, with neighbouring gaps connected when they overlap
  vertically; costs for length, climbing and low clearance, plus a preference for flying a set
  height above the floor (above traffic) and a cruise band chosen by heading so opposite
  directions separate vertically. Hierarchical layer (16 to 32 m) if long routes are slow.
- Trajectory: string-pull to corners, round corners with arcs/clothoid blends sized from speed,
  check against the map (tighter and slower where they collide), speed profile from curvature
  and climb rate with forward/backward acceleration passes. Replan the next few hundred metres
  when the route changes.
- Following (the new flight mode): L1-style guidance with speed-scaled lookahead and curvature
  feedforward a fraction of a second ahead, then desired acceleration to attitude targets: below
  about 10 m/s fly like a drone (level, sway and lift), above about 25 m/s coordinated turns
  (yaw on velocity, bank = atan(a_lat / g), pitch on climb angle), smooth blend between.
  Attitude targets feed the existing torque/damping path the other modes use.
- Local avoidance: closest-point-of-approach check against nearby vehicles, preferring a climb
  or descent that the map allows.
- Engage/cancel: destination from the tracked mappin (`AutoDriveSystem.GetAutodriveDestination`
  or the mappin system). Any stick/key input above a deadzone cancels. HUD line plus vanilla
  style notifications; set `player_in_autodrive` and a save lock like vanilla.

## Autopilot flight mode (2026-10-26)

`src/redscript/Flight/FlightModeAutopilot.reds`, added after Drone in `FlightComponent`'s mode
list. Destination: `FlightNavMap.SetTargetOverride`, else `AutoDriveSystem.GetAutodriveDestination`.
Plans with `PlanAsync`, then per frame: closest point on the route, a lookahead point (0.7 x speed
+ 12 m, capped 10 m past the next corner, and pulled back until the straight line to it is free
on the nav map), target speed from max speed (45 m/s), corners (judged 15 m either side,
5 m/s2 lateral), goal braking, slowdown when off the route, and a map check along 1.5 s of the
current velocity. Outputs go into the component's surge / yaw / sway / brake inputs; a smoothed
coordinated bank (route curvature + lateral correction, max 25 degrees) and a climb pitch tilt the
up target of `UpdateWithNormalLift`. Player input overrides for 1.5 s; replans when 30 m off the
route; reports "stuck" after 4 s without progress.

Test route: city centre (-1842, -891) to Dogtown (-1777, -2446), 1.65 km, 10 corners, about
45 s; it arrives within 1 m and holds. Bugs found in flight and fixed: over-banking (bank came from
the saturated velocity error, plus the mode's own roll-with-yaw), yaw-rate damping with the wrong
sign (heading oscillation), lookahead cutting corners into buildings, staircase routes from the
wide leg test, mid-speed weaving (sway faded out too early). Flight 8 stayed within 2 m of the
route most of the way (worst 7 m) with no contacts.

Recording: Xbox Game Bar (`tools/sendkey.ps1 -Keys lwin,lalt,r`, files in Videos\Captures,
with audio) looked clearly better than `tools/record.ps1` (ffmpeg ddagrab -> NVENC, then 720p);
both are capped by the game's ~20 to 25 fps at 4K with RT on this PC.

Vanilla autodrive integration (2026-10-26):
- Trigger: the vanilla autodrive hold (`Vehicle_Autodrive`: hold G / L3, 0.5 s) is added to the
  flight input context as `HoldAutodrive`; while flying, `FlightController.ToggleAutopilot`
  switches to the Autopilot mode (refuses without a destination) and back to the previous mode.
  Plain mode cycling and the HUD's "next mode" skip Autopilot. Keyboard Next Mode (also G) now
  switches on release and not after an autodrive hold. Turning flight off leaves Autopilot.
  `AutoDriveSystem.OnEnableAutoDriveRequest` is wrapped to ignore non-Delamain requests while
  flying, so ground autodrive cannot start under a flying car.
- UI: the mode publishes `UI_AutodriveData` (`AutoDriveEnabled`, remaining length / time), so
  vanilla's autodrive panel shows (counters only for a real GPS destination, as in vanilla). On
  arrival: vanilla's toast (LocKey#96672, "Autodrive: destination reached", type Autodrive), then
  the previous mode takes over.
- Cinematic camera: `HoldCinematicCamera` (camera key hold, Q / D-pad right) added to the flight
  context; while the Autopilot flies, `VehicleFlight.HandleFlightCameraInput` queues
  `vehicleCinematicCameraToggleEvent`. Vanilla's shots work on a flying car; the camera is turned
  off when the Autopilot ends.
- Rough edges: the cinematic camera hint does not show (hint list full? a stray vanilla "Fast
  Swim" hint takes a slot); flight hints stay on screen during cinematic shots; "Autopilot" /
  "Disable Autopilot" / the no-destination message are not localized; after the hand-back the car
  can drift some metres before Hover & Fly settles.

Not done: Mod Settings entries for the tuning fields, a cleaner response
when braking for blocked space (it can stall at 0 m/s until the route turns), tuning on other
vehicles (bike, Hellhound), NPC use.

## Open questions

- Ship base-game sectors as well as ep1 (players without Phantom Liberty), or ep1 only?
- Cell size and coverage of the shipped map vs mod download size.
- Dynamic world state not in the static bake (destructibles, quest-only geometry, doors).
- NPC flying traffic on top of the same planner (later).

## Hand-off: remaining work

State on 2026-10-26: everything above is committed on main. The bake outputs of the last run are
in `D:\cp2077_work\build\heightmap\` (navmap.bin and the span map; the 20 GB of intermediate
sector data was not kept, a full rebake is about 3 hours, see `tools/heightmap/README.md`).
How to run and test in game: "Driving the game" in CLAUDE.md (autopilot test flow, Game Bar for
video). Roughly in priority order:

1. Shipping navmap.bin (blocker for a release). 15 MB binary, built from game files on a PC with
   the game. Options: commit it (git LFS or plain), attach it to the GitHub release and have CMake
   fetch it, or have `configure_install` copy it from a local path. Today it is copied into
   `game_dir/red4ext/plugins/let_there_be_flight/` by hand. Without it the plugin logs
   "navmap.bin not found" and the Autopilot mode just holds position. Decide with Jack.
2. Players without Phantom Liberty: the bake uses the ep1 sectors (they override almost all base
   sectors). Check how different a base-only world is; maybe ship two maps or detect which one
   to load.
3. Autopilot polish:
   - Localize the new strings ("Autopilot", "Disable Autopilot", the no-destination message in
     `FlightController.ToggleAutopilot`): keys in the localization json, then
     `python tools/pack_archive.py`.
   - Tuning fields of `FlightModeAutopilot` (max speed, lateral accel, gains) as Mod Settings
     entries (also need localization keys).
   - The cinematic camera hint does not appear in the hint list (a stray vanilla "Fast Swim" hint
     also shows; the list may be full); hide the flight hints while the cinematic camera is on.
   - Braking for map-blocked space ahead can stall the car near 0 m/s until the route turns; make
     it steer along the route instead of only braking.
   - After arrival the previous mode takes over and the car drifted ~70 m and climbed before Hover
     & Fly settled; hold the position on hand-back.
   - Test the gamepad path (hold L3) with `tools/gamepad.py`; with "Gamepad Toggle Requires Hold"
     on, the same hold also toggles flight at its hold time.
   - Test with a real GPS pin (vanilla's panel then shows distance / ETA counters).
   - Tune and test on the bike (Kusanagi) and the Hellhound; only the Shion was flown.
   - Vanilla autodrive also sets facts (`player_in_autodrive`, `disable_combat_events`) and a save
     lock; decide whether the Autopilot should.
4. Planner: the Watson route took 2.3M expansions (1 s) because the climb term weakens the
   distance heuristic. Add a coarse layer (16 to 32 m) or better pruning; let a running request be
   cancelled; join or detach-safe worker threads at plugin unload (a search running at quit could
   crash).
5. Map quality: 3 sectors fail to serialize in WolvenKit 10.0.0-beta.1 (`exterior_-17_21_0_0`,
   `exterior_-21_-1_0_0`, `exterior_-23_1_4_0`; retry with a newer WolvenKit); 0.15% of city
   columns have no data and count as blocked; 30 building proxy placements have no exported mesh;
   quest sectors are not included; upper megablock floors are not validated in game; the 3 m
   car radius is conservative (2 to 2.5 m, or per vehicle size).
6. Game patches: rebake when a patch changes world sectors (the format notes in the heightmap
   README list what 2.5 changed and what to recheck).
7. Later: NPC flying vehicles using the same planner (`FlightComponent` already flies AI-driven
   cars from their drive inputs); fly to arbitrary points from other mods via
   `FlightNavMap.SetTargetOverride`.
