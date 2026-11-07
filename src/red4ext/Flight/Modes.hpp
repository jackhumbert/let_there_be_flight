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

// Drone's skid: just over the ground, a soft spring holds the chassis clearance m off it, so a car let down onto a
// road does not slide on it. Where two road meshes meet flush PhysX still pushes at the joint, and a chassis dragged
// over joints was thrown up 0.1 to 0.3 m each 10 m (docs/history.md, 2026-11-06). It fades in over band m above the
// clearance, only lifts (gravity fed forward, so it holds at the clearance), gives way as lift down is held (the car
// lands), and does nothing past minUp of tilt (body up's world z) or at a clearance of 0.
// turn: the share of the points' moment DroneSkidPoints turns the car with. The springs at full stiffness turn it at
// about 12 rad/s, too fast for the body's 30 ms late answer: a roll wobble grew at 40 km/h in flightsim.
struct DroneSkidParams {
  float clearance = 0.03f, band = 0.15f, p = 60.0f, d = 12.0f, maxAccel = 2.0f * 9.81f, minUp = 0.8f, turn = 0.25f;
};
// The world-up acceleration (m/s2) to add: aClearance is the chassis's lowest point over the ground (m), aVerticalSpeed
// how fast it closes on the ground (ClosingSpeed; m/s, + away), aBodyUpZ its up's world z, aLift the lift control
// (-1..1).
float DroneSkid(float aClearance, float aVerticalSpeed, float aBodyUpZ, float aLift, const DroneSkidParams &k);

// A point's vertical speed off the ground under it (m/s, + away): its velocity along the ground's normal, as height.
// The world's vertical speed alone made the skid sag 0.2 s x the climb rate up a slope (7 cm at 40 km/h on 3.4 %,
// more than its clearance: the car dragged its thruster spheres up road_grade in flightsim).
float ClosingSpeed(Vec3 aVelocity, Vec3 aGroundNormal);

// One point the skid holds off the ground: where it is from the centre of mass (body frame, m) and its clearance there
// (m; the ground under it less its height over the car's lowest point).
struct SkidPoint {
  Vec3 offset;
  float clearance = 0.0f;
  Vec3 normal{0.0f, 0.0f, 1.0f}; // the ground's under it
};

// The skid at each of aCount points (the thrusters, where FindGround's rays start) instead of one through the centre of
// mass: each carries 1 / aCount of the car and pushes up where it is by DroneSkid of its own clearance and its own
// vertical speed (the body's plus the spin's there). On level ground at an even clearance it is DroneSkid exactly, no
// torque; a corner nearer the ground pushes harder and turns the car off it, as springs under each corner would.
// The force is a body-frame acceleration, the torque mode torque (the angular acceleration over TorqueResponse's
// aResponse, axis by axis).
ModeOutput DroneSkidPoints(const SkidPoint *aPoints, int aCount, const StatsState &s, const StatsStatic &st,
                           Vec3 aResponse, float aLift, const DroneSkidParams &k);

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

// Where one of FindGround's rays ends: straight down from its wheel emitter aTire, maxHoverHeight + 10 m.
Vec3 GroundRayEnd(Vec3 aTire, float aMaxHoverHeight);

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
  float outOfBand = 0; // s the ground has been out of the hover band (HoverFlyAssist)
};

// FlightModeHoverFly.Update. The hover PID is stepped only while hovering is above 0.
ModeOutput HoverFly(HoverFlyState &m, const StatsState &s, const StatsStatic &st, const Controls &c,
                    const StandardParams &p, const HoverFlyParams &h, const Ground &g, Pid &hoverPid, Pid &pitchPid,
                    Pid &rollPid, float dt);

