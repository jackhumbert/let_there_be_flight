#pragma once

// The surge hold (AutopilotSurge.Update): whether the Autopilot pushes the surge toward full, and how far.
// The hold itself (held) has the hysteresis that keeps it from flickering at a sample's edge; how far
// (weight) falls off over the slack above the plan speed, so the car settles inside the slack with a
// steady surge instead of cycling between full and the controller's. Pure code over the script's fields.

#include "Trajectory.hpp"

namespace Navigation {

// AutopilotSurge's fields of the same name
struct SurgeParams {
  bool fullSurge = true;
  float slack = 4.0f;  // m/s over the trajectory's speed the push toward full surge falls off over
  float dwell = 0.3f;  // s a change of the hold has to last before it is taken
  float rate = 4.0f;   // share of full per s the weight moves at most (a safety release lets go at once)
};

// what carries over from one frame to the next
struct SurgeState {
  bool held = false;     // the trajectory's speed is the car's own here: the surge is pushed toward full
  float changing = 0.0f; // s the hold has wanted the other way
  float weight = 0.0f;   // 0..1: how far the surge is pushed from the controller's toward full
};

// aFree: nothing forbids the hold (the caller passes free && on the line && not braking for blocked
// space && no emergency brake); when it does, the hold and its weight let go at once. aValid: there is a
// trajectory with at least two samples; without one the limit reads as Top, and the hold must not start.
// aLimit and aPlanSpeed are the trajectory's at the car's distance, aSpeed the car's.
void UpdateSurge(SurgeState &aState, const SurgeParams &aParams, bool aFree, bool aValid, TrajectorySample::Limit aLimit,
                 float aPlanSpeed, float aSpeed, float aDt);

} // namespace Navigation
