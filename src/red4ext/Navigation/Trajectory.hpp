#pragma once

// The racing line with time on it (docs/design/trajectory.md): at every sample the speed, velocity,
// acceleration and time the car flies it with, inside the limits of Vehicle.hpp. The Autopilot
// tracks this instead of working out speeds, braking and turn-ins while it flies.

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
};

struct TrajectorySample {
  enum class Limit : unsigned char { Top, Turn, Crest, PullUp, Sink, Arrive, Brake, Accel, Start, End };
  float s = 0.0f;             // m along the line
  float t = 0.0f;             // s from the start
  Vec3 position{}, velocity{}, acceleration{};
  float speed = 0.0f;         // m/s
  float along = 0.0f;         // m/s2 along the line (speeding up > 0)
  float lateral = 0.0f;       // m/s2 across the line (the bend at this speed)
  Limit limit = Limit::Top;   // what sets the speed here
};

struct Trajectory {
  std::vector<TrajectorySample> samples;
  float top = 0.0f;
  std::vector<std::string> violations; // samples outside the car's limits (a planner bug), for the log
  float Duration() const { return samples.empty() ? 0.0f : samples.back().t; }
};

Trajectory BuildTrajectory(const VehicleSpec &aSpec, const std::vector<Vec3> &aLine, const TrajectoryParams &aParams = {});

const char *LimitName(TrajectorySample::Limit aLimit);

} // namespace Navigation
