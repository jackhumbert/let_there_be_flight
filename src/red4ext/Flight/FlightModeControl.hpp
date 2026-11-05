#pragma once

// The mode controllers' maths (Flight/Modes.hpp) for the script modes, which fill the params and the inputs from their
// fields each frame, call one method and copy the outputs back (and the in/out fields for the modes that keep state).

#include "Flight/FlightPID.hpp"
#include "Flight/FlightStats.hpp"
#include "Flight/Modes.hpp"

#include <RED4ext/Scripting/IScriptable.hpp>
#include <RED4ext/Scripting/Natives/Generated/Vector4.hpp>
#include <RedLib.hpp>

struct FlightModeControl : RED4ext::IScriptable {
  // Flight::ThrustScale for the car with the developer settings' thrustStrength and thrustPerThruster, against
  // aReference m/s2 (1 at strength 0, or with no thrusters measured)
  static float Thrust(const Flight::StatsStatic &aStatic, float aReference);

  // params (the mode's and FlightSettings' fields of the same name, defaults as Flight::StandardParams, AutomaticParams,
  // HoverFlyParams and DroneParams)
  float standardModeSurgeFactor = 15.0f;
  float standardModeYawFactor = 2.5f;
  float standardModeSwayFactor = 5.0f;
  float standardModePitchFactor = 3.0f;
  float standardModeLiftFactor = 25.0f;
  float standardModeHoverFactor = 10.0f;
  float standardModePitchInputAngle = 45.0f;
  float standardModeRollFactor = 3.0f;
  float standardModeRollInputAngle = 45.0f;
  float generalRollWithYaw = 0.15f;
  float pitchWithLift = 0.0f;
  float pitchWithSurge = 0.0f;
  float brakeFactorLinear = 1.0f;
  float brakeFactorAngular = 1.0f;
  float hoverClamp = 10.0f;
  float hoverModeMinHoverHeight = 1.0f;
  float hoverModeMaxHoverHeight = 7.0f;
  float gravityFactor = 1.0f;
  float altitudeHoldFactor = 1.0f;
  float heightDampening = 1.0f;
  float heightCorrectionFactor = 0.5f;
  float automaticModeAutoBrakingFactor = 3.0f;
  float automaticModeYawDirectionality = 300.0f;
  float automaticModeLiftFactor = 25.0f;
  float automaticModeHoverFactor = 40.0f;
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
  bool foundGround = false;
  float distance = 0.0f;
  RED4ext::Vector4 normal{};

  // in and out (the mode's fields and FlightComponent.hoverHeight)
  float hoverHeight = 0.0f;
  float hovering = 1.0f;
  float referenceZ = 0.0f;
  bool holdingAltitude = false;

  // outputs, car frame
  RED4ext::Vector4 force{};
  RED4ext::Vector4 torque{};

  // Flight::StandardLift / StandardDistance with the given normal and lift force or height difference
  void StandardLift(RED4ext::Handle<FlightStats> aStats, RED4ext::Handle<FlightPID> aPitchPid,
                    RED4ext::Handle<FlightPID> aRollPid, RED4ext::Vector4 aNormal, float aLiftForce);
  void StandardDistance(RED4ext::Handle<FlightStats> aStats, RED4ext::Handle<FlightPID> aHoverPid,
                        RED4ext::Handle<FlightPID> aPitchPid, RED4ext::Handle<FlightPID> aRollPid,
                        RED4ext::Vector4 aNormal, float aHeightDifference);
  // Flight::HoverFly and Flight::Automatic on foundGround, distance, normal and the in/out fields, which they write back
  void HoverFly(RED4ext::Handle<FlightStats> aStats, RED4ext::Handle<FlightPID> aHoverPid,
                RED4ext::Handle<FlightPID> aPitchPid, RED4ext::Handle<FlightPID> aRollPid);
  void Automatic(RED4ext::Handle<FlightStats> aStats, RED4ext::Handle<FlightPID> aHoverPid,
                 RED4ext::Handle<FlightPID> aPitchPid, RED4ext::Handle<FlightPID> aRollPid);
  void Drone(RED4ext::Handle<FlightStats> aStats);

  // Flight/ScriptMath on the script's types, to compare with the game's own in game
  static RED4ext::Vector4 RotateAxis(RED4ext::Vector4 aV, RED4ext::Vector4 aAxis, float aAngle);
  static RED4ext::Vector4 Interpolate(RED4ext::Vector4 aA, RED4ext::Vector4 aB, float aT);
  static float Lerp(float aAlpha, float aA, float aB);

  RTTI_IMPL_TYPEINFO(FlightModeControl);
  RTTI_IMPL_ALLOCATOR();

private:
  // the params, surge and sway scaled by the car's thrust (Thrust)
  Flight::StandardParams Standard(const Flight::StatsStatic &aStatic) const;
  Flight::Controls Controls() const;
  Flight::Ground GroundOf() const;
  void SetOutput(const Flight::ModeOutput &aOut);
};

RTTI_DEFINE_CLASS(FlightModeControl, {
  RTTI_PROPERTY(standardModeSurgeFactor);
  RTTI_PROPERTY(standardModeYawFactor);
  RTTI_PROPERTY(standardModeSwayFactor);
  RTTI_PROPERTY(standardModePitchFactor);
  RTTI_PROPERTY(standardModeLiftFactor);
  RTTI_PROPERTY(standardModeHoverFactor);
  RTTI_PROPERTY(standardModePitchInputAngle);
  RTTI_PROPERTY(standardModeRollFactor);
  RTTI_PROPERTY(standardModeRollInputAngle);
  RTTI_PROPERTY(generalRollWithYaw);
  RTTI_PROPERTY(pitchWithLift);
  RTTI_PROPERTY(pitchWithSurge);
  RTTI_PROPERTY(brakeFactorLinear);
  RTTI_PROPERTY(brakeFactorAngular);
  RTTI_PROPERTY(hoverClamp);
  RTTI_PROPERTY(hoverModeMinHoverHeight);
  RTTI_PROPERTY(hoverModeMaxHoverHeight);
  RTTI_PROPERTY(gravityFactor);
  RTTI_PROPERTY(altitudeHoldFactor);
  RTTI_PROPERTY(heightDampening);
  RTTI_PROPERTY(heightCorrectionFactor);
  RTTI_PROPERTY(automaticModeAutoBrakingFactor);
  RTTI_PROPERTY(automaticModeYawDirectionality);
  RTTI_PROPERTY(automaticModeLiftFactor);
  RTTI_PROPERTY(automaticModeHoverFactor);
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
  RTTI_PROPERTY(foundGround);
  RTTI_PROPERTY(distance);
  RTTI_PROPERTY(normal);
  RTTI_PROPERTY(hoverHeight);
  RTTI_PROPERTY(hovering);
  RTTI_PROPERTY(referenceZ);
  RTTI_PROPERTY(holdingAltitude);
  RTTI_PROPERTY(force);
  RTTI_PROPERTY(torque);
  RTTI_METHOD(StandardLift);
  RTTI_METHOD(StandardDistance);
  RTTI_METHOD(HoverFly);
  RTTI_METHOD(Automatic);
  RTTI_METHOD(Drone);
  RTTI_METHOD(RotateAxis);
  RTTI_METHOD(Interpolate);
  RTTI_METHOD(Lerp);
});
