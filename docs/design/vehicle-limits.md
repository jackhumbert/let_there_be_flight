# Vehicle limits

One description of what a car can do in flight, used by everything that plans or measures a racing
line. Code: `src/red4ext/Navigation/Vehicle.hpp/.cpp` (no game types, so navplan builds it too).

## VehicleSpec

`Navigation::VehicleSpec` holds three groups of numbers:

- the car, from its vehicle record (FlightStats): `mass`, `airResistance`, `brakingFriction`;
- the flight tuning (FlightMode_Standard, FlightMode, FlightSettings): `surge`, `sway`, `lift`,
  `gravityFactor`, `brakeLinear`, `drag` (PhysX flight physics applies air drag);
- the Autopilot's race tuning (FlightModeAutopilot): `drone`, `maxTilt`, `brakeShare`, `tiltFloor`, `driftMax`, `speedCap`,
  `margin`, `planDecel`, `pushOver`.

The limits are functions of it: `Drag`, `DroneAccel`, `TopSpeed`, `Lateral`, `LateralAt`, `Accel`, `Decel`,
`PlanDecel`, `PullUp`. The speed along a line built from them is the trajectory's
(`Navigation/Trajectory.hpp`, `docs/design/trajectory.md`).

- Pull-up is the lift over gravity, planned at `margin` (`margin x lift`). The old `dips.py` used a
  fixed 40 m/s2.
- `surgeShare` (1.0) is fitted from telemetry (`tools/telemetry/fit_surge.py`): the share of the
  surge the car gets speeding up in level flight near top speed; `TopSpeed` and `Accel` use it. In
  between (30 to 70 m/s) the body leans further into a hard push and the car gets more (about 20
  m/s2), not modelled: the plan errs on the safe side there. Refit after a change to the flight
  physics or the follower.
- `LateralAt(vertical)`: the turn left where the path also needs `vertical` m/s2 upwards (gravity
  plus the bend's own, less over a crest). Drone style the body's tilt turns the car only through the
  lift, so the tilt's part scales with what the lift has left; pushing over a crest only sway and the
  drifted surge turn the car. The trajectory solves each crest bend's speed against it.
- `tiltFloor` (1 g): the vertical the drone body's tilt is worked out against at least. Over a crest
  the lift pushes less or down; against the small vertical left, a modest turn read as a thrust near
  horizontal and rolled the body to its 55 degree cap with the lift sideways.
- Drone `Lateral` is the sway envelope plus `surge x surgeShare x sin(driftMax)`: the surge held through
  a bend with the nose drifted into it (the trajectory plans the drift, `docs/design/trajectory.md`).
- `pushOver` is already a planned value (the margin is in it): 21 m/s2 drone style, 15 plane style.

## Who reads it

| Reader | How it gets the car |
|---|---|
| Planner, `TurnCost` (`Planner.cpp`) | `PlanParams::vehicle`; uses `Lateral`, `TopSpeed`, `Accel` and `PlanDecel` to weight each bend by the speed it costs |
| Autopilot in game | `FlightVehicleModel` (`src/red4ext/Navigation/FlightVehicleModel.hpp`, script side `src/redscript/Navigation/FlightVehicleModel.reds`): `FlightModeAutopilot.Vehicle()` fills it from the mounted car and the mode's tuning; the Race helpers (`RaceTopSpeed`, `RaceDecel`, `RaceLateral`, `DragAccel`, `DroneAccel`) delegate to it, and `FlightNavMap.PlanRaceAsync` hands it to the planner |
| Trajectory (`Trajectory.cpp`) | the spec it is built for: speeds, braking and acceleration from these limits |
| navplan | `NAVPLAN_VEHICLE=name=value,...` (VehicleSpec field names); writes `<prefix>_profile.csv` (`d,t,x,y,z,v,why,clearance`) and `<prefix>_trajectory.csv` from the trajectory |
| tune.py, bench.py, dips.py | `tools/navplan/vehicles.json` names cars and passes them to navplan; dips.py reads the profile CSV and has no physics of its own |

The Autopilot flies the trajectory's speeds; it has no speed profile of its own (the redscript one
was removed).

## Adding a car

1. Mount it in game and read its vehicle model (`FlightModeAutopilot.Vehicle()`, a test read-out): the
   car's line as the Autopilot plans with it, in `name=value` form.
2. Add an entry to `tools/navplan/vehicles.json` with the fields that differ from the defaults.
3. Plan with it: `NAVPLAN_VEHICLE` from that entry (tune.py and bench.py take `--vehicle <name>`).
