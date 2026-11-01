#include "FlightStats.hpp"

#include <RED4ext/CName.hpp>
#include <RED4ext/RTTISystem.hpp>

namespace {
Flight::Vec3 ToVec(const RED4ext::Vector4 &aV) { return {aV.X, aV.Y, aV.Z}; }
Flight::Quat ToQuat(const RED4ext::Quaternion &aQ) { return {aQ.i, aQ.j, aQ.k, aQ.r}; }

void Set(RED4ext::Vector4 &aTo, const Flight::Vec3 &aV) {
  aTo.X = aV.x;
  aTo.Y = aV.y;
  aTo.Z = aV.z;
}

void Set(RED4ext::Quaternion &aTo, const Flight::Quat &aQ) {
  aTo.i = aQ.i;
  aTo.j = aQ.j;
  aTo.k = aQ.k;
  aTo.r = aQ.r;
}
} // namespace

Flight::StatsState FlightStats::State() const {
  Flight::StatsState s;
  s.d_position = ToVec(d_position);
  s.d_visualPosition = ToVec(d_visualPosition);
  s.d_orientation = ToQuat(d_orientation);
  s.d_lastOrientation = ToQuat(d_lastOrientation);
  s.d_orientationChange = ToQuat(d_orientationChange);
  s.d_forward = ToVec(d_forward);
  s.d_right = ToVec(d_right);
  s.d_up = ToVec(d_up);
  s.d_localUp = ToVec(d_localUp);
  s.d_forward2D = ToVec(d_forward2D);
  s.d_angularVelocity = ToVec(d_angularVelocity);
  s.d_angularAcceleration = ToVec(d_angularAcceleration);
  s.d_velocity = ToVec(d_velocity);
  s.d_localVelocity = ToVec(d_localVelocity);
  s.d_velocity2D = ToVec(d_velocity2D);
  s.d_speed = d_speed;
  s.d_speedRatio = d_speedRatio;
  s.d_speedRatioSquared = d_speedRatioSquared;
  s.d_speed2D = d_speed2D;
  s.d_direction = ToVec(d_direction);
  s.d_localDirection = ToVec(d_localDirection);
  s.d_direction2D = ToVec(d_direction2D);
  s.d_localDirection2D = ToVec(d_localDirection2D);
  return s;
}

void FlightStats::SetState(const Flight::StatsState &aState) {
  Set(d_position, aState.d_position);
  Set(d_visualPosition, aState.d_visualPosition);
  Set(d_orientation, aState.d_orientation);
  Set(d_lastOrientation, aState.d_lastOrientation);
  Set(d_orientationChange, aState.d_orientationChange);
  Set(d_forward, aState.d_forward);
  Set(d_right, aState.d_right);
  Set(d_up, aState.d_up);
  Set(d_localUp, aState.d_localUp);
  Set(d_forward2D, aState.d_forward2D);
  Set(d_angularVelocity, aState.d_angularVelocity);
  Set(d_angularAcceleration, aState.d_angularAcceleration);
  Set(d_velocity, aState.d_velocity);
  Set(d_localVelocity, aState.d_localVelocity);
  Set(d_velocity2D, aState.d_velocity2D);
  d_speed = aState.d_speed;
  d_speedRatio = aState.d_speedRatio;
  d_speedRatioSquared = aState.d_speedRatioSquared;
  d_speed2D = aState.d_speed2D;
  Set(d_direction, aState.d_direction);
  Set(d_localDirection, aState.d_localDirection);
  Set(d_direction2D, aState.d_direction2D);
  Set(d_localDirection2D, aState.d_localDirection2D);
}

FlightStats *FlightStats::Of(RED4ext::IScriptable *aComponent) {
  auto rtti = RED4ext::CRTTISystem::Get();
  static auto *prop = [rtti] {
    auto cls = rtti->GetClass("FlightComponent");
    return cls ? cls->GetProperty("stats") : nullptr;
  }();
  static auto *statsClass = rtti->GetClass("FlightStats");
  if (!aComponent || !prop || !statsClass) {
    return nullptr;
  }
  auto stats = prop->GetValuePtr<RED4ext::Handle<RED4ext::IScriptable>>(aComponent)->GetPtr();
  return stats && stats->GetType()->IsA(statsClass) ? static_cast<FlightStats *>(stats) : nullptr;
}
