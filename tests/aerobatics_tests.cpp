// The aerobatics maths (src/red4ext/Navigation/Aerobatics): FlightAttitude.RotationError / Across, FlightManeuver's
// Create, Ease, RollUp, BarrelOffset and Sample, and FlightModeAutopilot's bank and pitch eases and attitude torque.
// Expected values are worked by hand from the scripts.
#include "check.hpp"

#include "Navigation/Aerobatics.hpp"

#include <cmath>

using namespace Navigation;

namespace {
constexpr double kPi = 3.14159265358979323846;

void CheckVec(const Flight::Vec3 &v, double x, double y, double z, double tol) {
  CHECK_NEAR(v.x, x, tol);
  CHECK_NEAR(v.y, y, tol);
  CHECK_NEAR(v.z, z, tol);
}

// Rotation error of the world frame to the given target axes.
Flight::Vec3 ErrorTo(Flight::Vec3 tr, Flight::Vec3 tf, Flight::Vec3 tu) {
  return RotationError({1, 0, 0}, {0, 1, 0}, {0, 0, 1}, tr, tf, tu);
}

// Loop / Immelmann / split-S from the origin heading +X at 20 m/s, accel 10: radius 35 (400 / 10 clamped), half roll 1.5 s.
Maneuver FromOrigin(ManeuverKind aKind, float aSide) {
  return CreateManeuver(aKind, aSide, {0, 0, 0}, {20, 0, 0}, 10.0f, 1.5f);
}

// pi r / v for radius 35 and speed 20, the script's expression and order
const float kHalf = 3.1415927f * 35.0f / 20.0f;
// v / r, the loop's angular rate
constexpr double kW = 20.0 / 35.0;
} // namespace

TEST(aerobatics_shape_rotation_error) {
  // identical frames: sinHalf is 0 (trace 3, w = 1)
  CheckVec(ErrorTo({1, 0, 0}, {0, 1, 0}, {0, 0, 1}), 0, 0, 0, 1e-5);
  // 90 degrees about Z: trace 1 > 0, w = x = ... = 0.7071, the angle 2 atan2(0.7071, 0.7071) = pi/2
  CheckVec(ErrorTo({0, 1, 0}, {-1, 0, 0}, {0, 0, 1}), 0, 0, kPi / 2, 1e-5);
  // 90 degrees about X: the target's forward is the first frame's up
  CheckVec(ErrorTo({1, 0, 0}, {0, 0, 1}, {0, -1, 0}), kPi / 2, 0, 0, 1e-5);
  // 180 degrees about X: trace -1, m00 is the largest, s = 2 sqrt(4) = 4, x = 1
  CheckVec(ErrorTo({1, 0, 0}, {0, -1, 0}, {0, 0, -1}), kPi, 0, 0, 1e-5);
  // 180 degrees about Y: m00 = -1 is not the largest, m11 beats m22 (1 > -1), y = 1
  CheckVec(ErrorTo({-1, 0, 0}, {0, 1, 0}, {0, 0, -1}), 0, kPi, 0, 1e-5);
  // 180 degrees about Z: m00 = m11 = -1 (not strictly greater), m11 > m22 false, z = 1
  CheckVec(ErrorTo({-1, 0, 0}, {0, -1, 0}, {0, 0, 1}), 0, 0, kPi, 1e-5);
  // 200 degrees about X is 160 the other way: the m00 branch gives w = cos(100 deg) < 0, so all four flip and the
  // vector is -160 degrees about X
  const double c = std::cos(200.0 * kPi / 180.0), s = std::sin(200.0 * kPi / 180.0);
  CheckVec(ErrorTo({1, 0, 0}, {0, (float)c, (float)s}, {0, (float)-s, (float)c}), -160.0 * kPi / 180.0, 0, 0, 1e-5);
  // the same turn in a tilted first frame is expressed in the first frame's coordinates, not the world's: the first
  // frame is turned 90 degrees about Z (r = (0,1,0), f = (-1,0,0)) and the target adds 90 degrees about the first
  // frame's X (its tf is the first frame's up)
  CheckVec(RotationError({0, 1, 0}, {-1, 0, 0}, {0, 0, 1}, {0, 1, 0}, {0, 0, 1}, {1, 0, 0}), kPi / 2, 0, 0, 1e-5);
}

TEST(aerobatics_shape_across) {
  CheckVec(Across({0, 1, 1}, {0, 1, 0}), 0, 0, 1, 1e-6);
  // along f it is zero; not normalised: (3,4,0) across (1,0,0) keeps its length 4
  CheckVec(Across({2, 0, 0}, {1, 0, 0}), 0, 0, 0, 1e-6);
  CheckVec(Across({3, 4, 0}, {1, 0, 0}), 0, 4, 0, 1e-6);
}

TEST(aerobatics_shape_create) {
  // Loop: 20^2 / 10 = 40 clamps to 35; duration 2 pi 35 / 20
  Maneuver loop = CreateManeuver(ManeuverKind::Loop, -0.3f, {1, 2, 3}, {20, 0, 0}, 10.0f, 1.5f);
  CHECK(loop.kind == ManeuverKind::Loop);
  CHECK_NEAR(loop.side, -1, 1e-6);
  CHECK_NEAR(loop.speed, 20, 1e-4);
  CHECK_NEAR(loop.radius, 35, 1e-4);
  CHECK_NEAR(loop.duration, 10.99557, 1e-4);
  CHECK_NEAR(loop.rollTime, 1.5, 1e-6);
  CHECK_NEAR(loop.time, 0, 1e-6);
  CheckVec(loop.origin, 1, 2, 3, 1e-6);
  // side 0 counts as +1
  CHECK_NEAR(CreateManeuver(ManeuverKind::Loop, 0.0f, {}, {20, 0, 0}, 10.0f, 1.5f).side, 1, 1e-6);
  // Immelmann at 5 m/s: speed 10 (floor), 100 / 10 = 10 clamps up to 14; duration pi 14 / 10 + the half roll 1.5
  Maneuver imm = CreateManeuver(ManeuverKind::Immelmann, 1.0f, {}, {5, 0, 0}, 10.0f, 1.5f);
  CHECK_NEAR(imm.speed, 10, 1e-4);
  CHECK_NEAR(imm.radius, 14, 1e-4);
  CHECK_NEAR(imm.duration, 5.89823, 1e-4);
  // between the clamps, and the speed is the horizontal length: (12,16,7) is 20, not 21.1; 400 / 16 = 25
  Maneuver mid = CreateManeuver(ManeuverKind::Loop, 1.0f, {}, {12, 16, 7}, 16.0f, 1.5f);
  CHECK_NEAR(mid.speed, 20, 1e-4);
  CHECK_NEAR(mid.radius, 25, 1e-4);
  CHECK_NEAR(mid.duration, 2.0 * kPi * 25.0 / 20.0, 1e-4);
  // split-S uses the same duration as Immelmann: half circle + roll (pi 20 / 20 + 1.5)
  Maneuver splitS = CreateManeuver(ManeuverKind::SplitS, 1.0f, {}, {20, 0, 0}, 20.0f, 1.5f);
  CHECK_NEAR(splitS.radius, 20, 1e-4);
  CHECK_NEAR(splitS.duration, kPi + 1.5, 1e-4);
  // the overlay shapes ignore speed and accel
  Maneuver aileron = CreateManeuver(ManeuverKind::AileronRoll, 1.0f, {}, {20, 0, 0}, 10.0f, 1.5f);
  CHECK_NEAR(aileron.duration, 2.2, 1e-5);
  CHECK_NEAR(aileron.radius, 0, 1e-6);
  Maneuver barrel = CreateManeuver(ManeuverKind::BarrelRoll, 1.0f, {}, {20, 0, 0}, 10.0f, 1.5f);
  CHECK_NEAR(barrel.radius, 5, 1e-6);
  CHECK_NEAR(barrel.duration, 3.5, 1e-5);
  CHECK_NEAR(loop.dir.x, 1, 1e-6);
  CheckVec(loop.right, 0, -1, 0, 1e-6);

  CHECK(IsOverlay(aileron));
  CHECK(IsOverlay(barrel));
  CHECK(!IsOverlay(loop));
  CHECK(!IsOverlay(imm));
  CHECK(!IsOverlay(splitS));
  CHECK(!IsOverlay(Maneuver{}));
  CHECK(!IsDone(aileron, 2.1f));
  CHECK(IsDone(aileron, 2.2f));
  CHECK(IsDone(aileron, 3.0f));
}

