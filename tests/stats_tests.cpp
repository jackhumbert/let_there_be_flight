// FlightStats.UpdateDynamic as pure code (src/red4ext/Flight/Stats.hpp).
#include "check.hpp"

#include "Flight/Stats.hpp"

#include <cmath>

using namespace Flight;

namespace {
// values under 15 in float carry about 1e-6 rounding; positions near 30 and the 60 carry about 4e-6
constexpr double kSmall = 1e-5;
constexpr double kLarge = 1e-4;

const Quat kIdentity{0, 0, 0, 1};
// 90 degrees about Z
const Quat kYaw90{0, 0, 0.70710678f, 0.70710678f};

void CheckVec(const Vec3 &a, double x, double y, double z, double tol = kSmall) {
  CHECK_NEAR(a.x, x, tol);
  CHECK_NEAR(a.y, y, tol);
  CHECK_NEAR(a.z, z, tol);
}

StatsInput Input(const Quat &q, Vec3 velocity) {
  StatsInput in;
  in.orientation = q;
  in.velocity = velocity;
  return in;
}
} // namespace

TEST(stats_update_at_rest) {
  StatsState s;
  StatsInput in = Input(kIdentity, {0, 0, 0});
  in.origin = {10, 20, 30};
  in.centerOfMass = {0, 0.5f, 0.25f};
  UpdateDynamic(s, in);
  CheckVec(s.d_position, 10, 20.5, 30.25, kLarge);
  CheckVec(s.d_forward, 0, 1, 0);
  CheckVec(s.d_right, 1, 0, 0);
  CheckVec(s.d_up, 0, 0, 1);
  CheckVec(s.d_localUp, 0, 0, 1);
  CHECK_NEAR(s.d_speed, 0, kSmall);
  CHECK_NEAR(s.d_speedRatio, 0, kSmall);
  CHECK_NEAR(s.d_speed2D, 0, kSmall);
  // zero vectors come back unchanged
  CheckVec(s.d_direction, 0, 0, 0);
  CheckVec(s.d_direction2D, 0, 0, 0);
  // the default before this frame
  CheckVec(s.d_visualPosition, 0, 0, 0);
}

TEST(stats_update_yawed) {
  StatsState s;
  StatsInput in = Input(kYaw90, {0, 10, 0});
  in.centerOfMass = {0, 1, 0};
  UpdateDynamic(s, in);
  // the centre of mass (0,1,0) turned 90 degrees about Z is (-1,0,0)
  CheckVec(s.d_position, -1, 0, 0);
  CheckVec(s.d_forward, -1, 0, 0);
  // world +Y seen from the yawed body is its local +X
  CheckVec(s.d_localVelocity, 10, 0, 0);
  CheckVec(s.d_localDirection, 1, 0, 0);
  CHECK_NEAR(s.d_speed, 10, kSmall);
  CHECK_NEAR(s.d_speedRatio, 0.1, kSmall);
  CHECK_NEAR(s.d_speedRatioSquared, 0.01, kSmall);
  CHECK_NEAR(s.d_speed2D, 10, kSmall);
  CheckVec(s.d_direction, 0, 1, 0);
  CheckVec(s.d_direction2D, 0, 1, 0);
  CheckVec(s.d_velocity2D, 0, 10, 0);
}

TEST(stats_update_climb_keeps_z) {
  StatsState s;
  UpdateDynamic(s, Input(kIdentity, {3, 4, 12}));
  CHECK_NEAR(s.d_speed, 13, kLarge);
  CHECK_NEAR(s.d_speed2D, 5, kLarge);
  CheckVec(s.d_direction, 3.0 / 13, 4.0 / 13, 12.0 / 13, kLarge);
  // Normalize2D keeps Z, and velocity2D multiplies all three components
  CheckVec(s.d_direction2D, 0.6, 0.8, 12, kLarge);
  CheckVec(s.d_velocity2D, 3, 4, 60, kLarge);
  CheckVec(s.d_localDirection2D, 0.6, 0.8, 12, kLarge);
}

