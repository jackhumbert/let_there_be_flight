#pragma once

// Route planning over the NavMap (docs/autodrive-flight-plan.md, docs/autopilot-research.md).
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
// 5. Racing line (optional): the pulled route spread into wide bends inside the free space.
//
// The result is a polyline of car-centre positions; the flight mode turns it into a trajectory.

#include "NavMap.hpp"

#include <string>
#include <vector>

namespace Navigation {

struct Vec3 {
  float x, y, z;
};

struct PlanParams {
  float preferredHeight = 25.0f; // metres above the gap floor (the floor is already a car-centre height)
  float climbCost = 3.0f;        // extra cost per metre of rise in preferred altitude
  float underCost = 0.5f;        // extra cost per metre flown with something overhead
  float heuristicWeight = 1.5f;  // > 1 trades optimality for speed
  float turnCost = 0.0f;         // cost (as metres flown) per 45 degrees the route turns over a few
                                 // cells, at every step: it prefers straighter, more open corridors,
                                 // whose bends the racing line can take wide
  float minPassage = 0.0f;       // m of free height (car-centre gap, the car's size already taken off)
                                 // the route may squeeze through between columns; 0 = any
  uint32_t maxExpansions = 3000000;
  float maxGradient = 1.0f;      // rise per horizontal metre allowed in the height profile
  float legClearance = 2.0f;     // straight legs must also be free this far to either side (m), on top
                                 // of the bake's car inflation, so a follower can stray a little
  bool tryDirect = false;
  // racing line (the Autopilot's Race option): the route evened out to points lineSpacing apart
  // and bent as little as the free space allows (least summed squared curvature), coarse to fine
  bool racingLine = false;
  float lineSpacing = 5.0f;      // metres between the finished line's points
  float lineSide = 3.0f;         // the line also stays free this far to either side (m)
  // blocked space this close across the line pushes it away, linePush per pass (where the move
  // stays free). Dogtown at 100 m, tools/navplan/laptime.py: no point within 5 m of blocked space
  // (1.7% within 3 m without), 55.6 s against 52.4 s; 8 m or a push only on the fine passes cost
  // more time for no more room
  float lineClearance = 6.0f;    // m
  // m above the floor below (car-centre height, the car's size already taken off) the line keeps,
  // but within lineFloorEnds of its start and end: street furniture (traffic lights, signs, lamps)
  // is streamed in as props and not on the map; a between-the-buildings line flew 6 m over a
  // street at 85 m/s and hit a traffic light's arm, 2026-10-27
  float lineFloorClearance = 12.0f;
  float lineFloorEnds = 60.0f;   // m
  float linePush = 0.1f;         // m
  float lineStep = 0.08f;        // gradient step per pass (stable below 0.125)
  // weight of the vertical bend against the sideways one: crests and dips are pushed over and
  // pulled through with the lift alone, so they are rounded off more (stable while
  // lineStep x weight stays below 0.125). 1.5: from the street start to Dogtown half the line at 90%
  // of top speed instead of 30%, a crest that slowed the car to 41 m/s rounded off
  float lineVerticalWeight = 1.5f;
  float directMargin = 20.0f;    // cruise this far above the highest floor along the line
  float maxCruise = 700.0f;
};

struct PlanResult {
  bool ok = false;
  bool direct = false;
  std::vector<Vec3> points;
  std::vector<Vec3> rejectedLine; // a racing line that was not free all the way (diagnostics)
  uint32_t expansions = 0;
  uint32_t gridNodes = 0; // nodes on the A* path before simplification
  float length = 0;
  double milliseconds = 0;
  std::string info;
};

PlanResult Plan(const NavMap &aMap, Vec3 aStart, Vec3 aGoal, const PlanParams &aParams);

} // namespace Navigation
