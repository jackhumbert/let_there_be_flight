// Modes (src/red4ext/Flight/Modes.hpp): FlightModeStandard.UpdateWithNormalLift / UpdateWithNormalDistance and
// FlightModeDrone.Update. Expected values are worked by hand from the scripts.
#include "check.hpp"

#include "Flight/Modes.hpp"

using namespace Flight;

namespace {
constexpr double kTol = 1e-5;
// where acos or sin enter, or magnitudes reach 100 and more
constexpr double kLoose = 1e-4;

// The car level at identity: the body axes are the world axes.
StatsState Level() {
  StatsState s;
  s.d_forward = {0, 1, 0};
  s.d_right = {1, 0, 0};
  s.d_up = {0, 0, 1};
  s.d_localUp = {0, 0, 1};
  return s;
}

StatsStatic Static() {
  StatsStatic st;
  st.brakingFrictionFactor = 1.0f;
  return st;
}

Pid Gains(float P, float I, float D) {
  Pid p;
  p.gains = {P, I, D};
  return p;
}

void CheckVec(const Vec3 &v, double x, double y, double z, double tol) {
  CHECK_NEAR(v.x, x, tol);
  CHECK_NEAR(v.y, y, tol);
  CHECK_NEAR(v.z, z, tol);
}
} // namespace

TEST(modes_standard_level) {
  Pid pitch = Gains(1, 0, 0), roll = Gains(1, 0, 0);
  ModeOutput o = StandardLift(Level(), Static(), Controls{}, StandardParams{}, pitch, roll, kUp, 9.81f, 0.5f);
  CheckVec(o.force, 0, 0, 9.81, kTol);
  CheckVec(o.torque, 0, 0, 0, kTol);
}

TEST(modes_standard_tilted_normal) {
  // the normal is 30 degrees toward the forward axis: pitchOffNormal = 90 - 60 = 30, error 1/3. With gains
  // {1, 0.2, 0.05} and dt 0.5 the correction is 0.3333 + 0.2 * (1/3 * 0.5) + 0.05 * (1/3 / 0.5) = 0.4, times
  // pitchFactor 3 = 1.2. The roll projection (onto the plane of the forward axis) is along Z, 90 degrees from the
  // right axis, so rollOffNormal is 0.
  Pid pitch = Gains(1, 0.2f, 0.05f), roll = Gains(1, 0.5f, 0);
  ModeOutput o =
      StandardLift(Level(), Static(), Controls{}, StandardParams{}, pitch, roll, {0, 0.5f, 0.8660254f}, 9.81f, 0.5f);
  CHECK_NEAR(o.torque.x, -1.2, kLoose);
  CHECK_NEAR(o.torque.y, 0, kLoose);
  CHECK_NEAR(o.torque.z, 0, kLoose);
}

TEST(modes_standard_inputs) {
  // pitchOffInput 0.5 * 45 = 22.5 and rollOffInput 0.4 * 45 = 18, so the PIDs (P only) see 0.25 and 0.2: times the
  // factor 3 and (1 - angularBrake 0.5), pitch 0.375 and roll 0.3. Yaw: -((0.2 - 18 * 22.5 / 1800) * 2.5) = 0.0625.
  // Force: lift 9.81; surge 1 * 15 forward; sway -1 * 5 right; the damping {2, 4, 0} * 0.5 is subtracted.
  Controls c;
  c.pitch = 0.5f;
  c.roll = 0.4f;
  c.yaw = 0.2f;
  c.surge = 1.0f;
  c.sway = -1.0f;
  c.linearBrake = 0.5f;
  c.angularBrake = 0.5f;
  StatsState s = Level();
  s.d_localVelocity = {2, 4, 0};
  StandardParams p;
  p.rollWithYaw = 0;
  Pid pitch = Gains(1, 0, 0), roll = Gains(1, 0, 0);
  ModeOutput o = StandardLift(s, Static(), c, p, pitch, roll, kUp, 9.81f, 0.5f);
  CheckVec(o.torque, -0.375, 0.3, 0.0625, kTol);
  CheckVec(o.force, -6, 13, 9.81, kTol);
}

TEST(modes_standard_gate) {
  // the normal along the forward axis: pitchOffNormal 90, plus 45 from the input is 135, not under 120. The pitch PID
  // is not stepped (its lastError stays) and its correction is 0. The roll projection is a zero vector, so the angle
  // is 90 and rollOffNormal 0: the roll PID is stepped, its lastError 0.3 becoming the error 0.
  Controls c;
  c.pitch = 1.0f;
  Pid pitch = Gains(1, 0, 0), roll = Gains(1, 0, 0);
  pitch.state.lastError = 0.3f;
  roll.state.lastError = 0.3f;
  ModeOutput o = StandardLift(Level(), Static(), c, StandardParams{}, pitch, roll, {0, 1, 0}, 9.81f, 0.5f);
  CHECK(pitch.state.lastError == 0.3f);
  CHECK(roll.state.lastError == 0.0f);
  CheckVec(o.torque, 0, 0, 0, kTol);
}

TEST(modes_standard_roll_with_yaw) {
  // yaw 1 turns the normal 0.15 rad about the forward axis: n = (sin 0.15, 0, cos 0.15) = (0.149438, 0, 0.988771), its
  // angle from the right axis is 81.405633 degrees, rollOffNormal 8.594367, error 0.0954930, times 3 = 0.286479.
  // The sign of torque.y rests on the RotateAxis direction assumed in ScriptMath (right-handed).
  Controls c;
  c.yaw = 1.0f;
  Pid pitch = Gains(1, 0, 0), roll = Gains(1, 0, 0);
  ModeOutput o = StandardLift(Level(), Static(), c, StandardParams{}, pitch, roll, kUp, 9.81f, 0.5f);
  CHECK_NEAR(o.torque.y, 0.286479, kLoose);
  CHECK_NEAR(o.torque.z, -2.5, kLoose);
  CHECK_NEAR(o.torque.x, 0, kLoose);
}

