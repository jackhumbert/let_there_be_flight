#include "Surge.hpp"

namespace Navigation {

void UpdateSurge(SurgeState &aState, const SurgeParams &aParams, bool aFree, bool aValid, TrajectorySample::Limit aLimit,
                 float aPlanSpeed, float aSpeed, float aDt) {
  if (!aParams.fullSurge || !aFree) {
    aState.held = false;
    aState.changing = 0.0f;
    return;
  }
  bool want = aValid && (aLimit == TrajectorySample::Limit::Top || aLimit == TrajectorySample::Limit::Accel) &&
              aSpeed < aPlanSpeed + aParams.slack;
  if (want != aState.held) {
    aState.changing += aDt;
    if (aState.changing >= aParams.dwell) {
      aState.held = want;
      aState.changing = 0.0f;
    }
  } else {
    aState.changing = 0.0f;
  }
}

} // namespace Navigation
