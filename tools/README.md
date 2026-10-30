# Tools

One folder per job. Python scripts put `tools/` on `sys.path` and import shared code from
`common/`; machine paths come from `common/paths.py` (each overridable by an environment variable).
Offline tools never touch the game; everything that drives the game is in `ingame/` and runs inside
`../cp2077_tools/game.py run`.

| Folder | What | Inputs, outputs |
|---|---|---|
| `common/` | `navmap.py` (the one navmap.bin reader), `paths.py`, `toolpath.py` (sibling folders on sys.path) | |
| `navplan/` | `navplan.cpp` (the planner offline, CMake target `navplan`), `bench.py` (benchmark routes, compare builds or parameters), `tune.py` (Optuna search), `measure.py` (shared by both), `dips.py`, `plot.py`, `city.py`, `navviz.py` (planner's view over a recorded run), `routes.json`, `vehicles.json` | navmap.bin, routes; CSV, PNG, video |
| `navmap_bake/` | the nav map bake: `sectors`, `geometry_cache`, `presets`, `proxies`, `glb`, `rasterize`, `preview`, `spans`, `navmap`, `doorways`, `gates`; steps in its README | game archives; navmap.bin, `src/navigation/gates.tsv` |
| `world/` | city collision: `blockers.py` (hidden colliders), `proxy_colliders.py` (+ `physx_cook.py`) | sectors; `src/navigation/blockers.txt`, collider entities |
| `pads/` | landing pads: `pads.py`, `pad_*.py`, `flat_spots.py`, overrides and sector placement | sectors, navmap.bin; `src/navigation/helipads.tsv`, pad entities, our sector |
| `ingame/` | `ltbf_bridge.reds` (LtbfBridge test helpers), `debug_draw/ltbf_draw.reds` (needs debug_draw), `lib.py`, `fly_route.py`, `free_roam.py`, `validate_bake.py`, `check_navmap.py` | the running game; videos, telemetry |
| `telemetry/` | `autopilot.py` (the Autopilot's per-frame CSV), `aerobatics.py` (`[AutopilotA]` log lines) | telemetry; summaries, plots |
| `packaging/` | `pack_archive.py` (run by the build), `icons/` (map pin and AUTOPILOT logo), `physx_cook/` (the C++ PhysX cooker) | archive source; the .archive |
| `maintenance/` | `nexus_comments.py`, `minidump_addrs.py` | |
