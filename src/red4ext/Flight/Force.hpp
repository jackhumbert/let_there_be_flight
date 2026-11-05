#pragma once

// FlightMode.ApplyPhysics and the tail of FlightComponent.OnUpdate (dt, mass, orientation, thruster tensor,
// collision recovery) as one pure function, for the native force path.

#include "Flight/ScriptMath.hpp"
#include "Flight/Stats.hpp"

namespace Flight {

// The fleet-wide factor on the shape's drag: 4.42 keeps the vanilla fleet's median top speed at today's 103 m/s
// (tools/flightsim, 2026-11-04).
constexpr float kShapeDragK = 4.42f;

// The FlightSettings the force tail reads, with the script's names and defaults; shapeDrag and dragK are the developer
// settings (Flight::DeveloperSettings) of the same names.
struct ForceParams {
  bool physxFlightPhysics = true;
  bool shapeDrag = false;
  float dragK = kShapeDragK;
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

// Air drag from the vehicle's shape (docs/research/2026-11-04-aero-drag.md; not used by the game yet): on each body
// axis i, -dragArea_i |v| v_i / mass, an acceleration in the body frame. dragArea is 0.5 rho K CdA per axis (N per
// (m/s)^2): the chassis collision shape's area across that axis times a drag coefficient for the body class, and K
// one fleet-wide factor (about 4.1 keeps a mid-size car's top speed). Sideways, where the area is 2 to 4 times the
// front, it pushes a slide back toward the nose.
Vec3 ShapeDragAccel(Vec3 localVelocity, Vec3 dragArea, float mass);

// How much a car's surge and sway are scaled for its thrust (not used by the game yet): the thrust its thrusters give
// (count x perThruster newtons) over its mass, against the acceleration every car gets today (reference, m/s2),
// raised to strength (0: today, every car the same; 1: by its thrusters alone, a 300 kg bike 3 times a mid-size car,
// an 8 t truck a third). 1 with no thrusters or no mass.
float ThrustScale(float thrusters, float mass, float perThruster, float reference, float strength);

// The angular damping ApplyPhysics takes from a spin (body frame): the damping factor plus the angular brake, off on an
// axis while its input is held, the yaw factor, CapDamping per axis and the generalDampFactorAngularMax cap. The
// Autopilot gives back exactly this at the rate it wants (Navigation::AttitudeControl), so the two cancel on every car.
Vec3 AngularDamping(const ForceParams &p, const StatsStatic &st, Vec3 spin, float pitch, float roll, float yaw,
                    float angularBrake, Vec3 thrusterTensor);

// FlightMode.ApplyPhysics. The aero side terms use the unflipped velocity direction, as the script does.
void ApplyPhysics(const ForceParams &p, const StatsStatic &st, const StatsState &s, const ForceInput &in,
                  ForceOutput &out);

// ApplyPhysics (when in.applyPhysics) and the rest of OnUpdate down to the two += lines.
ForceOutput ForceTail(const ForceParams &p, const StatsStatic &st, const StatsState &s, const ForceInput &in);

} // namespace Flight