TEST(aerobatics_shape_dir_keeps_z) {
  // the game's Normalize2D keeps Z (Flight/ScriptMath.hpp): a climbing car's plane tilts, reproduced
  Maneuver m = CreateManeuver(ManeuverKind::Loop, 1.0f, {}, {20, 0, 2}, 10.0f, 1.5f);
  CheckVec(m.dir, 1, 0, 2, 1e-6);
  CheckVec(m.right, 0, -1, 0, 1e-6);
  CHECK_NEAR(m.speed, 20, 1e-5);
}

TEST(aerobatics_shape_ease) {
  float rate = -1;
  CHECK_NEAR(ManeuverEase(1.0f, 2.0f, rate), 0.5, 1e-5);
  CHECK_NEAR(rate, kPi / 4, 1e-5);
  // past the end: 1, no rate
  CHECK_NEAR(ManeuverEase(3.0f, 2.0f, rate), 1.0, 1e-5);
  CHECK(std::fabs(rate) < 1e-6);
  // before the start: 0, no rate
  CHECK_NEAR(ManeuverEase(-1.0f, 2.0f, rate), 0.0, 1e-5);
  CHECK(std::fabs(rate) < 1e-6);
  // a quarter of the way: 0.5 (1 - cos(pi/4)) and pi/4 sin(pi/4)
  CHECK_NEAR(ManeuverEase(0.5f, 2.0f, rate), 0.5 * (1.0 - std::sqrt(0.5)), 1e-5);
  CHECK_NEAR(rate, kPi / 4 * std::sqrt(0.5), 1e-5);
  // the rate scales with 1 / d: d 4 at the midpoint is pi/8
  ManeuverEase(2.0f, 4.0f, rate);
  CHECK_NEAR(rate, kPi / 8, 1e-5);
}

TEST(aerobatics_shape_roll_up) {
  const Flight::Vec3 f{0, 1, 0}, u{0, 0, 1};
  Maneuver m = CreateManeuver(ManeuverKind::AileronRoll, 1.0f, {}, {20, 0, 0}, 10.0f, 1.5f);
  // ease 0.25 at a third of the way: phi = pi/2, the up vector falls onto Cross(f, u) = +X
  CheckVec(RollUp(m, 2.2f / 3.0f, f, u), 1, 0, 0, 1e-5);
  // the midpoint: phi = pi, upside down
  CheckVec(RollUp(m, 1.1f, f, u), 0, 0, -1, 1e-5);
  // ease 0.75: phi = 3 pi / 2
  CheckVec(RollUp(m, 2.2f * 2.0f / 3.0f, f, u), -1, 0, 0, 1e-5);
  // the ends are upright
  CheckVec(RollUp(m, 0.0f, f, u), 0, 0, 1, 1e-5);
  CheckVec(RollUp(m, 5.0f, f, u), 0, 0, 1, 1e-5);
  // the other side rolls the other way
  Maneuver left = CreateManeuver(ManeuverKind::AileronRoll, -1.0f, {}, {20, 0, 0}, 10.0f, 1.5f);
  CheckVec(RollUp(left, 2.2f / 3.0f, f, u), -1, 0, 0, 1e-5);
}

TEST(aerobatics_shape_barrel) {
  Flight::Vec3 offset, offsetVelocity, up;
  Maneuver m = CreateManeuver(ManeuverKind::BarrelRoll, 1.0f, {}, {20, 0, 0}, 10.0f, 1.5f);
  // the midpoint: theta = pi, offset = (z (1 - cos) + s sin) 5 = (0, 0, 10); the rate of theta there is
  // 2 pi (0.5 pi / 3.5), so the velocity is s cos(pi) * 5 thetaRate = (0, 5 * pi^2 / 3.5, 0) (14.09943)
  BarrelOffset(m, 1.75f, offset, offsetVelocity, up);
  CheckVec(offset, 0, 0, 10, 1e-4);
  CheckVec(offsetVelocity, 0, 5.0 * kPi * kPi / 3.5, 0, 1e-4);
  CheckVec(up, 0, 0, -5, 1e-4);
  // a quarter of the time: ease 0.5 (1 - cos(pi/4)), theta = pi (1 - sqrt(0.5)), s = (0, -1, 0)
  const double theta = kPi * (1.0 - std::sqrt(0.5));
  const double thetaRate = 2.0 * kPi * (0.5 * kPi / 3.5) * std::sqrt(0.5);
  BarrelOffset(m, 0.875f, offset, offsetVelocity, up);
  CheckVec(offset, 0, -5.0 * std::sin(theta), 5.0 * (1.0 - std::cos(theta)), 1e-4);
  CheckVec(offsetVelocity, 0, -5.0 * thetaRate * std::cos(theta), 5.0 * thetaRate * std::sin(theta), 1e-4);
  CheckVec(up, 0, 5.0 * std::sin(theta), 5.0 - 5.0 * (1.0 - std::cos(theta)), 1e-4);
  // the other side swings to +Y
  Maneuver left = CreateManeuver(ManeuverKind::BarrelRoll, -1.0f, {}, {20, 0, 0}, 10.0f, 1.5f);
  BarrelOffset(left, 0.875f, offset, offsetVelocity, up);
  CheckVec(offset, 0, 5.0 * std::sin(theta), 5.0 * (1.0 - std::cos(theta)), 1e-4);
  // the start is on the route
  BarrelOffset(m, 0.0f, offset, offsetVelocity, up);
  CheckVec(offset, 0, 0, 0, 1e-4);
  CheckVec(up, 0, 0, 5, 1e-4);
}

