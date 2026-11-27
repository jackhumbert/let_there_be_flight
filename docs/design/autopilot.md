# Autopilot

The flying autodrive: hold G with flight on and a destination tracked (a GPS pin, a landing pad, or
`FlightNavMap.SetTargetOverride` in tests) and the car flies there and lands; with no destination it
free roams. Vanilla's analogue is autodrive: the same key, the same facts and HUD panel, and the
controls handed back on arrival.

The Autopilot is C++ in two layers:

- `Navigation::Autopilot` (`src/red4ext/Navigation/Autopilot*`): the whole behaviour, game-free. It
  runs over a `World`, a `PlanQueue`, its `AutopilotParams` and an `Rng`, and takes one `Step` a frame.
  The game and the offline simulation drive the same code.
- `FlightModeAutopilot` (`src/red4ext/Flight/FlightModeAutopilot.*`): the flight mode in the game. It
  gathers the frame's input, calls `Step`, writes the force and torque, and hands what the output asks
  for to its redscript side (`src/redscript/Flight/FlightModeAutopilot.reds`).

The nav map, planner, car limits, pads and gates are in `src/red4ext/Navigation/`
(`docs/design/planner.md`, `docs/design/vehicle-limits.md`); the speed and attitude plan is
`docs/design/trajectory.md`.

## Parts

| File (`Navigation/`) | What |
|---|---|
| `Autopilot.hpp/.cpp` | `Activate`, `Deactivate`, `Teleported`, `Step` (the override, the follower, the state machine), `FlyAerobatic` (attitude, torque, thrust) |
| `AutopilotRoute.cpp` | the route: goal, plan requests, splices, replans, wedge recovery, the trajectory, the landing recheck |
| `AutopilotFreeRoam.cpp` | free roam legs and gates, `ToggleFreeRoam` |
| `AutopilotDescent.cpp` | hold, descent and yaw: `HoldVelocity`, the descent's profile (`DescentProfile`, `DescentTarget`), `Heading`, `TimeToDescent`, `LandSlewYaw`, `ChooseYaw` |
| `Landing.hpp/.cpp` | where it lands: ground spots, pads, landing headings, moving the route's end |
| `AutopilotParams.hpp/.cpp` | the tuning, every field by name |
| `Plans.hpp/.cpp` | the plan requests and the planner's params for each kind |
| `World.hpp/.cpp`, `FlightWorld.hpp/.cpp` | what the Autopilot asks of the world, offline and in game |
| `Tracker`, `Surge`, `DroneAttitude`, `Aerobatics` | the follower, the surge hold, the drone attitude, the attitude control |
| `AutopilotTelemetry.hpp/.cpp` | the telemetry CSV's header and rows |

The state is a String `status` (the names script and tools compare against):

| Status | Meaning |
|---|---|
| idle | not flying anywhere |
| no nav map | the nav map is not loaded |
| planning | a route has been asked for and there is none to follow meanwhile |
| following | flying the route |
| no route | the planner found none |
| free roam | between free roam legs |
| descending | coming down onto the landing spot |
| landed | down (set down on the wheels, or touched down); with the player in the car it asks for flight off |
| arrived | held where the route ended, or above a spot it could not land on |

Being wedged, recovering after a hit and the player's override are timers and flags beside it. A flight
to a destination does not retry `no nav map` or `no route`; free roam does.

## How a flight goes

1. Engage (`Activate`): the state is reset, and free roam is chosen when nothing is tracked. On the
   first frame the destination becomes a route goal (Route) and a plan is asked for from the car itself:
   the take-off is part of the plan (`PlanParams.startRadius`).
2. Plan: the planner runs on its own thread (Plans) with the Race preset (`Navigation::RacePreset`) and
   the car's limits. The result is a line: points 5 m apart, bent as little as the free space allows
   (`docs/design/planner.md`).
