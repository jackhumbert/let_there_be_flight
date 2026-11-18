# The flight simulator

`tools/flightsim` flies the plugin's own flight code offline, so how a mode feels, and how it meets the ground, can
be measured on every car and kept from regressing without the game. The modes it flies are described in
`docs/design/flight-modes.md`. A whole Autopilot trip on the same body (`Trip.hpp`) is in `docs/design/autopilot.md`.

```
cmake --build build --target flightsim          # not built by default
build/flightsim.exe [--mode standard|hover|fly|drone|agdrone|droneassist] [--dt 0.0333] [--csv <dir>] [--set name=value]... [scenario]...
build/flightsim.exe --vehicles tools/flightsim/vehicles.tsv [--match <name part>] [scenario]...
build/ltbf_tests.exe feel                         # the feel tests over the fleet
```

`flightsim.cpp`'s header lists every option.

## Sim

`Sim` (`Sim.hpp`) is a rigid body flown by the plugin's functions, called as the game calls them:

- the input smoothing (`Flight::StepControls`), the stats (`UpdateDynamic`), the mode (`Flight::Standard`,
  `Drone`, ...) and the force tail (`ForceTail`: `ApplyPhysics`, the mass and tensor scaling) are the plugin's;
- FindGround is modelled: four rays straight down from the wheel emitters, averaged with `GroundFromHits`, over a
  height field or, with `groundRay` set, over the city's collision (Colliders);
- the body is modelled. The linear part integrates the force tail's world force as an acceleration (the component
  scales it by mass, PhysX divides it out) plus gravity and PhysX's linear damping. The angular part applies the
  mode torque times the car's per-axis gain (`Flight::TorqueResponse`), answering `torqueDelay` (30 ms) late, the
  spin capped at 7 rad/s, PhysX's default max angular velocity;
- the ground contact: see Ground contact.

`--set` changes the tuning (the car's mass, gains, delay and gravity; Standard's gains as `a.*`; the force tail's
damping; the contact switches). `Car::responseError` gives the body a response off the one the controllers are
given, to see how much error a mode tolerates.