// Hover & Fly redesigned for beginners (docs/design/hover-fly-feel.md): every control asks for a speed, and letting
// go of everything brings the car to rest, level, where it was let go. Not called by the game yet: flown in
// tools/flightsim against the spec first.
// - attitude: PD on the rotation from the body's up to the target up (world up blended to the ground normal by
//   hovering, tilted by the pitch and roll inputs), so it holds at any tilt, upside down too, and has no integral to
//   wind up while a collision's recovery has the torque scaled down;
// - yaw: a turn rate about world up (yaw x maxYawRate), not a torque about the body's own up; at speed no faster
//   than the sideways grip can turn the velocity with it;
// - travel: surge and sway accelerate along the level forward and right; let go, the speed along each brakes to 0,
//   the sideways part harder, so the velocity follows the nose through a turn;
// - height: lift asks for a vertical speed (lift x maxClimbRate); let go, the car stops and holds that height. In the
//   hover band the hover height moves at the same rate, and the height error is damped on the vertical speed, not
//   on the change of the ground distance (a 1.5 m step under the car kicked the PID's D term at 92 m/s2).
//
// The attitude and yaw terms ask for an angular acceleration and divide it by the car's own response
// (TorqueResponse): over the surveyed cars that response spans about 30 times on each axis, so no fixed torque gain
// suits them all.
struct HoverFlyAssistParams {
  float attitudeP = 25, attitudeD = 8;   // rad/s2 per rad of tilt, per rad/s of tilting
  float attitudeLimit = 1.2f;            // rad of tilt error the P term counts at most
  float maxYawRate = 1.57f, yawP = 8;    // rad/s at full yaw input; rad/s2 per rad/s off it
  float brake = 1.5f, sideBrake = 3;    // 1/s: the share of the speed along forward, and across, braked a second
  float maxBrake = 12, maxSideGrip = 25; // m/s2 the braking gives at most, along and across (the grip in a turn)
  float maxClimbRate = 8, climbP = 5;    // m/s at full lift input; m/s2 per m/s off it
  float holdP = 4, holdD = 4;            // the height hold: m/s2 per m off, per m/s of vertical speed
  float hoverP = 6, hoverD = 5;          // the hover band: m/s2 per m off the hover height, per m/s
};

// The body's rad/s2 of angular acceleration per unit of mode torque x mass x thruster tensor / inertia (FlightComponent
// scales the mode torque by mass and tensor, PhysX divides by the inertia), per body axis: X pitch, Y roll, Z yaw.
// Yaw 0.95 is TorqueProbe's on an Archer Quartz (2026-10-26) and agrees with a Shion MZ1 recording (2.57 rad/s2 per
// unit measured, 2.65 from the Kyokotsu's survey numbers). Pitch 1.13 is fitted to that recording (TorqueProbe's 2.6
// was the first frames only). Roll 0.24 is fitted to the C++ Hover & Fly's roll holds in game on a Shion MZ1, a
// Bratsk and a Kusanagi (2026-11-05: they overshot 45 deg to 49 to 54 deg, which the simulator gives at 0.4 to 0.55 of
// TorqueProbe's 0.52).
constexpr Vec3 kEngineTorqueGain{1.13f, 0.24f, 0.95f};

// A car's rad/s2 per unit of mode torque on each body axis: kEngineTorqueGain x mass x tensor / inertia, with the
// tensor's Y kept at 1 or more as the force tail keeps it. Zero on an axis with no inertia.
Vec3 TorqueResponse(const StatsStatic &st, Vec3 thrusterTensor);

// The redesigned Hover & Fly. m's fields mean what they do for HoverFly (referenceZ is the held height, captured
// once the car has stopped climbing or sinking), except that the hover height is taken from the ground again only
// when the band is entered after kRegainHoverHeight s out of it. response is TorqueResponse's.
constexpr float kRegainHoverHeight = 0.5f;
ModeOutput HoverFlyAssist(HoverFlyState &m, const StatsState &s, const StatsStatic &st, const Controls &c,
                          const StandardParams &p, const HoverFlyAssistParams &a, const Ground &g, Vec3 response,
                          float dt);

// Drone mode made car-independent, a proposal (docs/design/hover-fly-feel.md, "Drone across the fleet"); not called
// by the game. Drone's torque is input x factor, so a held key spins a car at a rate that grows with its own response
// (TorqueResponse spans about 30 times over the fleet) until PhysX's 7 rad/s cap. Here each of pitch, roll and yaw asks
// for a body rate instead (input x maxRate, the angular brake slowing it to 0), and the torque is the angular
// acceleration that closes the gap divided by the car's response, as HoverFlyAssist does. Let go, every rate goes to
// 0. The force is Drone's: it is an acceleration already (the force tail scales it by mass).
struct DroneAssistParams {
  Vec3 maxRate{3.0f, 3.0f, 2.5f}; // rad/s at full input: pitch (X), roll (Y), yaw (Z)
  float rateP = 10;               // rad/s2 per rad/s off the rate asked for
};

// DroneAssist: Drone's force, the rate command's torque. response is TorqueResponse's.
ModeOutput DroneAssist(const StatsState &s, const StatsStatic &st, const Controls &c, const DroneParams &p,
                       const DroneAssistParams &a, Vec3 response);

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