3. Trajectory (`BuildTrajectory`, `docs/design/trajectory.md`): the speed, its rate, the time and the
   body tilt along the route from the car's limits, rebuilt whenever the route changes. Every route
   gets one, a free roam leg or a two-point hop (corners rounded by `RoundCorners`) too. The route is a
   `Navigation::Route` (`Route.hpp`): the points, their distances, and the geometry the follower asks
   of it each frame.
4. Follow (`Navigation/Tracker`, `Track` and `Command`): the closest point on the route, and the
   trajectory's speed and rate there (`lineLead` ahead). On a line: the line's direction at that speed,
   its bend and the speed's rate fed forward, a spring back onto it, the correction capped
   (`trajectoryCorrectionMax`). On a route with corners: an aim point ahead along it, pulled back
   where the straight line to it is blocked. Blocked space the map lacks (`PredictBlocked`) lowers the
   speed to stop short of it, braking hard (`emergencyBrake`) when there is no room.
5. Attitude (`FlyAerobatic`, drone style): the nose along the line a little ahead, the body tilted as
   the trajectory plans (part of the way to the thrust the acceleration needs, `droneBodyShare`) plus
   what the follower's correction adds, every angle eased (`EaseAngle`). In a bend the nose is turned
   into it by the trajectory's drift, so the surge pushes round it. Thrust is limited to what the
   player's inputs give (surge, sway, lift). The drone attitude is `Navigation/DroneAttitude`, the
   torque and thrust `Navigation/Aerobatics` (`AttitudeControl`).
6. Surge (`Navigation/Surge`, tunables `Autopilot::surgeHold`), flown as a player holds it: the thrust
   holds off the air drag along the velocity (`dragFeedForward`). Where the trajectory's speed is the
   car's own (limit `Top` or `Accel`), the surge is pushed along the path toward full (`fullSurge`) by
   the hold's weight: full at or under the plan speed, falling to none at `slack` (4 m/s) over it, so
   the car settles inside the slack with a steady surge. The weight moves at most `rate` (4 a second)
   either way, so a join, a replan or a bend fades the push in or out. A change of the hold lasts
   `dwell` (0.3 s) before it is taken; a safety release (braking for blocked space, after a hit, off
   the line) lets go at once. Slowing, the body takes less of the thrust's tilt than speeding up
   (`droneBrakeShare` 0.25 against `droneBodyShare` 0.5) and reverse surge does the rest. Taking off,
   the nose leans at least `droneTakeoffTilt` (10 deg) forward, whole from 5 to 25 m/s and gone by 50,
   over the route's first `droneTakeoffLength` while speeding up. The surge and sway the engine sound
   hears are the thrust's share of full, not eased.
7. Arrive and land (Landing, Descent): the spot is rechecked on the approach, the nose turns into the
   landing heading, and the car sinks once over the clear shaft.

## Route

`AutopilotRoute.cpp`. Each frame `UpdateRoute` follows the destination: tracked for the first time, or
moved more than 20 m, it picks the goal again (`ChooseRouteGoal`) and asks for a route. A summon's own
goal (`externalDestination`) wins over the tracked one. Untracked on the way, the Autopilot hands back
as vanilla autodrive does; untracked once it is descending, landed or arrived (the game untracks a map
pin when the player reaches it), it finishes the landing.

- **Goal.** With `landingMode` not Hover, the destination's pad (`FindLandingPad`) or a ground spot near it
  (`FindLandingSpot`); the route ends `approachHeight` over the spot, under the room above it. Without a
  spot the route ends at the destination itself and the car arrives in the air.
- **Requests** (`RequestRoute`). With no route to keep, the trip is planned ahead: a first leg of
  `ahead.horizon` metres flown at once, its rest planned while the car flies it and spliced on at the
  join (`Navigation/PlanAhead`, `docs/design/trajectory.md`, Plan ahead); a trip shorter than
  `ahead.minTrip` comes back whole. With a route to keep, and in free roam, it is one Race plan. A
  request still pending is replaced, not dropped. With nothing to follow
  meanwhile the status is `planning` and the car holds where it is.