TEST(modes_standard_pitch_with_lift_and_surge) {
  // the direction is along the forward axis, so Dot(forward, direction) = 1. lift 0.5 * pitchWithLift 0.4 = 0.2 rad and
  // surge 0.5 * pitchWithSurge 0.2 = 0.1 rad, both about the right axis: Up turned 0.3 rad is (0, -sin 0.3, cos 0.3).
  // Its angle from the forward axis is 90 + 17.1887 degrees, so pitchOffNormal = -17.1887, error -0.190986, times 3
  // and the PID gain 1: the pitch torque is +0.572958. The roll projection stays along Z, so the roll torque is 0.
  // The sign rests on the RotateAxis direction assumed in ScriptMath.
  Controls c;
  c.lift = 0.5f;
  c.surge = 0.5f;
  StatsState s = Level();
  s.d_direction = {0, 1, 0};
  StandardParams p;
  p.pitchWithLift = 0.4f;
  p.pitchWithSurge = 0.2f;
  Pid pitch = Gains(1, 0, 0), roll = Gains(1, 0, 0);
  ModeOutput o = StandardLift(s, Static(), c, p, pitch, roll, kUp, 9.81f, 0.5f);
  CHECK_NEAR(o.torque.x, 0.572958, kLoose);
  CHECK_NEAR(o.torque.y, 0, kLoose);
  CHECK_NEAR(o.force.y, 7.5, kTol);

  // a direction across the forward axis zeroes the lift part: only the surge 0.1 rad acts,
  // 0.1 * 57.29578 / 90 * 3 = 0.190986
  s.d_direction = {1, 0, 0};
  Pid pitch2 = Gains(1, 0, 0), roll2 = Gains(1, 0, 0);
  ModeOutput o2 = StandardLift(s, Static(), c, p, pitch2, roll2, kUp, 9.81f, 0.5f);
  CHECK_NEAR(o2.torque.x, 0.190986, kLoose);
}

TEST(modes_standard_distance) {
  // hoverPid {1, 0.005, 0.5}, error 2, dt 0.5: P 2, I 0.005 * (2 * 0.5) = 0.005, D 0.5 * (2 / 0.5) = 2, so 4.005;
  // 4.005 * liftFactor 25 + 9.81 = 109.935
  Pid hover = Gains(1, 0.005f, 0.5f), pitch = Gains(1, 0, 0), roll = Gains(1, 0, 0);
  ModeOutput o = StandardDistance(Level(), Static(), Controls{}, StandardParams{}, hover, pitch, roll, kUp, 2.0f, 0.5f);
  CHECK_NEAR(o.force.z, 109.935, kLoose);

  // error 20 on a fresh PID: 20 + 0.05 + 20 = 40.05, clamped to hoverClamp 10: 10 * 25 + 9.81
  Pid hover2 = Gains(1, 0.005f, 0.5f);
  ModeOutput o2 =
      StandardDistance(Level(), Static(), Controls{}, StandardParams{}, hover2, pitch, roll, kUp, 20.0f, 0.5f);
  CHECK_NEAR(o2.force.z, 259.81, kLoose);

  // gravityFactor scales only the gravity term: error 0 and gravityFactor 2 give 2 * 9.81
  Pid hover3 = Gains(1, 0.005f, 0.5f);
  StandardParams p;
  p.gravityFactor = 2.0f;
  ModeOutput o3 = StandardDistance(Level(), Static(), Controls{}, p, hover3, pitch, roll, kUp, 0.0f, 0.5f);
  CHECK_NEAR(o3.force.z, 2 * 9.81000042, kLoose);
}

TEST(modes_standard_turned_body) {
  Controls c;
  c.surge = 1.0f;
  {
    // nose up 30 degrees about the right axis: forward (0, cos 30, sin 30), and the world up seen from the body is
    // (0, sin 30, cos 30). The world Up is 60 degrees from the forward axis: pitchOffNormal 30, torque.x = -1.2 as in
    // the tilted-normal case. The force is lift along localUp, and surge 15 along the car's own forward axis.
    StatsState s = Level();
    s.d_forward = {0, 0.8660254f, 0.5f};
    s.d_up = {0, -0.5f, 0.8660254f};
    s.d_localUp = {0, 0.5f, 0.8660254f};
    Pid pitch = Gains(1, 0.2f, 0.05f), roll = Gains(1, 0, 0);
    ModeOutput o = StandardLift(s, Static(), c, StandardParams{}, pitch, roll, kUp, 9.81f, 0.5f);
    CHECK_NEAR(o.torque.x, -1.2, kLoose);
    CHECK_NEAR(o.torque.y, 0, kLoose);
    CheckVec(o.force, 0, 9.81 * 0.5 + 15, 9.81 * 0.8660254, kLoose);
  }
  {
    // turned 90 degrees about Z: forward (-1, 0, 0), right (0, 1, 0). A world normal 30 degrees toward the body's
    // forward axis, (-0.5, 0, cos 30), gives pitchOffNormal 30 and rollOffNormal 0: torque (-1.2, 0, 0)
    StatsState s = Level();
    s.d_forward = {-1, 0, 0};
    s.d_right = {0, 1, 0};
    Pid pitch = Gains(1, 0.2f, 0.05f), roll = Gains(1, 0, 0);
    ModeOutput o =
        StandardLift(s, Static(), Controls{}, StandardParams{}, pitch, roll, {-0.5f, 0, 0.8660254f}, 9.81f, 0.5f);
    CHECK_NEAR(o.torque.x, -1.2, kLoose);
    CHECK_NEAR(o.torque.y, 0, kLoose);
  }
  {
    // the same turn, the normal 30 degrees toward the body's right axis, (0, 0.5, cos 30): rollOffNormal 30, the roll
    // PID (P only) gives 30 / 90 * 3 = 1, and the pitch torque is 0
    StatsState s = Level();
    s.d_forward = {-1, 0, 0};
    s.d_right = {0, 1, 0};
    Pid pitch = Gains(1, 0, 0), roll = Gains(1, 0, 0);
    ModeOutput o =
        StandardLift(s, Static(), Controls{}, StandardParams{}, pitch, roll, {0, 0.5f, 0.8660254f}, 9.81f, 0.5f);
    CHECK_NEAR(o.torque.x, 0, kLoose);
    CHECK_NEAR(o.torque.y, 1.0, kLoose);
  }
}

TEST(modes_drone) {
  Controls c;
  c.lift = 0.5f;
  c.surge = 1.0f;
  c.sway = -0.5f;
  c.pitch = 0.2f;
  c.roll = -0.4f;
  c.yaw = 0.6f;
  c.linearBrake = 0.5f;
  c.angularBrake = 0.5f;
  StatsState s = Level();
  s.d_localVelocity = {2, 4, 6};
  s.d_angularVelocity = {1, -2, 3};

  // damping {1, 2, 3} and {0.5, -1, 1.5}; force (-7.5 - 1, 15 - 2, 20 - 3); torque (-(1 + 0.5), -3.2 + 1, -(3 + 1.5))
  ModeOutput o = Drone(s, Static(), c, DroneParams{});
  CheckVec(o.force, -8.5, 13, 17, kTol);
  CheckVec(o.torque, -1.5, -2.2, -4.5, kTol);

  // brakeFactorLinear 2, brakeFactorAngular 0.5, brakingFrictionFactor 3: damping {6, 12, 18} and {0.75, -1.5, 2.25};
  // force (-7.5 - 6, 15 - 12, 20 - 18); torque (-(1 + 0.75), -3.2 + 1.5, -(3 + 2.25))
  DroneParams p;
  p.brakeFactorLinear = 2.0f;
  p.brakeFactorAngular = 0.5f;
  StatsStatic st = Static();
  st.brakingFrictionFactor = 3.0f;
  ModeOutput o2 = Drone(s, st, c, p);
  CheckVec(o2.force, -13.5, 3, 2, kTol);
  CheckVec(o2.torque, -1.75, -1.7, -5.25, kTol);
}