The modes (`Mode`): `standard` (also `assist` and `hoverfly`, its older names), `hover`, `fly`, `drone`, `agdrone`
(Drone plus its script's anti-gravity line, so it is modelled exactly) and `droneassist`. Hover & Fly as releases up
to 0.3.20 flew it (`FlightModeHoverFlyClassic`) is not one of them.

## Scenarios and the feel tests

`Scenarios.cpp` holds the situations a beginner meets, each a scripted input over a small world and the numbers it
is judged by:

- Standard's: `hands_off`, `yaw_hold`, `yaw_tap`, `surge_release`, `surge_turn`, `sway_release`, `brake_hold`,
  `top_speed`, `climb_short`, `climb_long`, `descend_land`, `roof_edge_off`, `roof_edge_on`, `bump` (a parked car
  passing under), `pitch_hold`, `tilt_recovery`, `tilt_recovery_yaw`, and the knocks (`knock_roll`, `knock_pitch`,
  each with `_yaw` for a turn held, and `knock_low`, knocked while hovering low);
- Drone's: `drone_pitch`, `drone_roll`, `drone_yaw` (a key held 3 s then let go), `drone_spin_release`,
  `drone_lift`, `drone_surge`, `drone_angular_brake`, and its skid over the ground (`drone_skid_tilt`, `_drop`,
  `_grade`).

Any scenario can be flown in any mode. Without scenario names `flightsim` flies the mode's own kind.

`tests/feel_tests.cpp` (`ltbf_tests feel`) loads `tools/flightsim/vehicles.tsv`, flies the scenarios over every row
on all cores, and checks each metric against a bound, naming the cars out of it. What it checks is the feel spec in
`docs/design/flight-modes.md`.

## The fleet run

`python tools/flightsim/fleet.py` flies every distinct car of the table through `flightsim --vehicles` and prints, for
each metric, the spread over the fleet and the cars at either end. Records with the same physics (most records are
paint and trim variants) are flown once. `--metric yaw_hold.yaw_rate_3s` picks metrics, `--mode` the mode, `--csv`
keeps the table. `--terrain` flies the terrain runs instead (Terrain runs).

## The car table

`vehicles.tsv` has one row per physical car: a drive model, a wheel setup and an entity template shared by many
records. Per car: mass, air resistance, braking friction, inertia (`s_momentOfInertia`), thruster tensor, wheel
emitter offsets, body bottom and thruster count, then the chassis's drag areas, box, underside profile and body
class. Records on CDPR's stand-in drive model (AVs, drones, trains, quest props) are left out: no player flies them.

Where the numbers come from:

- **TweakDB** (`tweak_physics.py`, from a dump of the game's vehicle records): mass, air resistance, braking
  friction and inertia come from the drive model. Where a drive model's inertia is 0,0,0 (some trucks, vans and
  buses) the game works it out from the collision shape, so it comes from a car measured in game.
- **The rig** (`rig_geometry.py`): the thruster tensor, wheel offsets, bottom and thruster count depend on where the
  wheels sit on the model, which TweakDB does not hold. They are worked out from the game files, nothing spawned:
  the wheel emitters from the entity template, the thrusters from the bones of the `vehicle_rig` and
  `deformation_rig`, the centre of mass from the drive model, and the thruster configuration from the wheel slots
  each appearance binds, picked as `FlightConfiguration.cpp` picks it (read from LTBF's sources by
  `thruster_slots.py`'s parser). `GetThrusterTensor` is a sum over the thrusters. A reading from a spawned car takes
  precedence where there is one; `tweak_physics.py --geometry` fills the rest from the rig.
- **The chassis shape** (`aero.py`): each car's chassis collision shapes (the `.phys` convex hulls, boxes and
  capsules) give its drag area per body axis, its box in the centre of mass frame (`boxMinX` .. `boxMaxZ`) and its
  underside profile (`underside`: the lowest point per 0.5 m square over the footprint, from each convex piece's
  downward facets). `tweak_physics.py --aero` carries them into the table.

`thruster_slots.py` (ctest `thruster_slots`) checks, with no game, that the configuration each vanilla template and
appearance gets finds every bone and wheel slot its thrusters need, that the thrusters sit on their wheels, that no
wheel or brake is left showing, and that the count matches the table. Its rig facts are `vehicle_slots.tsv`, which
`vehicle_slots.py` regenerates from the game files after a patch. `ALLOW` lists what it lets pass, each with its
reason.

## Checking against the game

`flightsim --replay <ltbf_hoverfly_*.csv>` takes a recording written in game through `FlightTelemetry` (a row a
frame: the car's state, the controls and the ground distance), cuts it where each control starts, flies each piece
offline from the recorded state on the recorded controls and ground, and prints the game's and the simulator's
numbers side by side (`Replay.hpp`). With `--vehicles <tsv> --match <car>` the piece is flown on that car's row.
What differs is the body model: its gains, its delay, and what it leaves out.

## Colliders: the city's collision

The nav map is the Autopilot's world, not the ground a car's rays meet: it holds where a car's centre can be, every
solid grown sideways and up and rounded to steps. Hover flown on it would see a ledge before its edge and a smooth
ramp as stairs. So the simulator flies over the collision the nav map was baked from.

`region_pack.py` cuts a box out of the nav map bake (`tools/navmap_bake`) into one region file (`.ltbfregion`):

- the sector triangles of the presets that block vehicles, as the bake keeps them, less the blockers the plugin
  moves away (`src/navigation/blockers.txt`) and the hidden colliders listed by hand
  (`tools/world/hidden_colliders.json`);
- LTBF's proxy colliders;
- the terrain height fields as triangles, the finest level present winning;
- a mesh id per triangle: which of the game's PhysX geometries it belongs to.

```
python -P -E tools/flightsim/region_pack.py x0,y0,x1,y1 -o <region.ltbfregion> [--margin 20]
python -P -E tools/flightsim/region_pack.py --regions tools/flightsim/terrain_regions.tsv [--force]
```

`terrain_regions.tsv` lists the boxes the terrain runs fly over: city streets, Dogtown and Badlands rock. Region
files are tens of megabytes, so they live in the work folder (`<regions>/<name>.ltbfregion`, by default
`flightsim_regions` under `tools/common/paths.py`'s work folder), not in the repo. Tests build small worlds in memory.

`Colliders` (`Colliders.hpp`, which has the file format) loads a region, bins the triangles on a grid and casts
rays: the first hit and its normal, both faces. The bake has no water, so the sea bed is a surface.

## Ground contact

How the body meets the ground (`Sim.hpp`):

- **The stop** (the default): the body's lowest point never goes below the ground, and downward speed into it is
  removed. Over a height field the lowest point is the body's bottom. With `groundRay` it is the car's underside:
  each square of the chassis's underside profile and the thruster spheres' bottoms at the four wheel emitters, each
  cast from the centre's height down. A car without a profile uses its centre at the bottom.
- **Points** (`--set pointContact=1`): an impulse at each point that touches, with friction, so a corner down turns
  the body. `sphereContacts`, `sphereRadius` and `openBottom` follow the developer settings `thrusterContacts`,
  `thrusterRadius` and `thrusterOpenBottom`.
- **PhysX** (`--physx`, with `--terrain`): the game's own PhysX 3.4 moves the body (`PhysXWorld.hpp`): the line's
  ground within 30 m as one PhysX mesh per mesh id, as the game has it, and the car its thruster colliders and
  chassis box. This shows what the Sim's own contact cannot: PhysX does not know a triangle's neighbours across two
  meshes and pushes a shape sliding over a flush joint. It loads the game's DLLs at run time and builds only against
  the PhysX 3.4 headers (`LTBF_PHYSX34`); nothing of PhysX is shipped or committed. `--physx-joint` slides a body
  over flush mesh joints alone.

The Sim models height only: nothing stops the car sideways. A collision is `Knock` (a spin and a velocity change at
once, and the recovery timer) or `Collide` (pushed out of a box).

`flightsim --seams <region>` lists where a region's drivable ground meets without a shared edge, and the step or gap
a car finds there (`Seams.hpp`).

## Terrain runs

`TerrainRuns.hpp` flies Standard along lines over real ground, holding speed and heading and never touching lift,
so the mode alone keeps the height.

- **The lines** (`terrain_runs.tsv`): name, region, start and end, the start's surface, and tags for what the
  ground is (`Profile::Tags`: flat, slope, kerbs, steps, drop, crest, dip, rough, terrain, gaps, wall).
  `flightsim --sample <region> <count>` draws random lines in a region as rows for the file; lines picked by hand go
  at the end.
- **The profile** a line is flown over: from its start surface, the ground every 0.5 m, cast from a little over the
  last point so the car goes under decks and off edges as it would; cut at the first wall. A line must leave room
  for the widest car's half width to either side.
- **The driver**: pure pursuit to a point ahead on the line, the yaw key in proportion to the heading error, surge
  holding the speed.
- **A run** stops when its nose runs into something half a metre over the underside, or the contact lifts the car
  too far in one frame (`blocked`). Rising ground under the nose counts as a touch (`nose_hits`).
- **The metrics**: the height over and under the hover height (p95), `lag` (the delay that best lines the car's
  height up with the ground's; 1.5 s means it did not follow), contacts, time in contact, impact speed, the lowest
  clearance and where on the body, `pitch_wobble` and `roll_wobble`, vertical acceleration and `jerk_p95`, rays a
  frame.

```
build/flightsim.exe --terrain tools/flightsim/terrain_runs.tsv [--only <line or tag>] [--speeds 40,80,130] [--hover 0] [--vehicles <tsv> [--match <car>]]
python -P -E tools/flightsim/fleet.py --terrain [--speeds 40,80,130] [--save runs.tsv | --from runs.tsv]
```

The hover height is each car's resting height when flight comes on (`--hover 0`), as in game. `fleet.py --terrain`
flies every distinct car (the underside counted in what makes two cars the same) and sums the runs up by class
(`fleet.car_class`: bike, the 12 and 8 thruster cars, long cars over 7 m, then the body class from the chassis
shape), by kind of ground and by speed, then names the cars that touch most in each class. Touching is judged in
metres on every car; following relative to what the car's climb can do.

`tests/terrain_runs_tests.cpp` and `tests/colliders_tests.cpp` check the profile, the sampler, the driver, the
contact and the ground on worlds built in memory (a kerb, a wall, a ramp, a crest, an overpass, a ledge under one
corner).

## Checking the terrain runs against the game

A car is driven along a line in game by the same driver, and its recording logs FindGround's four rays a frame
(start, hit, hit point, normal) beside the Hover & Fly recording's columns (`TerrainCheck.hpp`).

- `flightsim --ground-check <recording> --region <name>` casts each recorded ray again through the region: where
  both hit, how far apart and how differently tilted; where only one did, missing or extra collision (streaming, a
  blocker the region kept, water). It also checks where the wheel emitters sit on the recorded body against the
  car's row, and how often the game's car touched at each recorded pose.
- `flightsim --replay <recording> --region <name> [--window 3]` flies the recording in windows from the recorded
  state, on the recorded controls, over the region, and sets the heights side by side.
- `python -P -E tools/flightsim/terrain_compare.py <dir>` runs both over a folder of recordings
  (`<line>@<kmh>_<record>.csv`) and sums them up a recording a line. A recording whose car found no ground on most
  frames (it fell through the world) is left out; one is cut where the car stood still at full surge (it ran into
  something the bake does not hold, such as traffic).
