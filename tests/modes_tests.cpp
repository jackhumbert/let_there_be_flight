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
