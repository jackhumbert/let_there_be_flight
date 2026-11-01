#pragma once

// The Autopilot's aerobatics (src/redscript/Flight/FlightAerobatics.reds): FlightAttitude.RotationError and
// Across, and FlightManeuver's Create, IsOverlay, IsDone, Ease, RollUp, BarrelOffset and Sample, as pure
// functions over the script's own vector maths (Flight/ScriptMath.hpp), with the script's literals and operand
// order. PointClear and Fits query the nav map and ChooseManeuver picks a shape, so they stay in the script.

#include "Flight/ScriptMath.hpp"

#include <cstdint>

namespace Navigation {

// FlightManeuverKind, same values
enum class ManeuverKind : int32_t { None = 0, AileronRoll = 1, BarrelRoll = 2, Loop = 3, Immelmann = 4, SplitS = 5 };

// FlightManeuver's fields of the same name
struct Maneuver {
  ManeuverKind kind = ManeuverKind::None;
  float side = 0.0f, time = 0.0f, duration = 0.0f;
  Flight::Vec3 origin{}, dir{}, right{};
  float speed = 0.0f, radius = 0.0f, rollTime = 0.0f;
};

// FlightAttitude.RotationError: the rotation vector (axis * angle) that turns the frame (r, f, u) into
// (tr, tf, tu), in the first frame's coordinates; zero when they are the same.
Flight::Vec3 RotationError(const Flight::Vec3 &r, const Flight::Vec3 &f, const Flight::Vec3 &u,
                           const Flight::Vec3 &tr, const Flight::Vec3 &tf, const Flight::Vec3 &tu);

// FlightAttitude.Across: u made perpendicular to the unit vector f (not normalised).
Flight::Vec3 Across(const Flight::Vec3 &u, const Flight::Vec3 &f);

// FlightManeuver.Create. dir keeps the velocity's Z, as the game's Normalize2D does.
Maneuver CreateManeuver(ManeuverKind aKind, float aSide, const Flight::Vec3 &aPosition, const Flight::Vec3 &aVelocity,
                        float aAccel, float aRollTime);

bool IsOverlay(const Maneuver &aManeuver);
bool IsDone(const Maneuver &aManeuver, float aTime);

// FlightManeuver.Ease: 0 -> 1 eased in and out over aD; aRate is its rate.
float ManeuverEase(float aT, float aD, float &aRate);

// FlightManeuver.RollUp: the up vector turned about the nose by the aileron roll at time t.
Flight::Vec3 RollUp(const Maneuver &aManeuver, float aT, const Flight::Vec3 &aF, const Flight::Vec3 &aU);

// FlightManeuver.BarrelOffset: offset from the route, its rate of change and the up vector at time t.
void BarrelOffset(const Maneuver &aManeuver, float aT, Flight::Vec3 &aOffset, Flight::Vec3 &aOffsetVelocity,
                  Flight::Vec3 &aUp);

// FlightManeuver.Sample: position, velocity, acceleration and attitude of a pitch manoeuvre at time t.
void SampleManeuver(const Maneuver &aManeuver, float aT, Flight::Vec3 &aPos, Flight::Vec3 &aVel, Flight::Vec3 &aAcc,
                    Flight::Vec3 &aFwd, Flight::Vec3 &aUp);

} // namespace Navigation
