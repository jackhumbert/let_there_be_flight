#pragma once

// The surge hold (AutopilotSurge.Update): whether the Autopilot holds the surge full, with the
// hysteresis that keeps it from flickering at a sample's edge. Pure code over the script's fields.

#include "Trajectory.hpp"

namespace Navigation {

// AutopilotSurge's fields of the same name
struct SurgeParams {
  bool fullSurge = true;
  float slack = 4.0f;  // m/s over the trajectory's speed the surge is still held full
  float dwell = 0.3f;  // s a change of the hold has to last before it is taken
};

// what carries over from one frame to the next
struct SurgeState {
  bool held = false;     // the surge was held full this frame
  float changing = 0.0f; // s the hold has wanted the other way
};

// aFree: nothing forbids the hold (the caller passes free && on the line && not braking for blocked
// space && no emergency brake); when it does, the hold lets go at once. aValid: there is a trajectory
// with at least two samples; without one the limit reads as Top, and the hold must not start.
// aLimit and aPlanSpeed are the trajectory's at the car's distance, aSpeed the car's.
void UpdateSurge(SurgeState &aState, const SurgeParams &aParams, bool aFree, bool aValid, TrajectorySample::Limit aLimit,
                 float aPlanSpeed, float aSpeed, float aDt);

} // namespace Navigation
