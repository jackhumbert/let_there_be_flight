#pragma once

// The flight modes' maths as pure code, over the ground FindGround left.
//
// - Standard (FlightModeStandard, C++): the mode flight starts in, the Hover & Fly redesign
//   (docs/design/settings-audit.md). Every control asks for a speed; let go, the car comes to rest, level, where it was. Hover and
//   Fly (behind Extra Flight Modes) are Standard with the hover band always on or always off (HoverBand).
// - AttitudeLift: the old standard modes' attitude PIDs and lift (the deleted script
//   FlightModeStandard.UpdateWithNormalLift), kept as the Autopilot's manual flight. The script was the truth: the products keep its
//   operand order and the same float32 steps.
// - Drone (FlightModeDrone.Update; the anti-gravity drone adds d_localUp * 9.81000042 * gravityFactor in its script).

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

// The standard factors, at the defaults the old script FlightModeStandard's Mod Settings had (developer values now):
// Standard reads surge, sway, lift, the input angles, the hover band, brakeFactorLinear and gravityFactor;
// AttitudeLift (the Autopilot) the rest too. rollWithYaw is FlightSettings.generalRollWithYaw, pitchWithLift,
// pitchWithSurge and hoverClamp are FlightSettings floats and gravityFactor is FlightMode's.
struct StandardParams {
  float surgeFactor = 15, yawFactor = 2.5f, swayFactor = 5, pitchFactor = 3, liftFactor = 25, hoverFactor = 10,
        pitchInputAngle = 45, rollFactor = 3, rollInputAngle = 45;
  float rollWithYaw = 0.15f, pitchWithLift = 0, pitchWithSurge = 0, brakeFactorLinear = 1;
  float hoverClamp = 10, minHoverHeight = 1, maxHoverHeight = 7, gravityFactor = 1;
};

// The script's UpdateWithNormalLift, the Autopilot's manual flight. A PID is stepped only while its offset is under 120
// degrees; otherwise its correction is 0 and its state is left alone.
ModeOutput AttitudeLift(const StatsState &s, const StatsStatic &st, const Controls &c, const StandardParams &p,
                        Pid &pitchPid, Pid &rollPid, Vec3 normal, float liftForce, float dt);

// FlightModeDrone's Mod Settings and FlightSettings.brakeFactorLinear / brakeFactorAngular.
struct DroneParams {
  float liftFactor = 40, pitchFactor = 5, rollFactor = 8, surgeFactor = 15, yawFactor = 5, swayFactor = 15,
        brakeFactorLinear = 1, brakeFactorAngular = 1;
};

// FlightModeDrone.Update.
ModeOutput Drone(const StatsState &s, const StatsStatic &st, const Controls &c, const DroneParams &p);

// Drone's skid: just over the ground, a soft spring holds the chassis clearance m off it, so a car let down onto a
// road does not slide on it. Where two road meshes meet flush PhysX still pushes at the joint, and a chassis dragged
// over joints was thrown up 0.1 to 0.3 m each 10 m. It fades in over band m above the
// clearance, only lifts (gravity fed forward, so it holds at the clearance), gives way as lift down is held (the car
// lands), and does nothing past minUp of tilt (body up's world z) or at a clearance of 0.
// turn: the share of the points' moment DroneSkidPoints turns the car with. The springs at full stiffness turn it at
// about 12 rad/s, too fast for the body's 30 ms late answer: a roll wobble grew at 40 km/h in flightsim.
struct DroneSkidParams {
  float clearance = 0.03f, band = 0.15f, p = 60.0f, d = 12.0f, maxAccel = 2.0f * 9.81f, minUp = 0.8f, turn = 0.25f;
  float press = 0.5f * 9.81f; // GroundedLift's
  // the skid is the vehicle's only floor: lift down does not let it go, and near the ground it holds the vehicle upright
  // (DroneSkidPoints). A car's chassis rests on the road when the skid lets go, and its four points hold it level; a
  // bike's chassis does not collide with the road and its two points are on its centreline: let go it sank 0.3 m into
  // the road, and steered on the road it banked 33 degrees, and both bounced it along the road collider's joints.
  bool floor = false;
  float uprightP = 40.0f, uprightD = 8.0f; // 1/s2 per rad of roll, 1/s: the floor's hold upright
  // the ground's grip on a point touching it (DroneSkidPoints, SkidPoint.friction): full at the clearance, none from
  // touch m up (SkidScrapeParams.touch, where its scrape is heard); under softSlide m/s it fades with the slide, so a
  // car at rest is not pushed back and forth
  float touch = 0.06f, softSlide = 0.5f;
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
  // Coulomb coefficient of the ground against it sliding (Ground Friction times the ground's material; 0: none)
  float friction = 0.0f;
};

// Lift down near the ground: Drone pushes the car down at its lift factor times the input (40 m/s2 at full), and on the
// road that push only pressed it harder into it; friction grew with it and a car set down at 10 m/s stopped dead within
// a second. Within the skid's band the push fades to press (m/s2), so a car set down slides along. aLiftAccel is the
// lift's push (m/s2 along the body's up, - down), aClearance the skid's, aBodyUpZ the body's up's world z; returns the
// push kept. Past the skid's tilt (minUp) it is the push as given: upside down, lift down pushes the car off the
// ground, and held to press (half of gravity) it could not lift off.
float GroundedLift(float aLiftAccel, float aClearance, float aBodyUpZ, const DroneSkidParams &k);