TEST(stats_update_vertical) {
  StatsState s;
  UpdateDynamic(s, Input(kIdentity, {0, 0, -5}));
  CheckVec(s.d_direction, 0, 0, -1);
  CHECK_NEAR(s.d_speed2D, 0, 1e-6);
  // a zero horizontal length leaves the vector unchanged
  CheckVec(s.d_direction2D, 0, 0, -5);
  // zero times Z, which may be -0
  CheckVec(s.d_velocity2D, 0, 0, 0, 1e-6);
}

TEST(stats_update_angular_two_frames) {
  StatsState s;
  StatsInput in = Input(kIdentity, {0, 0, 0});
  in.worldAngularVelocity = {0, 0, 1};
  in.origin = {1, 2, 3};
  UpdateDynamic(s, in);
  CheckVec(s.d_angularVelocity, 0, 0, 1);
  CheckVec(s.d_angularAcceleration, 0, 0, 1);

  in.orientation = kYaw90;
  in.worldAngularVelocity = {1, 0, 0};
  in.origin = {4, 5, 6};
  UpdateDynamic(s, in);
  // world +X seen from the yawed body is its local -Y; the acceleration is that minus the last (0,0,1)
  CheckVec(s.d_angularVelocity, 0, -1, 0);
  CheckVec(s.d_angularAcceleration, 0, -1, -1);
  CHECK_NEAR(s.d_lastOrientation.i, 0, 1e-6);
  CHECK_NEAR(s.d_lastOrientation.j, 0, 1e-6);
  CHECK_NEAR(s.d_lastOrientation.k, 0, 1e-6);
  CHECK_NEAR(s.d_lastOrientation.r, 1, 1e-6);
  // q * conjugate(identity) is q
  CHECK_NEAR(s.d_orientationChange.i, kYaw90.i, 1e-6);
  CHECK_NEAR(s.d_orientationChange.j, kYaw90.j, 1e-6);
  CHECK_NEAR(s.d_orientationChange.k, kYaw90.k, 1e-6);
  CHECK_NEAR(s.d_orientationChange.r, kYaw90.r, 1e-6);
  CheckVec(s.d_visualPosition, 1, 2, 3);
  CheckVec(s.d_position, 4, 5, 6);

  // then nose up 45 degrees about X, from the yaw: the change is now * conjugate(last), worked by the
  // Hamilton product with a = sin 22.5, b = cos 22.5, h = sin 45: (a h, a h, -b h, b h)
  const float a = 0.38268343f, b = 0.92387953f, h = 0.70710678f;
  in.orientation = {a, 0, 0, b};
  UpdateDynamic(s, in);
  CHECK_NEAR(s.d_orientationChange.i, a * h, 1e-6);
  CHECK_NEAR(s.d_orientationChange.j, a * h, 1e-6);
  CHECK_NEAR(s.d_orientationChange.k, -b * h, 1e-6);
  CHECK_NEAR(s.d_orientationChange.r, b * h, 1e-6);
}

TEST(stats_update_first_frame_change) {
  // the last orientation is the all-zero default, so the change is zero, not the orientation
  StatsState s;
  UpdateDynamic(s, Input(kYaw90, {0, 0, 0}));
  CHECK_NEAR(s.d_orientationChange.i, 0, 1e-6);
  CHECK_NEAR(s.d_orientationChange.j, 0, 1e-6);
  CHECK_NEAR(s.d_orientationChange.k, 0, 1e-6);
  CHECK_NEAR(s.d_orientationChange.r, 0, 1e-6);
}