- **Taking a plan.** A plan of more than two points about 5 m apart is a line (`routeLine`): followed
  with the trajectory, replanned when left by `replanOffLine`. Anything else gets its corners rounded
  (`RoundCorners`: full cut, then smaller, sharp below `raceRoundMin`) and is replanned when left by
  `replanDistance`.
- **Splices** (`SpliceRoute`): the route kept from just behind the car up to the join, then the new
  leg, the bend where they meet smoothed over `freeRoamSmoothSpan` where the map allows
  (`Route.hpp`, `SmoothJoin`). Used by free roam and by a planned-ahead trip's rest.
- **Hits** (`WatchHits`): a drop in the component's `collisionTimer` while following is a hit. The place
  is kept for `hitAvoidTime` as a spot the next plans route round (`hitAvoidRadius`), and once the
  controls' damping is over (`collisionRecoveryDelay`) the car replans from its own state. While
  recovering the follower holds the car's velocity.
- **Replans** (`Replan`): back onto the line ahead of the car (`PlanQueue::Rejoin`, at least
  `rejoinMin` or `rejoinAhead` seconds of its speed ahead), never to a point behind it. With too little
  line left, on a first leg, or on a route with corners, a full plan from where the car is.
- **Wedged**: barely moving for 4 s while the trajectory asks for speed, the car goes out to the
  nearest free point (`NearestFree`: up, sideways or down, 1 to 8 m) and a route is asked for from there.
- **Jumps** (`CheckedPosition`): a position further than the car could have flown is held off for up to
  `positionJumpHold` before it counts as a real move.
- **Progress**: with the player in the car and not free roaming, the remaining length and time go to
  vanilla's autodrive panel each frame.

## Free roam

`AutopilotFreeRoam.cpp`. Engaged with nothing tracked (or switched to with a tap of the autodrive key,
`ToggleFreeRoam`), the car flies legs to points or gates ahead, chained into one route without
stopping, around where it began (`freeRoamCentre`, `freeRoamRadius`).

- A leg goes to a gate ahead if one fits (`PickFreeRoamGate`: within reach and `freeRoamSpread` of the
  heading, not one of the last `gateMemory` flown, with room along its straight run), else to a point
  ahead (`PickFreeRoamGoal`: `freeRoamLegMin` to `freeRoamLegMax` out, open sky, `freeRoamRouteHeight`
  over the ground). The spread widens only when nothing fits. At the edge of the area the centre moves
  to the car. No goal is picked across a world boundary in force (`World::CrossesBoundary`).
- A gate leg is planned to just before the gate (`gateLead`), then flown straight through it.
- With `freeRoamChainAt` metres of route left, the next leg is planned from a point ahead on it
  (`freeRoamJoinAhead` seconds of speed, at least `freeRoamJoinMin`) and spliced on there. A leg steeper
  than `freeRoamMaxPitch` anywhere is thrown away for another. A leg that comes too late, or none, is
  tried again; reaching the route's end first, the car plans on from there.
- Free roam legs fly lower and keep round buildings (`freeRoamRouteHeight`, `freeRoamHeightFree`,
  `freeRoamOverCost`; optionally near roads, `freeRoamGuideCost`). Gates are `src/navigation/gates.tsv`
  (`tools/navmap_bake/gates.py`).
- The speed is capped at `freeRoamTopSpeed` (60 m/s): faster, the world does not stream in time and the
  game stops on its loading overlay.
- The picks draw from the Autopilot's `Rng` (splitmix64). The game seeds it per mode from the clock, so
  legs differ between runs; a test seeds it and gets the same legs.

## Descent

