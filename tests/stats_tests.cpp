// FlightStats.UpdateDynamic as pure code (src/red4ext/Flight/Stats.hpp).
#include "check.hpp"

#include "Flight/Stats.hpp"

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
