#pragma once

// The Autopilot's follower (FlightModeAutopilot.Update, the following branch): where the car is on the
// route, whether it must replan, the aim point, the speed to fly and whether blocked space ahead needs
// braking for (Track); then the acceleration that follows the line or the aim point, rate limited, and
// the surge hold (Command). Two calls because the script's state machine (arrival, wedge, landing) runs
// between them and may clear the route. Pure code over the script's fields.

#include "Route.hpp"
#include "Space.hpp"
#include "Surge.hpp"
#include "Trajectory.hpp"
#include "Vehicle.hpp"

#include <cstdint>

namespace Navigation {

// FlightModeAutopilot's fields of the same name (brakeAccel is the Braking Mod Setting, passed in)
struct TrackerParams {
  float replanOffLine = 8.0f, replanDistance = 30.0f, cornerSpan = 15.0f, cornerOvershoot = 10.0f,
        collisionHorizon = 1.5f, raceOffLine = 4.0f, lineLead = 0.25f, speedGain = 1.0f, brakeAccel = 6.0f,
        brakeMax = 9.0f, velocityGain = 1.2f, accelJerk = 20.0f, raceJerk = 60.0f, lineSpring = 1.0f,
        lineVelocityGain = 2.5f, lineSpan = 10.0f, trajectoryCorrectionMax = 25.0f;
  SurgeParams surge;
};

// the script fields of the same name; they carry over from one frame to the next
struct TrackerState {
  int32_t segment = 0;
  float closestS = 0.0f, offRouteNow = 0.0f, remaining = 0.0f;
  Vec3 closestNow{};
  float aheadSmooth = 15.0f, curvatureAhead = 0.0f;
  Vec3 pathCurvature{};
  float blockedTimer = 0.0f;
  bool blockedAhead = false, emergencyBrake = false;
  float targetSpeed = 0.0f, speedCmd = 0.0f, speedRate = 0.0f;
  Vec3 accelCmd{};
  SurgeState surge;
};

struct TrackInput {
  float dt = 0.0f;
  Vec3 position{}, velocity{};
  float speed = 0.0f;
  bool routeLine = true;     // the route is a line with a trajectory (replanned when left); else an aim route
  bool planned = false;      // a trajectory is followed (honoured only with at least two samples)
  bool recovering = false;   // the controls are damped after a hit
  float replanTimer = 0.0f;
  bool requestPending = false;
  Vec3 aimOffset{};          // added to the aim point (the barrel roll's circle)
};

struct TrackOutput {
  Vec3 target{};             // the aim point
  bool replanLine = false;   // off the line: a new trajectory from here
  bool requestRoute = false; // off the route: ask for a new one
};

struct CommandInput {
  float dt = 0.0f;
  Vec3 position{}, velocity{};
  float speed = 0.0f;
  Vec3 forward{0.0f, 1.0f, 0.0f}; // the car's nose, where the route has no direction
  bool following = false;         // the status is "following" with a route and no shaped manoeuvre
  bool routeLine = true, planned = false, recovering = false;
  Vec3 target{};                  // Track's aim point; the hold point when not following
  Vec3 overlayVelocity{};
  // not following: what the caller worked out (the hold, the descent), and its jerk limit
  Vec3 rawAccel{}, rawDesiredVelocity{};
  float rawJerk = 20.0f;
  bool surgeFree = false;         // nothing but the follower forbids the surge hold (no recovery, moving, a trajectory)
};

struct CommandOutput {
  Vec3 accel{}, desiredVelocity{}, lookahead{};
  bool onLine = false;
};

// From the car's place on the route through the emergency brake (before "barely moving while it should
// fly"); it resets targetSpeed and emergencyBrake itself. aTrajectory may be null.
TrackOutput Track(TrackerState &aState, const TrackerParams &aParams, const VehicleSpec &aSpec, const TrackInput &aInput,
                  const Route &aRoute, const Trajectory *aTrajectory, const Space &aSpace);

// From the velocity error to the acceleration through the jerk limit, then the surge hold. aTrajectory may be null.
// The script clears targetSpeed and emergencyBrake at the top of every frame, following or not. Track (only
// called when following) sets them afresh; Command, when not following, resets targetSpeed, emergencyBrake,
// speedCmd (to the car's speed), speedRate and aheadSmooth before the rate limit, so a frame that did not
// follow never reads a stale emergency brake or target speed.
CommandOutput Command(TrackerState &aState, const TrackerParams &aParams, const CommandInput &aInput,
                      const Route &aRoute, const Trajectory *aTrajectory);

} // namespace Navigation