TEST(stats_update_pitched) {
  // nose up 45 degrees about X: forward (0, h, h), so forward2D keeps its Z: (0, 1, h); the world up seen
  // from the body is (0, h, h) (it would be (0, -h, h) without the conjugate); flying world +Y at 10, the
  // body sees (0, 10h, -10h), whose 2D direction is (0, 1, -10h) where the world one is (0, 1, 0)
  const float a = 0.38268343f, b = 0.92387953f, h = 0.70710678f;
  StatsState s;
  UpdateDynamic(s, Input({a, 0, 0, b}, {0, 10, 0}));
  CheckVec(s.d_forward, 0, h, h);
  CheckVec(s.d_forward2D, 0, 1, h);
  CheckVec(s.d_localUp, 0, h, h);
  CheckVec(s.d_localVelocity, 0, 10 * h, -10 * h, kLarge);
  CheckVec(s.d_localDirection2D, 0, 1, -10 * h, kLarge);
  CheckVec(s.d_direction2D, 0, 1, 0);
}

TEST(stats_hud_level) {
  // acos(0) times the degree constant is 90.0000025, hence the tolerance
  StatsState s;
  UpdateDynamic(s, Input(kIdentity, {0, 0, 0}));
  CHECK_NEAR(s.d_pitchDeg, 0, 1e-3);
  CHECK_NEAR(s.d_rollDeg, 0, 1e-3);
}

TEST(stats_hud_nose_up) {
  // 30 degrees about X: forward (0, cos 30, sin 30) is 60 degrees from up, so the pitch is 30; the body up
  // stays in the plane of the axis the roll is measured about, so the roll is 0
  const float s15 = 0.25881905f, c15 = 0.96592583f;
  StatsState s;
  UpdateDynamic(s, Input({s15, 0, 0, c15}, {0, 0, 0}));
  CHECK_NEAR(s.d_pitchDeg, 30, 1e-3);
  CHECK_NEAR(s.d_rollDeg, 0, 1e-3);
}

TEST(stats_hud_rolled) {
  // 20 degrees about Y (the forward axis): localUp = Rotate(Conjugate(q), Z) = (-sin 20, 0, cos 20); from it to
  // world up, a x b = (0, sin 20, 0) points along +Y, the axis, so the angle is +20
  const float s10 = 0.17364818f, c10 = 0.98480775f;
  StatsState s;
  UpdateDynamic(s, Input({0, s10, 0, c10}, {0, 0, 0}));
  CheckVec(s.d_localUp, -0.34202014, 0, 0.93969262);
  CHECK_NEAR(s.d_rollDeg, 20, 1e-3);
  CHECK_NEAR(s.d_pitchDeg, 0, 1e-3);
}

TEST(stats_hud_nose_down_and_both) {
  const float s15 = 0.25881905f, c15 = 0.96592583f, s10 = 0.17364818f, c10 = 0.98480775f;
  const double rad = 3.14159265358979 / 180;
  {
    // -30 degrees about X: forward (0, cos 30, -sin 30), pitch -30; the body up is again in the axis plane
    StatsState s;
    UpdateDynamic(s, Input({-s15, 0, 0, c15}, {0, 0, 0}));
    CHECK_NEAR(s.d_pitchDeg, -30, 1e-3);
    CHECK_NEAR(s.d_rollDeg, 0, 1e-3);
  }
  {
    // 30 degrees about X, then 20 about Y (q = qRoll * qPitch): forward = Ry(20) (0, cos 30, sin 30) =
    // (sin 30 sin 20, cos 30, sin 30 cos 20), so the pitch is asin(0.5 cos 20); localUp =
    // Rx(-30) Ry(-20) Z = (-sin 20, 0.5 cos 20, cos 30 cos 20), whose part in the plane of the Y axis is
    // (-sin 20, cos 30 cos 20), tilted +atan(tan 20 / cos 30) from up (a x b along +Y)
    StatsState s;
    Quat q = Mul({0, s10, 0, c10}, {s15, 0, 0, c15});
    UpdateDynamic(s, Input(q, {0, 0, 0}));
    CheckVec(s.d_forward, 0.5 * std::sin(20 * rad), std::cos(30 * rad), 0.5 * std::cos(20 * rad));
    CHECK_NEAR(s.d_pitchDeg, std::asin(0.5 * std::cos(20 * rad)) / rad, 1e-3);
    CHECK_NEAR(s.d_rollDeg, std::atan(std::tan(20 * rad) / std::cos(30 * rad)) / rad, 1e-3);
  }
}