`AutopilotDescent.cpp`: the velocity wanted when not following, and the nose's yaw. The Mod Setting Land
at Destination (`landingMode`, `Navigation::LandingMode`) picks how a flight to a destination on the
ground ends: Wheels (the default) sets the car down just over its wheels and switches flight off, as
vanilla autodrive stops at the curb; Touch Down sinks until the floor stops the car; Hover holds over
the destination and hands back.

- **Hold** (`HoldVelocity`): toward the hold point at 0.8 of the distance a second, at most 10 m/s.
- **Descent**: first over the spot at the height the descent began (a car sliding across while sinking
  can hit what is over the next columns), then, within `landingOverSpot`, the descent's profile
  (`DescentProfile`): a reference height that sinks speeding up at `descentAccel` and braking at it,
  capped at `descentRate`, so a descent of up to 32 m is half speeding up and half braking (the height
  two parabola halves). It ends still on the target (Wheels) or at `touchdownRate` on the spot (Touch
  Down, then sinking on at it until the floor stops the car). The car follows the reference's height,
  speed and acceleration with a critically damped spring of `descentFrequency`. A target that moves (the
  rays finding the ground) reshapes what is left; braking past 2 x `descentAccel` is not asked for.
  Landed, it keeps settling at `touchdownRate` while flight is switched off.
- **Set down** (Wheels, `DescentTarget`): the target is `wheelClearance` over the height the car's
  centre rests at on its wheels (`AutopilotInput::restHeight`), over the ground straight down from the
  four wheel emitters (`wheelGround*`, read while descending), or over the nav map's floor until those
  rays find it. Held within `wheelTolerance` of it, slower than `wheelStill` and within 5 degrees of
  level for `wheelSettle`, it is `landed` and asks for flight off: the car drops the clearance onto its
  wheels. The rays' ground more than 1 m off the map's floor (a canopy, a hole) is not used; ground
  uneven by more than `wheelGroundSpread` (a kerb under one side), no rest height, or not settled
  within 4 s of reaching the target, and it touches down instead.
- **Rest height** (`Flight/WheelRest`): when flight comes on (`OnActivationCore`, before the thruster
  colliders), a car standing level and still on its wheels has its origin's height over the ground
  under them measured and kept per vehicle record (a summoned car of the same model has it too); never
  measured, it is 2.2 times the depth of the chassis collision shape's lowest point under the origin
  (four cars within 1.5 cm of what was measured), or 0.4 m. The shell adds the centre's height over the
  origin.
- **Heading** (`Heading`): along the wanted velocity; descending, from the arrival heading to the
  landing heading in step with the height lost, so the turn and the descent end together.
- **Landing turn** (`ChooseYaw`, `LandSlewYaw`, `TimeToDescent`): with `droneBrakeTurn`, the nose keeps
  to the line until `droneLandSlewBefore` before the descent by the trajectory's clock, then makes one
  turn into the landing heading, finished `droneLandSlewDuring` into the descent. The turn runs over
  time, not distance, so a long brake does not do most of it early; a moved route end stretches what is
  left of it.

The arrival (`Autopilot::Arrive`): following, with less than `landingEntry` of route left below
`landingEntrySpeed` (landing) or `arriveDistance` below 3 m/s (not landing), the status becomes
`descending` or `arrived`. A first leg's end is not an arrival. Descending, the car is set down as above (Wheels), or has touched down
once it has made no new low point for 0.7 s within 3 m of the floor (`landed`); held up for 1.5 s higher
than that (something the map lacks), it hovers there (`arrived`). Arrived with the player in the car, it
settles until still (or 5 s) and hands back with a toast.

## Landing

`Landing.hpp/.cpp`, over a `World`.

