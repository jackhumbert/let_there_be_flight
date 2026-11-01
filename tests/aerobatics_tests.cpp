// The aerobatics maths (src/red4ext/Navigation/Aerobatics): FlightAttitude.RotationError / Across and FlightManeuver's
// Create, Ease, RollUp, BarrelOffset and Sample. Expected values are worked by hand from FlightAerobatics.reds.
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
