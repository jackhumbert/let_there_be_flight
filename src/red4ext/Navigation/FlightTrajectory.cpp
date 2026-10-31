#include "FlightTrajectory.hpp"

#include <algorithm>
#include <cmath>
#include <string>

namespace {
RED4ext::Vector4 ToVector(const Navigation::Vec3 &aV, float aW) { return {aV.x, aV.y, aV.z, aW}; }
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
  // the room across the line caps the speed where the map is loaded
  const auto &map = Navigation::NavMap::Get();
  handle->trajectory = Navigation::BuildTrajectory(spec, line, params, map.IsLoaded() ? &map : nullptr);
  return handle;
}

float FlightTrajectory::Speed(float aS) { return Navigation::SampleAt(trajectory, aS).speed; }

float FlightTrajectory::Along(float aS) { return Navigation::SampleAt(trajectory, aS).along; }

float FlightTrajectory::Time(float aS) { return Navigation::SampleAt(trajectory, aS).t; }

RED4ext::Vector4 FlightTrajectory::Position(float aS) {
  if (!IsValid()) {
    return {};
  }
  return ToVector(Navigation::SampleAt(trajectory, aS).position, 1.0f);
}

RED4ext::Vector4 FlightTrajectory::Velocity(float aS) {
  if (!IsValid()) {
    return {};
  }
  return ToVector(Navigation::SampleAt(trajectory, aS).velocity, 0.0f);
}

RED4ext::Vector4 FlightTrajectory::Acceleration(float aS) {
  if (!IsValid()) {
    return {};
  }
  return ToVector(Navigation::SampleAt(trajectory, aS).acceleration, 0.0f);
}

float FlightTrajectory::Yaw(float aS) { return Navigation::SampleAt(trajectory, aS).yaw; }

float FlightTrajectory::Pitch(float aS) { return Navigation::SampleAt(trajectory, aS).pitch; }

float FlightTrajectory::Roll(float aS) { return Navigation::SampleAt(trajectory, aS).roll; }

float FlightTrajectory::Drift(float aS) { return Navigation::SampleAt(trajectory, aS).drift; }

Navigation::TrajectorySample::Limit FlightTrajectory::Limit(float aS) {
  return Navigation::SampleAt(trajectory, aS).limit;
}

RED4ext::CString FlightTrajectory::Violations() {
  std::string out;
  for (const auto &v : trajectory.violations) {
    out += (out.empty() ? "" : "; ") + v;
  }
  return out.c_str();
}