- **A ground spot** (`FindLandingSpotWithRoom`): the nearest column within `landingSearch` rings of 2 m
  that is at the destination's level, open above, flat (`IsFlat`), with a clear shaft (`ShaftClear`), no
  vehicle in it (`SpotTaken`), the game's collision under the whole car (`SpotSupported`) and away from
  the keep-clear point (a summon keeps clear of V). Each ring is checked on the nav map first, and the
  rays are cast only for that ring's candidates, nearest first, until one passes. `FindLandingSpot`
  asks first for room to come straight down from the approach height, then for 6 m. A destination
  more than 4 m over its floor has no ground spot.
- **A pad** (`FindLandingPad`): the one the destination is on, else with `padRange` set the nearest
  within it. A pad summon's `padLandingOffset` puts the spot across the deck from V.
- **The heading** (`LandingHeading`, `PadHeading`): along the most open of eight lines through the spot
  (a street, a plaza's long side), or the pad's yaw, whichever of its two ways is nearer the arrival.
- **Rechecked on the approach** (`RecheckLandingSpot`): every 0.5 s within `landingRecheck` of the end,
  once the cars round the spot have streamed in. Taken or not under the whole car, the route's last
  `landingShiftSpan` metres move to the nearest free spot (`ShiftRouteEnd`) and the trajectory is
  rebuilt. On arrival the spot is checked once more; with no free spot near, the car hovers.

## Tuning

`Navigation::AutopilotParams`: every field with its name and default and what it does. `params` is
held by reference, so a change reaches the next frame.

- `AutopilotParamTable` names each field; `SetAutopilotParam` sets one by name.
- Mod Settings: `enabled`, `landAtDestination` (the field `landingMode`: a list of Hover, Wheels and
  Touch Down, the old Bool's saved false and true its first two; on a Mod Settings without lists, before its
  `ApiVersion` 1, the old switch with on as Wheels), `padRange` and `style` (the field `attitudeStyle`: Drone or
  Spinner; Spinner sets `VehicleSpec::levelPitch`, so the body banks but its nose is held level, and no takeoff
  lean; the planner's speeds along the line are the surge thrusters' in both; without lists a switch with on as
  Spinner), registered from C++ under the class
  `FlightModeAutopilot` (`docs/design/flight-modes.md`, Settings). They are written into the params
  when the mode is set up and engaged and when the player changes one, so a value set on such a field
  before engaging is lost.
- Any field can be overridden with the developer setting `autopilot.<name>` (`developer.ini` or
  `FlightDeveloperSettings.Set`), which wins over the Mod Setting.
- Every field is also a property of the mode under its name (Script shell).
- The surge hold's and plan-ahead's tunables are `Autopilot::surgeHold` and `Autopilot::ahead`, exposed
  as `surge*` and `ahead*`.
- New behaviour gets a field, switched for tests, not a Mod Setting.
- The Race's route character (height, climb cost, passage, search weight, turn cost) is
  `Navigation::RacePreset`; the params' `race*` route fields pass the same values and must match it.

The manoeuvres' pure geometry is still in `Navigation/Aerobatics` with its tests; the Autopilot flies
drone style only and does not use it.

## Plans

`Plans.hpp/.cpp`. `PlanQueue` holds plan requests by id: `Race` (a route to a goal, or a free roam
leg), `Trip` (a first leg and its rest) and `Rejoin` (back onto a line). Each is taken once; a first
leg's request stays for its rest. `Drop` frees a request no longer wanted. The last plan's info,
partial flag and join are the Autopilot's own (`lastPlanInfo`), so two cars do not read each other's.

- In game the queue is `FlightNavMap::Queue()`, threaded: each request plans on a thread of its own.
- Offline the queue is not threaded: it plans when asked and hands the plan over a fixed delay later
  (the rest of a trip that delay after that), so a simulated flight waits as the game's does and flies
  the same on any machine.
- `CarPlanParams`, `RaceParams` and `FreeRoamParams` build the planner's params from `FlightNavMap`'s
  base params, the car's limits, the spots to avoid and the car's velocity.

## World

`World.hpp` is what the Autopilot asks of the world: free space (`Space`: `IsFree`, `SegmentClear`),
a column's gaps, the car's half height the solids were grown by, the pads and gates, whether the map
is loaded, the map itself (for the trajectory's room caps), world boundaries, and two rays for the
landing checks: `VehicleRay` (cars, `SpotTaken`) and `GroundRay` (static world, terrain and vehicle
blockers, `SpotSupported`). Every other check asks the nav map.

- `MapWorld` is the nav map, pads and gates, with rays that hit lists of boxes (offline: parked cars
  and the statics the rays should see).
- `FlightWorld` (in game) is `MapWorld` over `NavMap::Get()`, `Helipads::Get()` and `Gates::Get()`, with
  the rays cast through the engine from the component's spatial queries system, as `FlightGround`
  does: `SyncRaycastByCollisionGroup` with `Vehicle`, and `SyncRaycastByQueryFilter` with Static,
  Terrain and VehicleBlocker. A missing function or system means a ray hits nothing (warned once), as
  an unstreamed world does.

## Across the fleet

The Autopilot reads three numbers of the car (`RefreshVehicle`): mass, airResistance and
brakingFriction. brakingFriction is the same on every vanilla car, so the cars differ in drag
(airResistance / mass) and so in top speed, up to `raceSpeedCap`. Turning, climbing, pushing over and
the drone body are the same on every car.

- With PhysX-owned flight the torque goes through `FlightComponent.AngularImpulseFor` (the body's own
  world inertia, which PushToPhysX applies the impulse with) and the force tail's mass x tensor, so the
  body turns at the angular acceleration asked on every car.
- What differs per car is ApplyPhysics' angular damping. The attitude control gives it back at the rate
  wanted with `Flight::AngularDamping`, the very damping ApplyPhysics applies (CapDamping and its cap
  included), so every car tracks its attitude target alike. A hit is left alone: the contact's spin is
  not damped and the recovery fades every torque the mode gives, the give-back too.
- Without PhysX-owned flight (the PhysX Flight Physics Mod Setting off) the torque uses `torqueGain*`.

Tests: `ltbf_tests autopilot_fleet` (`tests/autopilot_fleet_tests.cpp`, no game and no nav map): the
limits for every car (`tools/flightsim/vehicles.tsv`), the follower on a synthetic route with every
distinct car, the attitude impulse's exactness, the attitude loop's spread, a knock
(`autopilot_fleet_knock`) and the character (`autopilot_fleet_character`). `tools/navplan/fleet.py`
flies the benchmark routes with every distinct car.

## Character

A car's shape drag and thrust, behind developer settings and off by default (`shapeDrag`,
`thrustStrength`, `Flight/DeveloperSettings.hpp`): every car flies as above unless they are on.

- When flight starts the configuration measures the car's drag area from its chassis collision
  resource (`Flight/ChassisShape`, `Flight/Aero`) into `FlightStats.s_dragArea` and `s_thrusters`.
- With `shapeDrag`, `ApplyPhysics` drags on each body axis (`Flight::ShapeDragAccel`, the fleet factor
  `Flight::kShapeDragK`). With `thrustStrength`, the manual modes' surge and sway scale by
  `Flight::ThrustScale`.
- `VehicleSpec` takes the shape (`cdaFront`, `cdaTop`) and thrust (`thrusters`, `thrustStrength`), and
  `Effective` scales surge and sway. The limits take the path's slope: a near-level drone body meets a
  climb or a dive with its roof, so `DragAt`, `AccelAt` and `TopAt` mix the front and roof areas in the
  path's proportions, and the trajectory caps each sample at `TopAt` and speeds up with `AccelAt`.
- In a bend the drift's share is the surge the drag leaves at that speed (`Lateral(spec, speed)`,
  `LateralAt`), and a bend at the plan's top is checked at `FlownTop`, the speed full surge holds.