TEST(aerobatics_shape_sample) {
  Flight::Vec3 pos, vel, acc, fwd, up;
  // the loop's top: a = pi
  Maneuver loop = FromOrigin(ManeuverKind::Loop, 1.0f);
  SampleManeuver(loop, kHalf, pos, vel, acc, fwd, up);
  CheckVec(pos, 0, 0, 70, 1e-3);
  CheckVec(vel, -20, 0, 0, 1e-3);
  CheckVec(acc, 0, 0, -20.0 * kW, 1e-3);
  CheckVec(fwd, -1, 0, 0, 1e-5);
  CheckVec(up, 0, 0, -1, 1e-5);

  // a quarter, heading +Y from (100, 200, 300): a = pi/2
  Maneuver north = CreateManeuver(ManeuverKind::Loop, 1.0f, {100, 200, 300}, {0, 20, 0}, 10.0f, 1.5f);
  SampleManeuver(north, kHalf / 2.0f, pos, vel, acc, fwd, up);
  CheckVec(pos, 100, 235, 335, 1e-3);
  CheckVec(vel, 0, 0, 20, 1e-3);
  CheckVec(acc, 0, -20.0 * kW, 0, 1e-3);
  CheckVec(fwd, 0, 0, 1, 1e-5);
  CheckVec(up, 0, -1, 0, 1e-5);
  // the loop closes: past the end it stays at the start; before the start it clamps to it
  SampleManeuver(loop, loop.duration + 5.0f, pos, vel, acc, fwd, up);
  CheckVec(pos, 0, 0, 0, 1e-3);
  CheckVec(vel, 20, 0, 0, 1e-3);
  SampleManeuver(loop, -1.0f, pos, vel, acc, fwd, up);
  CheckVec(pos, 0, 0, 0, 1e-3);

  // Immelmann: the half loop is the loop's first half, with acceleration
  Maneuver imm = FromOrigin(ManeuverKind::Immelmann, 1.0f);
  SampleManeuver(imm, kHalf / 2.0f, pos, vel, acc, fwd, up);
  CheckVec(pos, 35, 0, 35, 1e-3);
  CheckVec(acc, -20.0 * kW, 0, 0, 1e-3);
  // half-way through the roll (psi = pi/2): back along -X from the top, up is s = (0, -1, 0) for side 1
  SampleManeuver(imm, kHalf + 0.75f, pos, vel, acc, fwd, up);
  CheckVec(pos, -15, 0, 70, 1e-3);
  CheckVec(vel, -20, 0, 0, 1e-3);
  CheckVec(acc, 0, 0, 0, 1e-6);
  CheckVec(fwd, -1, 0, 0, 1e-5);
  CheckVec(up, 0, -1, 0, 1e-5);
  // at the top the roll has not begun: inverted
  SampleManeuver(imm, kHalf, pos, vel, acc, fwd, up);
  CheckVec(pos, 0, 0, 70, 1e-3);
  CheckVec(up, 0, 0, -1, 1e-5);
  // after the roll: upright
  SampleManeuver(imm, kHalf + 1.5f, pos, vel, acc, fwd, up);
  CheckVec(pos, -30, 0, 70, 1e-3);
  CheckVec(up, 0, 0, 1, 1e-5);
  // the other side rolls through +Y
  SampleManeuver(FromOrigin(ManeuverKind::Immelmann, -1.0f), kHalf + 0.75f, pos, vel, acc, fwd, up);
  CheckVec(up, 0, 1, 0, 1e-5);

  // split-S: rolling inverted straight on first (psi = pi/2 at 0.75 s)
  Maneuver split = FromOrigin(ManeuverKind::SplitS, 1.0f);
  SampleManeuver(split, 0.75f, pos, vel, acc, fwd, up);
  CheckVec(pos, 15, 0, 0, 1e-3);
  CheckVec(vel, 20, 0, 0, 1e-3);
  CheckVec(acc, 0, 0, 0, 1e-6);
  CheckVec(up, 0, -1, 0, 1e-5);
  // the other side rolls through +Y
  SampleManeuver(FromOrigin(ManeuverKind::SplitS, -1.0f), 0.75f, pos, vel, acc, fwd, up);
  CheckVec(up, 0, 1, 0, 1e-5);
  // before the start the position holds at the origin
  SampleManeuver(split, -1.0f, pos, vel, acc, fwd, up);
  CheckVec(pos, 0, 0, 0, 1e-3);
  // the end of the roll is the start of the circle, inverted
  SampleManeuver(split, 1.5f, pos, vel, acc, fwd, up);
  CheckVec(pos, 30, 0, 0, 1e-3);
  CheckVec(up, 0, 0, -1, 1e-5);
  // a quarter of the circle: b = pi/2, down and round
  SampleManeuver(split, 1.5f + kHalf / 2.0f, pos, vel, acc, fwd, up);
  CheckVec(pos, 65, 0, -35, 1e-3);
  CheckVec(vel, 0, 0, -20, 1e-3);
  CheckVec(acc, -20.0 * kW, 0, 0, 1e-3);
  CheckVec(fwd, 0, 0, -1, 1e-5);
  CheckVec(up, -1, 0, 0, 1e-5);
  // the bottom of the circle: b = pi, heading back, upright
  SampleManeuver(split, 1.5f + kHalf, pos, vel, acc, fwd, up);
  CheckVec(pos, 30, 0, -70, 1e-3);
  CheckVec(vel, -20, 0, 0, 1e-3);
  CheckVec(acc, 0, 0, 20.0 * kW, 1e-3);
  CheckVec(up, 0, 0, 1, 1e-5);
  // past the circle's end it holds there
  SampleManeuver(split, 1.5f + kHalf + 3.0f, pos, vel, acc, fwd, up);
  CheckVec(pos, 30, 0, -70, 1e-3);

  // an overlay shape (and None) is straight on
  Maneuver aileron = CreateManeuver(ManeuverKind::AileronRoll, 1.0f, {5, 6, 7}, {20, 0, 0}, 10.0f, 1.5f);
  SampleManeuver(aileron, 1.0f, pos, vel, acc, fwd, up);
  CheckVec(pos, 25, 6, 7, 1e-3);
  CheckVec(vel, 20, 0, 0, 1e-3);
  CheckVec(acc, 0, 0, 0, 1e-6);
  CheckVec(fwd, 1, 0, 0, 1e-5);
  CheckVec(up, 0, 0, 1, 1e-5);
  SampleManeuver(FromOrigin(ManeuverKind::None, 1.0f), 1.0f, pos, vel, acc, fwd, up);
  CheckVec(pos, 20, 0, 0, 1e-3);
}

