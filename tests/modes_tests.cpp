// Modes (src/red4ext/Flight/Modes.hpp): AttitudeLift (the script FlightModeStandard.UpdateWithNormalLift, now the
// Autopilot's manual flight), FlightModeDrone.Update and Standard with its hover bands. Expected values are worked by
// hand from the scripts.
#include "check.hpp"

#include "Flight/Modes.hpp"

#include <array>

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

TEST(modes_attitude_lift_level) {
  Pid pitch = Gains(1, 0, 0), roll = Gains(1, 0, 0);
  ModeOutput o = AttitudeLift(Level(), Static(), Controls{}, StandardParams{}, pitch, roll, kUp, 9.81f, 0.5f);
  CheckVec(o.force, 0, 0, 9.81, kTol);
  CheckVec(o.torque, 0, 0, 0, kTol);
}

TEST(modes_attitude_lift_tilted_normal) {
  // the normal is 30 degrees toward the forward axis: pitchOffNormal = 90 - 60 = 30, error 1/3. With gains
  // {1, 0.2, 0.05} and dt 0.5 the correction is 0.3333 + 0.2 * (1/3 * 0.5) + 0.05 * (1/3 / 0.5) = 0.4, times
  // pitchFactor 3 = 1.2. The roll projection (onto the plane of the forward axis) is along Z, 90 degrees from the
  // right axis, so rollOffNormal is 0.
  Pid pitch = Gains(1, 0.2f, 0.05f), roll = Gains(1, 0.5f, 0);
  ModeOutput o =
      AttitudeLift(Level(), Static(), Controls{}, StandardParams{}, pitch, roll, {0, 0.5f, 0.8660254f}, 9.81f, 0.5f);
  CHECK_NEAR(o.torque.x, -1.2, kLoose);
  CHECK_NEAR(o.torque.y, 0, kLoose);
  CHECK_NEAR(o.torque.z, 0, kLoose);
}

TEST(modes_attitude_lift_inputs) {
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
  ModeOutput o = AttitudeLift(s, Static(), c, p, pitch, roll, kUp, 9.81f, 0.5f);
  CheckVec(o.torque, -0.375, 0.3, 0.0625, kTol);
  CheckVec(o.force, -6, 13, 9.81, kTol);
}

TEST(modes_attitude_lift_gate) {
  // the normal along the forward axis: pitchOffNormal 90, plus 45 from the input is 135, not under 120. The pitch PID
  // is not stepped (its lastError stays) and its correction is 0. The roll projection is a zero vector, so the angle
  // is 90 and rollOffNormal 0: the roll PID is stepped, its lastError 0.3 becoming the error 0.
  Controls c;
  c.pitch = 1.0f;
  Pid pitch = Gains(1, 0, 0), roll = Gains(1, 0, 0);
  pitch.state.lastError = 0.3f;
  roll.state.lastError = 0.3f;
  ModeOutput o = AttitudeLift(Level(), Static(), c, StandardParams{}, pitch, roll, {0, 1, 0}, 9.81f, 0.5f);
  CHECK(pitch.state.lastError == 0.3f);
  CHECK(roll.state.lastError == 0.0f);
  CheckVec(o.torque, 0, 0, 0, kTol);
}

TEST(modes_attitude_lift_roll_with_yaw) {
  // yaw 1 turns the normal 0.15 rad about the forward axis: n = (sin 0.15, 0, cos 0.15) = (0.149438, 0, 0.988771), its
  // angle from the right axis is 81.405633 degrees, rollOffNormal 8.594367, error 0.0954930, times 3 = 0.286479.
  // The sign of torque.y rests on the RotateAxis direction assumed in ScriptMath (right-handed).
  Controls c;
  c.yaw = 1.0f;
  Pid pitch = Gains(1, 0, 0), roll = Gains(1, 0, 0);
  ModeOutput o = AttitudeLift(Level(), Static(), c, StandardParams{}, pitch, roll, kUp, 9.81f, 0.5f);
  CHECK_NEAR(o.torque.y, 0.286479, kLoose);
  CHECK_NEAR(o.torque.z, -2.5, kLoose);
  CHECK_NEAR(o.torque.x, 0, kLoose);
}

