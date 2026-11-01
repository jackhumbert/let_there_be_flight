#pragma once

// FlightStats.UpdateDynamic (src/redscript/Flight/FlightStats.reds) as pure code, for the native FlightStats.
// The script reads the vehicle's orientation, angular velocity and linear velocity, and its position from an
// entity position provider; StatsInput carries those as plain values.

#include "Flight/ScriptMath.hpp"

namespace Flight {

// The s_ fields UpdateStatic fills in script that UpdateDynamic or its callers use.
struct StatsStatic {
  float mass = 0, airResistanceFactor = 0, brakingFrictionFactor = 0;
  Vec3 momentOfInertia;
  Vec3 centerOfMass;
};

// The d_ fields, with the script's names. All zero like a script default.
struct StatsState {
  Vec3 d_position, d_visualPosition;
  Quat d_orientation, d_lastOrientation, d_orientationChange;
  Vec3 d_forward, d_right, d_up, d_localUp, d_forward2D, d_angularVelocity, d_angularAcceleration, d_velocity,
      d_localVelocity, d_velocity2D;
  float d_speed = 0, d_speedRatio = 0, d_speedRatioSquared = 0, d_speed2D = 0;
  Vec3 d_direction, d_localDirection, d_direction2D, d_localDirection2D;
};

// What the script reads from the vehicle each frame. origin is the entity's world position and centerOfMass is
// s_centerOfMass (local). The script's reset flag is never read by UpdateDynamic (its use is commented out).
struct StatsInput {
  Quat orientation;
  Vec3 origin;
  Vec3 centerOfMass;
  Vec3 worldAngularVelocity;
  Vec3 velocity;
};

// The angular acceleration is the per-frame difference of the local angular velocity (not divided by dt), and
// d_velocity2D is Normalize2D(velocity) times the horizontal speed, so its Z is the velocity's Z times that speed
// over the horizontal length, as in the script.
// The position stands for IPositionProvider.CreateEntityPositionProvider(vehicle, s_centerOfMass).CalculatePosition
// and is ASSUMED to be the entity's position plus the orientation applied to the centre of mass; it is not
// compared with the game yet, so the binding step must check it in game.
void UpdateDynamic(StatsState &s, const StatsInput &in);

} // namespace Flight
