# Trajectory

The planner hands the Autopilot a geometric line (points, no time). The trajectory puts time on it:
at every sample the speed, acceleration and attitude the car will have, all inside what
`Navigation/Vehicle.hpp` says the car can do. The follower then only tracks it: the planned
acceleration fed forward plus a small correction for position and velocity error. Anything the
follower would have had to anticipate (braking points, turn-ins, the roll into a bend) is already in
the plan, so it is the planner's call, not a rule decided while flying.

Code: `Navigation/Trajectory.hpp/.cpp` (the build), `Navigation/Tracker.hpp/.cpp` (the follower),
`Navigation/PlanAhead.hpp/.cpp` (plan ahead). The Autopilot around them: `docs/design/autopilot.md`.

## The trajectory

`BuildTrajectory` runs on the line after the planner (the line's shape does not change; this adds
time to it), sampled every 2 m. Each sample holds `s`, `t`, position, velocity, acceleration, speed,
the acceleration along and across the line, what limits the speed there (`Limit`: Top, Turn, Crest,
PullUp, Sink, Arrive, Brake, Accel, Start, End, Room), the room across the line and what it costs, and
the drone's attitude (nose yaw, body pitch and roll, the drift). `SampleAt` reads it at any distance,
interpolated.

1. **Speed.** Forward and backward passes. In a bend, the fastest speed whose bend fits, by bisection:
   the drifted nose turns the car only with the surge the drag leaves at that speed, over a crest the
   lift goes into pushing over and turns the car less (`LateralAt`), and a bend at the car's top is
   checked at `FlownTop`. A climb or a dive meets the air with the roof (`DragAt`), so it has its own top
   speed. Braking and the bend share one envelope (a friction circle, down to `brakeGripMin`); so does
   speeding up from the start. Push-over and pull-up, the sink limit on the approach (`maxSink` over
   `sinkApproach`), the gentle arrival (`arriveDecel` over `arriveLength`), and a stop at the end. Two
   slowings close together are merged into one (`humpSpan`, `humpMerge`).
2. **Room.** The car strays off its line more the faster it flies, so where the free room across the
   line (the nearest blocked space left, right, up and down) is short, the speed is capped to what
   leaves `roomMin + roomGrow x (v / roomSpeed)^2` of it, never below `roomSpeedMin`. A slot whose cap
   costs too much speed is routed round instead by the planner (`slotDetourLoss`, `slotDetourShare`;
   `docs/design/planner.md`).
3. **Attitude.** Drone style is differentially flat: the acceleration and gravity fix the thrust
   direction, `bodyShare` of it is the body tilt, and the nose follows the line's direction turned into
   bends by the drift (`driftMax` at `driftFull` sideways, none below `driftStart`, taken up at most
   `driftRateMax`). Slowing, the body takes the car's brake share of the tilt instead: reverse surge does
   more of it. The tilt and roll rates are limited (`rollRateMax`, `pitchRateMax`), so each tilt starts
   early enough to reach its peak, and a turn's roll is held from a gentler bend before it.
4. **Check.** Every sample is checked against the car's full limits (the plan uses a margin of them).
   A violation is a planner bug and is logged with the sample (`violations`).

A trajectory is built after every plan, and again when the route changes under the car (a splice, a
moved landing spot). From the car's speed when the route starts at the car, free to start at any
speed when rebuilt round a car already on it.

## The follower

`Navigation/Tracker` (`Track`, then `Command`), run by `Navigation::Autopilot` in game and offline.
Each frame:

1. The reference by progress, not by clock: the car's projection on the route near the segment it was
   on, so a car that lags is not dragged toward a reference that ran ahead. The trajectory is read
   `lineLead` seconds of speed ahead of it.
2. On a line, the command is the line's direction at the trajectory's speed plus a spring back onto
   the line (`lineSpring`), turned into an acceleration by `lineVelocityGain`; the correction is capped
   (`trajectoryCorrectionMax`) and the bend's acceleration and the trajectory's rate along the line go
   in whole. The command's change is rate limited (`raceJerk`).
3. A route with corners (no line) is flown toward an aim point ahead, pulled back along the route where
   the straight line to it is blocked, with the bend's acceleration fed forward.

The attitude is the trajectory's plus what the correction needs (`docs/design/autopilot.md`). Kept as a
safety layer only: `PredictBlocked`, for things the map does not know about (cars, streamed-in props).
While it finds blocked space ahead the speed flown is the lower of the trajectory's and the speed that
stops short of it, braked for at twice `speedGain` of the gap, at most the car's decel; with no room to
stop, the emergency brake.

## Leaving the plan

- **Small error:** the correction handles it.
- **Off the line by `replanOffLine`:** a new route from the car's position and velocity, joining the
  old line ahead of the car (`PlanRejoin`, at least `rejoinMin` or `rejoinAhead` seconds of speed
  along it), never behind. No reversing to a point it has passed.
- **After a hit:** the place goes into the next plans as a spot to keep away from (`hitAvoidRadius`,
  for `hitAvoidTime`), the follower holds the car's velocity while the controls are damped
  (`collisionRecoveryDelay`), and then it replans from the car's state.
- **Wedged** (not moving while the trajectory asks for speed): out to the nearest free point first,
  then a new route.

## Plan ahead

A trip with no route to keep is planned as a short first leg the car takes off on at once, and the
rest while it flies it, so the car never sits planning and long trips stay cheap.

- The first leg (`PlanFirstLeg`) follows the coarse route over the doorways (`PlanParams::horizon`
  metres along it, 500 m), so it rarely points the wrong way.
- The rest (`PlanRest`) is planned from where the car will be on the first leg once the rest's
  expected plan time plus `margin` (2 s) have passed, at its velocity there, and spliced on at that
  join (`SpliceRest`, the Autopilot's `SpliceRoute`). A trip shorter than `minTrip` comes back whole.
- Both run on the planning thread (`FlightNavMap.PlanTripAsync`, `PlanQueue::Trip`). The Autopilot only
  hands them over: it takes the first leg, splices the rest at its join, or plans the trip again from
  where it is when the rest comes too late.
- The rest is planned to the goal in one go: there is no rolling horizon.

## One line

The car flies one line, never the plain route. Where the line is blocked over a few short stretches,
only those stretches follow the route, rounded where the room lets (`RepairLine`,
`docs/design/planner.md`).

## Where things go

- `Navigation/Trajectory.hpp/.cpp`: the build and the check. navplan writes `<prefix>_trajectory.csv`
  beside the profile, and `tools/navplan/measure.py` scores it.
- `Navigation/Tracker.hpp/.cpp`: the follower, pure code over the Autopilot's fields.
- `FlightNavMap`: the plans, threaded (`PlanRaceAsync`, `PlanTripAsync`).
- The offline check is the whole trip (`tools/flightsim/Trip.hpp`, `docs/design/autopilot.md`, Trip):
  the Autopilot's own C++ from the engage to landed on flightsim's rigid body with the force tail,
  flown by navplan with `NAVPLAN_TRIP=1` (`<prefix>_trip.csv` and a `trip` line; `bench.py` and
  `fleet.py` report it) and over nav maps made in memory by `ltbf_tests autopilot_trip`. The follower
  alone on a point mass (`tools/navplan/follow.cpp`, the body taking the attitude target exactly) is
  for `ltbf_tests follow_` and `autopilot_fleet_follow`.
- Plan ahead offline: navplan `NAVPLAN_AHEAD` and `tools/navplan/ahead_sweep.py`.