TEST(modes_attitude_lift_pitch_with_lift_and_surge) {
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
  ModeOutput o = AttitudeLift(s, Static(), c, p, pitch, roll, kUp, 9.81f, 0.5f);
  CHECK_NEAR(o.torque.x, 0.572958, kLoose);
  CHECK_NEAR(o.torque.y, 0, kLoose);
  CHECK_NEAR(o.force.y, 7.5, kTol);

  // a direction across the forward axis zeroes the lift part: only the surge 0.1 rad acts,
  // 0.1 * 57.29578 / 90 * 3 = 0.190986
  s.d_direction = {1, 0, 0};
  Pid pitch2 = Gains(1, 0, 0), roll2 = Gains(1, 0, 0);
  ModeOutput o2 = AttitudeLift(s, Static(), c, p, pitch2, roll2, kUp, 9.81f, 0.5f);
  CHECK_NEAR(o2.torque.x, 0.190986, kLoose);
}

TEST(modes_attitude_lift_turned_body) {
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
    ModeOutput o = AttitudeLift(s, Static(), c, StandardParams{}, pitch, roll, kUp, 9.81f, 0.5f);
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
        AttitudeLift(s, Static(), Controls{}, StandardParams{}, pitch, roll, {-0.5f, 0, 0.8660254f}, 9.81f, 0.5f);
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
        AttitudeLift(s, Static(), Controls{}, StandardParams{}, pitch, roll, {0, 0.5f, 0.8660254f}, 9.81f, 0.5f);
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

TEST(modes_attitude_lift_distinct_factors) {
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
  ModeOutput o = AttitudeLift(s, st, c, p, pitch, roll, kUp, 9.81f, 0.5f);
  CheckVec(o.torque, -0.222222, 0.444444, 0.0666667, kLoose);
  CheckVec(o.force, -10, 5, 9.81, kLoose);
}

TEST(modes_attitude_lift_gate_negative) {
  // the normal along -forward: 90 - 180 = -90, plus the input -45 is -135, past -120 too: the pitch PID is not
  // stepped; the roll projection is again a zero vector, so the roll PID is
  Controls c;
  c.pitch = -1.0f;
  Pid pitch = Gains(1, 0, 0), roll = Gains(1, 0, 0);
  pitch.state.lastError = 0.3f;
  roll.state.lastError = 0.3f;
  ModeOutput o = AttitudeLift(Level(), Static(), c, StandardParams{}, pitch, roll, {0, -1, 0}, 9.81f, 0.5f);
  CHECK(pitch.state.lastError == 0.3f);
  CHECK(roll.state.lastError == 0.0f);
  CheckVec(o.torque, 0, 0, 0, kTol);
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
// Ground under the car, and none.
Ground Found(float distance, Vec3 normal) { return Ground{true, distance, normal}; }
Ground NotFound(float distance) { return Ground{false, distance, Vec3{}}; }
} // namespace

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
  // distance = 10 - 7 = 3; the normal (0, 0, 2) is normalised to (0, 0, 1)
  Hit h = MakeHit({0, 0, 10}, {0, 0, 7}, {0, 0, 2});
  Ground g = GroundFromHits(&h, 1, 99);
  CHECK(g.found);
  CHECK_NEAR(g.distance, 3, kTol);
  CheckVec(g.normal, 0, 0, 1, kTol);
}

