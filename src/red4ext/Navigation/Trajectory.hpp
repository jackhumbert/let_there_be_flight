#pragma once

// The line with time on it (docs/design/trajectory.md): at every sample the speed, velocity,
// acceleration and time the car flies it with, inside the limits of Vehicle.hpp. The Autopilot
// tracks this instead of working out speeds, braking and turn-ins while it flies.

#include "NavMap.hpp"
#include "Planner.hpp"
#include "Vehicle.hpp"

#include <string>
#include <vector>

namespace Navigation {

struct TrajectoryParams {
  float step = 2.0f;          // m between samples
  float radiusSpan = 15.0f;   // m either side a sample's bend radius is taken over (lineRadiusSpan)
  float bendSpan = 10.0f;     // m either side its direction and bend vector are taken over (lineSpan)
  float maxSink = 15.0f;      // m/s down at most on the approach (raceMaxSink)
  float sinkApproach = 250.0f; // m before the end where maxSink applies (raceSinkApproach)
  float arriveLength = 120.0f; // m before the end braked at most arriveDecel (raceArriveLength)
  float arriveDecel = 5.0f;   // m/s2 (raceArriveDecel)
  float brakeGripMin = 0.3f;  // share of the braking left at the full lateral envelope (raceBrakeGripMin)
  float humpSpan = 150.0f;    // m either side two slowings are looked for (speedHumpSpan)
  float humpMerge = 6.0f;     // m/s a rise between two slowings may be at most to be held down (speedHumpMerge)
  float startSpeed = 0.0f;    // m/s at the first sample
  bool stopAtEnd = true;      // 0 m/s at the last sample (a landing or an arrival)
  // drone attitude (FlightModeAutopilot's drone tuning)
  float bodyShare = 0.5f;     // share of the thrust's tilt the body takes (droneBodyShare)
  float rollLook = 1.5f;      // s ahead a coming turn's roll is held for (droneRollLook)
  float rollRateMax = 1.0f;   // rad/s (raceRollRateMax)
  float pitchRateMax = 1.0f;  // rad/s (racePitchRateMax)
  // drift: in a bend the nose turns into it, the car's driftMax at driftFull sideways (none below
  // driftStart), so the surge held pushes round the bend (Lateral counts it)
  float driftStart = 8.0f;    // m/s2 (raceDriftStart)
  float driftFull = 28.0f;    // m/s2 (raceDriftFull)
  float driftRateMax = 0.5f;  // rad/s the drift is taken up and let go at most
  // room: the car strays off its line (telemetry: p99 about 5 m above 80 m/s), so where the free
  // room across the line is short the speed is capped to what leaves roomMin + roomGrow x
  // (v / roomSpeed)^2 of it, never below roomSpeedMin (a map: aMap given to BuildTrajectory)
  float roomMin = 2.0f;        // m
  float roomGrow = 4.0f;       // m more at roomSpeed
  float roomSpeed = 90.0f;     // m/s
  float roomSpeedMin = 15.0f;  // m/s
  float roomLook = 12.0f;      // m out each way the room is looked for
};

struct TrajectorySample {
  enum class Limit : unsigned char { Top, Turn, Crest, PullUp, Sink, Arrive, Brake, Accel, Start, End, Room };
  float s = 0.0f;             // m along the line
  float t = 0.0f;             // s from the start
  Vec3 position{}, velocity{}, acceleration{};
  float speed = 0.0f;         // m/s
  float along = 0.0f;         // m/s2 along the line (speeding up > 0)
  float lateral = 0.0f;       // m/s2 across the line (the bend at this speed)
  Limit limit = Limit::Top;   // what sets the speed here
  float room = 0.0f;          // m free across the line here, the nearest of left, right, up and down (roomLook at most)
  float roomLoss = 0.0f;      // m/s the room costs here: the speed without the room's cap less the speed with it
  // the drone's attitude: nose yaw (rad, world, the line's direction plus the drift), body pitch
  // (nose down when speeding up) and roll (into the turn), rad; the tilts start early enough to
  // reach each peak at the rate limits, and a turn's roll is held from a gentler bend before it
  float yaw = 0.0f, pitch = 0.0f, roll = 0.0f;
  float drift = 0.0f;         // rad the nose is turned off the line into the bend (left > 0)
};

struct Trajectory {
  std::vector<TrajectorySample> samples;
  float top = 0.0f;
  std::vector<std::string> violations; // samples outside the car's limits (a planner bug), for the log
  float Duration() const { return samples.empty() ? 0.0f : samples.back().t; }
};

// aMap: the room across the line caps the speed (nullptr: no room caps)
Trajectory BuildTrajectory(const VehicleSpec &aSpec, const std::vector<Vec3> &aLine, const TrajectoryParams &aParams = {},
                           const NavMap *aMap = nullptr);

const char *LimitName(TrajectorySample::Limit aLimit);

// A trajectory read at a distance along the line: every field interpolated between the two samples
// around it (the yaw the short way round, the limit of the nearer sample).
struct TrajectoryPoint {
  float s = 0.0f, t = 0.0f, speed = 0.0f, along = 0.0f, yaw = 0.0f, pitch = 0.0f, roll = 0.0f, drift = 0.0f;
  Vec3 position{}, velocity{}, acceleration{};
  TrajectorySample::Limit limit = TrajectorySample::Limit::Top;
};

// at distance s along the line, clamped to its ends, interpolated; all zero with fewer than two samples
TrajectoryPoint SampleAt(const Trajectory &aTrajectory, float aS);

// m/s lost in dips: each slowdown the car speeds up out of again (by aDip m/s or more both ways),
// outside the last aApproach metres (the landing), summed as top less low (tools/navplan/dips.py)
float DipLoss(const Trajectory &aTrajectory, float aApproach = 300.0f, float aDip = 5.0f);

} // namespace Navigation