TEST(modes_standard_distinct_factors) {
  // every factor its own value, so a swapped pair moves a result. Level, normal Up (both normal offsets 0):
  // pitchOff = 0.5 * 40 = 20, error 20/90, times pitchFactor 2 = 0.444444, halved by angularBrake 0.5: torque.x
  // -0.222222; rollOff = 0.4 * 50 = 20, times rollFactor 4 = 0.888889, halved: torque.y 0.444444; yaw:
  // -((0.2 - 20 * 20 / 1800) * 3) = 0.0666667. Force: surge 1 * 11 forward, sway -1 * 7 right, less the damping
  // (2, 4, 0) * (0.5 * 2 * 1.5) = (3, 6, 0): (-10, 5, 9.81).
  StandardParams p;
  p.surgeFactor = 11;
  p.swayFactor = 7;
  p.pitchFactor = 2;
  p.rollFactor = 4;
  p.yawFactor = 3;
  p.pitchInputAngle = 40;
  p.rollInputAngle = 50;
  p.rollWithYaw = 0;
  p.brakeFactorLinear = 2;
  StatsStatic st = Static();
  st.brakingFrictionFactor = 1.5f;
  StatsState s = Level();
  s.d_localVelocity = {2, 4, 0};
  Controls c;
  c.pitch = 0.5f;
  c.roll = 0.4f;
  c.yaw = 0.2f;
  c.surge = 1;
  c.sway = -1;
  c.linearBrake = 0.5f;
  c.angularBrake = 0.5f;
  Pid pitch = Gains(1, 0, 0), roll = Gains(1, 0, 0);
  ModeOutput o = StandardLift(s, st, c, p, pitch, roll, kUp, 9.81f, 0.5f);
  CheckVec(o.torque, -0.222222, 0.444444, 0.0666667, kLoose);
  CheckVec(o.force, -10, 5, 9.81, kLoose);
}

TEST(modes_standard_gate_negative) {
  // the normal along -forward: 90 - 180 = -90, plus the input -45 is -135, past -120 too: the pitch PID is not
  // stepped; the roll projection is again a zero vector, so the roll PID is
  Controls c;
  c.pitch = -1.0f;
  Pid pitch = Gains(1, 0, 0), roll = Gains(1, 0, 0);
  pitch.state.lastError = 0.3f;
  roll.state.lastError = 0.3f;
  ModeOutput o = StandardLift(Level(), Static(), c, StandardParams{}, pitch, roll, {0, -1, 0}, 9.81f, 0.5f);
  CHECK(pitch.state.lastError == 0.3f);
  CHECK(roll.state.lastError == 0.0f);
  CheckVec(o.torque, 0, 0, 0, kTol);
}

TEST(modes_standard_hover_clamp) {
  // hoverClamp is the mode's own, not the PIDs' 10: 40.05 clamped to 5 gives 5 * 25 + 9.81
  StandardParams p;
  p.hoverClamp = 5;
  Pid hover = Gains(1, 0.005f, 0.5f), pitch = Gains(1, 0, 0), roll = Gains(1, 0, 0);
  ModeOutput o = StandardDistance(Level(), Static(), Controls{}, p, hover, pitch, roll, kUp, 20.0f, 0.5f);
  CHECK_NEAR(o.force.z, 134.81, kLoose);
}

TEST(modes_drone_distinct_factors) {
  // lift 0.5 * 30, surge 1 * 11, sway -0.5 * 7; pitch 0.2 * 2, roll -0.4 * 4, yaw 0.6 * 6; no brakes
  DroneParams p;
  p.liftFactor = 30;
  p.surgeFactor = 11;
  p.swayFactor = 7;
  p.pitchFactor = 2;
  p.rollFactor = 4;
  p.yawFactor = 6;
  Controls c;
  c.lift = 0.5f;
  c.surge = 1;
  c.sway = -0.5f;
  c.pitch = 0.2f;
  c.roll = -0.4f;
  c.yaw = 0.6f;
  ModeOutput o = Drone(Level(), Static(), c, p);
  CheckVec(o.force, -3.5, 11, 15, kTol);
  CheckVec(o.torque, -0.4, -1.6, -3.6, kTol);
}

namespace {
// The hover tests use the StandardParams defaults unless stated (min 1, max 7, liftFactor 25, hoverFactor 10, hoverClamp 10,
// gravity 1) and PIDs {1, 0, 0} with zero state.
Ground Found(float distance, Vec3 normal) { return Ground{true, distance, normal}; }
Ground NotFound(float distance) { return Ground{false, distance, Vec3{}}; }
} // namespace

TEST(modes_hoverfly_band) {
  // distance 4 in the band 1..7: hovering 1 - 3/6 = 0.5 (was 0.5, no reset); no lift input, hoverHeight stays 5; diff 1,
  // hc 1; fly = 0 - (-2) * 1 = 2; lf = 2 + (1 * 10 - 2) * 0.5 = 6, plus gravity
  HoverFlyState m;
  m.hovering = 0.5f;
  m.hoverHeight = 5;
  StatsState s = Level();
  s.d_velocity.z = -2;
  s.d_position.z = 10;
  Pid hover = Gains(1, 0, 0), pitch = Gains(1, 0, 0), roll = Gains(1, 0, 0);
  ModeOutput o = HoverFly(m, s, Static(), Controls{}, StandardParams{}, HoverFlyParams{}, Found(4, kUp), hover, pitch,
                          roll, 0.5f);
  CHECK_NEAR(m.hovering, 0.5, kTol);
  CHECK_NEAR(m.hoverHeight, 5, kTol);
  CHECK(!m.holdingAltitude);
  CHECK_NEAR(o.force.z, 15.81, kTol);
  CHECK_NEAR(hover.state.lastError, 1, kTol);
}

TEST(modes_hoverfly_enter_band) {
  // from hovering 0 into the band at distance 6: hovering 1 - 5/6; the target jumps to the distance and the PID is reset
  // (integral and lastError 0), then stepped on the error 0, so the hover correction is 0 and only gravity is left
  HoverFlyState m;
  m.hovering = 0;
  m.hoverHeight = 2;
  Pid hover = Gains(1, 0.005f, 0.5f), pitch = Gains(1, 0, 0), roll = Gains(1, 0, 0);
  hover.state.lastError = 5;
  hover.state.integral = 3;
  ModeOutput o = HoverFly(m, Level(), Static(), Controls{}, StandardParams{}, HoverFlyParams{}, Found(6, kUp), hover,
                          pitch, roll, 0.5f);
  CHECK_NEAR(m.hovering, 0.1666667, kTol);
  CHECK_NEAR(m.hoverHeight, 6, kTol);
  CHECK(hover.state.integral == 0.0f);
  CHECK(hover.state.lastError == 0.0f);
  CHECK_NEAR(o.force.z, 9.81, kTol);
}

