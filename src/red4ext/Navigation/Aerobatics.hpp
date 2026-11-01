#pragma once

// The Autopilot's aerobatics (src/redscript/Flight/FlightAerobatics.reds): FlightAttitude.RotationError and
// Across, and FlightManeuver's Create, IsOverlay, IsDone, Ease, RollUp, BarrelOffset and Sample, as pure
// functions over the script's own vector maths (Flight/ScriptMath.hpp), with the script's literals and operand
// order. PointClear and Fits query the nav map and ChooseManeuver picks a shape, so they stay in the script.
// FlightModeAutopilot's bank and pitch eases, Unsteep, SyncPitch, SyncBank and the tail of FlyAerobatic (the attitude
// torque and the thrust clamps) are here too; the surge hold, the commands for audio and the logs stay in the script.

#include "Flight/ScriptMath.hpp"
#include "Flight/Stats.hpp"

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

// FlightModeAutopilot's raceRoll* and racePitch* fields, with their defaults.
struct EaseParams {
  float raceRollFrequency = 2.5f, raceRollRateMax = 1.0f, raceRollAccelMax = 3.0f;
  float racePitchFrequency = 2.5f, racePitchRateMax = 1.0f, racePitchAccelMax = 3.0f;
};

// FlightModeAutopilot's bankAngle, bankRateNow, pitchAngle and pitchRateNow.
struct EaseState {
  float bankAngle = 0.0f, bankRateNow = 0.0f, pitchAngle = 0.0f, pitchRateNow = 0.0f;
};

// The rows m.X, m.Y, m.Z of FlightComponent.worldInertia, and whether it is valid. Valid stands for FlightComponent's
// physxOwned && worldInertiaValid: AngularImpulseFor returns zeros otherwise (Flight/Component.hpp), and the caller
// must pass exactly that.
struct WorldInertia {
  bool valid = false;
  Flight::Vec3 x{}, y{}, z{};
};

// FlightModeAutopilot's attitude fields, FlightSettings' give-back factors, FlightMode's gravityFactor and the
// Standard mode's input factors, with their defaults.
struct AttitudeControlParams {
  float attitudeFrequency = 5.0f, attitudeDamping = 0.8f, maxAngularAccel = 20.0f;
  float torqueGainPitch = 2.0f, torqueGainRoll = 0.5f, torqueGainYaw = 0.9f;
  float generalDampFactorAngular = 2.0f, generalYawDampFactor = 1.0f, generalDampFactorAngularMax = 3.5f;
  float standardModeSurgeFactor = 15.0f, standardModeSwayFactor = 5.0f, standardModeLiftFactor = 25.0f;
  float gravityFactor = 1.0f;
};

// The fields FlyAerobatic keeps between frames: the last target frame, the smoothed rate of the target and the bank.
struct AttitudeControlState {
  Flight::Vec3 targetRight{}, targetForward{}, targetUp{}, omegaRef{};
  float bankAngle = 0.0f, bankRateNow = 0.0f;
};

// What the tail reads: the wanted nose and top, whether a shape set them (SyncBank), the commanded acceleration, the
// surge hold's drag, DragAccel(speed) and RaceDecel(speed), the thruster tensor and the world inertia.
struct AttitudeControlInput {
  float dt = 0.001f;
  Flight::Vec3 fwd{}, up{};
  bool shape = false;
  Flight::Vec3 accel{}, drag{};
  float dragAccel = 0.0f, raceDecel = 0.0f;
  Flight::Vec3 thrusterTensor{1.0f, 1.0f, 1.0f};
  WorldInertia inertia;
};

// What it writes: this.torque, this.thrustCmd (and force), this.attitudeError, and rateRef for the caller.
struct AttitudeControlOutput {
  Flight::Vec3 torque{}, thrust{}, rateRef{};
  float attitudeError = 0.0f;
};

// FlightModeAutopilot.BankFrame: the level direction and the side the bank is measured from; false with the nose too
// near vertical to tell.
bool BankFrame(const Flight::Vec3 &aFwd, Flight::Vec3 &aLevel, Flight::Vec3 &aSide);

// FlightModeAutopilot.Unsteep: a steep path made nearer level, towards the heading.
Flight::Vec3 Unsteep(const Flight::Vec3 &aFwd, const Flight::Vec3 &aHeadingRef);

// FlightModeAutopilot.EaseBank: the wanted up vector with its bank eased (rate and acceleration limited).
Flight::Vec3 EaseBank(EaseState &aState, const EaseParams &aParams, float aDt, const Flight::Vec3 &aFwd,
                      const Flight::Vec3 &aUp);

// FlightModeAutopilot.EasePitch. The level direction keeps the nose's Z (the game's Normalize2D does). aLineTangent is
// what LineFrameOver gives, read when aOnLine.
Flight::Vec3 EasePitch(EaseState &aState, const EaseParams &aParams, float aDt, const Flight::Vec3 &aFwd, bool aOnLine,
                       const Flight::Vec3 &aLineTangent, const Flight::Vec3 &aHeadingRef);

void SyncPitch(EaseState &aState, const Flight::Vec3 &aFwd);
void SyncBank(float &aBankAngle, float &aBankRateNow, const Flight::Vec3 &aFwd, const Flight::Vec3 &aUp);

// FlightComponent.AngularImpulseFor: the world inertia times aA, zero when it is not valid (aValid says which).
Flight::Vec3 AngularImpulse(const WorldInertia &aInertia, const Flight::Vec3 &aA, bool &aValid);

// FlyAerobatic from `let right = ...` to the three thrust clamps: the torque for the attitude wanted, and the thrust
// in the car's frame. Updates the target frame, the smoothed rate and, for a shape, the bank.
AttitudeControlOutput AttitudeControl(AttitudeControlState &aState, const AttitudeControlParams &aParams,
                                      const Flight::StatsStatic &aStatic, const Flight::StatsState &aStats,
                                      const AttitudeControlInput &aInput);

} // namespace Navigation