// ---- the bank and pitch eases and the attitude torque (FlightModeAutopilot.reds) ----

namespace {
// The wanted up vector for a bank of a radians about the nose +Y: level (0,0,1), side (1,0,0).
Flight::Vec3 UpBanked(float a) { return {std::sin(a), 0.0f, std::cos(a)}; }

// A car sitting at the world axes, not turning.
Flight::StatsState IdentityStats() {
  Flight::StatsState s;
  s.d_right = {1, 0, 0};
  s.d_forward = {0, 1, 0};
  s.d_up = {0, 0, 1};
  s.d_orientation = {0, 0, 0, 1};
  return s;
}

AttitudeControlState IdentityTargets() {
  AttitudeControlState st;
  st.targetRight = {1, 0, 0};
  st.targetForward = {0, 1, 0};
  st.targetUp = {0, 0, 1};
  return st;
}

Flight::StatsStatic Car() {
  Flight::StatsStatic ss;
  ss.mass = 1500.0f;
  ss.momentOfInertia = {2850, 2850, 2850};
  return ss;
}

// Holding the nose along +Y, top +Z, with no command: the settled car.
AttitudeControlInput Settled() {
  AttitudeControlInput in;
  in.dt = 0.1f;
  in.fwd = {0, 1, 0};
  in.up = {0, 0, 1};
  in.raceDecel = 20.0f;
  return in;
}
} // namespace

TEST(aerobatics_control_ease_bank) {
  // defaults: frequency 2.5, rate max 1, accel max 3. fwd +Y: level (0,0,1), side (1,0,0)
  EaseParams p;
  EaseState s;
  // wanted atan2(0.5, 0.866) = 0.523599; f^2 * error = 3.27 clamps to 3; rate 0 + 3 * 0.1 = 0.3; bank 0.03
  Flight::Vec3 up = EaseBank(s, p, 0.1f, {0, 1, 0}, {0.5f, 0, 0.8660254f});
  CHECK_NEAR(s.bankRateNow, 0.3, 1e-5);
  CHECK_NEAR(s.bankAngle, 0.03, 1e-5);
  CheckVec(up, std::sin(0.03), 0, std::cos(0.03), 1e-5);
  // dt 1: rate 0 + 3 = 3 clamps to the rate max 1; bank 1.0
  s = EaseState{};
  up = EaseBank(s, p, 1.0f, {0, 1, 0}, {0.5f, 0, 0.8660254f});
  CHECK_NEAR(s.bankRateNow, 1.0, 1e-5);
  CHECK_NEAR(s.bankAngle, 1.0, 1e-5);
  CheckVec(up, std::sin(1.0), 0, std::cos(1.0), 1e-5);
  // the negative side: the accel clamps to -3, then the rate to -1
  s = EaseState{};
  up = EaseBank(s, p, 0.1f, {0, 1, 0}, {-0.5f, 0, 0.8660254f});
  CHECK_NEAR(s.bankRateNow, -0.3, 1e-5);
  CHECK_NEAR(s.bankAngle, -0.03, 1e-5);
  CheckVec(up, -std::sin(0.03), 0, std::cos(0.03), 1e-5);
  s = EaseState{};
  EaseBank(s, p, 1.0f, {0, 1, 0}, {-0.5f, 0, 0.8660254f});
  CHECK_NEAR(s.bankRateNow, -1.0, 1e-5);
  CHECK_NEAR(s.bankAngle, -1.0, 1e-5);

  // nothing clamps, and the three params differ: frequency 2, accel max 10, rate max 0.5, dt 0.1, bank 0.1, rate 0.1,
  // wanted 0.3. error 0.2; accel = 4 * 0.2 - 2 * 2 * 0.1 = 0.4; rate 0.1 + 0.04 = 0.14; bank 0.1 + 0.014 = 0.114
  EaseParams q;
  q.raceRollFrequency = 2.0f;
  q.raceRollAccelMax = 10.0f;
  q.raceRollRateMax = 0.5f;
  s = EaseState{};
  s.bankAngle = 0.1f;
  s.bankRateNow = 0.1f;
  up = EaseBank(s, q, 0.1f, {0, 1, 0}, UpBanked(0.3f));
  CHECK_NEAR(s.bankRateNow, 0.14, 1e-5);
  CHECK_NEAR(s.bankAngle, 0.114, 1e-5);
  CheckVec(up, std::sin(0.114), 0, std::cos(0.114), 1e-5);
  // the same with a rate max of 0.12: the rate clamps to it, the bank 0.1 + 0.012
  q.raceRollRateMax = 0.12f;
  s.bankAngle = 0.1f;
  s.bankRateNow = 0.1f;
  EaseBank(s, q, 0.1f, {0, 1, 0}, UpBanked(0.3f));
  CHECK_NEAR(s.bankRateNow, 0.12, 1e-5);
  CHECK_NEAR(s.bankAngle, 0.112, 1e-5);
  // the same with an accel max of 0.2: accel 0.4 clamps to 0.2, rate 0.12 (not clamped by the 0.5 rate max)
  q.raceRollRateMax = 0.5f;
  q.raceRollAccelMax = 0.2f;
  s.bankAngle = 0.1f;
  s.bankRateNow = 0.1f;
  EaseBank(s, q, 0.1f, {0, 1, 0}, UpBanked(0.3f));
  CHECK_NEAR(s.bankRateNow, 0.12, 1e-5);
  CHECK_NEAR(s.bankAngle, 0.112, 1e-5);

  // a bank near pi: the bank is 3.14 and the wanted -3.1, so the error wraps from -6.24 to +0.0431853 (unwrapped it
  // would clamp the accel to -3 and send the bank the other way). accel = 6.25 * 0.0431853 = 0.269908; rate 0.0269908;
  // bank 3.14 + 0.00269908 = 3.1426991 is past pi and wraps to -3.1404862
  s = EaseState{};
  s.bankAngle = 3.14f;
  up = EaseBank(s, p, 0.1f, {0, 1, 0}, UpBanked(-3.1f));
  CHECK_NEAR(s.bankRateNow, 0.0269908, 1e-5);
  CHECK_NEAR(s.bankAngle, -3.1404862, 1e-5);
  CheckVec(up, -0.0011064, 0, -0.9999994, 1e-5);

  // the nose vertical: no frame, the rate is zeroed and the top comes back as it was
  s = EaseState{};
  s.bankAngle = 0.4f;
  s.bankRateNow = 0.7f;
  up = EaseBank(s, p, 0.1f, {0, 0, 1}, {0.5f, 0.1f, 0.8f});
  CheckVec(up, 0.5, 0.1, 0.8, 1e-6);
  CHECK_NEAR(s.bankRateNow, 0.0, 1e-6);
  CHECK_NEAR(s.bankAngle, 0.4, 1e-6);
}