TEST(modes_hoverfly_hold) {
  // no ground, hovering 0: the PID is not stepped. Holding starts at z 100 + vz 2 / heightDampening 1 = 102; fly = -2 +
  // clamp((102 - 100) * 1) = 0, so the force is gravity alone. A second call keeps the reference: -0.5 + 1 = 0.5
  HoverFlyState m;
  m.hovering = 0;
  m.hoverHeight = 3;
  StatsState s = Level();
  s.d_position.z = 100;
  s.d_velocity.z = 2;
  Pid hover = Gains(1, 0, 0), pitch = Gains(1, 0, 0), roll = Gains(1, 0, 0);
  hover.state.lastError = 0.7f;
  ModeOutput o = HoverFly(m, s, Static(), Controls{}, StandardParams{}, HoverFlyParams{}, NotFound(2), hover, pitch,
                          roll, 0.5f);
  CHECK(m.holdingAltitude);
  CHECK_NEAR(m.referenceZ, 102, kTol);
  CHECK_NEAR(o.force.z, 9.81, kTol);
  CHECK(hover.state.lastError == 0.7f);
  CheckVec(o.torque, 0, 0, 0, kTol);
  s.d_position.z = 101;
  s.d_velocity.z = 0.5f;
  o = HoverFly(m, s, Static(), Controls{}, StandardParams{}, HoverFlyParams{}, NotFound(2), hover, pitch, roll, 0.5f);
  CHECK_NEAR(m.referenceZ, 102, kTol);
  CHECK_NEAR(o.force.z, 10.31, kTol);
}

TEST(modes_hoverfly_lift_brake) {
  // lift 1 at dt 1/60 moves the target by heightCorrectionFactor 0.5; fly = 1 * 25, halved by the linear brake, plus
  // gravity: 12.5 + 9.81
  HoverFlyState m;
  m.hovering = 0;
  m.hoverHeight = 3;
  Controls c;
  c.lift = 1;
  c.linearBrake = 0.5f;
  Pid hover = Gains(1, 0, 0), pitch = Gains(1, 0, 0), roll = Gains(1, 0, 0);
  ModeOutput o = HoverFly(m, Level(), Static(), c, StandardParams{}, HoverFlyParams{}, NotFound(2), hover, pitch, roll,
                          1.0f / 60.0f);
  CHECK_NEAR(m.hoverHeight, 3.5, kTol);
  CHECK(!m.holdingAltitude);
  CHECK_NEAR(o.force.z, 22.31, kTol);
}

TEST(modes_hoverfly_distinct) {
  // every value its own: min 2, max 10, liftFactor 20, hoverFactor 3, hoverClamp 4, gravity 0.5; altitudeHold 2,
  // heightDampening 4, heightCorrection 0.25; hover PID P 12.
  // In the band: distance 6, hovering 1 - 4/8 = 0.5. lift 0.2 moves the target 0.2 * 0.25 * 0.5 * 60 = 1.5, 5 to 6.5;
  // diff 0.5, hc 6 clamped to 4. vz 1.5: fly = 0.2 * 20 - 1.5 * 4 = -2; lf = -2 + (4 * 3 + 2) * 0.5 = 5; force.z =
  // 5 + 9.81 * 0.5 = 9.905.
  // No ground: hovering 0, z 50, vz 2: reference 50 + 2 / 4 = 50.5; fly = -2 * 4 + (0.5 * 2) = -7; force.z =
  // -7 + 4.905 = -2.095.
  StandardParams p;
  p.minHoverHeight = 2;
  p.maxHoverHeight = 10;
  p.liftFactor = 20;
  p.hoverFactor = 3;
  p.hoverClamp = 4;
  p.gravityFactor = 0.5f;
  HoverFlyParams h;
  h.altitudeHoldFactor = 2;
  h.heightDampening = 4;
  h.heightCorrectionFactor = 0.25f;
  Controls c;
  c.lift = 0.2f;
  StatsState s = Level();
  s.d_velocity.z = 1.5f;
  HoverFlyState m;
  m.hovering = 0.3f;
  m.hoverHeight = 5;
  Pid hover = Gains(12, 0, 0), pitch = Gains(1, 0, 0), roll = Gains(1, 0, 0);
  ModeOutput o = HoverFly(m, s, Static(), c, p, h, Found(6, kUp), hover, pitch, roll, 0.5f);
  CHECK_NEAR(m.hovering, 0.5, kTol);
  CHECK_NEAR(m.hoverHeight, 6.5, kTol);
  CHECK_NEAR(o.force.z, 9.905, kTol);

  HoverFlyState n;
  n.hovering = 0;
  n.hoverHeight = 4;
  s.d_position.z = 50;
  s.d_velocity.z = 2;
  o = HoverFly(n, s, Static(), Controls{}, p, h, NotFound(6), hover, pitch, roll, 0.5f);
  CHECK(n.holdingAltitude);
  CHECK_NEAR(n.referenceZ, 50.5, kTol);
  CHECK_NEAR(o.force.z, -2.095, kTol);
}

TEST(modes_automatic_aero) {
  // hovering 1, target at the distance: no hover force, gravity only. surge 1 gives forward 15. aeroFactor 0.8,
  // yawDirectionality 0.5 * 300 = 150, directionFactor |-0.6| = 0.6: forward += 0.6 * 150 * 0.8 = 72 on y;
  // -localDirection * 72 = (-43.2, -57.6). x -43.2, y 15 + 72 - 57.6 = 29.4
  AutomaticState m;
  m.hovering = 1;
  m.hoverHeight = 1;
  StatsState s = Level();
  s.d_direction = {0.6f, 0.8f, 0};
  s.d_localDirection = {0.6f, 0.8f, 0};
  s.d_speedRatio = 0.5f;
  Controls c;
  c.surge = 1;
  Pid hover = Gains(1, 0, 0), pitch = Gains(1, 0, 0), roll = Gains(1, 0, 0);
  ModeOutput o = Automatic(m, s, Static(), c, StandardParams{}, AutomaticParams{}, Found(1, kUp), hover, pitch, roll,
                           0.5f);
  CheckVec(o.force, -43.2, 29.4, 9.81, kLoose);
}

TEST(modes_automatic_braking) {
  // surge 0.5: forward 7.5; braking (1 - 0.5) * 3 * 1 = 1.5 times localVelocity (2, 4, 6) takes (3, 6) off x and y,
  // not z
  AutomaticState m;
  m.hovering = 1;
  m.hoverHeight = 1;
  StatsState s = Level();
  s.d_localVelocity = {2, 4, 6};
  Controls c;
  c.surge = 0.5f;
  Pid hover = Gains(1, 0, 0), pitch = Gains(1, 0, 0), roll = Gains(1, 0, 0);
  ModeOutput o = Automatic(m, s, Static(), c, StandardParams{}, AutomaticParams{}, Found(1, kUp), hover, pitch, roll,
                           0.5f);
  CheckVec(o.force, -3, 1.5, 9.81, kLoose);
}