## Script shell

`FlightModeAutopilot` is a native `FlightMode` under the class name script uses, so every reader keeps
working (`FlightController`, `FlightSummonSystem`, `AutoDriveController`, `VehicleFlight`, the HUD,
`Audio/Inputs/Gather.cpp`). It owns the params, the `Rng`, a `FlightWorld` and the
`Navigation::Autopilot`. Each frame (`OnUpdateNative`) it:

1. gathers an `AutopilotInput` as `tools/flightsim/Trip.cpp` does offline: the stats (`FlightStats::Of`),
   the component's input fields as `FlightComponent` wrote them this frame, `isPlayerMounted`,
   `collisionTimer`, `thrusterTensor`, the world inertia (`GetWorldInertiaRow`), FlightSettings' fields
   and named floats, the Standard factors (the code's own `Flight::StandardParams`, so another mode's
   sliders do not change the Autopilot's car), and the tracked destination from the script's
   `TrackedDestination` (unless a summon set its own goal);
2. applies changed Mod Settings and developer overrides, and takes the summon's fields script wrote;
3. writes the telemetry row (`ltbf_autopilot_<time>.csv`, `Navigation/AutopilotTelemetry`) with
   `telemetry` on, from the position the frame flies from;
4. calls `Autopilot::Step`;
5. writes `force` and `torque`, and the component's surge, sway, lift, yaw and brakes as the output asks
   (for the thrusters' look, the engine sound and the force tail's damping; no yaw input, because the
   force tail damps less on an axis with input). A frame the player has taken over is flown with
   `Flight::AttitudeLift`, the Standard factors scaled by the car's thrust and the component's pitch
   and roll PIDs;
6. hands the output's events to the script by name: `PublishProgress`, `PublishFreeRoamUI`,
   `ShowNotification`, `RequestFlightOff`, `ExitAutopilot`. Engaging and disengaging with the player in
   the car call `AutodriveEngaged`.

`FlightMode`'s `Initialize`, `IsEnabled` and `Teleported` hooks set it up, report the `enabled` Mod
Setting and replan from a teleport.

Fields script reads keep their names and types. The tuning and the state's numbers and flags are RTTI
properties straight over the C++ members (`DescribeFields`, at their offsets), so a write from script
reaches the next frame. The Strings and Vector4s (`status`, `destination`, ...) are copies taken after
each call; the summon's three (`externalDestination`, `padLandingOffset`, `landingKeepClear`) are read
back before the next one when script changed them. The route and trajectory are read through
`RouteSize`, `RoutePoint`, `RouteDistance`, `RouteLength`, `HasTrajectory` and `PlannedSpeed`;
`GroundSpot` (and the script's `FindGroundSpot` over it), `ToggleFreeRoam`, `BuildTrajectory`,
`RefreshVehicle`, `Vehicle` and the static `Flat` are the other natives. `Audio/Inputs/Gather.cpp`
reads `commandSurge`, `commandSway` and `standardModeLiftFactor` from the C++ fields.

`FlightModeAutopilot.reds` declares those fields and holds the glue that is the game's script world:
`TrackedDestination` (`GetDestination`: the target override, `FlightHelipadSystem`'s tracked pad,
`AutoDriveSystem`), `AutodriveEngaged` (the `disable_combat_events` and `player_in_autodrive` facts,
vanilla's autodrive panel and the cinematic camera listener), `PublishProgress`, `PublishFreeRoamUI`,
`ShowNotification`, `ExitAutopilot`, `RequestFlightOff`, and the statics `IsEngaged`, `IsFreeRoaming`,
`Notify` and `GetDestination`. The C++ finds each glue function once per class and warns once if one is
missing. A mod extends the Autopilot by subclassing it and overriding `FlightMode`'s hooks.

## Trip

`tools/flightsim/Trip.hpp/.cpp` flies a whole trip offline: `Navigation::Autopilot` from `Activate` to
`landed` (or another end) on flightsim's rigid body through the real force tail, over a `MapWorld` on a
nav map (`navmap.bin`, or one made in memory). `ltbf_tests autopilot_trip_*` and navplan
(`NAVPLAN_TRIP=1`, run by `bench.py` and `fleet.py`) share it.

- The Autopilot, the force tail and the stats are the plugin's. The body is `Sim`'s, and the Autopilot
  is given the world inertia that body answers with, as the game gives it PhysX's.
- Plans come from the plugin's planner on an unthreaded `PlanQueue` after `planDelay`.
- The floor is the nav map's (a gap's bottom less the car's half height), a stop under the body.
- What the map leaves out is boxes: parked cars (seen by `VehicleRay`) and statics (seen by
  `GroundRay`), solid to the body if asked, there from the start, from a time on, or once the car is
  near (streaming). The ground ray also hits the map's floor. A box run into faster than 2 m/s is a
  hit (the recovery timer the Autopilot reads).
- Knocks (`Sim::Knock`), teleports and an unmounted car's drive inputs can be scripted.
- Not modelled: contact friction, the spin a hit gives, the body's extent against the map's solids
  (the check is the centre's depth), the engine's streaming and rays (boxes stand in), real input
  timing.

Per frame it records position, status, distance off the line, the plan's speed where the follower
commands, attitude, rays cast, Step time, depth into a nav map solid and contact; per trip
(`TripResult`) whether it landed, the status changes, time, off-line percentiles, speed error, landing
distance and heading, touchdown speed, replans, rays and Step time.

## Trip checks

What every trip must keep to (`CheckTrip`, `TripLimits`):

- no NaN;
- never deeper than 0.5 m into a nav map solid in flight (more allowed low over the floor, where the
  map's floor sits a half height up);
- only the status changes the state machine makes (`LegalStatusChange`);
- no plan request left in the queue at the end;
- at most 200 rays in a frame;
- landed within 2 m and 20 deg of the spot, no faster than `touchdownRate` plus 1 m/s.

`SameTrip`: the same setup and seed fly the same trip, bit for bit.

Budgets (`autopilot_trip_budgets`), so a change that makes the Autopilot expensive again fails: rays
per frame, per trip and per landing search round a taken spot, Step time, and the plan's wait.

The trip tests (`tests/autopilot_trip_tests.cpp`) fly a ground spot, a pad and a summon to it, a spot
taken from the start, late and on arrival, a descent held up by something the map lacks, a prop missing
from the map across the route, knocks on the approach over several seeds, the same seed twice, free
roam, no route, a teleport mid-route, an arrival in the air handed back, and the budgets.
`tests/autopilot_tests.cpp` flies the class over a small nav map with a point mass that takes the
attitude asked for at once: engage, land, arrive, a moved or untracked destination, no route, no nav
map, a hit and the rejoin, a taken spot, free roam and gates, the override, a teleport, plan ahead and
the free roam toggle. `tests/autopilot_params_tests.cpp` checks the tuning's defaults and setter.

## Shell tests

`tests/autopilot_shell_tests.cpp` checks what other tools and players' saved files depend on:

- the Mod Settings keys: the class, names, types, categories, localization keys, defaults and ranges
  (Mod Settings keys saved values by class and field name);
- the developer overrides by name (`autopilot.<name>`);
- the telemetry CSV's header and row width (`tools/telemetry`, `tools/navplan/plot.py` and
  `tools/acoustics/flyby.py` read its columns by name);
- every member of `FlightModeAutopilot` that script in this repo uses is declared (and in the folders
  in `LTBF_EXTRA_SCRIPT_DIRS`), and every native field and function declared is registered with its
  type.

## Checking a change

Offline first: `ltbf_tests autopilot`, then `tools/navplan/bench.py` (the benchmark routes, the line's
dips and bends, and the trips) and `tools/navplan/dips.py`. Then in game with a video; the telemetry is
read by `tools/telemetry/autopilot.py`.
