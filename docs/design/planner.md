# Planner

Route planning over the nav map, in the plugin and in navplan (the same sources). Code:
`src/red4ext/Navigation/` (`Planner.hpp` for `PlanParams` and the entry points).

| File | What |
|---|---|
| `NavMap.hpp/.cpp` | the baked map: per 2 m column, the vertical gaps a car's centre can occupy (`docs/design/navmap.md`) |
| `Planner.hpp/.cpp` | `PlanParams`, `Plan`, `PlanWithRetries`, `RacePreset`, `TryDirect` |
| `Doorways.hpp/.cpp` | the doorway layer for long routes, `WorkOutDoorways` for the bake |
| `RacingLine.cpp` | the racing line passes |
| `PlannerCommon.hpp/.cpp` | grid keys, gaps, leg checks shared by the above |
| `Vehicle.hpp/.cpp` | the car's limits (`docs/design/vehicle-limits.md`) |
| `NavDraw.hpp/.cpp` | geometry for drawing the map in game |
| `FlightNavMap.*` | script bindings (async plans, queries, pads, gates, drawing) |

## Plan

1. Search: A* over (2 m column, gap) nodes, 8-connected, from every free gap within `startRadius` of
   the car. Step cost: distance, `underCost` under cover, a climb cost growing exponentially with
   the rise in one step (`ClimbCost`, `climbScale`), and `turnCost` per 45 degrees turned (straighter
   corridors). `minPassage` keeps the route out of slots too low to fly fast.
2. Long routes (over `doorwaysFrom`) search the doorways between 32 m blocks first, then plan inside
   the blocks that route passes. Doorways are worked out per block on first use and kept for the
   session; the Race's set is baked into `navmap.bin` (`tools/navmap_bake/doorways.py`) and has to be
   rebaked when the Race's step costs change.
3. The route is pulled straight (only the corners every free leg needs).
4. Racing line (`RacingLine`): the route spread into even points (`lineSpacing`), smoothed and pushed
   away from blocked space (`lineClearance`, spread over `lineSpread`), then:
   - `SmoothLateral`: the path across solved in a corridor of free width either side, smoothest bend
     plus its rate of change, then `lineTurnRounds` rounds weighting each bend by the speed it costs
     the car (`TurnCost`);
   - `SmoothHeights`: the heights solved alone within each point's band (floor clearance below,
     ceiling clearance above), coarse to fine, with a height cost over the terrain
     (`lineHeightFree`, `lineHeightScale`, `lineHeightWavelength`); thin obstacles in the way are
     tried over the top (`lineOver*`). Solved twice with the points respaced evenly (`Respace`).
   - Every leg is checked free with the grazes either side taken as one run (`LegRunFree`); a line
     not free all the way is rejected and the plain route flown.
5. `PlanWithRetries`: a search that runs out of expansions retries with a greedier heuristic (1.5,
   then 2.0); a route with no way through a `minPassage` squeeze retries without it.

## Offline

`build/navplan.exe` (`cmake --build build --target navplan`) plans with the same code:
`NAVPLAN_RACE=1` for the Race preset, `NAVPLAN_PARAMS=name=value,...` for any `PlanParams` field,
`NAVPLAN_VEHICLE` for the car. It writes the route, the line and its speed profile as CSV.
`tools/navplan/bench.py` plans the benchmark routes (`routes.json`) and compares two builds or
parameter sets; `tune.py` searches parameters with Optuna; `plot.py`, `city.py`, `navviz.py` draw
lines over the map or a recorded run.