TEST(aerobatics_control_ease_pitch) {
  // frequency 2.5, so f^2 = 6.25; dt 0.1: a first step from rest turns 6.25 * 0.01 = 0.0625 of the wanted pitch
  EaseParams p;
  p.racePitchFrequency = 2.5f;
  p.racePitchAccelMax = 100.0f;
  p.racePitchRateMax = 10.0f;
  EaseState s;
  // off the line: wanted asin(0.5) = 0.523599; pitch 0.0625 * 0.523599 = 0.0327249. The level direction keeps the
  // nose's Z: (0,1,0.5), not (0,1,0); the result is (0, cos p, 0.5 cos p + sin p)
  Flight::Vec3 r = EasePitch(s, p, 0.1f, {0, 0.8660254f, 0.5f}, false, {0, 0, 0}, {1, 0, 0});
  CHECK_NEAR(s.pitchAngle, 0.0327249, 1e-5);
  CHECK_NEAR(s.pitchRateNow, 0.327249, 1e-5);
  CheckVec(r, 0, 0.9994646, 0.5324514, 1e-5);
  // on the line, tangent (0,0.6,0.8): slope asin(0.8) = 0.927295; level * 0.6 + up * 0.8 = (0,0.6,1.1), steep 1, so
  // 0.4 * that + 0.6 * (1,0,0) = (0.6,0.24,0.44), normalised (0.767467,0.306987,0.562809); wanted asin(0.562809) =
  // 0.597780; pitch 0.0625 * 0.597780 = 0.0373613 (the nose's own asin(0.5) would give 0.0327249)
  s = EaseState{};
  r = EasePitch(s, p, 0.1f, {0, 0.8660254f, 0.5f}, true, {0, 0.6f, 0.8f}, {1, 0, 0});
  CHECK_NEAR(s.pitchAngle, 0.0373613, 1e-5);
  CheckVec(r, 0, std::cos(0.0373613), 0.5 * std::cos(0.0373613) + std::sin(0.0373613), 1e-5);
  // the tangent is ignored off the line
  s = EaseState{};
  EasePitch(s, p, 0.1f, {0, 0.8660254f, 0.5f}, false, {0, 0.6f, 0.8f}, {1, 0, 0});
  CHECK_NEAR(s.pitchAngle, 0.0327249, 1e-5);

  // the pitch clamps: from 1.49 at rate 10, wanted 0.523599: accel = 6.25 * (0.523599 - 1.49) - 50 = -56.04, rate
  // 10 - 5.604 = 4.396, pitch 1.49 + 0.4396 = 1.9296 clamps to 1.5
  s = EaseState{};
  s.pitchAngle = 1.49f;
  s.pitchRateNow = 10.0f;
  EasePitch(s, p, 0.1f, {0, 0.8660254f, 0.5f}, false, {0, 0, 0}, {1, 0, 0});
  CHECK_NEAR(s.pitchRateNow, 4.396, 1e-3);
  CHECK_NEAR(s.pitchAngle, 1.5, 1e-6);
  // and the other way: -1.49 at rate -10, wanted -0.523599: rate -4.396, pitch -1.9296 clamps to -1.5
  s = EaseState{};
  s.pitchAngle = -1.49f;
  s.pitchRateNow = -10.0f;
  EasePitch(s, p, 0.1f, {0, 0.8660254f, -0.5f}, false, {0, 0, 0}, {1, 0, 0});
  CHECK_NEAR(s.pitchRateNow, -4.396, 1e-3);
  CHECK_NEAR(s.pitchAngle, -1.5, 1e-6);

  // the accel and rate clamps, with the params all different: frequency 2, accel max 1, rate max 0.05. accel =
  // 4 * 0.523599 = 2.094 clamps to 1; rate 0.1 clamps to 0.05; pitch 0.005 (swapping the two maxima gives 0.0005)
  EaseParams q;
  q.racePitchFrequency = 2.0f;
  q.racePitchAccelMax = 1.0f;
  q.racePitchRateMax = 0.05f;
  s = EaseState{};
  EasePitch(s, q, 0.1f, {0, 0.8660254f, 0.5f}, false, {0, 0, 0}, {1, 0, 0});
  CHECK_NEAR(s.pitchRateNow, 0.05, 1e-6);
  CHECK_NEAR(s.pitchAngle, 0.005, 1e-6);
  s = EaseState{};
  EasePitch(s, q, 0.1f, {0, 0.8660254f, -0.5f}, false, {0, 0, 0}, {1, 0, 0});
  CHECK_NEAR(s.pitchRateNow, -0.05, 1e-6);
  CHECK_NEAR(s.pitchAngle, -0.005, 1e-6);
  // the accel clamp alone: accel max 0.2 clamps 2.094, rate 0.02 (under the 0.05 rate max), pitch 0.002
  q.racePitchAccelMax = 0.2f;
  s = EaseState{};
  EasePitch(s, q, 0.1f, {0, 0.8660254f, 0.5f}, false, {0, 0, 0}, {1, 0, 0});
  CHECK_NEAR(s.pitchRateNow, 0.02, 1e-6);
  CHECK_NEAR(s.pitchAngle, 0.002, 1e-6);

  // the level direction keeps the nose's Z, so its length is 1 or more whenever the nose has a horizontal part: even a
  // nose 0.1 off vertical does not take the sync branch. level (1,0,0.994987); wanted asin(0.994987) = 1.470620;
  // accel = 6.25 * (1.470620 - 0.2) - 2 * 2.5 * 0.4 = 5.9414; rate 0.4 + 0.59414 = 0.99414; pitch 0.2 + 0.099414 =
  // 0.299414; the result is (cos p, 0, 0.994987 cos p + sin p)
  s = EaseState{};
  s.pitchAngle = 0.2f;
  s.pitchRateNow = 0.4f;
  r = EasePitch(s, p, 0.1f, {0.1f, 0, 0.994987f}, false, {0, 0, 0}, {1, 0, 0});
  CHECK_NEAR(s.pitchRateNow, 0.99414, 1e-4);
  CHECK_NEAR(s.pitchAngle, 0.299414, 1e-5);
  CheckVec(r, std::cos(0.299414), 0, 0.994987 * std::cos(0.299414) + std::sin(0.299414), 1e-4);
  // the sync branch is the nose with no horizontal part and a length under 0.5 (Normalize2D leaves it as it is): it
  // comes back as it is, the pitch follows it (asin 0.4 = 0.411517) and the rate is zeroed
  s = EaseState{};
  s.pitchAngle = 0.2f;
  s.pitchRateNow = 0.4f;
  r = EasePitch(s, p, 0.1f, {0, 0, 0.4f}, false, {0, 0, 0}, {1, 0, 0});
  CheckVec(r, 0, 0, 0.4, 1e-6);
  CHECK_NEAR(s.pitchAngle, 0.411517, 1e-5);
  CHECK_NEAR(s.pitchRateNow, 0.0, 1e-6);
  // a straight-up unit nose does not sync either: level (0,0,1) has length 1, wanted asin(1) = pi/2
  s = EaseState{};
  EasePitch(s, p, 0.1f, {0, 0, 1}, false, {0, 0, 0}, {1, 0, 0});
  CHECK_NEAR(s.pitchAngle, 0.0625 * kPi / 2, 1e-5);
}

