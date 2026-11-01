#pragma once

// IFlightThruster.Update's maths (Thruster/IFlightThruster.reds), the bike overrides of GetPitch and GetYaw
// (Thruster/BikeThrusters.reds) and the four UpdateGetDisplacement bodies (FlightThrusterFX.reds) as pure code.
// The script is the truth: the products keep its operand order and the same float32 steps.
//
// The quaternion steps are not here: MakeEulerAngles, EulerAngles.ToQuat and Quaternion.Slerp are the game's
// natives, and the binding calls them. These functions give the angles to pass them (ThrusterEuler), the scale and
// bone deviation to store, and the displacement each effect reports.

#include "Flight/ScriptMath.hpp"

#include <cstdint>

namespace Flight {

// Which GetPitch / GetYaw the thruster uses: IFlightThruster's own (Car), FlightThrusterFront's overrides
// (BikeFront) or FlightThrusterBack's (BikeBack).
enum class ThrusterShape : int32_t { Car = 0, BikeFront = 1, BikeBack = 2 };

// IFlightThrusterFX, RegularFlightThrusterFX, MainFlightThrusterFX and SideFlightThrusterFX.
enum class ThrusterFxKind : int32_t { Base = 0, Regular = 1, Main = 2, Side = 3 };

// The thruster fields the maths reads, at IFlightThruster's initialisers. mainThrusterYawFactor is set by OnSetup
// (30 without a retro thruster, 5 with), so the caller passes the value in use.
struct ThrusterParams {
  ThrusterShape shape = ThrusterShape::Car;
  bool isFront = false, isRight = false;
  float maxThrusterAnglePitch = 90.0f, maxThrusterAngleOutside = 60.0f, maxThrusterAngleInside = 15.0f,
        thrusterAngleAllowance = 15.0f, mainThrusterYawFactor = 0.5f, forceThreshold = 10.0f,
        torqueThreshold = 1.0f, boneLerpAmount = 0.25f;
};

// Degrees, in the order the script passes them to MakeEulerAngles(GetPitch(), GetRoll(), GetYaw()).
struct ThrusterAngles {
  float pitch = 0, roll = 0, yaw = 0;
};

// The literals in Update.
constexpr float kThrusterScaleLerp = 0.1f;
constexpr float kThrusterOrientationLerp = 0.1f;
constexpr float kAnimDeviationCenter = 0.0f;
constexpr float kAnimDeviationScale = 0.025f;

// Update's force and torque: normalised above the threshold, divided by it below.
Vec3 ThrusterNormalize(const Vec3 &v, float threshold);

// GetPitch with the thruster's force f and torque t (already through ThrusterNormalize).
float ThrusterPitch(const ThrusterParams &p, const Vec3 &f, const Vec3 &t);

// GetRoll, the same for every shape.
float ThrusterRoll(const ThrusterParams &p);

// GetYaw; active is FlightComponent.active.
float ThrusterYaw(const ThrusterParams &p, const Vec3 &f, const Vec3 &t, bool active);

// GetEulerAngles.
ThrusterAngles ThrusterEuler(const ThrusterParams &p, const Vec3 &f, const Vec3 &t, bool active);

// The mesh's next visualScale: Vector4.Interpolate toward one when active and zero when not.
Vec3 ThrusterScale(const Vec3 &current, bool active);

// One effect's UpdateGetDisplacement. up is Quaternion.GetUp of the mesh's local orientation. The return value is
// what Update adds to its amount; aBlackboard is what the effect writes to thruster_amount.
float ThrusterFxDisplacement(ThrusterFxKind k, const Vec3 &up, const Vec3 &f, const Vec3 &t, bool isFront,
                             bool isRight, float &aBlackboard);

// The next animDeviation. amount is Dot(up, f) plus every effect's displacement.
float ThrusterDeviation(const ThrusterParams &p, float deviation, float amount);

} // namespace Flight
