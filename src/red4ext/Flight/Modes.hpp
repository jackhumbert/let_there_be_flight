#pragma once

// FlightModeStandard.UpdateWithNormalLift / UpdateWithNormalDistance and FlightModeDrone.Update as pure code, for the
// native mode controllers. The script is the truth: the products keep its operand order and the same float32 steps.
//
// FlightModeStandard has no Update of its own: its subclasses (Fly, Hover, HoverFly, Automatic, Autopilot) choose the
// normal and the lift force and call UpdateWithNormalLift or UpdateWithNormalDistance, so those two are all that is
// ported here. The anti-gravity drone adds d_localUp * 9.81000042 * gravityFactor to the drone's force in its own
// Update; that stays in its script.

#include "Flight/Controls.hpp"
#include "Flight/Pid.hpp"
#include "Flight/ScriptMath.hpp"
#include "Flight/Stats.hpp"

namespace Flight {

// FlightUtils.Up, Forward and Right.
constexpr Vec3 kUp{0.0f, 0.0f, 1.0f};
constexpr Vec3 kForward{0.0f, 1.0f, 0.0f};
constexpr Vec3 kRight{1.0f, 0.0f, 0.0f};

// The literal the mode scripts add for gravity.
constexpr float kGravity = 9.81000042f;

// A mode's force and torque, in the car frame.
struct ModeOutput {
  Vec3 force, torque;
};

// FlightModeStandard's Mod Settings and the FlightSettings values it reads, at their script defaults.
// rollWithYaw is generalRollWithYaw, brakeFactorLinear is FlightSettings.brakeFactorLinear, pitchWithLift, pitchWithSurge,
// hoverClamp are FlightSettings floats and gravityFactor is FlightMode's. hoverFactor, minHoverHeight and
// maxHoverHeight are carried for the subclasses' choice of lift force and are not read here.
struct StandardParams {
  float surgeFactor = 15, yawFactor = 2.5f, swayFactor = 5, pitchFactor = 3, liftFactor = 25, hoverFactor = 10,
        pitchInputAngle = 45, rollFactor = 3, rollInputAngle = 45;
  float rollWithYaw = 0.15f, pitchWithLift = 0, pitchWithSurge = 0, brakeFactorLinear = 1;
  float hoverClamp = 10, minHoverHeight = 1, maxHoverHeight = 7, gravityFactor = 1;
};

// UpdateWithNormalLift. A PID is stepped only while its offset is under 120 degrees; otherwise its correction is 0 and
// its state is left alone.
ModeOutput StandardLift(const StatsState &s, const StatsStatic &st, const Controls &c, const StandardParams &p,
                        Pid &pitchPid, Pid &rollPid, Vec3 normal, float liftForce, float dt);

// UpdateWithNormalDistance: the hover PID's clamped correction times liftFactor, plus gravity, as the lift force.
ModeOutput StandardDistance(const StatsState &s, const StatsStatic &st, const Controls &c, const StandardParams &p,
                            Pid &hoverPid, Pid &pitchPid, Pid &rollPid, Vec3 normal, float heightDifference, float dt);

// FlightModeDrone's Mod Settings and FlightSettings.brakeFactorLinear / brakeFactorAngular.
struct DroneParams {
  float liftFactor = 40, pitchFactor = 5, rollFactor = 8, surgeFactor = 15, yawFactor = 5, swayFactor = 15,
        brakeFactorLinear = 1, brakeFactorAngular = 1;
};

// FlightModeDrone.Update.
ModeOutput Drone(const StatsState &s, const StatsStatic &st, const Controls &c, const DroneParams &p);

} // namespace Flight
