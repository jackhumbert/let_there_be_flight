#pragma once

// FlightController.UpdateInputs and the normalisation in FlightComponent.OnUpdate's player-mounted branch, as pure code.
//
// UpdateInputs only calls InputPID.GetValue on the eight axes; the component then reads the values and scales each
// group down when its length passes 1. The axes keep the unnormalised values: only the component's copies are scaled.

#include "Flight/Pid.hpp"

namespace Flight {

// FlightComponent's fields of the same names.
struct Controls {
  float surge = 0, lift = 0, sway = 0, yaw = 0, roll = 0, pitch = 0, linearBrake = 0, angularBrake = 0;
};

// One InputPID: its state and gains.
struct InputAxis {
  PidState state;
  float P = 0, P_dec = 0;
};

// The controller's eight InputPIDs.
struct ControlAxes {
  InputAxis surge, lift, sway, yaw, roll, pitch, linearBrake, angularBrake;
};

// The gains FlightController.Initialize gives them (P, P_dec): the brakes 0.5, 0.5; lift, surge and sway 0.1, 1.0;
// roll, pitch and yaw 0.5, 1.0.
ControlAxes DefaultControlAxes();

// Scales surge, lift and sway by their length when it is over 1, then yaw, roll and pitch the same way. The brakes are
// untouched. The script's PowF(x, 2.0) is written x * x; the two agree to within an ulp.
Controls NormalizeControls(Controls c);

// InputPidValue on each of the eight axes (independent; no dt, as InputPID never reads it), then NormalizeControls of
// the eight values.
Controls StepControls(ControlAxes &a);

} // namespace Flight
