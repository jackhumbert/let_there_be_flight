#pragma once

// FlightModeStandard.UpdateWithNormalLift / UpdateWithNormalDistance and FlightModeDrone.Update as pure code, for the
// native mode controllers. The script is the truth: the products keep its operand order and the same float32 steps.
//
// FlightModeStandard has no Update of its own: its subclasses (Fly, Hover, HoverFly, Automatic, Autopilot) choose the
// normal and the lift force and call UpdateWithNormalLift or UpdateWithNormalDistance, so those two are all that is
// ported here. The anti-gravity drone adds d_localUp * 9.81000042 * gravityFactor to the drone's force in its own
// Update; that stays in its script.
//
// The modes: Fly (normal Up, a lift force from the lift input), Hover (the ground normal and the hover PID's height
// difference), HoverFly (Fly and Hover blended by how far into the hover band the car is), Automatic (the same blend
// with its own gains plus the aero and braking terms), Drone (its own Update) and Autopilot (its script). Each one
// here is a pure function over the ground the script's FindGround left.

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
// hoverClamp are FlightSettings floats and gravityFactor is FlightMode's. StandardLift and StandardDistance do not read
// hoverFactor, minHoverHeight and maxHoverHeight; the subclass functions (HoverWeight, HoverFly, Automatic, Hover) do.
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

// FlightComponent.FindGround as the mode sees it. distance is FlightComponent.distance after the call, the last hit's
// when nothing was found. normal is zero when nothing was found (the script's unset out parameter).
struct Ground {
  bool found = false;
  float distance = 0;
  Vec3 normal;
};

// One ground ray of FlightComponent.FindGround: valid is TraceResult.IsValid, tire is the ray's origin (the wheel
// emitter's position), position and normal are the hit's.
struct Hit {
  bool valid = false;
  Vec3 tire, position, normal;
};

// The averaging at the end of FlightComponent.FindGround, over its ray hits (the script casts four, one per wheel
// emitter; the raycasts stay in script). The ground is found only when every hit is valid. Then the distance is the mean
// of Distance(tire, position) and the normal is the normalised mean of the normalised hit normals. Otherwise found is
// false, the distance stays lastDistance (FlightComponent.distance is not written) and the normal is zero. No hits
// counts as nothing found (the script always has four).
Ground GroundFromHits(const Hit *hits, int count, float lastDistance);

// FlightModeFly.Update: normal Up; the lift input scales the lift force by dt, as the script does.
ModeOutput Fly(const StatsState &s, const StatsStatic &st, const Controls &c, const StandardParams &p, Pid &pitchPid,
               Pid &rollPid, float dt);

// FlightModeHover.Update: hoverHeight (FlightComponent.hoverHeight) is raised to minHoverHeight; with ground the normal
// is the ground's and the height difference is hoverHeight - distance, without it the normal is Up and the difference 0.
// The hover PID is stepped either way.
ModeOutput Hover(float &hoverHeight, const StatsState &s, const StatsStatic &st, const Controls &c,
                 const StandardParams &p, const Ground &g, Pid &hoverPid, Pid &pitchPid, Pid &rollPid, float dt);

// FlightModeHoverFly's Mod Settings.
struct HoverFlyParams {
  float altitudeHoldFactor = 1, heightDampening = 1, heightCorrectionFactor = 0.5f;
};

// FlightModeHoverFly's fields; hoverHeight is FlightComponent.hoverHeight.
struct HoverFlyState {
  float hovering = 1, referenceZ = 0, hoverHeight = 0;
  bool holdingAltitude = false;
};

// FlightModeHoverFly.Update. The hover PID is stepped only while hovering is above 0.
ModeOutput HoverFly(HoverFlyState &m, const StatsState &s, const StatsStatic &st, const Controls &c,
                    const StandardParams &p, const HoverFlyParams &h, const Ground &g, Pid &hoverPid, Pid &pitchPid,
                    Pid &rollPid, float dt);

// FlightModeAutomatic's Mod Settings. Its lift and hover factors are its own, not Standard's.
struct AutomaticParams {
  float autoBrakingFactor = 3, yawDirectionality = 300, liftFactor = 25, hoverFactor = 40;
};

// FlightModeAutomatic's fields; hoverHeight is FlightComponent.hoverHeight.
struct AutomaticState {
  float hovering = 1, hoverHeight = 0;
};

// FlightModeAutomatic.Update. The hover PID is stepped every frame, with no ground too, on the last distance.
ModeOutput Automatic(AutomaticState &m, const StatsState &s, const StatsStatic &st, const Controls &c,
                     const StandardParams &p, const AutomaticParams &a, const Ground &g, Pid &hoverPid, Pid &pitchPid,
                     Pid &rollPid, float dt);

} // namespace Flight