TEST(aerobatics_control_unsteep) {
  // |z| = 0.6 is not steep: unchanged
  CheckVec(Unsteep({0, 0.8f, 0.6f}, {1, 0, 0}), 0, 0.8, 0.6, 1e-6);
  // z = 0.8: steep = (0.8 - 0.6) / 0.3 = 0.6667; fwd * (1 - 0.4) + (1,0,0) * 0.4 = (0.4,0.36,0.48), length
  // sqrt(0.16 + 0.1296 + 0.2304) = 0.721110, so (0.554700,0.499230,0.665640)
  CheckVec(Unsteep({0, 0.6f, 0.8f}, {1, 0, 0}), 0.554700, 0.499230, 0.665640, 1e-4);
  // pointing down, z = -0.9797959: steep clamps to 1; fwd * 0.4 + (1,0,0) * 0.6 = (0.6,0.08,-0.391918), length
  // 0.721110, so (0.832050,0.110940,-0.543493)
  CheckVec(Unsteep({0, 0.2f, -0.9797959f}, {1, 0, 0}), 0.832050, 0.110940, -0.543493, 1e-4);
}

TEST(aerobatics_control_impulse) {
  WorldInertia m;
  m.x = {1, 2, 3};
  m.y = {4, 5, 6};
  m.z = {7, 8, 9};
  bool valid = true;
  // invalid: zero, and the flag says so, whatever the rows hold
  Flight::Vec3 r = AngularImpulse(m, {1, 1, 1}, valid);
  CHECK(!valid);
  CheckVec(r, 0, 0, 0, 1e-6);
  // valid: out.x = X.x a.x + Y.x a.y + Z.x a.z, out.y = X.y a.x + Y.y a.y + Z.y a.z, ...; so a unit axis picks a
  // row (a transposed product would give the columns, (1,4,7) for the first)
  m.valid = true;
  r = AngularImpulse(m, {1, 0, 0}, valid);
  CHECK(valid);
  CheckVec(r, 1, 2, 3, 1e-6);
  r = AngularImpulse(m, {0, 1, 0}, valid);
  CheckVec(r, 4, 5, 6, 1e-6);
  r = AngularImpulse(m, {0, 0, 1}, valid);
  CheckVec(r, 7, 8, 9, 1e-6);
  // (1,2,3): x = 1 + 8 + 21 = 30, y = 2 + 10 + 24 = 36, z = 3 + 12 + 27 = 42
  r = AngularImpulse(m, {1, 2, 3}, valid);
  CheckVec(r, 30, 36, 42, 1e-6);
}

TEST(aerobatics_control_settled) {
  AttitudeControlState st = IdentityTargets();
  AttitudeControlParams p;
  AttitudeControlOutput out = AttitudeControl(st, p, Car(), IdentityStats(), Settled());
  CheckVec(out.torque, 0, 0, 0, 1e-5);
  CHECK_NEAR(out.attitudeError, 0.0, 1e-5);
  // accel 0, drag 0: gravity only; surge between dragAccel - raceDecel = -20 and 15
  CheckVec(out.thrust, 0, 0, 9.81, 1e-5);
  CheckVec(out.rateRef, 0, 0, 0, 1e-5);
  CheckVec(st.omegaRef, 0, 0, 0, 1e-5);
  CheckVec(st.targetForward, 0, 1, 0, 1e-6);
}

TEST(aerobatics_control_yaw) {
  // the nose turned 0.2 about Z in 0.1 s: step z 0.2, omega 2, omegaRef 2 (clamp(0.1 / 0.05) = 1 takes all of it)
  const float a = 0.2f;
  AttitudeControlInput in = Settled();
  in.fwd = {-std::sin(a), std::cos(a), 0};
  AttitudeControlState st = IdentityTargets();
  AttitudeControlParams p;
  AttitudeControlOutput out = AttitudeControl(st, p, Car(), IdentityStats(), in);
  CheckVec(out.rateRef, 0, 0, 2, 1e-3);
  CHECK_NEAR(out.attitudeError, 0.2, 1e-3);
  // alpha z = 0.2 * 5^2 + 2 * 2 * 0.8 * 5 = 5 + 16 = 21, capped to 20. Without the inertia: 20 * 2850 / (0.9 * 1500)
  // = 42.2222, and the give-back rateRef * 2 = 4 is capped to 3.5: 45.7222
  CheckVec(out.torque, 0, 0, 45.7222, 1e-3);
  CheckVec(st.targetForward, -std::sin(a), std::cos(a), 0, 1e-6);
  CheckVec(st.targetRight, std::cos(a), std::sin(a), 0, 1e-6);
  CheckVec(st.omegaRef, 0, 0, 2, 1e-3);

  // with the world inertia diag 3000: impulse 60000, local z / (1500 * 1) = 40, plus the give-back 3.5: 43.5
  in.inertia.valid = true;
  in.inertia.x = {3000, 0, 0};
  in.inertia.y = {0, 3000, 0};
  in.inertia.z = {0, 0, 3000};
  st = IdentityTargets();
  out = AttitudeControl(st, p, Car(), IdentityStats(), in);
  CheckVec(out.torque, 0, 0, 43.5, 1e-3);

  // a frame time under 0.001 is floored to it, as FlyAerobatic does: omega z = 200, omegaRef = 200 * 0.02 = 4
  in.dt = 0.0f;
  st = IdentityTargets();
  out = AttitudeControl(st, p, Car(), IdentityStats(), in);
  CHECK_NEAR(st.omegaRef.z, 4.0, 1e-2);

  // pitch: the nose up 0.1 about X (top (0,-sin,cos)); error x 0.1, omegaRef x 1, alpha x = 0.1 * 25 + 1 * 8 = 10.5.
  // The tensor (1.5,2,2.5) and the inertia (2000,3000,4000) differ per axis; torque x = 10.5 * 2000 / (2 * 1500 * 1.5)
  // = 4.66667 plus the give-back 1 * 2 = 2 (under the 3.5 cap): 6.66667
  Flight::StatsStatic ss = Car();
  ss.momentOfInertia = {2000, 3000, 4000};
  in = Settled();
  in.thrusterTensor = {1.5f, 2.0f, 2.5f};
  in.fwd = {0, std::cos(0.1f), std::sin(0.1f)};
  in.up = {0, -std::sin(0.1f), std::cos(0.1f)};
  st = IdentityTargets();
  out = AttitudeControl(st, p, ss, IdentityStats(), in);
  CheckVec(out.rateRef, 1, 0, 0, 1e-3);
  CheckVec(out.torque, 6.66667, 0, 0, 1e-3);
  // with the world inertia diag (2000,3000,4000): impulse x 21000, local x / (1500 * 1.5) = 9.33333, plus 2: 11.33333
  in.inertia.valid = true;
  in.inertia.x = {2000, 0, 0};
  in.inertia.y = {0, 3000, 0};
  in.inertia.z = {0, 0, 4000};
  st = IdentityTargets();
  out = AttitudeControl(st, p, ss, IdentityStats(), in);
  CheckVec(out.torque, 11.33333, 0, 0, 1e-3);

  // roll: the top banked 0.1 about the nose (top (sin,0,cos)); error y 0.1, omegaRef y 1, alpha y 10.5. Tensor y 2:
  // torque y = 10.5 * 3000 / (0.5 * 1500 * 2) = 21, plus the give-back 2: 23
  in = Settled();
  in.thrusterTensor = {1.5f, 2.0f, 2.5f};
  in.up = UpBanked(0.1f);
  st = IdentityTargets();
  out = AttitudeControl(st, p, ss, IdentityStats(), in);
  CheckVec(out.rateRef, 0, 1, 0, 1e-3);
  CheckVec(out.torque, 0, 23, 0, 1e-3);
  // tensor y 0.5 is raised to 1 by max(tY, 1): 31500 / (0.5 * 1500 * 1) = 42, plus 2: 44 (not 84)
  in.thrusterTensor = {1.5f, 0.5f, 2.5f};
  st = IdentityTargets();
  out = AttitudeControl(st, p, ss, IdentityStats(), in);
  CheckVec(out.torque, 0, 44, 0, 1e-3);
  // yaw with the same car and tensor: z gain 0.9 and tZ 2.5: 20 * 4000 / (0.9 * 1500 * 2.5) = 23.7037 (alpha 21 is
  // capped to 20), plus the give-back 3.5
  in = Settled();
  in.thrusterTensor = {1.5f, 2.0f, 2.5f};
  in.fwd = {-std::sin(a), std::cos(a), 0};
  st = IdentityTargets();
  out = AttitudeControl(st, p, ss, IdentityStats(), in);
  CheckVec(out.torque, 0, 0, 27.2037, 1e-3);
}

