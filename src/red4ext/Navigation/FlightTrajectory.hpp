#pragma once

// The Autopilot's handle on a trajectory (Navigation/Trajectory.hpp): FlightTrajectory.Build turns
// the route the Autopilot holds into one for its car, and the follower reads it by distance along
// the route.

#include "FlightVehicleModel.hpp"
#include "Navigation/Trajectory.hpp"

#include <RED4ext/Scripting/IScriptable.hpp>
#include <RED4ext/Scripting/Natives/Generated/Vector4.hpp>
#include <RedLib.hpp>

struct FlightTrajectory : RED4ext::IScriptable {
  Navigation::Trajectory trajectory;

  // The trajectory along `route` for the car in `vehicle`, from `startSpeed` m/s, stopping at its end
  // when `stop`.
  static RED4ext::Handle<FlightTrajectory> Build(RED4ext::DynArray<RED4ext::Vector4> aRoute,
                                                 RED4ext::Handle<FlightVehicleModel> aVehicle, float aStartSpeed, bool aStop);

  bool IsValid() { return trajectory.samples.size() >= 2; }
  float Length() { return IsValid() ? trajectory.samples.back().s : 0.0f; }
  float Duration() { return trajectory.Duration(); }
  // at distance s along the route (clamped to its ends), interpolated
  float Speed(float aS);
  float Along(float aS);
  float Time(float aS);
  RED4ext::Vector4 Position(float aS);
  RED4ext::Vector4 Velocity(float aS);
  RED4ext::Vector4 Acceleration(float aS);
  // the drone's planned nose yaw, body pitch and roll, rad
  float Yaw(float aS);
  float Pitch(float aS);
  float Roll(float aS);
  // rad the nose is turned off the line into the bend (left > 0)
  float Drift(float aS);
  // what sets the speed at s; Top with no trajectory, so callers check IsValid
  Navigation::TrajectorySample::Limit Limit(float aS);
  // samples outside the car's limits (a planner bug), one line each
  RED4ext::CString Violations();

  RTTI_IMPL_TYPEINFO(FlightTrajectory);
  RTTI_IMPL_ALLOCATOR();
};

// FlightTrajectory.reds declares the same values in this order
RTTI_DEFINE_ENUM(Navigation::TrajectorySample::Limit, "FlightTrajectoryLimit");

RTTI_DEFINE_CLASS(FlightTrajectory, {
  RTTI_METHOD(Build);
  RTTI_METHOD(IsValid);
  RTTI_METHOD(Length);
  RTTI_METHOD(Duration);
  RTTI_METHOD(Speed);
  RTTI_METHOD(Along);
  RTTI_METHOD(Time);
  RTTI_METHOD(Position);
  RTTI_METHOD(Velocity);
  RTTI_METHOD(Acceleration);
  RTTI_METHOD(Yaw);
  RTTI_METHOD(Pitch);
  RTTI_METHOD(Roll);
  RTTI_METHOD(Drift);
  RTTI_METHOD(Limit);
  RTTI_METHOD(Violations);
});
