#include "Surge.hpp"

#include <algorithm>

namespace Navigation {

void UpdateSurge(SurgeState &aState, const SurgeParams &aParams, bool aFree, bool aValid, TrajectorySample::Limit aLimit,
                 float aPlanSpeed, float aSpeed, float aDt) {
  if (!aParams.fullSurge || !aFree) {
    aState.held = false;
    aState.changing = 0.0f;
    aState.weight = 0.0f;
    return;
  }
  // the speed is not part of the hold: over the plan the weight falls off instead, so the surge does
  // not switch between full and the controller's each time the car crosses the slack's edge
  const bool want =
      aValid && (aLimit == TrajectorySample::Limit::Top || aLimit == TrajectorySample::Limit::Accel);
  if (want != aState.held) {
    aState.changing += aDt;
    if (aState.changing >= aParams.dwell) {
      aState.held = want;
      aState.changing = 0.0f;
    }
  } else {
    aState.changing = 0.0f;
  }
  // full at or under the plan speed, none at slack over it; the hold waits dwell to start, but the fade out
  // starts as soon as the limit stops being the car's own, so braking for a bend is not later than a let-go
  // after dwell
  float target = 0.0f;
  if (aState.held && want) {
    target = aParams.slack > 0.0f ? std::clamp((aPlanSpeed + aParams.slack - aSpeed) / aParams.slack, 0.0f, 1.0f)
                                  : (aSpeed <= aPlanSpeed ? 1.0f : 0.0f);
  }
  // rate limited both ways, so a step in the plan's speed (a join, a replan) or the hold letting go for a bend
  // fades the push in or out instead of stepping it; only the safety releases above let go at once
  const float step = std::max(aParams.rate, 0.0f) * aDt;
  aState.weight += std::clamp(target - aState.weight, -step, step);
}

} // namespace Navigation