TEST(aerobatics_control_rate_term) {
  // the nose up 0.1 about X, dt 0.1: error x 0.1, omegaRef x 1 so rateRef (1,0,0). The car turns at (0.3,-0.2,0.5), so
  // rateRef - rate = (0.7, 0.2, -0.5) and alpha = error * 25 + (rateRef - rate) * (2 * 0.8 * 5 = 8)
  // = (2.5 + 5.6, 1.6, -4) = (8.1, 1.6, -4); length 9.17 is under the cap 20.
  // Gains 2, 0.5, 0.9, mass 1500, tensor (1.5,2,2.5), inertia (2000,3000,4000):
  // x = 8.1 * 2000 / (2 * 1500 * 1.5) = 3.6, y = 1.6 * 3000 / (0.5 * 1500 * 2) = 3.2,
  // z = -4 * 4000 / (0.9 * 1500 * 2.5) = -4.740741. The give-back rateRef * 2 = (2,0,0) is under 3.5.
  Flight::StatsStatic ss = Car();
  ss.momentOfInertia = {2000, 3000, 4000};
  Flight::StatsState stats = IdentityStats();
  stats.d_angularVelocity = {0.3f, -0.2f, 0.5f};
  AttitudeControlInput in = Settled();
  in.thrusterTensor = {1.5f, 2.0f, 2.5f};
  in.fwd = {0, std::cos(0.1f), std::sin(0.1f)};
  in.up = {0, -std::sin(0.1f), std::cos(0.1f)};
  AttitudeControlState st = IdentityTargets();
  AttitudeControlParams p;
  AttitudeControlOutput out = AttitudeControl(st, p, ss, stats, in);
  CheckVec(out.rateRef, 1, 0, 0, 1e-3);
  CheckVec(out.torque, 5.6, 3.2, -4.740741, 1e-3);

  // the rate alone: no error, no rateRef, rate (0.3,-0.2,0.5): alpha = -rate * 8 = (-2.4, 1.6, -4), no give-back
  // x = -2.4 * 2000 / 4500 = -1.066667, y = 1.6 * 3000 / 1500 = 3.2, z = -4 * 4000 / 3375 = -4.740741
  in = Settled();
  in.thrusterTensor = {1.5f, 2.0f, 2.5f};
  st = IdentityTargets();
  out = AttitudeControl(st, p, ss, stats, in);
  CheckVec(out.torque, -1.066667, 3.2,-4.740741, 1e-3);
}

TEST(aerobatics_control_give_back) {
  // no error and a rate equal to rateRef, so alpha is 0 and the torque is the give-back alone. dt 0.025: omegaRef takes
  // half of the way (dt / 0.05 = 0.5) from the omegaRef it held to the target's rate 0, so it is half of the one set.
  AttitudeControlParams p;
  p.generalDampFactorAngular = 1.5f;
  p.generalYawDampFactor = 0.5f;
  p.generalDampFactorAngularMax = 4.0f;
  AttitudeControlInput in = Settled();
  in.dt = 0.025f;

  // under the cap: omegaRef (2,-2,4) halves to rateRef (1,-1,2); x1.5 = (1.5,-1.5,3), z x0.5 = 1.5; length 2.598 < 4
  Flight::StatsState stats = IdentityStats();
  stats.d_angularVelocity = {1, -1, 2};
  AttitudeControlState st = IdentityTargets();
  st.omegaRef = {2, -2, 4};
  AttitudeControlOutput out = AttitudeControl(st, p, Car(), stats, in);
  CheckVec(out.rateRef, 1, -1, 2, 1e-4);
  CheckVec(out.torque, 1.5, -1.5, 1.5, 1e-4);

  // over the cap: omegaRef (4,-2,8) halves to rateRef (2,-1,4); x1.5 = (3,-1.5,6), z x0.5 = 3: (3,-1.5,3), length
  // sqrt(9 + 2.25 + 9) = 4.5 > 4, so it is scaled by 4 / 4.5 along its own direction: (2.666667,-1.333333,2.666667)
  stats.d_angularVelocity = {2, -1, 4};
  st = IdentityTargets();
  st.omegaRef = {4, -2, 8};
  out = AttitudeControl(st, p, Car(), stats, in);
  CheckVec(out.rateRef, 2, -1, 4, 1e-4);
  CheckVec(out.torque, 2.666667, -1.333333, 2.666667, 1e-4);
}