TEST(modes_ground_from_hits_four) {
  // heights over the hits (the rays are straight down): (0,0,10)->(0,0,8) 2; (1,0,10)->(1,0,6) 4; (3,0,10)->(3,0,5) 5;
  // (0,0,10)->(0,0,9) 1; the mean is 12 / 4 = 3. Normals are normalised first, whatever their length: (0,0,5) -> (0,0,1), (3,0,4) -> (0.6,0,0.8),
  // (0,-3,4) -> (0,-0.6,0.8), (0,0,0.3) -> (0,0,1). Their sum is (0.6, -0.6, 3.6), mean (0.15, -0.15, 0.9), length
  // sqrt(0.0225 + 0.0225 + 0.81) = 0.924662, normalised (0.162221, -0.162221, 0.973329).
  Hit hits[4] = {MakeHit({0, 0, 10}, {0, 0, 8}, {0, 0, 5}), MakeHit({1, 0, 10}, {1, 0, 6}, {3, 0, 4}),
                 MakeHit({3, 0, 10}, {3, 0, 5}, {0, -3, 4}), MakeHit({0, 0, 10}, {0, 0, 9}, {0, 0, 0.3f})};
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
  // FindGround's rays (FlightComponent.reds, native FlightGround): straight down from just over the wheel emitter to
  // the max hover height plus 10 m under it
  const Vec3 start = GroundRayStart({3.0f, -2.0f, 41.5f});
  CHECK_NEAR(start.x, 3.0f, 0.0f);
  CHECK_NEAR(start.y, -2.0f, 0.0f);
  CHECK_NEAR(start.z, 41.5f + kGroundRayRise, 1e-5f);
  const Vec3 end = GroundRayEnd({3.0f, -2.0f, 41.5f}, 7.0f);
  CHECK_NEAR(end.x, 3.0f, 0.0f);
  CHECK_NEAR(end.y, -2.0f, 0.0f);
  CHECK_NEAR(end.z, 41.5f - 17.0f, 1e-5f);
}

TEST(modes_ground_from_hits_above_the_emitters) {
  // a bike's emitters at the bottom of its tyres, 0.3 m under the road collider's surface: found, 0.3 m below it
  Hit hits[2] = {MakeHit({0, 1, 7.36f}, {0, 1, 7.66f}, kUp), MakeHit({0, -1, 7.36f}, {0, -1, 7.66f}, kUp)};
  Ground g = GroundFromHits(hits, 2, 0);
  CHECK(g.found);
  CHECK_NEAR(g.distance, -0.3f, 1e-4f);
}

TEST(modes_standard_keeps_hover_height_through_a_gap) {
  // hovering 1 m up, the ground lost for a frame (a ray through a seam) and found again 0.3 m lower: the hover height
  // stays; lost for 0.6 s, the band is entered anew at the distance then
  StandardState m;
  m.hoverHeight = 1.0f;
  const StatsState s = Level();
  const StatsStatic st = Static();
  const Controls c;
  const StandardParams p;
  const StandardGains a;
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
  Standard(m, s, st, c, p, a, ground, response, dt);
  Standard(m, s, st, c, p, a, none, response, dt);
  Standard(m, s, st, c, p, a, lower, response, dt);
  CHECK_NEAR(m.hoverHeight, 1.0f, kTol);
  for (int i = 0; i < 36; ++i)
    Standard(m, s, st, c, p, a, none, response, dt);
  Standard(m, s, st, c, p, a, lower, response, dt);
  CHECK_NEAR(m.hoverHeight, 1.3f, kTol);
}

TEST(modes_standard_hover_bands) {
  // the hover weight by band, the car level and still, the PIDs' defaults (StandardParams: band 1 to 7 m)
  const StatsState s = Level();
  const StatsStatic st = Static();
  const Controls c;
  const StandardParams p;
  const StandardGains a;
  const Vec3 response{1, 1, 1};
  const float dt = 1.0f / 60.0f;
  auto weight = [&](HoverBand band, Ground g) {
    StandardState m;
    m.hoverHeight = 2.0f;
    Standard(m, s, st, c, p, a, g, response, dt, band);
    return m.hovering;
  };
  // Standard (Blend): 1 at the band's bottom, 0.5 halfway, 0 at the top and above it, 0 without ground
  CHECK_NEAR(weight(HoverBand::Blend, Found(1, kUp)), 1.0, kTol);
  CHECK_NEAR(weight(HoverBand::Blend, Found(4, kUp)), 0.5, kTol);
  CHECK_NEAR(weight(HoverBand::Blend, Found(20, kUp)), 0.0, kTol);
  CHECK_NEAR(weight(HoverBand::Blend, NotFound(4)), 0.0, kTol);
  // Hover (Always): 1 on any ground found, 20 m up too
  CHECK_NEAR(weight(HoverBand::Always, Found(4, kUp)), 1.0, kTol);
  CHECK_NEAR(weight(HoverBand::Always, Found(20, kUp)), 1.0, kTol);
  CHECK_NEAR(weight(HoverBand::Always, NotFound(4)), 0.0, kTol);
  // Fly (Never): 0, the ground ignored
  CHECK_NEAR(weight(HoverBand::Never, Found(1, kUp)), 0.0, kTol);
  CHECK_NEAR(weight(HoverBand::Never, Found(4, kUp)), 0.0, kTol);
}