TEST(modes_automatic_no_ground) {
  // no ground: hovering 0, target 3 against the stale distance 2: diff 1, the PID is stepped (lastError 1).
  // lf = Lerp(0, 0 - (-20) * 0.1, 1) = 2, times hoverFactor 40 plus gravity
  AutomaticState m;
  m.hovering = 0;
  m.hoverHeight = 3;
  StatsState s = Level();
  s.d_velocity.z = -20;
  Pid hover = Gains(1, 0, 0), pitch = Gains(1, 0, 0), roll = Gains(1, 0, 0);
  ModeOutput o = Automatic(m, s, Static(), Controls{}, StandardParams{}, AutomaticParams{}, NotFound(2), hover, pitch,
                           roll, 0.5f);
  CHECK_NEAR(hover.state.lastError, 1, kTol);
  CHECK_NEAR(o.force.z, 89.81, kLoose);
}

TEST(modes_automatic_enter_band) {
  // from hovering 0 into the band at distance 4: hovering 0.5; the target is the distance plus 1 * 0.1 * 25
  AutomaticState m;
  m.hovering = 0;
  m.hoverHeight = 0;
  Controls c;
  c.lift = 1;
  Pid hover = Gains(1, 0, 0), pitch = Gains(1, 0, 0), roll = Gains(1, 0, 0);
  Automatic(m, Level(), Static(), c, StandardParams{}, AutomaticParams{}, Found(4, kUp), hover, pitch, roll, 0.1f);
  CHECK_NEAR(m.hovering, 0.5, kTol);
  CHECK_NEAR(m.hoverHeight, 6.5, kTol);
}

TEST(modes_automatic_distinct) {
  // min 2, max 6, gravity 0.5; autoBraking 2, yawDirectionality 100, liftFactor 10, hoverFactor 20; braking friction 1.5.
  // distance 4: hovering 1 - 2/4 = 0.5. lift 0.5 at dt 0.5 moves the target -1 + 0.5 * 0.5 * 10 = 1.5, raised to the
  // minimum 2; diff -2, hc -2. vz 3: lf = Lerp(0.5, 0.5 - 0.3, -2) = 0.2 + (-2.2) * 0.5 = -0.9; force.z =
  // -0.9 * 20 + 4.905 = -13.095.
  // surge 0.25 gives y 3.75. speedRatio 0.4, direction (0.6, 0.8, 0), localDirection (0.8, 0.6, 0): aero 0.8, yd 40,
  // df 0.6; forward += 0.6 * 40 * 0.8 = 19.2 on y; -localDirection * 19.2 = (-15.36, -11.52). Braking
  // 0.75 * 2 * 1.5 = 2.25 times localVelocity (4, 2, 7) = (9, 4.5): x -15.36 - 9 = -24.36, y 3.75 + 19.2 - 11.52 - 4.5 =
  // 6.93.
  StandardParams p;
  p.minHoverHeight = 2;
  p.maxHoverHeight = 6;
  p.gravityFactor = 0.5f;
  AutomaticParams a;
  a.autoBrakingFactor = 2;
  a.yawDirectionality = 100;
  a.liftFactor = 10;
  a.hoverFactor = 20;
  StatsStatic st = Static();
  st.brakingFrictionFactor = 1.5f;
  StatsState s = Level();
  s.d_velocity.z = 3;
  s.d_direction = {0.6f, 0.8f, 0};
  s.d_localDirection = {0.8f, 0.6f, 0};
  s.d_speedRatio = 0.4f;
  s.d_localVelocity = {4, 2, 7};
  Controls c;
  c.lift = 0.5f;
  c.surge = 0.25f;
  AutomaticState m;
  m.hovering = 0.5f;
  m.hoverHeight = -1;
  Pid hover = Gains(1, 0, 0), pitch = Gains(1, 0, 0), roll = Gains(1, 0, 0);
  ModeOutput o = Automatic(m, s, st, c, p, a, Found(4, kUp), hover, pitch, roll, 0.5f);
  CHECK_NEAR(m.hoverHeight, 2, kTol);
  CheckVec(o.force, -24.36, 6.93, -13.095, kLoose);
}

TEST(modes_fly) {
  // lift 0.5 at dt 0.5 with liftFactor 20 and gravity 0.5: 20 * 0.5 * 0.5 + 4.905, level
  StandardParams p;
  p.liftFactor = 20;
  p.gravityFactor = 0.5f;
  Controls c;
  c.lift = 0.5f;
  Pid pitch = Gains(1, 0, 0), roll = Gains(1, 0, 0);
  ModeOutput o = Fly(Level(), Static(), c, p, pitch, roll, 0.5f);
  CheckVec(o.force, 0, 0, 9.905, kTol);
  CheckVec(o.torque, 0, 0, 0, kTol);
}

TEST(modes_hover) {
  // ground at 3 with the normal 30 degrees toward forward; the target 0.2 is raised to the minimum 1: diff -2, hc -2,
  // lift force -2 * 25 + 9.81 = -40.19 along the car's up; the pitch error 1/3 gives torque.x -(1/3 * 3) = -1
  float hoverHeight = 0.2f;
  Pid hover = Gains(1, 0, 0), pitch = Gains(1, 0, 0), roll = Gains(1, 0, 0);
  ModeOutput o = Hover(hoverHeight, Level(), Static(), Controls{}, StandardParams{}, Found(3, {0, 0.5f, 0.8660254f}),
                       hover, pitch, roll, 0.5f);
  CHECK_NEAR(hoverHeight, 1, kTol);
  CHECK_NEAR(o.force.z, -40.19, kLoose);
  CHECK_NEAR(o.torque.x, -1, kLoose);

  // no ground: normal Up, difference 0, the PID is still stepped (lastError 0.7 becomes 0)
  hover.state.lastError = 0.7f;
  o = Hover(hoverHeight, Level(), Static(), Controls{}, StandardParams{}, NotFound(3), hover, pitch, roll, 0.5f);
  CHECK(hover.state.lastError == 0.0f);
  CheckVec(o.force, 0, 0, 9.81, kTol);
}

