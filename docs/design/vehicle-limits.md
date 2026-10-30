# Vehicle limits

One description of what a car can do in flight, used by everything that plans or measures a racing
line. Code: `src/red4ext/Navigation/Vehicle.hpp/.cpp` (no game types, so navplan builds it too).

## VehicleSpec

`Navigation::VehicleSpec` holds three groups of numbers:

- the car, from its vehicle record (FlightStats): `mass`, `airResistance`, `brakingFriction`;
- the flight tuning (FlightMode_Standard, FlightMode, FlightSettings): `surge`, `sway`, `lift`,
  `gravityFactor`, `brakeLinear`, `drag` (PhysX flight physics applies air drag);
- the Autopilot's race tuning (FlightModeAutopilot): `drone`, `maxTilt`, `driftMax`, `speedCap`,
  `margin`, `planDecel`, `pushOver`.

The limits are functions of it: `Drag`, `DroneAccel`, `TopSpeed`, `Lateral`, `Accel`, `Decel`,
`PlanDecel`, `PullUp`. `Profile` gives the fastest speed along a polyline (turns at
`margin x Lateral`, crests at `pushOver`, pull-ups at `margin x PullUp`, then speeding up from the
start and braking to 0 at the end), with the reason for each point's limit.

- Pull-up is the lift over gravity, planned at `margin` (`margin x lift`). The old `dips.py` used a
  fixed 40 m/s2.
- `pushOver` is already a planned value (the margin is in it): 21 m/s2 drone style, 15 plane style.

## Who reads it

| Reader | How it gets the car |
|---|---|
| Planner, `TurnCost` (`Planner.cpp`) | `PlanParams::vehicle`; uses `Lateral`, `TopSpeed`, `Accel` and `PlanDecel` to weight each bend by the speed it costs |
| Autopilot in game | `FlightVehicleModel` (`src/red4ext/Navigation/FlightVehicleModel.hpp`, script side `src/redscript/Navigation/FlightVehicleModel.reds`): `FlightModeAutopilot.Vehicle()` fills it from the mounted car and the mode's tuning; the Race helpers (`RaceTopSpeed`, `RaceDecel`, `RaceLateral`, `DragAccel`, `DroneAccel`) delegate to it, and `FlightNavMap.PlanRaceAsync` hands it to the planner |
| navplan | `NAVPLAN_VEHICLE=name=value,...` (VehicleSpec field names); writes `<prefix>_profile.csv` (`d,x,y,z,v,limit,why,clearance`) from `Profile` |
| tune.py, bench.py, dips.py | `tools/navplan/vehicles.json` names cars and passes them to navplan; dips.py reads the profile CSV and has no physics of its own |

The Autopilot's own speed profile, `FlightModeAutopilot.BuildSpeedProfile` (redscript), still adds
the landing approach and corner braking on top of these limits. It stays in redscript for now and
moves to C++ in the plan-centric trajectory project.

## Adding a car

1. Mount it in game and call `LtbfBridge.VehicleModel` (through `bridge.py call`): it prints the
   car's line as the Autopilot plans with it, in `name=value` form.
2. Add an entry to `tools/navplan/vehicles.json` with the fields that differ from the defaults.
3. Plan with it: `NAVPLAN_VEHICLE` from that entry (tune.py and bench.py take `--vehicle <name>`).
