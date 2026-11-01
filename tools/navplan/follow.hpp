#pragma once

// The follower flown offline (docs/design/trajectory.md, "Where things go"): the Autopilot's controller
// (Navigation/Tracker, DroneAttitude, Surge) and thrust limits (FlightModeAutopilot.FlyAerobatic) over a
// route and its trajectory, with a point mass for the car.
//
// The model: the body takes the attitude target exactly, so the attitude PD, the torques and the game's
// PhysX contact are not simulated. The thrust is the commanded acceleration, the drag it holds off and
// gravity, in the target's frame and inside the car's surge, sway and lift; the car then accelerates by
// that thrust less gravity and the air drag (semi-implicit Euler). Whatever the plan asks that the
// controller cannot fly shows here as an offset from the line or a speed error; what the real body adds
// on top (attitude lag, bumps) does not.

#include "Navigation/DroneAttitude.hpp"
#include "Navigation/Route.hpp"
#include "Navigation/Space.hpp"
#include "Navigation/Tracker.hpp"
#include "Navigation/Trajectory.hpp"
#include "Navigation/Vehicle.hpp"

#include <cstdint>
#include <vector>

namespace Navigation {

struct FollowParams {
  float dt = 1.0f / 60.0f;
  float maxTime = 0.0f; // s; 0: 2 x the trajectory's duration + 30 s
  // the script's arrival test: remaining < arriveDistance && speed < arriveSpeed
  float arriveDistance = 6.0f; // m, FlightModeAutopilot.arriveDistance
  float arriveSpeed = 3.0f;    // m/s, the literal 3.0 in FlightModeAutopilot's arrival test (d_speed < 3.0)
  // the simulation's own end of a route flown without stopping (the script has none: it keeps
  // flying until the arrival test passes): the run stops when this close to the route's end at any speed
  float endDistance = 0.5f; // m
  TrackerParams tracker;
  AttitudeParams attitude;
};

struct FollowFrame {
  // refSpeed: the plan's speed at the car's own distance s. leadSpeed: the plan's speed at the point the
  // tracker commands, s + speed x lineLead (Tracker.cpp sRef), the speed the controller is asked for.
  // pitch, roll, yaw: rad (the drone attitude's body angles and nose yaw)
  float t = 0.0f, s = 0.0f, off = 0.0f, speed = 0.0f, refSpeed = 0.0f, leadSpeed = 0.0f, pitch = 0.0f, roll = 0.0f,
        yaw = 0.0f;
  // accel: the car's net acceleration this frame; thrust: the thrust acceleration after the surge, sway and
  // lift clamps (and the surge hold), world frame, m/s2 (gravity and the drag it holds off included)
  Vec3 p{}, v{}, accel{}, thrust{};
  bool held = false, blocked = false, emergency = false;
};

struct FollowResult {
  std::vector<FollowFrame> frames;
  bool arrived = false; // the script's arrival test passed (remaining < arriveDistance and speed < arriveSpeed)
  bool ended = false;   // reached endDistance from the route's end (the simulation's end of a non-stop route)
  float time = 0.0f;
  float offP50 = 0.0f, offP95 = 0.0f, offP99 = 0.0f, offMax = 0.0f; // m off the route
  float speedErrMean = 0.0f, speedErrP95 = 0.0f;                     // |speed - leadSpeed|, m/s (the commanded speed)
  float speedBiasMean = 0.0f;                                        // mean of (speed - leadSpeed), signed
  // lost: runs off the line; replans, blocked, emergency, held: frames (replans: requests)
  int32_t lost = 0, replans = 0, blocked = 0, emergency = 0, held = 0;
  std::vector<float> lostAt;                                          // route distance where each lost run began
};

// Nearest rank: the smallest value with at least aQ of the sorted values at or below it, index
// ceil(aQ n) - 1 (aQ 0 gives the first). 0 for an empty array.
float Percentile(const std::vector<float> &aSorted, double aQ);

// From route.points[0] at the trajectory's first velocity until the script's arrival test, the end of
// the route (endDistance) or maxTime.
FollowResult Follow(const VehicleSpec &aSpec, const Route &aRoute, const Trajectory &aTrajectory, const Space &aSpace,
                    const FollowParams &aParams);

} // namespace Navigation