// The skid at each of aCount points (the thrusters, where FindGround's rays start) instead of one through the centre of
// mass: each carries 1 / aCount of the car and pushes up where it is by DroneSkid of its own clearance and its own
// vertical speed (the body's plus the spin's there). On level ground at an even clearance it is DroneSkid exactly, no
// torque; a corner nearer the ground pushes harder and turns the car off it, as springs under each corner would.
// A point touching the ground also drags on it: its friction times the load it carries, against its slide along the
// ground, where it is (so a corner dragging turns the car too). Not a bike's (k.floor): its points are its tyres.
// The force is a body-frame acceleration, the torque mode torque (the angular acceleration over TorqueResponse's
// aResponse, axis by axis).
// aLoads, when given, gets each point's load: its push up as a share of the car's weight (a quarter each at rest on
// four).
ModeOutput DroneSkidPoints(const SkidPoint *aPoints, int aCount, const StatsState &s, const StatsStatic &st,
                           Vec3 aResponse, float aLift, const DroneSkidParams &k, float *aLoads = nullptr);

// FlightComponent.FindGround as the mode sees it. distance is FlightComponent.distance after the call, the last hit's
// when nothing was found. normal is zero when nothing was found (the script's unset out parameter).
struct Ground {
  bool found = false;
  float distance = 0;
  Vec3 normal;
};

// One ground ray of FlightComponent.FindGround: valid is TraceResult.IsValid, tire is the ray's origin (the wheel
// emitter's position), position and normal are the hit's, material its physics material (the CName's hash; 0 unknown).
struct Hit {
  bool valid = false;
  Vec3 tire, position, normal;
  uint64_t material = 0;
};

// The averaging at the end of FlightComponent.FindGround, over its ray hits (the script casts four, one per wheel
// emitter; the raycasts stay in script). The ground is found only when every hit is valid. Then the distance is the mean
// of the emitters' heights over their hits (below 0 where the ground is above an emitter) and the normal is the
// normalised mean of the normalised hit normals. Otherwise found is
// false, the distance stays lastDistance (FlightComponent.distance is not written) and the normal is zero. No hits
// counts as nothing found (the script always has four).
Ground GroundFromHits(const Hit *hits, int count, float lastDistance);

// Where one of FindGround's rays starts: kGroundRayRise straight up from its wheel emitter aTire. A bike's emitters sit
// at the bottom of its tyres, 0.14 m under the road collider's surface as it sits, and rays cast from there found no
// ground: the Drone skid did not run and lift held down pinned the bike to the road. Started higher they would hit the
// bike itself, so the native rays see only the static world (FlightGround.cpp).
inline constexpr float kGroundRayRise = 0.5f; // m
Vec3 GroundRayStart(Vec3 aTire);
// Where one of FindGround's rays ends: straight down from its wheel emitter aTire, maxHoverHeight + 10 m.
Vec3 GroundRayEnd(Vec3 aTire, float aMaxHoverHeight);

// Standard's fields; hoverHeight is FlightComponent.hoverHeight.
struct StandardState {
  float hovering = 1, referenceZ = 0, hoverHeight = 0;
  bool holdingAltitude = false;
  float outOfBand = 0; // s the ground has been out of the hover band
};

// Standard (Hover & Fly redesigned for beginners, docs/design/hover-fly-feel.md): every control asks for a speed, and
// letting go of everything brings the car to rest, level, where it was let go.
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
struct StandardGains {
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

// How Standard follows the ground. Blend (Standard): by how far into the hover band [minHoverHeight, maxHoverHeight]
// the car is, 1 at the bottom, 0 at the top and without ground. Always (Hover): wherever the rays find ground, at any
// height, so a car above the band sinks into it. Never (Fly): the ground is ignored and the height is held.
enum class HoverBand { Blend, Always, Never };

// Standard. hovering is the hover weight, hoverHeight the height over the ground it holds in the band (lift moves
// it), referenceZ the height held out of it, captured once the car has stopped climbing or sinking. The hover height
// is taken from the ground again only when the band is entered after kRegainHoverHeight s out of it. response is
// TorqueResponse's.
constexpr float kRegainHoverHeight = 0.5f;
ModeOutput Standard(StandardState &m, const StatsState &s, const StatsStatic &st, const Controls &c,
                    const StandardParams &p, const StandardGains &a, const Ground &g, Vec3 response, float dt,
                    HoverBand band = HoverBand::Blend);

// Drone mode made car-independent, a proposal (docs/design/hover-fly-feel.md, "Drone across the fleet"); not called
// by the game. Drone's torque is input x factor, so a held key spins a car at a rate that grows with its own response
// (TorqueResponse spans about 30 times over the fleet) until PhysX's 7 rad/s cap. Here each of pitch, roll and yaw asks
// for a body rate instead (input x maxRate, the angular brake slowing it to 0), and the torque is the angular
// acceleration that closes the gap divided by the car's response, as Standard does. Let go, every rate goes to
// 0. The force is Drone's: it is an acceleration already (the force tail scales it by mass).
struct DroneAssistParams {
  Vec3 maxRate{3.0f, 3.0f, 2.5f}; // rad/s at full input: pitch (X), roll (Y), yaw (Z)
  float rateP = 10;               // rad/s2 per rad/s off the rate asked for
};

// DroneAssist: Drone's force, the rate command's torque. response is TorqueResponse's.
ModeOutput DroneAssist(const StatsState &s, const StatsStatic &st, const Controls &c, const DroneParams &p,
                       const DroneAssistParams &a, Vec3 response);

} // namespace Flight
