#include "PlanAhead.hpp"
#include "PlannerCommon.hpp"
#include "Trajectory.hpp"

#include <cmath>

namespace Navigation {

using namespace detail;

FirstLeg PlanFirstLeg(const NavMap &aMap, Vec3 aStart, Vec3 aGoal, const PlanParams &aParams, const AheadParams &aAhead) {
  FirstLeg out;
  auto params = aParams;
  params.horizon = Distance(aStart, aGoal) >= aAhead.minTrip ? aAhead.horizon : 0.0f;
  out.plan = PlanRace(aMap, aStart, aGoal, params);
  if (!out.plan.ok || !out.plan.partial || out.plan.points.size() < 2) {
    out.plan.partial = false;
    return out;
  }
  // where the car is when the rest's plan is due: on the leg's trajectory from the car's speed
  TrajectoryParams tp;
  const Vec3 &v = aParams.startVelocity;
  tp.startSpeed = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
  const auto leg = BuildTrajectory(aParams.vehicle, out.plan.points, tp, &aMap);
  if (leg.samples.empty()) {
    out.plan.partial = false;
    return out;
  }
  const float due = aAhead.restGuess + aAhead.margin;
  size_t k = 0;
  while (k + 1 < leg.samples.size() && leg.samples[k].t < due) {
    ++k;
  }
  // not inside the leg's braking for its own end: the rest has to be there before that
  while (k > 0 && leg.samples[k].limit == TrajectorySample::Limit::Arrive) {
    --k;
    out.shortLeg = true;
  }
  const auto &j = leg.samples[k];
  out.joinS = j.s;
  out.joinT = j.t;
  out.joinPosition = j.position;
  out.joinVelocity = j.velocity;
  return out;
}

PlanResult PlanRest(const NavMap &aMap, const FirstLeg &aLeg, Vec3 aGoal, const PlanParams &aParams) {
  auto params = aParams;
  params.horizon = 0.0f;
  params.startVelocity = aLeg.joinVelocity;
  return PlanRace(aMap, aLeg.joinPosition, aGoal, params);
}

std::vector<Vec3> SpliceRest(const FirstLeg &aLeg, const PlanResult &aRest) {
  std::vector<Vec3> out;
  const auto &first = aLeg.plan.points;
  if (first.empty()) {
    return aRest.points;
  }
  out.push_back(first[0]);
  float along = 0.0f;
  for (size_t i = 1; i < first.size(); ++i) {
    along += Distance(first[i - 1], first[i]);
    if (along >= aLeg.joinS) {
      break;
    }
    out.push_back(first[i]);
  }
  out.insert(out.end(), aRest.points.begin(), aRest.points.end());
  return out;
}

} // namespace Navigation