TEST(modes_standard_hover_sinks_into_the_band) {
  // Hover 20 m over the ground: the hover height is the band's top (7 m), so the car is pushed down; Standard there
  // holds its height (no push) and Fly at 4 m too
  const StatsState s = Level();
  const StatsStatic st = Static();
  const Controls c;
  const StandardParams p;
  const StandardGains a;
  const Vec3 response{1, 1, 1};
  const float dt = 1.0f / 60.0f;
  StandardState hover;
  hover.outOfBand = kRegainHoverHeight;
  const ModeOutput h = Standard(hover, s, st, c, p, a, Found(20, kUp), response, dt, HoverBand::Always);
  CHECK_NEAR(hover.hoverHeight, p.maxHoverHeight, kTol);
  CHECK(h.force.z < kGravity - 1.0f);
  StandardState standard;
  standard.holdingAltitude = true; // held where it is
  const ModeOutput d = Standard(standard, s, st, c, p, a, Found(20, kUp), response, dt, HoverBand::Blend);
  CHECK_NEAR(d.force.z, kGravity, kLoose);
  StandardState fly;
  fly.holdingAltitude = true;
  const ModeOutput f = Standard(fly, s, st, c, p, a, Found(4, kUp), response, dt, HoverBand::Never);
  CHECK_NEAR(f.force.z, kGravity, kLoose);
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

TEST(modes_drone_skid_floor) {
  // the skid as a bike's only floor: lift down does not let it go, and it holds the bike pressed at GroundedLift's press
  // a little under its clearance, short of maxAccel
  DroneSkidParams k;
  k.floor = true;
  CHECK_NEAR(DroneSkid(k.clearance, 0.0f, 1.0f, -1.0f, k), kGravity, kLoose);
  const float pressed = k.clearance - k.press / k.p;
  CHECK_NEAR(DroneSkid(pressed, 0.0f, 1.0f, -1.0f, k), kGravity + k.press, kLoose);
  CHECK(kGravity + k.press < k.maxAccel);
}

TEST(modes_drone_skid_floor_upright) {
  // a bike's two points on its centreline, rolled 20 degrees with its top to the right (+ about its forward): the floor
  // rolls it back (- about forward), harder while still rolling over; away from the ground, or as a car, it does not
  DroneSkidParams k;
  k.floor = true;
  StatsState s;
  const float half = 0.5f * 20.0f * 3.14159265f / 180.0f;
  s.d_orientation = {0, std::sin(half), 0, std::cos(half)};
  s.d_up = Rotate(s.d_orientation, kUp);
  StatsStatic st;
  st.mass = 380;
  st.momentOfInertia = {100, 40, 100};
  const Vec3 response{1, 1, 1};
  SkidPoint points[2] = {SkidPoint{{0, 0.8f, 0}, k.clearance}, SkidPoint{{0, -0.8f, 0}, k.clearance}};
  const ModeOutput still = DroneSkidPoints(points, 2, s, st, response, -1.0f, k);
  CHECK(still.torque.y < 0.0f);
  s.d_angularVelocity = {0, 1.0f, 0};
  const ModeOutput rolling = DroneSkidPoints(points, 2, s, st, response, -1.0f, k);
  CHECK(rolling.torque.y < still.torque.y);
  points[0].clearance = points[1].clearance = 1.0f;
  CHECK_NEAR(DroneSkidPoints(points, 2, s, st, response, -1.0f, k).torque.y, 0.0f, kTol);
  points[0].clearance = points[1].clearance = k.clearance;
  k.floor = false;
  CHECK_NEAR(DroneSkidPoints(points, 2, s, st, response, 0.0f, k).torque.y, 0.0f, kTol);
}

TEST(modes_grounded_lift) {
  // far from the ground lift down is kept; at the ground it is held to press, between the two it fades; lift up and a
  // light push are never touched
  const DroneSkidParams k;
  CHECK_NEAR(GroundedLift(-40.0f, 5.0f, 1.0f, k), -40.0f, kTol);
  CHECK_NEAR(GroundedLift(-40.0f, 0.0f, 1.0f, k), -k.press, kLoose);
  CHECK_NEAR(GroundedLift(-40.0f, k.clearance + 0.5f * k.band, 1.0f, k), 0.5f * (-40.0f - k.press), kLoose);
  CHECK_NEAR(GroundedLift(30.0f, 0.0f, 1.0f, k), 30.0f, kTol);
  CHECK_NEAR(GroundedLift(-2.0f, 0.0f, 1.0f, k), -2.0f, kTol);
  // upside down (or past the skid's tilt) lift down pushes it off the ground, all of it
  CHECK_NEAR(GroundedLift(-40.0f, 0.0f, -1.0f, k), -40.0f, kTol);
  CHECK_NEAR(GroundedLift(-40.0f, 0.0f, 0.5f, k), -40.0f, kTol);
}

TEST(modes_drone_skid_points) {
  // four corners at one clearance: DroneSkid's lift through the centre, no torque; a corner lower than the others
  // pushes harder and turns the car off it (front low: nose up, +X; left low: left up, +Y)
  const DroneSkidParams k;
  StatsState s;
  s.d_orientation = {0, 0, 0, 1};
  s.d_up = {0, 0, 1};
  StatsStatic st;
  st.mass = 1600;
  st.momentOfInertia = {2551, 817, 2896};
  const Vec3 response{1, 1, 1};
  auto corners = [](float fl, float fr, float bl, float br) {
    return std::array<SkidPoint, 4>{SkidPoint{{-1.1f, 1.7f, 0}, fl}, SkidPoint{{1.1f, 1.7f, 0}, fr},
                                    SkidPoint{{-1.1f, -1.7f, 0}, bl}, SkidPoint{{1.1f, -1.7f, 0}, br}};
  };
  const auto even = corners(0.05f, 0.05f, 0.05f, 0.05f);
  ModeOutput out = DroneSkidPoints(even.data(), 4, s, st, response, 0.0f, k);
  CHECK_NEAR(out.force.z, DroneSkid(0.05f, 0.0f, 1.0f, 0.0f, k), kLoose);
  CHECK_NEAR(Length(out.torque), 0.0f, kLoose);
  const auto front = corners(0.0f, 0.0f, 0.05f, 0.05f);
  out = DroneSkidPoints(front.data(), 4, s, st, response, 0.0f, k);
  CHECK(out.torque.x > 0.0f);
  CHECK_NEAR(out.torque.y, 0.0f, kLoose);
  const auto left = corners(0.0f, 0.05f, 0.0f, 0.05f);
  out = DroneSkidPoints(left.data(), 4, s, st, response, 0.0f, k);
  CHECK(out.torque.y > 0.0f);
  CHECK_NEAR(out.torque.x, 0.0f, kLoose);
  // a corner rising (the car pitching nose up) is damped there: less push at the front than at rest
  StatsState pitching = s;
  pitching.d_angularVelocity = {1.0f, 0, 0};
  out = DroneSkidPoints(even.data(), 4, pitching, st, response, 0.0f, k);
  CHECK(out.torque.x < 0.0f);
  // none at all with lift down held or above the band
  CHECK_NEAR(Length(DroneSkidPoints(front.data(), 4, s, st, response, -1.0f, k).force), 0.0f, kTol);
  const auto high = corners(0.5f, 0.5f, 0.5f, 0.5f);
  CHECK_NEAR(Length(DroneSkidPoints(high.data(), 4, s, st, response, 0.0f, k).force), 0.0f, kTol);
}

TEST(modes_drone_skid_friction) {
  // a car on its skid at the clearance, sliding 10 m/s forward on ground of friction 0.5: the ground drags it back by
  // the friction times the load (gravity at rest), and the drag fades out as the thrusters lift past touch; a bike's
  // points are its tyres and do not drag; none without friction, and none at rest
  DroneSkidParams k;
  StatsState s;
  s.d_orientation = {0, 0, 0, 1};
  s.d_up = {0, 0, 1};
  s.d_velocity = {0, 10, 0};
  StatsStatic st;
  st.mass = 1600;
  st.momentOfInertia = {2551, 817, 2896};
  const Vec3 response{1, 1, 1};
  auto corners = [](float c, float mu) {
    std::array<SkidPoint, 4> p{SkidPoint{{-1.1f, 1.7f, -0.4f}, c}, SkidPoint{{1.1f, 1.7f, -0.4f}, c},
                               SkidPoint{{-1.1f, -1.7f, -0.4f}, c}, SkidPoint{{1.1f, -1.7f, -0.4f}, c}};
    for (SkidPoint &q : p)
      q.friction = mu;
    return p;
  };
  const auto resting = corners(k.clearance, 0.5f);
  const ModeOutput out = DroneSkidPoints(resting.data(), 4, s, st, response, 0.0f, k);
  CHECK_NEAR(out.force.y, -0.5f * out.force.z, kLoose);
  CHECK_NEAR(out.force.z, DroneSkid(k.clearance, 0.0f, 1.0f, 0.0f, k), kLoose);
  CHECK(out.torque.x < 0.0f); // dragged at the bottom, the nose dips
  const auto lifted = corners(k.touch + 0.01f, 0.5f);
  CHECK_NEAR(DroneSkidPoints(lifted.data(), 4, s, st, response, 0.0f, k).force.y, 0.0f, kTol);
  const auto half = corners(0.5f * (k.clearance + k.touch), 0.5f);
  const ModeOutput h = DroneSkidPoints(half.data(), 4, s, st, response, 0.0f, k);
  CHECK_NEAR(h.force.y, -0.25f * h.force.z, kLoose);
  const auto none = corners(k.clearance, 0.0f);
  CHECK_NEAR(DroneSkidPoints(none.data(), 4, s, st, response, 0.0f, k).force.y, 0.0f, kTol);
  DroneSkidParams bike = k;
  bike.floor = true;
  CHECK_NEAR(DroneSkidPoints(resting.data(), 4, s, st, response, 0.0f, bike).force.y, 0.0f, kTol);
  StatsState still = s;
  still.d_velocity = {0, 0, 0};
  CHECK_NEAR(Length(Sub(DroneSkidPoints(resting.data(), 4, still, st, response, 0.0f, k).force,
                        DroneSkidPoints(none.data(), 4, still, st, response, 0.0f, k).force)),
             0.0f, kTol);
}

// HoverFlyClassic: the script FlightModeHoverFly.Update (v0.3.20) over AttitudeLift. The hover band is 1 to 7 m
// (StandardParams' defaults), so hovering is 1 - (distance - 1) / 6.

TEST(modes_hover_fly_classic_hover_pid) {
  // in the band at 2 m: hovering 1 - 1/6 = 0.833333. Holding 3 m, the height difference is 1; the hover PID (P 1)
  // gives 1, times hoverFactor 10. The fly side: lift 0, vertical speed 0.6 damped by 1, -0.6. LerpF: -0.6 + (10 +
  // 0.6) * 0.833333 = 8.233333, plus gravity 9.81 along the car's up.
  HoverFlyClassicState state;
  state.hoverHeight = 3.0f;
  StatsState s = Level();
  s.d_velocity = {0, 0, 0.6f};
  Ground g{true, 2.0f, kUp};
  Pid pitch = Gains(1, 0, 0), roll = Gains(1, 0, 0), hover = Gains(1, 0, 0);
  ModeOutput o = HoverFlyClassic(state, s, Static(), Controls{}, StandardParams{}, HoverFlyClassicParams{}, pitch, roll,
                                 hover, g, 0.5f);
  CHECK_NEAR(state.hovering, 0.8333333, kTol);
  CHECK_NEAR(state.hoverHeight, 3.0, kTol);
  CheckVec(o.force, 0, 0, 8.2333333 + 9.81, kLoose);
  CheckVec(o.torque, 0, 0, 0, kTol);
}

TEST(modes_hover_fly_classic_above_the_band) {
  // no ground: hovering 0, lift pushes straight (0.5 * liftFactor 25) less the vertical speed 2: 10.5 + 9.81. The
  // hover height still moves with lift, 0.5 * heightCorrection 0.5 a frame.
  HoverFlyClassicState state;
  state.hoverHeight = 3.0f;
  StatsState s = Level();
  s.d_velocity = {0, 0, 2.0f};
  Controls c;
  c.lift = 0.5f;
  Pid pitch = Gains(1, 0, 0), roll = Gains(1, 0, 0), hover = Gains(1, 0, 0);
  ModeOutput o = HoverFlyClassic(state, s, Static(), c, StandardParams{}, HoverFlyClassicParams{}, pitch, roll, hover,
                                 Ground{}, 0.5f);
  CHECK_NEAR(state.hovering, 0.0, kTol);
  CHECK_NEAR(state.hoverHeight, 3.25, kTol);
  CheckVec(o.force, 0, 0, 10.5 + 9.81, kLoose);
}

TEST(modes_hover_fly_classic_comes_into_the_band) {
  // from above the band (hovering 0) to ground at 4 m: hovering 0.5 and the hover height becomes 4, so no height
  // difference and no pull back to the old height
  HoverFlyClassicState state;
  state.hovering = 0.0f;
  state.hoverHeight = 1.5f;
  Pid pitch = Gains(1, 0, 0), roll = Gains(1, 0, 0), hover = Gains(1, 0, 0);
  ModeOutput o = HoverFlyClassic(state, Level(), Static(), Controls{}, StandardParams{}, HoverFlyClassicParams{}, pitch,
                                 roll, hover, Ground{true, 4.0f, kUp}, 0.5f);
  CHECK_NEAR(state.hovering, 0.5, kTol);
  CHECK_NEAR(state.hoverHeight, 4.0, kTol);
  CheckVec(o.force, 0, 0, 9.81, kLoose);
}

TEST(modes_hover_fly_classic_height_floor_and_brake) {
  // lift down at 1.2 m: 1.2 - 0.5 is under the band's floor, held at 1. Full linear brake takes the lift correction
  // away: gravity alone.
  HoverFlyClassicState state;
  state.hoverHeight = 1.2f;
  Controls c;
  c.lift = -1.0f;
  c.linearBrake = 1.0f;
  Pid pitch = Gains(1, 0, 0), roll = Gains(1, 0, 0), hover = Gains(1, 0, 0);
  ModeOutput o = HoverFlyClassic(state, Level(), Static(), c, StandardParams{}, HoverFlyClassicParams{}, pitch, roll,
                                 hover, Ground{true, 3.0f, kUp}, 0.5f);
  CHECK_NEAR(state.hoverHeight, 1.0, kTol);
  CheckVec(o.force, 0, 0, 9.81, kLoose);
}

TEST(modes_hover_fly_classic_follows_the_ground) {
  // at the band's floor (hovering 1) the attitude target is the ground's normal, 30 degrees toward forward: the same
  // pitch correction as modes_attitude_lift_tilted_normal, -1.2
  HoverFlyClassicState state;
  state.hoverHeight = 1.0f;
  Pid pitch = Gains(1, 0.2f, 0.05f), roll = Gains(1, 0.5f, 0), hover;
  ModeOutput o = HoverFlyClassic(state, Level(), Static(), Controls{}, StandardParams{}, HoverFlyClassicParams{}, pitch,
                                 roll, hover, Ground{true, 1.0f, {0, 0.5f, 0.8660254f}}, 0.5f);
  CHECK_NEAR(state.hovering, 1.0, kTol);
  CHECK_NEAR(o.torque.x, -1.2, kLoose);
}

TEST(modes_hover_fly_classic_activate) {
  // the height the car is at, no lower than the band's floor
  HoverFlyClassicState state;
  HoverFlyClassicActivate(state, 0.4f, StandardParams{});
  CHECK_NEAR(state.hoverHeight, 1.0, kTol);
  HoverFlyClassicActivate(state, 5.0f, StandardParams{});
  CHECK_NEAR(state.hoverHeight, 5.0, kTol);
}
