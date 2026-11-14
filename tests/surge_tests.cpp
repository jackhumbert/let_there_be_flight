// The surge hold's hysteresis and weight (src/red4ext/Navigation/Surge): UpdateSurge.
#include "check.hpp"

#include "Navigation/Surge.hpp"

#include <algorithm>

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
  // over the plan speed the hold stays, its weight falls off over the slack: none at slack over
  SurgeState s;
  Step(s, true, true, Limit::Accel, 30, 34);
  Step(s, true, true, Limit::Accel, 30, 34);
  CHECK(s.held);
  CHECK_NEAR(s.weight, 0, 1e-6);
}

TEST(surge_weight_falls_off_over_slack) {
  // rate high enough that the weight reaches its target in one step
  SurgeParams p = Params();
  p.rate = 100.0f;
  SurgeState s;
  s.held = true;
  UpdateSurge(s, p, true, true, Limit::Top, 30, 28, kDt);
  CHECK_NEAR(s.weight, 1, 1e-6);
  UpdateSurge(s, p, true, true, Limit::Top, 30, 31, kDt);
  CHECK_NEAR(s.weight, 0.75, 1e-6);
  UpdateSurge(s, p, true, true, Limit::Top, 30, 32, kDt);
  CHECK_NEAR(s.weight, 0.5, 1e-6);
  UpdateSurge(s, p, true, true, Limit::Top, 30, 40, kDt);
  CHECK_NEAR(s.weight, 0, 1e-6);
}

TEST(surge_weight_rate_limited) {
  // the default rate (4 per s): 0.5 a step at dt 0.125, so full takes two steps either way
  SurgeState s;
  s.held = true;
  Step(s, true, true, Limit::Top, 30, 28);
  CHECK_NEAR(s.weight, 0.5, 1e-6);
  Step(s, true, true, Limit::Top, 30, 28);
  CHECK_NEAR(s.weight, 1, 1e-6);
  // a step in the plan's speed (a join): the car is suddenly past the slack, the push fades out
  Step(s, true, true, Limit::Top, 30, 40);
  CHECK_NEAR(s.weight, 0.5, 1e-6);
  Step(s, true, true, Limit::Top, 30, 40);
  CHECK_NEAR(s.weight, 0, 1e-6);
}

TEST(surge_weight_released_at_once) {
  SurgeState s;
  s.held = true;
  s.weight = 1.0f;
  Step(s, false, true, Limit::Top, 30, 28);
  CHECK_NEAR(s.weight, 0, 1e-6);
}

TEST(surge_weight_follows_hold) {
  // the limit stops being the car's own: the weight fades out at once, the hold lets go after dwell
  SurgeState s;
  s.held = true;
  s.weight = 1.0f;
  Step(s, true, true, Limit::Turn, 30, 28);
  CHECK(s.held);
  CHECK_NEAR(s.weight, 0.5, 1e-6);
  Step(s, true, true, Limit::Turn, 30, 28);
  CHECK(!s.held);
  CHECK_NEAR(s.weight, 0, 1e-6);
}

TEST(surge_steady_where_full_surge_overshoots) {
  // a car whose full surge holds a speed well over the plan's top (the cycle in game: full, release, full every
  // 1.5 s): with the weight the speed settles inside the slack and the surge stops moving
  constexpr float dt = 1.0f / 60.0f, plan = 90.0f, full = 15.0f, cruise = 1.0f;
  const float k = full / (110.0f * 110.0f); // drag: full surge would hold 110 m/s
  SurgeParams p;
  SurgeState s;
  float speed = 85.0f, minSurge = full, maxSurge = 0.0f;
  for (int i = 0; i < 60 * 30; ++i) {
    UpdateSurge(s, p, true, true, Limit::Top, plan, speed, dt);
    const float surge = cruise + s.weight * (full - cruise);
    speed += (surge - k * speed * speed) * dt;
    if (i >= 60 * 20) {
      minSurge = std::min(minSurge, surge);
      maxSurge = std::max(maxSurge, surge);
    }
  }
  CHECK(s.held);
  CHECK(speed > plan && speed < plan + p.slack);
  CHECK(maxSurge - minSurge < 0.05f);
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