TEST(modes_hoverfly_normal_blend) {
  // Defaults: min 1, max 7. distance 5.5: hovering 1 - 4.5/6 = 0.25 (was 0.25, no reset). The ground normal is (0, 1, 0)
  // (horizontal, to make the blend easy to read): Interpolate(Up, n, 0.25) = Up + (n - Up) * 0.25 = (0, 0.25, 0.75).
  // Its angle from the forward axis is atan(0.75 / 0.25) = 71.565051 degrees, so pitchOffNormal = 18.434949, error
  // 0.204833, times pitchFactor 3 (PID P 1) = 0.614498: torque.x -0.614498. Swapped arguments would blend with 0.75
  // and give a pitchOffNormal of 71.57 instead. The roll projection is along Z: rollOffNormal 0, torque.y 0.
  HoverFlyState m;
  m.hovering = 0.25f;
  m.hoverHeight = 5.5f;
  Pid hover = Gains(1, 0, 0), pitch = Gains(1, 0, 0), roll = Gains(1, 0, 0);
  ModeOutput o = HoverFly(m, Level(), Static(), Controls{}, StandardParams{}, HoverFlyParams{}, Found(5.5f, {0, 1, 0}),
                          hover, pitch, roll, 0.5f);
  CHECK_NEAR(m.hovering, 0.25, kTol);
  CHECK_NEAR(o.torque.x, -0.614498, kLoose);
  CHECK_NEAR(o.torque.y, 0, kLoose);
}

TEST(modes_hoverfly_lerp_quarter) {
  // hovering 0.25 (distance 5.5), hoverHeight 6 stays 6: diff 0.5, hc 0.5. vz -2, no lift, hovering above 0 so not
  // holding: fly = 0 * 25 - (-2) * 1 = 2. lf = Lerp(0.25, 2, 0.5 * 10) = 2 + (5 - 2) * 0.25 = 2.75 (swapped it would be
  // 5 + (2 - 5) * 0.25 = 4.25), plus gravity: 12.56.
  HoverFlyState m;
  m.hovering = 0.25f;
  m.hoverHeight = 6;
  StatsState s = Level();
  s.d_velocity.z = -2;
  Pid hover = Gains(1, 0, 0), pitch = Gains(1, 0, 0), roll = Gains(1, 0, 0);
  ModeOutput o = HoverFly(m, s, Static(), Controls{}, StandardParams{}, HoverFlyParams{}, Found(5.5f, kUp), hover, pitch,
                          roll, 0.5f);
  CHECK_NEAR(o.force.z, 12.56, kTol);
}

TEST(modes_hoverfly_weight_clamp) {
  // Found(0.5): below minHoverHeight 1, the weight 1 - (0.5 - 1)/6 = 1.0833 clamps to 1. hoverHeight 3 stays 3 (clamped
  // to 1..7), diff 2.5, hc 2.5, vz 0: fly 0; lf = Lerp(1, 0, 2.5 * 10) = 25 (unclamped 27.08), force.z 34.81. The
  // normal (0, 1, 0) at weight 1 is used whole: along the forward axis, pitchOffNormal 90, error 1, torque.x -3.
  {
    HoverFlyState m;
    m.hovering = 1;
    m.hoverHeight = 3;
    Pid hover = Gains(1, 0, 0), pitch = Gains(1, 0, 0), roll = Gains(1, 0, 0);
    ModeOutput o = HoverFly(m, Level(), Static(), Controls{}, StandardParams{}, HoverFlyParams{}, Found(0.5f, {0, 1, 0}),
                            hover, pitch, roll, 0.5f);
    CHECK_NEAR(m.hovering, 1, kTol);
    CHECK_NEAR(o.force.z, 34.81, kLoose);
    CHECK_NEAR(o.torque.x, -3, kLoose);
  }
  // Found(9): above maxHoverHeight 7, the weight 1 - 8/6 = -0.333 clamps to 0 (from hovering 1, so no reset). The PID is
  // not stepped (lastError 0.7 stays); the normal is Up whatever the ground's, torque 0. Lift 0.2 moves the target
  // 0.2 * 0.5 * 0.5 * 60 = 3, 3 to 6, and is above 0.01 so no altitude hold: fly = 0.2 * 25 = 5, lf = Lerp(0, 5, 0) = 5
  // (unclamped it would be 5 + (0 - 5) * -0.333 = 6.667), force.z = 14.81.
  {
    HoverFlyState m;
    m.hovering = 1;
    m.hoverHeight = 3;
    Controls c;
    c.lift = 0.2f;
    Pid hover = Gains(1, 0, 0), pitch = Gains(1, 0, 0), roll = Gains(1, 0, 0);
    hover.state.lastError = 0.7f;
    ModeOutput o = HoverFly(m, Level(), Static(), c, StandardParams{}, HoverFlyParams{}, Found(9, {0, 1, 0}), hover, pitch,
                            roll, 0.5f);
    CHECK_NEAR(m.hovering, 0, kTol);
    CHECK_NEAR(m.hoverHeight, 6, kTol);
    CHECK(hover.state.lastError == 0.7f);
    CHECK_NEAR(o.force.z, 14.81, kLoose);
    CheckVec(o.torque, 0, 0, 0, kLoose);
  }
}

TEST(modes_hoverfly_holding_reset) {
  // A lift input above 0.01 ends the hold. z 40, a stale referenceZ 50, no ground, hovering 0: fly = 0.5 * 25 - 0 =
  // 12.5, no hold term (kept it would add clamp(10) = 10), force.z = 12.5 + 9.81 = 22.31.
  {
    HoverFlyState m;
    m.hovering = 0;
    m.hoverHeight = 3;
    m.holdingAltitude = true;
    m.referenceZ = 50;
    StatsState s = Level();
    s.d_position.z = 40;
    Controls c;
    c.lift = 0.5f;
    Pid hover = Gains(1, 0, 0), pitch = Gains(1, 0, 0), roll = Gains(1, 0, 0);
    ModeOutput o = HoverFly(m, s, Static(), c, StandardParams{}, HoverFlyParams{}, NotFound(2), hover, pitch, roll, 0.5f);
    CHECK(!m.holdingAltitude);
    CHECK_NEAR(o.force.z, 22.31, kTol);
  }
  // Being in the band ends it too: distance 4 as in modes_hoverfly_band (fly 2, lf 6, force.z 15.81), z 40, stale
  // referenceZ 50 and no lift; kept, the hold would add 10.
  {
    HoverFlyState m;
    m.hovering = 0.5f;
    m.hoverHeight = 5;
    m.holdingAltitude = true;
    m.referenceZ = 50;
    StatsState s = Level();
    s.d_position.z = 40;
    s.d_velocity.z = -2;
    Pid hover = Gains(1, 0, 0), pitch = Gains(1, 0, 0), roll = Gains(1, 0, 0);
    ModeOutput o = HoverFly(m, s, Static(), Controls{}, StandardParams{}, HoverFlyParams{}, Found(4, kUp), hover, pitch,
                            roll, 0.5f);
    CHECK(!m.holdingAltitude);
    CHECK_NEAR(o.force.z, 15.81, kTol);
  }
}

