#include "Flight/Controls.hpp"

#include <cmath>
#include <initializer_list>

namespace Flight {

namespace {
// Divides the three by their length when it is over 1; zero and unit-or-shorter vectors stay as they are.
void ScaleDown(float &a, float &b, float &c) {
  float total = std::sqrt(a * a + b * b + c * c);
  if (total > 1.0f) {
    a /= total;
    b /= total;
    c /= total;
  }
}
} // namespace

ControlAxes DefaultControlAxes() {
  ControlAxes a;
  a.linearBrake.P = a.linearBrake.P_dec = 0.5f;
  a.angularBrake.P = a.angularBrake.P_dec = 0.5f;
  for (InputAxis *x : {&a.lift, &a.surge, &a.sway}) {
    x->P = 0.1f;
    x->P_dec = 1.0f;
  }
  for (InputAxis *x : {&a.roll, &a.pitch, &a.yaw}) {
    x->P = 0.5f;
    x->P_dec = 1.0f;
  }
  return a;
}

Controls NormalizeControls(Controls c) {
  ScaleDown(c.surge, c.lift, c.sway);
  ScaleDown(c.yaw, c.roll, c.pitch);
  return c;
}

Controls StepControls(ControlAxes &a) {
  Controls c;
  c.yaw = InputPidValue(a.yaw.state, a.yaw.P, a.yaw.P_dec);
  c.sway = InputPidValue(a.sway.state, a.sway.P, a.sway.P_dec);
  c.roll = InputPidValue(a.roll.state, a.roll.P, a.roll.P_dec);
  c.pitch = InputPidValue(a.pitch.state, a.pitch.P, a.pitch.P_dec);
  c.lift = InputPidValue(a.lift.state, a.lift.P, a.lift.P_dec);
  c.linearBrake = InputPidValue(a.linearBrake.state, a.linearBrake.P, a.linearBrake.P_dec);
  c.angularBrake = InputPidValue(a.angularBrake.state, a.angularBrake.P, a.angularBrake.P_dec);
  c.surge = InputPidValue(a.surge.state, a.surge.P, a.surge.P_dec);
  return NormalizeControls(c);
}

} // namespace Flight
