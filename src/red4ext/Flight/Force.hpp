#pragma once

// FlightMode.ApplyPhysics and the tail of FlightComponent.OnUpdate (dt, mass, orientation, thruster tensor,
// collision recovery) as one pure function, for the native force path.

#include "Flight/ScriptMath.hpp"
#include "Flight/Stats.hpp"

namespace Flight {

// The FlightSettings the force tail reads, with the script's names and defaults.
struct ForceParams {
  bool physxFlightPhysics = true;
  float generalDampFactorAngular = 2.0f, brakeFactorAngular = 1.0f, generalYawDampFactor = 1.0f;
  float dampStabilityK = 0.19f, dampMaxPerFrame = 1.0f, generalDampFactorAngularMax = 3.5f;
  float generalDampFactorLinear = 0.0f, brakeFactorLinear = 1.0f;
  float generalSwayWithYaw = 0.0f, generalYawDirectionalityFactor = 5.0f, generalPitchDirectionalityFactor = 15.0f;
  float generalYawAeroFactor = 0.0f, generalPitchAeroFactor = 0.0f;
  float collisionRecoveryDelay = 0.8f, collisionRecoveryDuration = 0.8f;
};

// What the script reads from the component. force and torque are the car-frame sums before ApplyPhysics (the
// trick plus the mode's Update force times percentageAttached, and its torque); applyPhysics is false when the
// mode index is out of range. contactSpin is GetContactSpin (world).
struct ForceInput {
  float dt = 0;
  Vec3 force, torque;
  bool applyPhysics = true;
  float pitch = 0, roll = 0, yaw = 0, linearBrake = 0, angularBrake = 0;
  Vec3 thrusterTensor;
  Vec3 contactSpin;
  float collisionTimer = 0;
};

// physicsForce and physicsTorque are what ApplyPhysics leaves in the mode's force and torque (car frame);
// linearDamp is the mode's linearDamp. worldForce and worldTorque are what OnUpdate adds to the component's
// force and torque. collisionTimer is the component's timer after the call.
struct ForceOutput {
  Vec3 physicsForce, physicsTorque;
  float linearDamp = 0;
  Vec3 worldForce, worldTorque;
  float collisionTimer = 0;
};

// FlightMode.CapDamping: limits one axis of the damping so it removes at most dampMaxPerFrame of that axis'
// angular velocity in a frame. A negative gain is not capped.
float CapDamping(float damp, float angularVelocity, float tensor, float inertia, float mass, const ForceParams &p);

// FlightMode.ApplyPhysics. The aero side terms use the unflipped velocity direction, as the script does.
void ApplyPhysics(const ForceParams &p, const StatsStatic &st, const StatsState &s, const ForceInput &in,
                  ForceOutput &out);

// ApplyPhysics (when in.applyPhysics) and the rest of OnUpdate down to the two += lines.
ForceOutput ForceTail(const ForceParams &p, const StatsStatic &st, const StatsState &s, const ForceInput &in);

} // namespace Flight