TEST(modes_hoverfly_hold_clamp) {
  // liftFactor 20 and altitudeHoldFactor 2, holding (kept: no lift, no ground, hovering 0), vz 0, z 40, so the fly term
  // is only the hold term. referenceZ 100: (100 - 40) * 2 = 120 clamps to +20, force.z 29.81. referenceZ 0: -80 clamps
  // to -20, force.z -10.19. referenceZ 45: 10 is inside the clamp, 19.81.
  StandardParams p;
  p.liftFactor = 20;
  HoverFlyParams h;
  h.altitudeHoldFactor = 2;
  StatsState s = Level();
  s.d_position.z = 40;
  const float refs[3] = {100, 0, 45};
  const double expected[3] = {29.81, -10.19, 19.81};
  for (int i = 0; i < 3; ++i) {
    HoverFlyState m;
    m.hovering = 0;
    m.hoverHeight = 3;
    m.holdingAltitude = true;
    m.referenceZ = refs[i];
    Pid hover = Gains(1, 0, 0), pitch = Gains(1, 0, 0), roll = Gains(1, 0, 0);
    ModeOutput o = HoverFly(m, s, Static(), Controls{}, p, h, NotFound(2), hover, pitch, roll, 0.5f);
    CHECK(m.holdingAltitude);
    CHECK_NEAR(m.referenceZ, refs[i], kTol);
    CHECK_NEAR(o.force.z, expected[i], kLoose);
  }
}

TEST(modes_hoverfly_zero_dampening) {
  // heightDampening 0: the hold starts at referenceZ = z with no vz / 0 (z 100, vz 2). fly = 0 - 2 * 0 + clamp(0) = 0,
  // force.z is gravity alone.
  HoverFlyParams h;
  h.heightDampening = 0;
  HoverFlyState m;
  m.hovering = 0;
  m.hoverHeight = 3;
  StatsState s = Level();
  s.d_position.z = 100;
  s.d_velocity.z = 2;
  Pid hover = Gains(1, 0, 0), pitch = Gains(1, 0, 0), roll = Gains(1, 0, 0);
  ModeOutput o = HoverFly(m, s, Static(), Controls{}, StandardParams{}, h, NotFound(2), hover, pitch, roll, 0.5f);
  CHECK(m.holdingAltitude);
  CHECK_NEAR(m.referenceZ, 100, kTol);
  CHECK_NEAR(o.force.z, 9.81, kTol);
}

TEST(modes_automatic_normal_blend) {
  // As modes_hoverfly_normal_blend: distance 5.5, hovering 0.25, normal (0, 1, 0) blended to (0, 0.25, 0.75), torque.x
  // -0.614498 (pitchOffNormal 18.434949, error 0.204833, times 3).
  AutomaticState m;
  m.hovering = 0.25f;
  m.hoverHeight = 5.5f;
  Pid hover = Gains(1, 0, 0), pitch = Gains(1, 0, 0), roll = Gains(1, 0, 0);
  ModeOutput o = Automatic(m, Level(), Static(), Controls{}, StandardParams{}, AutomaticParams{}, Found(5.5f, {0, 1, 0}),
                           hover, pitch, roll, 0.5f);
  CHECK_NEAR(m.hovering, 0.25, kTol);
  CHECK_NEAR(o.torque.x, -0.614498, kLoose);
  CHECK_NEAR(o.torque.y, 0, kLoose);
}

TEST(modes_automatic_lerp_quarter) {
  // hovering 0.25 (distance 5.5), hoverHeight 6 stays 6: diff 0.5, hc 0.5. vz -20: the fly term is 0 - (-20) * 0.1 = 2.
  // lf = Lerp(0.25, 2, 0.5) = 2 + (0.5 - 2) * 0.25 = 1.625 (swapped 0.875); times hoverFactor 40 = 65, plus gravity 74.81.
  AutomaticState m;
  m.hovering = 0.25f;
  m.hoverHeight = 6;
  StatsState s = Level();
  s.d_velocity.z = -20;
  Pid hover = Gains(1, 0, 0), pitch = Gains(1, 0, 0), roll = Gains(1, 0, 0);
  ModeOutput o = Automatic(m, s, Static(), Controls{}, StandardParams{}, AutomaticParams{}, Found(5.5f, kUp), hover, pitch,
                           roll, 0.5f);
  CHECK_NEAR(o.force.z, 74.81, kLoose);
}

TEST(modes_automatic_weight_clamp) {
  // Found(0.5): weight 1.0833 clamps to 1. hoverHeight 3 stays 3: diff 2.5, hc 2.5; vz 0: lf = Lerp(1, 0, 2.5) = 2.5
  // (unclamped 2.708), times hoverFactor 40 plus gravity = 109.81. The normal (0, 1, 0) is used whole: torque.x -3.
  {
    AutomaticState m;
    m.hovering = 1;
    m.hoverHeight = 3;
    Pid hover = Gains(1, 0, 0), pitch = Gains(1, 0, 0), roll = Gains(1, 0, 0);
    ModeOutput o = Automatic(m, Level(), Static(), Controls{}, StandardParams{}, AutomaticParams{},
                             Found(0.5f, {0, 1, 0}), hover, pitch, roll, 0.5f);
    CHECK_NEAR(m.hovering, 1, kTol);
    CHECK_NEAR(o.force.z, 109.81, kLoose);
    CHECK_NEAR(o.torque.x, -3, kLoose);
  }
  // Found(9): weight -0.333 clamps to 0. The target stays 3: diff 3 - 9 = -6, and the PID is stepped regardless (hovering
  // 0 with ground, lastError -6, hc -6). vz -20: lf = Lerp(0, 2, -6) = 2 (unclamped 2 + (-6 - 2) * -0.333 = 4.667), times
  // 40 plus gravity = 89.81. The normal is Up whatever the ground's: torque 0.
  {
    AutomaticState m;
    m.hovering = 1;
    m.hoverHeight = 3;
    StatsState s = Level();
    s.d_velocity.z = -20;
    Pid hover = Gains(1, 0, 0), pitch = Gains(1, 0, 0), roll = Gains(1, 0, 0);
    ModeOutput o = Automatic(m, s, Static(), Controls{}, StandardParams{}, AutomaticParams{}, Found(9, {0, 1, 0}), hover,
                             pitch, roll, 0.5f);
    CHECK_NEAR(m.hovering, 0, kTol);
    CHECK_NEAR(hover.state.lastError, -6, kTol);
    CHECK_NEAR(o.force.z, 89.81, kLoose);
    CheckVec(o.torque, 0, 0, 0, kLoose);
  }
}

namespace {
Hit MakeHit(Vec3 tire, Vec3 position, Vec3 normal) { return Hit{true, tire, position, normal}; }
} // namespace

