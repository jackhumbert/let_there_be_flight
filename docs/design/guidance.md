# Flight guidance

The flight route shown to the player, published for other mods to draw: a navigation mod's path on
the minimap or as markers in the world, which would otherwise follow the road route while the car
flies over it. LTBF draws nothing itself.

Code: `src/red4ext/Navigation/Guidance.hpp/.cpp` (the logic, game-free), `FlightGuidance.hpp/.cpp`
(the game link and the script API), `src/redscript/Navigation/FlightGuidance.reds` (its
declaration). Tests: `tests/guidance_tests.cpp`.

Vanilla analogue: the GPS system's path to the tracked mappin, which the minimap's
`MinimapContainerController` reads and draws (`UpdateGPSPath`). The route here is the same kind of
data (world points from the player to the destination), made for flight.

## What the route is

While the player's vehicle is in flight and something is tracked (`FlightModeAutopilot.GetDestination`:
the target override, a tracked landing pad's deck, or vanilla autodrive's destination):

- **The Autopilot engaged:** its own route, as it flies it. Copied again whenever it changes (its size,
  length or first point), so a replan shows at once.
- **Flown by hand:** a Race to the destination (`PlanQueue::Race`, the Autopilot's race shape from its
  tuning: `raceRouteHeight`, `raceClimbCost`, `raceMinPassage`, `raceSearchWeight`, `raceTurnCost`),
  without the car's limits and without slot detours, since nothing flies it. It runs on the Autopilot's
  plan queue, on a worker thread.
  - Planned again when the destination moves more than `destinationMoved` (25 m); the old line is
    dropped at once, since it leads to the wrong place.
  - A rejoin (`PlanQueue::Rejoin`) when the car is more than `offLine` (40 m) from the line, joining it
    `rejoinAhead` (150 m) on. Over `lost` (300 m), a teleport or a respawn, planned again from the car.
  - After a failed plan, nothing for `retry` (3 s).
  - When the Autopilot disengages, its route stays and is kept as the hand-flown line.

No flight, nothing tracked, or the nav map not loaded: no route, and any plan still running is dropped.
The world detaching clears everything.

Each frame the car's place on the line is found (`Route::Nearest` from its last segment on), and the
route given out starts there: the point on the line nearest the car, then the line's points after it.
The planner's line has a point about every 5 m. Its end is the planner's goal: over the destination
within its free gap, so not always on the ground.

## The API

`FlightGuidance`, static, safe from any thread (a snapshot made once a frame by `FlightSystem` after
the components update, behind a mutex):

| Function | Returns |
|---|---|
| `IsFlying()` | the player's vehicle is in flight (not only mounted) |
| `HasRoute()` | flying, something tracked, and a route to it |
| `IsAutopilotRoute()` | the route is the engaged Autopilot's |
| `GetRoute()` | `array<Vector4>`, world metres, W 1, car end first; empty without a route |
| `GetRouteRevision()` | changes when the route is replaced, not as the car moves along it |
| `GetDistanceToRoute()` | metres from the car to the route |

Another mod tests for LTBF in redscript with `@if(ModuleExists("LetThereBeFlight"))` on the code that
calls `FlightGuidance`, so it still compiles without LTBF. No import is needed: `FlightGuidance` is in
`packed.reds`, outside any module; `module.reds` declares the `LetThereBeFlight` module the test looks for. A RED4ext plugin looks for the class with `CRTTISystem::GetClass("FlightGuidance")` and calls
its static functions through RTTI.

The tracked destination is read four times a second (a script call into vanilla's autodrive system);
the route is copied for script every frame.
