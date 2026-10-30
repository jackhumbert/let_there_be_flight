#include "FlightTrajectory.hpp"

#include <algorithm>
#include <cmath>
#include <string>

namespace {
RED4ext::Vector4 Lerp(const Navigation::Vec3 &aA, const Navigation::Vec3 &aB, float aT, float aW) {
  return {aA.x + (aB.x - aA.x) * aT, aA.y + (aB.y - aA.y) * aT, aA.z + (aB.z - aA.z) * aT, aW};
}
} // namespace

RED4ext::Handle<FlightTrajectory> FlightTrajectory::Build(RED4ext::DynArray<RED4ext::Vector4> aRoute,
                                                          RED4ext::Handle<FlightVehicleModel> aVehicle, float aStartSpeed,
                                                          bool aStop) {
  auto handle = RED4ext::MakeHandle<FlightTrajectory>();
  std::vector<Navigation::Vec3> line;
  line.reserve(aRoute.size);
  for (const auto &p : aRoute) {
    line.push_back({p.X, p.Y, p.Z});
  }
  Navigation::VehicleSpec spec;
  if (aVehicle) {
    spec = aVehicle->Spec();
  }
  Navigation::TrajectoryParams params;
  params.startSpeed = std::max(aStartSpeed, 0.0f);
  params.stopAtEnd = aStop;
  handle->trajectory = Navigation::BuildTrajectory(spec, line, params);
  return handle;
}

size_t FlightTrajectory::Find(float aS, float &aT) const {
  const auto &q = trajectory.samples;
  if (aS <= q.front().s) {
    aT = 0.0f;
    return 0;
  }
  if (aS >= q.back().s) {
    aT = 0.0f;
    return q.size() - 1;
  }
  auto it = std::upper_bound(q.begin(), q.end(), aS, [](float s, const Navigation::TrajectorySample &x) { return s < x.s; });
  size_t i = size_t(it - q.begin()) - 1;
  aT = std::clamp((aS - q[i].s) / std::max(q[i + 1].s - q[i].s, 1e-6f), 0.0f, 1.0f);
  return i;
}

#define SAMPLE_AT(field)                                                                                                 \
  if (!IsValid())                                                                                                        \
    return {};                                                                                                           \
  float t;                                                                                                               \
  size_t i = Find(aS, t);                                                                                                \
  const auto &a = trajectory.samples[i];                                                                                 \
  const auto &b = trajectory.samples[std::min(i + 1, trajectory.samples.size() - 1)];

float FlightTrajectory::Speed(float aS) {
  SAMPLE_AT(speed)
  return a.speed + (b.speed - a.speed) * t;
}

float FlightTrajectory::Along(float aS) {
  SAMPLE_AT(along)
  return a.along + (b.along - a.along) * t;
}

float FlightTrajectory::Time(float aS) {
  SAMPLE_AT(t)
  return a.t + (b.t - a.t) * t;
}

RED4ext::Vector4 FlightTrajectory::Position(float aS) {
  SAMPLE_AT(position)
  return Lerp(a.position, b.position, t, 1.0f);
}

RED4ext::Vector4 FlightTrajectory::Velocity(float aS) {
  SAMPLE_AT(velocity)
  return Lerp(a.velocity, b.velocity, t, 0.0f);
}

RED4ext::Vector4 FlightTrajectory::Acceleration(float aS) {
  SAMPLE_AT(acceleration)
  return Lerp(a.acceleration, b.acceleration, t, 0.0f);
}

float FlightTrajectory::Yaw(float aS) {
  SAMPLE_AT(yaw)
  float d = b.yaw - a.yaw;
  d -= 6.2831853f * std::round(d / 6.2831853f); // the short way round
  return a.yaw + d * t;
}

float FlightTrajectory::Pitch(float aS) {
  SAMPLE_AT(pitch)
  return a.pitch + (b.pitch - a.pitch) * t;
}

float FlightTrajectory::Roll(float aS) {
  SAMPLE_AT(roll)
  return a.roll + (b.roll - a.roll) * t;
}

RED4ext::CString FlightTrajectory::Limit(float aS) {
  SAMPLE_AT(limit)
  return Navigation::LimitName(t < 0.5f ? a.limit : b.limit);
}

#undef SAMPLE_AT

RED4ext::CString FlightTrajectory::Violations() {
  std::string out;
  for (const auto &v : trajectory.violations) {
    out += (out.empty() ? "" : "; ") + v;
  }
  return out.c_str();
}
