#pragma once

// The mode controllers' maths (Flight/Modes.hpp) for the script modes (Drone and the anti-gravity drone), which fill
// the params and the inputs from their fields each frame, call one method and copy the outputs back.

#include "Flight/FlightStats.hpp"
#include "Flight/Modes.hpp"

#include <RED4ext/Scripting/IScriptable.hpp>
#include <RED4ext/Scripting/Natives/Generated/Vector4.hpp>
#include <RedLib.hpp>

struct FlightModeControl : RED4ext::IScriptable {
  // Flight::ThrustScale for the car with the developer settings' thrustStrength and thrustPerThruster, against
  // aReference m/s2 (1 at strength 0, or with no thrusters measured)
  static float Thrust(const Flight::StatsStatic &aStatic, float aReference);

  // params (the mode's and FlightSettings' fields of the same name, defaults as Flight::DroneParams; the brakes
  // already scaled by Brake Strength, the drone's factors by Drone Thrust and Drone Turn Speed)
  float brakeFactorLinear = 1.0f;
  float brakeFactorAngular = 1.0f;
  float droneModeLiftFactor = 40.0f;
  float droneModePitchFactor = 5.0f;
  float droneModeRollFactor = 8.0f;
  float droneModeSurgeFactor = 15.0f;
  float droneModeYawFactor = 5.0f;
  float droneModeSwayFactor = 15.0f;

  // inputs, this frame
  float dt = 0.0f;
  float surge = 0.0f;
  float lift = 0.0f;
  float sway = 0.0f;
  float yaw = 0.0f;
  float roll = 0.0f;
  float pitch = 0.0f;
  float linearBrake = 0.0f;
  float angularBrake = 0.0f;

  // outputs, car frame
  RED4ext::Vector4 force{};
  RED4ext::Vector4 torque{};

  // FlightModeDrone.Update. aComponent: the mode's FlightComponent, for Drone's skid (Flight::DroneSkid)
  void Drone(RED4ext::Handle<FlightStats> aStats, RED4ext::Handle<RED4ext::IScriptable> aComponent);

  // Flight/ScriptMath on the script's types, to compare with the game's own in game
  static RED4ext::Vector4 RotateAxis(RED4ext::Vector4 aV, RED4ext::Vector4 aAxis, float aAngle);
  static RED4ext::Vector4 Interpolate(RED4ext::Vector4 aA, RED4ext::Vector4 aB, float aT);
  static float Lerp(float aAlpha, float aA, float aB);

  RTTI_IMPL_TYPEINFO(FlightModeControl);
  RTTI_IMPL_ALLOCATOR();

private:
  Flight::Controls Controls() const;
  void SetOutput(const Flight::ModeOutput &aOut);
};

RTTI_DEFINE_CLASS(FlightModeControl, {
  RTTI_PROPERTY(brakeFactorLinear);
  RTTI_PROPERTY(brakeFactorAngular);
  RTTI_PROPERTY(droneModeLiftFactor);
  RTTI_PROPERTY(droneModePitchFactor);
  RTTI_PROPERTY(droneModeRollFactor);
  RTTI_PROPERTY(droneModeSurgeFactor);
  RTTI_PROPERTY(droneModeYawFactor);
  RTTI_PROPERTY(droneModeSwayFactor);
  RTTI_PROPERTY(dt);
  RTTI_PROPERTY(surge);
  RTTI_PROPERTY(lift);
  RTTI_PROPERTY(sway);
  RTTI_PROPERTY(yaw);
  RTTI_PROPERTY(roll);
  RTTI_PROPERTY(pitch);
  RTTI_PROPERTY(linearBrake);
  RTTI_PROPERTY(angularBrake);
  RTTI_PROPERTY(force);
  RTTI_PROPERTY(torque);
  RTTI_METHOD(Drone);
  RTTI_METHOD(RotateAxis);
  RTTI_METHOD(Interpolate);
  RTTI_METHOD(Lerp);
});
