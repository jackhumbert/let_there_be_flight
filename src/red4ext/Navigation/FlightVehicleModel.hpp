#pragma once

// The Autopilot's handle on the car's limits (Navigation/Vehicle.hpp): FlightModeAutopilot fills the
// fields from the car's stats and its tuning (UpdateVehicleModel) and asks it for the limits, and
// FlightNavMap.PlanRaceAsync hands it to the planner, so both plan with the same car. The car's measured drag area
// and thrusters (FlightStats.s_dragArea, s_thrusters) count only as far as the developer settings turn them on
// (shapeDrag, thrustStrength), as they do for the flying: Spec is the effective car (Navigation::Effective).

#include "Flight/DeveloperSettings.hpp"
#include "Flight/Force.hpp"
#include "Vehicle.hpp"
#include <RED4ext/Scripting/IScriptable.hpp>
#include <RED4ext/Scripting/Natives/Generated/Quaternion.hpp>
#include <RED4ext/Scripting/Natives/Generated/Vector4.hpp>
#include <RedLib.hpp>

struct FlightVehicleModel : RED4ext::IScriptable {
  float mass = 1500.0f;
  float airResistance = 2.0f;
  float brakingFriction = 1.4f;
  float surge = 15.0f;
  float surgeShare = 1.0f;
  float sway = 5.0f;
  float lift = 25.0f;
  float gravityFactor = 1.0f;
  float brakeLinear = 1.0f;
  bool drag = true;
  bool drone = true;
  float maxTilt = 55.0f;
  bool levelPitch = false;
  float brakeShare = 0.25f;
  float tiltFloor = 1.0f;
  float driftMax = 20.0f;
  float speedCap = 120.0f;
  float margin = 0.85f;
  float planDecel = 18.0f;
  float pushOver = 21.0f;
  float cdaFront = 0.0f;  // FlightStats.s_dragArea.Y
  float cdaSide = 0.0f;   // FlightStats.s_dragArea.X
  float cdaTop = 0.0f;    // FlightStats.s_dragArea.Z
  float thrusters = 0.0f; // FlightStats.s_thrusters

  Navigation::VehicleSpec Spec() const {
    Navigation::VehicleSpec s;
    s.mass = mass;
    s.airResistance = airResistance;
    s.brakingFriction = brakingFriction;
    s.surge = surge;
    s.surgeShare = surgeShare;
    s.sway = sway;
    s.lift = lift;
    s.gravityFactor = gravityFactor;
    s.brakeLinear = brakeLinear;
    s.drag = drag;
    s.drone = drone;
    s.maxTilt = maxTilt;
    s.levelPitch = levelPitch;
    s.brakeShare = brakeShare;
    s.tiltFloor = tiltFloor;
    s.driftMax = driftMax;
    s.speedCap = speedCap;
    s.margin = margin;
    s.planDecel = planDecel;
    s.pushOver = pushOver;
    const auto &dev = Flight::Developer();
    s.cdaFront = dev.shapeDrag ? cdaFront : 0.0f;
    s.cdaTop = dev.shapeDrag ? cdaTop : 0.0f;
    s.dragK = dev.dragK;
    s.thrusters = thrusters;
    s.thrustStrength = dev.thrustStrength;
    s.thrustPerThruster = dev.thrustPerThruster;
    return Navigation::Effective(s);
  }

  float Drag(float aSpeed) { return Navigation::Drag(Spec(), aSpeed); }
  // The acceleration the air takes from the car at aVelocity (world), as ApplyPhysics takes it, as a world vector along
  // which to thrust it back: with shapeDrag the shape's drag on each body axis (the car turned by aOrientation, so a
  // slide or a climb costs what it does in game), else Drag along the velocity.
  RED4ext::Vector4 DragVector(RED4ext::Vector4 aVelocity, RED4ext::Quaternion aOrientation) {
    const Flight::Vec3 v{aVelocity.X, aVelocity.Y, aVelocity.Z};
    const float speed = Flight::Length(v);
    if (speed < 0.5f)
      return {};
    const auto &dev = Flight::Developer();
    Flight::Vec3 out;
    if (dev.shapeDrag && cdaFront > 0.0f && mass > 0.0f) {
      const Flight::Quat q{aOrientation.i, aOrientation.j, aOrientation.k, aOrientation.r};
      const Flight::Vec3 area = Flight::Mul(Flight::Vec3{cdaSide, cdaFront, cdaTop}, 0.5f * 1.2f * dev.dragK);
      out = Flight::Neg(Flight::Rotate(q, Flight::ShapeDragAccel(Flight::Rotate(Flight::Conjugate(q), v), area, mass)));
    } else {
      out = Flight::Mul(v, Drag(speed) / speed);
    }
    return {out.x, out.y, out.z, 0.0f};
  }
  // surge and sway as the car flies them (scaled by its thrust)
  float Surge() { return Spec().surge; }
  float Sway() { return Spec().sway; }
  float DroneAccel(float aThrusters) { return Navigation::DroneAccel(Spec(), aThrusters); }
  float TopSpeed() { return Navigation::TopSpeed(Spec()); }
  float Lateral() { return Navigation::Lateral(Spec()); }
  float Accel(float aSpeed) { return Navigation::Accel(Spec(), aSpeed); }
  float Decel(float aSpeed) { return Navigation::Decel(Spec(), aSpeed); }
  float PlanDecel(float aSpeed) { return Navigation::PlanDecel(Spec(), aSpeed); }
  float DroneBodyAngle(float aAngle, float aThrust, float aThrusters, float aShare) {
    return Navigation::DroneBodyAngle(Spec(), aAngle, aThrust, aThrusters, aShare);
  }
  RED4ext::CString Describe() { return Spec().Describe().c_str(); }

  RTTI_IMPL_TYPEINFO(FlightVehicleModel);
  RTTI_IMPL_ALLOCATOR();
};

RTTI_DEFINE_CLASS(FlightVehicleModel, {
  RTTI_PROPERTY(mass);
  RTTI_PROPERTY(airResistance);
  RTTI_PROPERTY(brakingFriction);
  RTTI_PROPERTY(surge);
  RTTI_PROPERTY(surgeShare);
  RTTI_PROPERTY(sway);
  RTTI_PROPERTY(lift);
  RTTI_PROPERTY(gravityFactor);
  RTTI_PROPERTY(brakeLinear);
  RTTI_PROPERTY(drag);
  RTTI_PROPERTY(drone);
  RTTI_PROPERTY(maxTilt);
  RTTI_PROPERTY(levelPitch);
  RTTI_PROPERTY(brakeShare);
  RTTI_PROPERTY(tiltFloor);
  RTTI_PROPERTY(driftMax);
  RTTI_PROPERTY(speedCap);
  RTTI_PROPERTY(margin);
  RTTI_PROPERTY(planDecel);
  RTTI_PROPERTY(pushOver);
  RTTI_PROPERTY(cdaFront);
  RTTI_PROPERTY(cdaSide);
  RTTI_PROPERTY(cdaTop);
  RTTI_PROPERTY(thrusters);
  RTTI_METHOD(Drag);
  RTTI_METHOD(DragVector);
  RTTI_METHOD(Surge);
  RTTI_METHOD(Sway);
  RTTI_METHOD(DroneAccel);
  RTTI_METHOD(TopSpeed);
  RTTI_METHOD(Lateral);
  RTTI_METHOD(Accel);
  RTTI_METHOD(Decel);
  RTTI_METHOD(PlanDecel);
  RTTI_METHOD(DroneBodyAngle);
  RTTI_METHOD(Describe);
});
