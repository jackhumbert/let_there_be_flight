#pragma once

// The Autopilot's handle on the car's limits (Navigation/Vehicle.hpp): FlightModeAutopilot fills the
// fields from the car's stats and its tuning (UpdateVehicleModel) and asks it for the limits, and
// FlightNavMap.PlanRaceAsync hands it to the planner, so both plan with the same car.

#include "Vehicle.hpp"
#include <RED4ext/Scripting/IScriptable.hpp>
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
  float brakeShare = 0.25f;
  float driftMax = 20.0f;
  float speedCap = 120.0f;
  float margin = 0.85f;
  float planDecel = 18.0f;
  float pushOver = 21.0f;

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
    s.brakeShare = brakeShare;
    s.driftMax = driftMax;
    s.speedCap = speedCap;
    s.margin = margin;
    s.planDecel = planDecel;
    s.pushOver = pushOver;
    return s;
  }

  float Drag(float aSpeed) { return Navigation::Drag(Spec(), aSpeed); }
  float DroneAccel(float aThrusters) { return Navigation::DroneAccel(Spec(), aThrusters); }
  float TopSpeed() { return Navigation::TopSpeed(Spec()); }
  float Lateral() { return Navigation::Lateral(Spec()); }
  float Accel(float aSpeed) { return Navigation::Accel(Spec(), aSpeed); }
  float Decel(float aSpeed) { return Navigation::Decel(Spec(), aSpeed); }
  float PlanDecel(float aSpeed) { return Navigation::PlanDecel(Spec(), aSpeed); }
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
  RTTI_PROPERTY(brakeShare);
  RTTI_PROPERTY(driftMax);
  RTTI_PROPERTY(speedCap);
  RTTI_PROPERTY(margin);
  RTTI_PROPERTY(planDecel);
  RTTI_PROPERTY(pushOver);
  RTTI_METHOD(Drag);
  RTTI_METHOD(DroneAccel);
  RTTI_METHOD(TopSpeed);
  RTTI_METHOD(Lateral);
  RTTI_METHOD(Accel);
  RTTI_METHOD(Decel);
  RTTI_METHOD(PlanDecel);
  RTTI_METHOD(Describe);
});