TEST(modes_ground_from_hits_none) {
  // nothing found: the distance stays the last one and the normal is zero
  Ground g = GroundFromHits(nullptr, 0, 2.5f);
  CHECK(!g.found);
  CHECK_NEAR(g.distance, 2.5, kTol);
  CheckVec(g.normal, 0, 0, 0, kTol);

  // one invalid ray among four: the script needs all four, so also not found
  Hit hits[4] = {MakeHit({0, 0, 10}, {0, 0, 8}, kUp), MakeHit({0, 0, 10}, {0, 0, 8}, kUp), Hit{},
                 MakeHit({0, 0, 10}, {0, 0, 8}, kUp)};
  g = GroundFromHits(hits, 4, 4.5f);
  CHECK(!g.found);
  CHECK_NEAR(g.distance, 4.5, kTol);
  CheckVec(g.normal, 0, 0, 0, kTol);
}

TEST(modes_ground_from_hits_one) {
  // distance = |(0,0,7) - (0,0,10)| = 3; the normal (0, 0, 2) is normalised to (0, 0, 1)
  Hit h = MakeHit({0, 0, 10}, {0, 0, 7}, {0, 0, 2});
  Ground g = GroundFromHits(&h, 1, 99);
  CHECK(g.found);
  CHECK_NEAR(g.distance, 3, kTol);
  CheckVec(g.normal, 0, 0, 1, kTol);
}

TEST(modes_ground_from_hits_four) {
  // distances: (0,0,10)->(0,0,8) 2; (1,0,10)->(1,0,6) 4; (0,0,10)->(3,0,6) sqrt(9 + 16) = 5; (0,0,10)->(0,0,9) 1; the
  // mean is 12 / 4 = 3. Normals are normalised first, whatever their length: (0,0,5) -> (0,0,1), (3,0,4) -> (0.6,0,0.8),
  // (0,-3,4) -> (0,-0.6,0.8), (0,0,0.3) -> (0,0,1). Their sum is (0.6, -0.6, 3.6), mean (0.15, -0.15, 0.9), length
  // sqrt(0.0225 + 0.0225 + 0.81) = 0.924662, normalised (0.162221, -0.162221, 0.973329).
  Hit hits[4] = {MakeHit({0, 0, 10}, {0, 0, 8}, {0, 0, 5}), MakeHit({1, 0, 10}, {1, 0, 6}, {3, 0, 4}),
                 MakeHit({0, 0, 10}, {3, 0, 6}, {0, -3, 4}), MakeHit({0, 0, 10}, {0, 0, 9}, {0, 0, 0.3f})};
  Ground g = GroundFromHits(hits, 4, 99);
  CHECK(g.found);
  CHECK_NEAR(g.distance, 3, kTol);
  CheckVec(g.normal, 0.162221, -0.162221, 0.973329, kLoose);
  CHECK_NEAR(Length(g.normal), 1, kLoose);
}

TEST(modes_ground_from_hits_cancelling_normals) {
  // opposite normals average to zero; Normalize returns a zero vector unchanged, so the ground is found with a zero normal
  Hit hits[2] = {MakeHit({0, 0, 5}, {0, 0, 3}, {0, 0, 1}), MakeHit({0, 0, 5}, {0, 0, 1}, {0, 0, -1})};
  Ground g = GroundFromHits(hits, 2, 0);
  CHECK(g.found);
  CHECK_NEAR(g.distance, 3, kTol);
  CheckVec(g.normal, 0, 0, 0, kTol);
}

TEST(ground_ray_layout) {
  // FindGround's rays (FlightComponent.reds, native FlightGround): straight down from the wheel emitter, the max hover
  // height plus 10 m long
  const Vec3 end = GroundRayEnd({3.0f, -2.0f, 41.5f}, 7.0f);
  CHECK_NEAR(end.x, 3.0f, 0.0f);
  CHECK_NEAR(end.y, -2.0f, 0.0f);
  CHECK_NEAR(end.z, 41.5f - 17.0f, 1e-5f);
}

TEST(modes_hover_fly_assist_keeps_hover_height_through_a_gap) {
  // hovering 1 m up, the ground lost for a frame (a ray through a seam) and found again 0.3 m lower: the hover height
  // stays; lost for 0.6 s, the band is entered anew at the distance then
  HoverFlyState m;
  m.hoverHeight = 1.0f;
  const StatsState s = Level();
  const StatsStatic st = Static();
  const Controls c;
  const StandardParams p;
  const HoverFlyAssistParams a;
  const Vec3 response{1, 1, 1};
  Ground ground;
  ground.found = true;
  ground.distance = 1.0f;
  ground.normal = {0, 0, 1};
  Ground none;
  none.found = false;
  Ground lower = ground;
  lower.distance = 1.3f;
  const float dt = 1.0f / 60.0f;
  HoverFlyAssist(m, s, st, c, p, a, ground, response, dt);
  HoverFlyAssist(m, s, st, c, p, a, none, response, dt);
  HoverFlyAssist(m, s, st, c, p, a, lower, response, dt);
  CHECK_NEAR(m.hoverHeight, 1.0f, kTol);
  for (int i = 0; i < 36; ++i)
    HoverFlyAssist(m, s, st, c, p, a, none, response, dt);
  HoverFlyAssist(m, s, st, c, p, a, lower, response, dt);
  CHECK_NEAR(m.hoverHeight, 1.3f, kTol);
}

TEST(modes_drone_skid) {
  // resting on the ground (clearance 0) it lifts by more than gravity; at the clearance, still, it holds gravity
  // exactly; above the band, tilted past minUp, with lift down held, or set to 0, nothing
  const DroneSkidParams k;
  CHECK(DroneSkid(0.0f, 0.0f, 1.0f, 0.0f, k) > kGravity);
  CHECK_NEAR(DroneSkid(k.clearance, 0.0f, 1.0f, 0.0f, k), kGravity, kLoose);
  CHECK_NEAR(DroneSkid(k.clearance + k.band + 0.01f, 0.0f, 1.0f, 0.0f, k), 0.0f, kTol);
  CHECK_NEAR(DroneSkid(0.0f, 0.0f, 0.5f, 0.0f, k), 0.0f, kTol);
  CHECK_NEAR(DroneSkid(0.0f, 0.0f, 1.0f, -1.0f, k), 0.0f, kTol);
  DroneSkidParams off = k;
  off.clearance = 0.0f;
  CHECK_NEAR(DroneSkid(0.0f, 0.0f, 1.0f, 0.0f, off), 0.0f, kTol);
  // never pulls down, and no more than maxAccel up
  CHECK_NEAR(DroneSkid(k.clearance + 0.1f, 5.0f, 1.0f, 0.0f, k), 0.0f, kTol);
  CHECK(DroneSkid(-0.5f, -10.0f, 1.0f, 0.0f, k) <= k.maxAccel + kTol);
  // half lift down, half the hold
  CHECK_NEAR(DroneSkid(k.clearance, 0.0f, 1.0f, -0.5f, k), 0.5f * kGravity, kLoose);
}
