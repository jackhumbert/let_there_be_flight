#pragma once

// Plan ahead (docs/design/trajectory.md step 7): a trip from rest planned as a short first leg the
// car takes off on at once, and the rest planned from where the car will be on that leg by the time
// the rest's plan is done, at its velocity there, to be spliced on at that point. The Autopilot runs
// both on its planning thread (FlightNavMap::PlanTripAsync); navplan simulates the same
// (NAVPLAN_AHEAD, tools/navplan/ahead_sweep.py).

#include "Planner.hpp"

#include <vector>

namespace Navigation {

struct AheadParams {
  float horizon = 500.0f;   // m along the planner's doorway route the first leg goes (PlanParams::horizon)
  float margin = 2.0f;      // s added to the expected time of the rest's plan before the join
  float minTrip = 600.0f;   // m: a shorter trip is planned in one go
  float restGuess = 2.0f;   // s the rest's plan is expected to take
};

struct FirstLeg {
  PlanResult plan;          // plan.partial false: the whole trip, nothing to follow
  float joinS = 0.0f;       // m along the first leg where the rest starts
  float joinT = 0.0f;       // s after take-off the car gets there
  Vec3 joinPosition{}, joinVelocity{};
  bool shortLeg = false;    // the car would reach the leg's end braking before the rest is due
};

// The first leg from aStart (a car at rest or flying at aParams.startVelocity) and its join.
FirstLeg PlanFirstLeg(const NavMap &aMap, Vec3 aStart, Vec3 aGoal, const PlanParams &aParams, const AheadParams &aAhead);

// The rest from a first leg's join, flying at the leg's velocity there.
PlanResult PlanRest(const NavMap &aMap, const FirstLeg &aLeg, Vec3 aGoal, const PlanParams &aParams);

// The first leg up to its join, then the rest (which starts there): the trip's whole line.
std::vector<Vec3> SpliceRest(const FirstLeg &aLeg, const PlanResult &aRest);

} // namespace Navigation
