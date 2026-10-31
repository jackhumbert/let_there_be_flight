// The surge hold's hysteresis (src/red4ext/Navigation/Surge): UpdateSurge.
#include "check.hpp"

#include "Navigation/Surge.hpp"

using namespace Navigation;
using Limit = TrajectorySample::Limit;

namespace {
// dt 0.125 and dwell 0.25 are exact in binary, so the dwell comparison has no rounding
constexpr float kDt = 0.125f;

SurgeParams Params() {
  SurgeParams p;
  p.dwell = 0.25f;
  return p;
}

void Step(SurgeState &aState, bool aFree, bool aValid, Limit aLimit, float aPlan, float aSpeed) {
  UpdateSurge(aState, Params(), aFree, aValid, aLimit, aPlan, aSpeed, kDt);
}
} // namespace

TEST(surge_dwell_before_hold) {
  SurgeState s;
  Step(s, true, true, Limit::Top, 30, 28);
  CHECK(!s.held);
  CHECK_NEAR(s.changing, 0.125, 1e-6);
  Step(s, true, true, Limit::Top, 30, 28);
  CHECK(s.held);
  CHECK_NEAR(s.changing, 0, 1e-6);
}

TEST(surge_release_at_once) {
  SurgeState s;
  s.held = true;
  Step(s, false, true, Limit::Top, 30, 28);
  CHECK(!s.held);
  CHECK_NEAR(s.changing, 0, 1e-6);
}

TEST(surge_brake_not_held) {
  SurgeState s;
  Step(s, true, true, Limit::Brake, 30, 28);
  Step(s, true, true, Limit::Brake, 30, 28);
  CHECK(!s.held);
  CHECK_NEAR(s.changing, 0, 1e-6);
}

TEST(surge_slack) {
  SurgeState s;
  Step(s, true, true, Limit::Accel, 30, 34);
  Step(s, true, true, Limit::Accel, 30, 34);
  CHECK(!s.held);
  CHECK_NEAR(s.changing, 0, 1e-6);
  Step(s, true, true, Limit::Accel, 30, 33.9f);
  CHECK_NEAR(s.changing, 0.125, 1e-6);
}

TEST(surge_flicker_resets) {
  SurgeState s;
  Step(s, true, true, Limit::Top, 30, 28);
  CHECK_NEAR(s.changing, 0.125, 1e-6);
  Step(s, true, true, Limit::Turn, 30, 28);
  CHECK(!s.held);
  CHECK_NEAR(s.changing, 0, 1e-6);
}

TEST(surge_full_off) {
  SurgeParams p = Params();
  p.fullSurge = false;
  SurgeState s;
  s.held = true;
  s.changing = 0.125f;
  UpdateSurge(s, p, true, true, Limit::Top, 30, 28, kDt);
  CHECK(!s.held);
  CHECK_NEAR(s.changing, 0, 1e-6);
  UpdateSurge(s, p, true, true, Limit::Top, 30, 28, kDt);
  UpdateSurge(s, p, true, true, Limit::Top, 30, 28, kDt);
  CHECK(!s.held);
}

TEST(surge_no_trajectory) {
  SurgeState s;
  Step(s, true, false, Limit::Top, 0, 1);
  Step(s, true, false, Limit::Top, 0, 1);
  CHECK(!s.held);
}

TEST(surge_held_dwell_before_release) {
  // free and valid, but the limit no longer wants it: the hold lasts dwell before letting go
  SurgeState s;
  s.held = true;
  Step(s, true, true, Limit::Top, 30, 28);
  CHECK(s.held);
  CHECK_NEAR(s.changing, 0, 1e-6);
  Step(s, true, true, Limit::Turn, 30, 28);
  CHECK(s.held);
  CHECK_NEAR(s.changing, 0.125, 1e-6);
  Step(s, true, true, Limit::Turn, 30, 28);
  CHECK(!s.held);
  CHECK_NEAR(s.changing, 0, 1e-6);
}

TEST(surge_held_flicker_resets) {
  SurgeState s;
  s.held = true;
  Step(s, true, true, Limit::Brake, 30, 28);
  CHECK_NEAR(s.changing, 0.125, 1e-6);
  Step(s, true, true, Limit::Accel, 30, 28);
  CHECK(s.held);
  CHECK_NEAR(s.changing, 0, 1e-6);
}

TEST(surge_only_top_and_accel) {
  // every other limit is set by something that is not the car's top speed: no hold starts
  for (Limit l : {Limit::Turn, Limit::Crest, Limit::PullUp, Limit::Sink, Limit::Arrive, Limit::Brake, Limit::Start,
                  Limit::End, Limit::Room}) {
    SurgeState s;
    Step(s, true, true, l, 30, 28);
    CHECK(!s.held);
    CHECK_NEAR(s.changing, 0, 1e-6);
  }
}