TEST(aerobatics_control_turned_inertia) {
  // the car turned 90 degrees about Z (Rotate sends (x,y,z) to (-y,x,z)), holding the attitude wanted, so error and
  // rateRef are 0. Rate (0.3,-0.2,0.5): alpha = -rate * 8 = (-2.4, 1.6, -4), under the cap. Into the world:
  // (-1.6, -2.4, -4). The symmetric world inertia rows (2000,300,100), (300,3000,200), (100,200,4000) give the impulse
  // x = -3200 - 720 - 400 = -4320, y = -480 - 7200 - 800 = -8480, z = -160 - 480 - 16000 = -16640. Back with the
  // Conjugate ((x,y,z) to (y,-x,z)): local (-8480, 4320, -16640). Divide by mass * tensor (1.5, 2, 2.5) * 1500:
  // x = -8480 / 2250 = -3.768889, y = 4320 / 3000 = 1.44, z = -16640 / 3750 = -4.437333. No give-back.
  Flight::StatsState stats = IdentityStats();
  stats.d_orientation = {0, 0, 0.70710678f, 0.70710678f};
  stats.d_right = {0, 1, 0};
  stats.d_forward = {-1, 0, 0};
  stats.d_up = {0, 0, 1};
  stats.d_angularVelocity = {0.3f, -0.2f, 0.5f};
  AttitudeControlInput in = Settled();
  in.fwd = {-1, 0, 0};
  in.thrusterTensor = {1.5f, 2.0f, 2.5f};
  in.inertia.valid = true;
  in.inertia.x = {2000, 300, 100};
  in.inertia.y = {300, 3000, 200};
  in.inertia.z = {100, 200, 4000};
  AttitudeControlState st;
  st.targetRight = {0, 1, 0};
  st.targetForward = {-1, 0, 0};
  st.targetUp = {0, 0, 1};
  AttitudeControlOutput out = AttitudeControl(st, AttitudeControlParams{}, Car(), stats, in);
  CHECK_NEAR(out.attitudeError, 0.0, 1e-4);
  CheckVec(out.torque, -3.768889, 1.44, -4.437333, 1e-3);
}

TEST(aerobatics_control_thrust_clamp) {
  // sway 4, surge 12, lift 20, gravity factor 1.5 (g = 14.715)
  AttitudeControlParams p;
  p.standardModeSwayFactor = 4.0f;
  p.standardModeSurgeFactor = 12.0f;
  p.standardModeLiftFactor = 20.0f;
  p.gravityFactor = 1.5f;
  AttitudeControlState st = IdentityTargets();
  AttitudeControlInput in = Settled();
  in.dragAccel = 2.0f;
  in.raceDecel = 12.0f;
  // upper ends: x 10 -> 4, y 30 -> 12, z -40 + 14.715 = -25.285 -> g - lift = -5.285
  in.accel = {10, 30, -40};
  AttitudeControlOutput out = AttitudeControl(st, p, Car(), IdentityStats(), in);
  CheckVec(out.thrust, 4, 12, -5.285, 1e-4);
  // lower ends: x -10 -> -4, y -30 -> dragAccel - raceDecel = -10, z 40 + 14.715 = 54.715 -> g + lift = 34.715
  in.accel = {-10, -30, 40};
  out = AttitudeControl(st, p, Car(), IdentityStats(), in);
  CheckVec(out.thrust, -4, -10, 34.715, 1e-4);
  // inside the limits it passes through, the drag added: (1 + 1, 2 + 1, 3 + 1 + 14.715)
  in.accel = {1, 2, 3};
  in.drag = {1, 1, 1};
  out = AttitudeControl(st, p, Car(), IdentityStats(), in);
  CheckVec(out.thrust, 2, 3, 18.715, 1e-4);

  // the defaults (sway 5, surge 15, lift 25): accel (10,30,-40) gives (5,15,9.81 - 25 = -15.19)
  AttitudeControlParams d;
  in = Settled();
  in.accel = {10, 30, -40};
  out = AttitudeControl(st, d, Car(), IdentityStats(), in);
  CheckVec(out.thrust, 5, 15, -15.19, 1e-4);
  // the car turned 90 degrees about Z: the world (0,10,9.81) is local (10,0,9.81) (Conjugate turns it back), and x 10
  // clamps to 5; with the identity orientation it would be y that clamps
  Flight::StatsState turned = IdentityStats();
  turned.d_orientation = {0, 0, 0.7071068f, 0.7071068f};
  turned.d_right = {0, 1, 0};
  turned.d_forward = {-1, 0, 0};
  in = Settled();
  in.fwd = {-1, 0, 0};
  in.accel = {0, 10, 0};
  st = IdentityTargets();
  out = AttitudeControl(st, d, Car(), turned, in);
  CheckVec(out.thrust, 5, 0, 9.81, 1e-4);
}

TEST(aerobatics_control_sync_bank) {
  // a shape: the bank follows the top given, the rate is zeroed. fwd +Y, top (0.5,0,0.866): atan2(0.5, 0.866) = 0.523599
  AttitudeControlState st = IdentityTargets();
  st.bankAngle = 0.0f;
  st.bankRateNow = 0.7f;
  AttitudeControlInput in = Settled();
  in.shape = true;
  in.up = {0.5f, 0, 0.8660254f};
  AttitudeControl(st, AttitudeControlParams{}, Car(), IdentityStats(), in);
  CHECK_NEAR(st.bankAngle, 0.523599, 1e-5);
  CHECK_NEAR(st.bankRateNow, 0.0, 1e-6);
  // not a shape: the bank and its rate are left alone
  st = IdentityTargets();
  st.bankAngle = 0.1f;
  st.bankRateNow = 0.7f;
  in.shape = false;
  AttitudeControl(st, AttitudeControlParams{}, Car(), IdentityStats(), in);
  CHECK_NEAR(st.bankAngle, 0.1, 1e-6);
  CHECK_NEAR(st.bankRateNow, 0.7, 1e-6);

  // SyncBank itself: the nose vertical has no frame, so the angle stays and the rate is zeroed
  float bank = 0.4f, rate = 0.7f;
  SyncBank(bank, rate, {0, 0, 1}, {0, 1, 0});
  CHECK_NEAR(bank, 0.4, 1e-6);
  CHECK_NEAR(rate, 0.0, 1e-6);
  // and SyncPitch: asin of the nose's Z, rate zeroed
  EaseState s;
  s.pitchRateNow = 0.4f;
  SyncPitch(s, {0, 0.8660254f, 0.5f});
  CHECK_NEAR(s.pitchAngle, 0.523599, 1e-5);
  CHECK_NEAR(s.pitchRateNow, 0.0, 1e-6);
  // BankFrame: the level direction and the side for fwd +Y; false from a nose 0.1 off vertical (length 0.1 < 0.2)
  Flight::Vec3 level, side;
  CHECK(BankFrame({0, 1, 0}, level, side));
  CheckVec(level, 0, 0, 1, 1e-6);
  CheckVec(side, 1, 0, 0, 1e-6);
  CHECK(!BankFrame({0.1f, 0, 0.994987f}, level, side));
}
