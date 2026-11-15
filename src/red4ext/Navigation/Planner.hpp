#pragma once

// Route planning over the NavMap (docs/design/planner.md).
//
// 1. Direct (optional): climb to a cruise altitude above everything along the straight line,
//    fly straight, descend. Used when every leg is free.
// 2. Otherwise A* over (2 m column, gap) nodes, 8-connected; neighbouring gaps connect when they
//    overlap vertically. Edge cost = horizontal distance, plus a climb penalty on the preferred
//    altitude (gap floor + preferredHeight, capped under a ceiling), plus a penalty per metre
//    flown under something.
// 3. Height profile: per node a target height inside the gaps it joins, raised ahead of climbs
//    and lowered gently after them (capped gradient).
// 4. String pulling: keep only the corners needed for every straight leg to stay free.
// 5. The line: the pulled route spread into wide bends inside the free space; where that leaves a
//    stretch blocked, that stretch alone follows the route, rounded as far as the room lets.
//
// The result is a polyline of car-centre positions; the flight mode turns it into a trajectory.

#include "NavMap.hpp"
#include "Vehicle.hpp"

#include <functional>
#include <string>
#include <vector>

namespace Navigation {

struct Vec3 {
  float x, y, z;
};

// A place the route search charges extra for: within radius of at (level), per metre flown, over
// the height range at.z +- radius. Slots too tight for the car's speed (PlanRace) go in here.
struct Avoid {
  Vec3 at;
  float radius = 15.0f;
  float cost = 10.0f;
};

struct PlanParams {
  float preferredHeight = 25.0f; // metres above the gap floor (the floor is already a car-centre height)
  float climbCost = 3.0f;        // extra cost per metre of rise in preferred altitude
  float climbScale = 0.0f;       // m of rise in one step per e-fold of its cost (ClimbCost); 0 = in proportion
  float underCost = 0.5f;        // extra cost per metre flown with something overhead
  float heuristicWeight = 1.5f;  // > 1 trades optimality for speed
  float turnCost = 0.0f;         // cost (as metres flown) per 45 degrees the route turns over a few
                                 // cells, at every step: it prefers straighter, more open corridors,
                                 // whose bends the line can take wide
  float bearingCost = 0.0f;      // extra cost per metre flown straight away from the goal (none
                                 // flown towards it): a route that heads off the other way looks
                                 // wrong to someone watching even when it is cheaper
  float minPassage = 0.0f;       // m of free height (car-centre gap, the car's size already taken off)
                                 // the route may squeeze through between columns; 0 = any
  uint32_t maxExpansions = 3000000;
  uint32_t maxDoorwayExpansions = 60000; // doorway search steps at most (long routes use about 11k)
  // The take-off is part of the plan: the search starts from every free gap within startRadius of
  // the start (the car, often on a street under a deck or between buildings), each at the cost of
  // getting there, and the route begins at the start itself. The first point off the ground is at
  // least startLift above its floor.
  float startRadius = 30.0f;     // m
  float startLift = 3.0f;        // m
  // the search in two layers for plans longer than doorwaysFrom: first over the doorways between
  // 32 m blocks (worked out from the 2 m map, kept for the session), then the plan itself in the
  // blocks that route passes and doorwayMargin blocks round them
  bool doorways = true;
  float doorwaysFrom = 300.0f;   // m
  int32_t doorwayMargin = 1;     // blocks
  float maxGradient = 1.0f;      // rise per horizontal metre allowed in the height profile
  float legClearance = 2.0f;     // straight legs must also be free this far to either side (m), on top
                                 // of the bake's car inflation, so a follower can stray a little
  bool tryDirect = false;
  // a first leg only (docs/design/trajectory.md, Plan ahead): when the doorway route is long, the plan
  // ends at its doorway about horizon metres along and the rest is planned while the car flies
  // this; 0: all the way
  float horizon = 0.0f;
  // flying over buildings: overCost per metre of a step whose floor is more than overHeight above
  // the street level of its 32 m block (the lowest floor in it), so routes go round buildings
  // rather than over them (free roam); 0: no cost
  float overCost = 0.0f;
  float overHeight = 10.0f;      // m
  // paths to keep to (the game's roads, say; a point with a NaN x breaks one path from the next, and
  // the route may leave one for another, paying for the stretch between): within guideRadius of one
  // across and between
  // guideLow and guideHigh over its height costs nothing extra; past either the search charges up
  // to guideCost per metre, reached over guideRamp metres, and the line keeps to the band where it
  // is within guideRadius and the band is free. A charge off the path rather than a discount on it,
  // so the heuristic (straight distance) stays a lower bound. Empty guide or guideCost 0: none.
  std::vector<Vec3> guide;
  float guideLow = 15.0f;        // m over the path
  float guideHigh = 40.0f;       // m over the path
  float guideRadius = 20.0f;     // m across
  float guideRamp = 20.0f;       // m
  float guideCost = 0.0f;
  bool guideLine = true;         // the line kept to the band too
  // the line: the route evened out to points lineSpacing apart and bent as little as the free space
  // allows (least summed squared curvature), coarse to fine. Off only for a piece the caller makes
  // its line from together with what follows (PlanRejoin's connector).
  bool line = true;
  float lineSpacing = 5.0f;      // metres between the finished line's points
  float lineSide = 3.0f;         // the line also stays free this far to either side (m)
  // blocked space this close across the line pushes it away, linePush per pass (where the move
  // stays free)
  float lineClearance = 4.685f;    // m
  // m above the floor below (car-centre height, the car's size already taken off) the line keeps,
  // but within lineFloorEnds of its start and end: street furniture (traffic lights, signs, lamps)
  // is streamed in as props and not on the map
  float lineFloorClearance = 12.0f;
  float lineFloorEnds = 60.0f;   // m
  // iterations per level of the coarse-to-fine height solve after the smoothing (SmoothHeights,
  // Planner.cpp); 0 = off
  int lineHeightIterations = 3000;
  // swells in the heights longer than this follow the terrain (the band's floor), shorter ones are
  // smoothed out (SmoothHeights); 0 = no pull down, the smoothest heights alone
  float lineHeightWavelength = 767.9f; // m
  float lineHeightFree = 56.73f;  // m over the band's floor (the floor clearance) that costs nothing
  float lineHeightScale = 30.0f; // m past that per e-fold of the height's cost
  // m from either end within which SmoothHeights keeps a point in its own gap rather than taking it
  // over what is overhead (the take-off and the landing may be under a deck)
  float lineOverEnds = 150.0f;
  float lineOverReach = 10.0f;   // m under an obstacle's top the line solved without it may pass and still go over
  float lineOverThick = 12.0f;   // m thick at most an obstacle the line goes over instead of under (a beam, a skybridge)
  float lineOverRise = 25.0f;    // m higher on average the whole line may come out for going over
  float lineOverGain = 0.4237f;     // going over must leave at most this share of the bend around it
  float lineOverWindow = 150.0f; // m either side of an obstacle its bend is compared over
  // m under anything overhead the line keeps (a skybridge, a deck); the middle of the gap where
  // there is not room for this and the floor clearance
  float lineCeilingClearance = 6.0f;
  float linePush = 0.08609f;         // m
  // m either side a point's clearance push is shared with its neighbours, tapering off (Line); 0 = only its own
  float lineSpread = 27.62f;
  // the path across solved in a corridor after the smoothing (SmoothLateral, Planner.cpp)
  int lineCorridorIterations = 3000; // per level, coarse to fine; 0 = off
  float lineCorridorMax = 30.2f;     // m the free width is looked for either side
  float lineCorridorMargin = 5.138f;   // m kept from blocked space across the line (lineClearance)
  float lineCurvatureRate = 0.5779f;    // weight of how fast the bend changes against the bend itself
  // rounds of weighting up the bends that cost the car speed and solving the path across again
  // (SmoothLateral, TurnCost); 0 = the smoothest shape alone. The car's limits come from vehicle.
  int lineTurnRounds = 1;
  float lineTurnWeightMax = 6.721f;  // the most a bend's weight grows
  // the car flying the line (Vehicle.hpp); the Autopilot passes its own (FlightVehicleModel), the
  // default is the Shion in drone style
  VehicleSpec vehicle;
  float lineStep = 0.08f;        // gradient step per pass (stable below 0.125)
  // weight of the vertical bend against the sideways one: crests and dips are pushed over and
  // pulled through with the lift alone, so they are rounded off more (stable while
  // lineStep x weight stays below 0.125)
  float lineVerticalWeight = 1.381f;
  float directMargin = 20.0f;    // cruise this far above the highest floor along the line
  float maxCruise = 700.0f;
  // the line: a slot whose room costs more than slotDetourLoss m/s, or slotDetourShare of the speed
  // there (and slotDetourLossMin at least), is routed around when that loses less (PlanRace)
  // a plan from a car already flying (|startVelocity| over 5 m/s): the route starts with a straight
  // lead-in of startLead seconds along the velocity (15 to 80 m), so the line leaves the start the way
  // the car is going, and the start is not smoothed as a take-off (lineTakeoff)
  Vec3 startVelocity{0.0f, 0.0f, 0.0f};
  float startLead = 1.0f;
  // and the search charges startHeadingCost per metre, times (1 - cos) / 2 of a step's turn away from
  // the velocity, fading out over startHeadingTime seconds of the car's speed (40 m at least) from
  // the start: the route bends away from where the car is going gradually, not across its path
  float startHeadingCost = 3.0f;
  float startHeadingTime = 1.5f;
  bool lineTakeoff = true;           // the line's first lineFloorEnds kept as planned (a take-off from the street)
  bool lineLanding = true;           // the line's last lineFloorEnds kept as planned (a landing)
  float lineSettle = 0.0005f;   // m per point a smoothing pass moves at most once settled (Line)
  float slotDetourLoss = 20.0f;
  float slotDetourShare = 0.25f;
  float slotDetourLossMin = 8.0f;
  float slotAvoidRadius = 30.0f;   // m round a slot the search charges slotAvoidCost per metre
  float slotAvoidCost = 10.0f;
  std::vector<Avoid> avoid;
};

struct PlanResult {
  bool ok = false;
  bool direct = false;
  std::vector<Vec3> points;
  int32_t repaired = 0;           // stretches of the line its smoothing left blocked, taken along the route
  bool partial = false;           // a first leg (PlanParams::horizon): it ends short of the goal
  uint32_t expansions = 0;
  uint32_t doorwayExpansions = 0; // of the search over doorways, not in expansions
  uint32_t doorwayBlocks = 0;     // blocks the doorway route passed
  uint32_t gridNodes = 0; // nodes on the A* path before simplification
  float length = 0;
  double milliseconds = 0;
  double searchMilliseconds = 0; // of milliseconds, the grid search and the route pulled straight
  std::string info;
};

PlanResult Plan(const NavMap &aMap, Vec3 aStart, Vec3 aGoal, const PlanParams &aParams);

// Plan, and when that fails, again less fussily: a search that ran out of expansions with a greedier
// heuristic (1.5, then 2.0), a route with no way through a minPassage squeeze without it. Each
// attempt, failed ones included, goes to aAttempt (for the log). The Autopilot and navplan both
// plan this way.
PlanResult PlanWithRetries(const NavMap &aMap, Vec3 aStart, Vec3 aGoal, const PlanParams &aParams,
                           const std::function<void(const PlanResult &)> &aAttempt = {});

// PlanWithRetries, then the line's slots whose room would cost the car too much speed
// (Trajectory.hpp's room caps, over slotDetour*): the route searched again round them (Avoid), and
// round any the new line finds, up to three searches; kept when its line has none left, takes at most
// 20% longer and loses less speed in dips (DipLoss); otherwise the first line, flown slower there. A small slowdown is flown, a large one routed round.
PlanResult PlanRace(const NavMap &aMap, Vec3 aStart, Vec3 aGoal, const PlanParams &aParams,
                    const std::function<void(const PlanResult &)> &aAttempt = {});

// Back onto the line after leaving it (a hit, knocked off it): a connector from aCar, flying at
// startVelocity (a lead-in along it), to the point aJoinAhead metres along aLine past the car, ending
// along the line's next lineFloorEnds, then the rest of aLine as it was. aLine: the line still ahead,
// from about the car's place on it.
PlanResult PlanRejoin(const NavMap &aMap, Vec3 aCar, const std::vector<Vec3> &aLine, float aJoinAhead,
                      const PlanParams &aParams);

// The Race's route character: what FlightNavMap::PlanRaceAsync starts from before the Autopilot's
// race route fields (raceRouteHeight, raceClimbCost, raceMinPassage, raceSearchWeight, raceTurnCost,
// whose defaults match these) and what navplan plans with under NAVPLAN_RACE=1. The baked doorways
// (tools/navmap_bake/doorways.py) use minPassage: rebake if it changes.
void RacePreset(PlanParams &aParams);

// Works out the doorways of every block over the area (metres), for a bake; how many there are.
struct DoorwayCount {
  uint32_t blocks = 0, doors = 0, links = 0;
  double milliseconds = 0;
  std::vector<uint8_t> saved; // the set as bytes (zlib it and append it to navmap.bin, tag LTBFDOOR)
};
DoorwayCount WorkOutDoorways(const NavMap &aMap, const PlanParams &aParams, float aX0, float aY0, float aX1, float aY1);

} // namespace Navigation
