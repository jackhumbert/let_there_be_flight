// Controls (src/red4ext/Flight/Controls.hpp): the eight InputPIDs of FlightController.UpdateInputs and the
// normalisation in FlightComponent.OnUpdate. Expected values are worked by hand from the scripts.
#include "check.hpp"

#include "Flight/Controls.hpp"

using namespace Flight;

namespace {
constexpr double kTol = 1e-5;

// P = P_dec = 1: one step reaches the input.
ControlAxes UnitGains() {
  ControlAxes a;
  for (InputAxis *x : {&a.surge, &a.lift, &a.sway, &a.yaw, &a.roll, &a.pitch, &a.linearBrake, &a.angularBrake}) {
    x->P = 1.0f;
    x->P_dec = 1.0f;
  }
  return a;
}

// FlightController.Initialize's gains.
} // namespace

TEST(controls_normalize_linear) {
  // a length of exactly 1 is not over 1, so it is left as it is (1 is exact in float)
  Controls one;
  one.sway = 1.0f;
  CHECK(NormalizeControls(one).sway == 1.0f);

  // the linear brake is not in the linear group: surge 2 scales to 1 and the brake stays 1
  Controls b;
  b.surge = 2.0f;
  b.linearBrake = 1.0f;
  Controls nb = NormalizeControls(b);
  CHECK_NEAR(nb.surge, 1, kTol);
  CHECK_NEAR(nb.linearBrake, 1, kTol);

  // t = sqrt(0.36 + 0.64 + 0.36) = sqrt(1.36) = 1.1661904: 0.6 / t, 0.8 / t, 0.6 / t
  Controls c;
  c.surge = 0.6f;
  c.lift = 0.8f;
  c.sway = 0.6f;
  Controls n = NormalizeControls(c);
  CHECK_NEAR(n.surge, 0.514496, kTol);
  CHECK_NEAR(n.lift, 0.685994, kTol);
  CHECK_NEAR(n.sway, 0.514496, kTol);
}

TEST(controls_normalize_angular) {
  // t = sqrt(2): 1 / t = 0.707107
  Controls c;
  c.yaw = 1.0f;
  c.roll = 1.0f;
  c.linearBrake = 1.0f;
  c.angularBrake = 1.0f;
  Controls n = NormalizeControls(c);
  CHECK_NEAR(n.yaw, 0.707107, kTol);
  CHECK_NEAR(n.roll, 0.707107, kTol);
  CHECK_NEAR(n.pitch, 0, kTol);
  CHECK_NEAR(n.linearBrake, 1, kTol);
  CHECK_NEAR(n.angularBrake, 1, kTol);

  // only roll and pitch: t = sqrt(1.5^2 + 2^2) = 2.5, so 0.6 and 0.8
  Controls p;
  p.roll = 1.5f;
  p.pitch = 2.0f;
  n = NormalizeControls(p);
  CHECK_NEAR(n.roll, 0.6, kTol);
  CHECK_NEAR(n.pitch, 0.8, kTol);
  CHECK_NEAR(n.yaw, 0, kTol);

  // the linear group over 1 does not scale the angular group, which is under 1 (0.3, 0.4: length 0.5)
  Controls g;
  g.surge = 3.0f;
  g.lift = 4.0f;
  g.yaw = 0.3f;
  g.roll = 0.4f;
  n = NormalizeControls(g);
  CHECK_NEAR(n.surge, 0.6, kTol);
  CHECK_NEAR(n.lift, 0.8, kTol);
  CHECK_NEAR(n.yaw, 0.3, kTol);
  CHECK_NEAR(n.roll, 0.4, kTol);
}

TEST(controls_normalize_zero) {
  Controls n = NormalizeControls(Controls());
  CHECK_NEAR(n.surge, 0, kTol);
  CHECK_NEAR(n.lift, 0, kTol);
  CHECK_NEAR(n.sway, 0, kTol);
  CHECK_NEAR(n.yaw, 0, kTol);
  CHECK_NEAR(n.roll, 0, kTol);
  CHECK_NEAR(n.pitch, 0, kTol);
  CHECK_NEAR(n.linearBrake, 0, kTol);
  CHECK_NEAR(n.angularBrake, 0, kTol);
}

TEST(controls_step_defaults) {
  ControlAxes a = DefaultControlAxes();
  a.yaw.state.input = 1.0f;
  a.lift.state.input = 1.0f;
  // yaw: error 1, P 0.5 -> 0.5; lift: error 1, P 0.1 -> 0.1
  Controls c = StepControls(a);
  CHECK_NEAR(c.yaw, 0.5, kTol);
  CHECK_NEAR(c.lift, 0.1, kTol);
  // yaw: error 0.5 -> 0.75; lift: error 0.9 -> 0.1 + 0.09 = 0.19
  c = StepControls(a);
  CHECK_NEAR(c.yaw, 0.75, kTol);
  CHECK_NEAR(c.lift, 0.19, kTol);
  // input 0 while the value is 0.75: |input| < |value| and same side, so P_dec 1 reaches 0
  a.yaw.state.input = 0.0f;
  c = StepControls(a);
  CHECK_NEAR(c.yaw, 0, kTol);
}

TEST(controls_step_keeps_raw_state) {
  ControlAxes a = UnitGains();
  a.surge.state.input = 0.6f;
  a.lift.state.input = 0.8f;
  a.sway.state.input = 0.6f;
  Controls c = StepControls(a);
  CHECK_NEAR(c.surge, 0.514496, kTol);
  CHECK_NEAR(c.lift, 0.685994, kTol);
  CHECK_NEAR(c.sway, 0.514496, kTol);
  CHECK_NEAR(a.surge.state.value, 0.6, kTol);
  CHECK_NEAR(a.lift.state.value, 0.8, kTol);
  CHECK_NEAR(a.sway.state.value, 0.6, kTol);
}

TEST(controls_step_each_axis) {
  // distinct inputs, each group's length under 1 so nothing is scaled: surge, lift, sway 0.1, 0.2, 0.3 and
  // yaw, roll, pitch 0.4, 0.5, 0.6 (length 0.877)
  ControlAxes a = UnitGains();
  a.surge.state.input = 0.1f;
  a.lift.state.input = 0.2f;
  a.sway.state.input = 0.3f;
  a.yaw.state.input = 0.4f;
  a.roll.state.input = 0.5f;
  a.pitch.state.input = 0.6f;
  a.linearBrake.state.input = 0.7f;
  a.angularBrake.state.input = 0.9f;
  Controls c = StepControls(a);
  CHECK_NEAR(c.surge, 0.1, kTol);
  CHECK_NEAR(c.lift, 0.2, kTol);
  CHECK_NEAR(c.sway, 0.3, kTol);
  CHECK_NEAR(c.yaw, 0.4, kTol);
  CHECK_NEAR(c.roll, 0.5, kTol);
  CHECK_NEAR(c.pitch, 0.6, kTol);
  CHECK_NEAR(c.linearBrake, 0.7, kTol);
  CHECK_NEAR(c.angularBrake, 0.9, kTol);
}
