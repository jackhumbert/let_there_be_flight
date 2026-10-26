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
  uint32_t maxExpansions = 3000000;
  float maxGradient = 1.0f;      // rise per horizontal metre allowed in the height profile
  float legClearance = 2.0f;     // straight legs must also be free this far to either side (m), on top
                                 // of the bake's car inflation, so a follower can stray a little
  bool tryDirect = false;
  float directMargin = 20.0f;    // cruise this far above the highest floor along the line
  float maxCruise = 700.0f;
};

struct PlanResult {
  bool ok = false;
  bool direct = false;
  std::vector<Vec3> points;
  uint32_t expansions = 0;
  uint32_t gridNodes = 0; // nodes on the A* path before simplification
  float length = 0;
  double milliseconds = 0;
  std::string info;
};

PlanResult Plan(const NavMap &aMap, Vec3 aStart, Vec3 aGoal, const PlanParams &aParams);

} // namespace Navigation
