// The script's Vector4 / Quaternion natives (src/red4ext/Flight/ScriptMath.hpp).
#include "check.hpp"

#include "Flight/ScriptMath.hpp"

using namespace Flight;

namespace {
// unit-sized values in float carry about 1e-7 rounding
constexpr double kVec = 1e-6;
// the float dot error (~6e-8) is under 1e-5 degrees away from 0 and 180
constexpr double kDeg = 1e-4;

void CheckVec(const Vec3 &a, double x, double y, double z) {
  CHECK_NEAR(a.x, x, kVec);
  CHECK_NEAR(a.y, y, kVec);
  CHECK_NEAR(a.z, z, kVec);
}
} // namespace

TEST(scriptmath_normalize_zero_and_tiny) {
  Vec3 zero = Normalize({0, 0, 0});
  CHECK(zero.x == 0 && zero.y == 0 && zero.z == 0);
  // no threshold: a tiny vector still normalizes
  CHECK_NEAR(Normalize({1e-7f, 0, 0}).x, 1.0, kVec);
  CheckVec(Normalize({3, 4, 0}), 0.6, 0.8, 0);
}

TEST(scriptmath_normalize2d_keeps_z) {
  CheckVec(Normalize2D({3, 4, 5}), 0.6, 0.8, 5);
  Vec3 vertical = Normalize2D({0, 0, 7});
  CHECK(vertical.x == 0 && vertical.y == 0 && vertical.z == 7);
}

TEST(scriptmath_rotate_axes) {
  // 90 degrees about Z
  Quat q{0, 0, 0.70710678f, 0.70710678f};
  CheckVec(Rotate(q, {1, 0, 0}), 0, 1, 0);
  Vec3 forward, right, up;
  Axes(q, forward, right, up);
  CheckVec(forward, -1, 0, 0);
  CheckVec(right, 0, 1, 0);
  CheckVec(up, 0, 0, 1);

  // 120 degrees about (1,1,1): X goes to Y, Y to Z
  Quat c{0.5f, 0.5f, 0.5f, 0.5f};
  CheckVec(Rotate(c, {1, 0, 0}), 0, 1, 0);
  CheckVec(Rotate(c, {0, 1, 0}), 0, 0, 1);
  CheckVec(Rotate(Conjugate(c), Rotate(c, {0.3f, -0.7f, 2})), 0.3, -0.7, 2);

  // the zero quaternion is a script default and leaves a vector alone
  CheckVec(Rotate(Quat{}, {1, 2, 3}), 1, 2, 3);
}

TEST(scriptmath_mul_inverse_identity) {
  Quat q{0.5f, 0.5f, 0.5f, 0.5f};
  Quat r = MulInverse(q, q);
  CHECK_NEAR(r.i, 0, kVec);
  CHECK_NEAR(r.j, 0, kVec);
  CHECK_NEAR(r.k, 0, kVec);
  CHECK_NEAR(r.r, 1, kVec);
}

TEST(scriptmath_angle_between) {
  CHECK_NEAR(AngleBetweenDeg({1, 0, 0}, {1, 1, 0}), 45, kDeg);
  // the dot is 1 and clamps to 0
  CHECK(AngleBetweenDeg({2, 0, 0}, {1, 0, 0}) == 0.0f);
  // the dot is -1 and clamps to pi: 3.14159274f * 57.2957763671875f rounds to 180.0f
  CHECK_NEAR(AngleBetweenDeg({1, 0, 0}, {-1, 0, 0}), 180, kDeg);
  // a zero vector gives dot 0
  CHECK_NEAR(AngleBetweenDeg({0, 0, 0}, {1, 0, 0}), 90, kDeg);
}

TEST(scriptmath_angle_around_axis_sign) {
  Vec3 z{0, 0, 1};
  CHECK_NEAR(AngleDegAroundAxis({0, 1, 0}, {1, 0, 0}, z), -90, kDeg);
  CHECK_NEAR(AngleDegAroundAxis({1, 0, 0}, {0, 1, 0}, z), 90, kDeg);
  CHECK_NEAR(AngleDegAroundAxis({1, 1, 0}, {1, 0, 0}, z), -45, kDeg);
  // the out-of-plane parts are removed first
  CHECK_NEAR(AngleDegAroundAxis({0, 1, 5}, {1, 0, -3}, z), -90, kDeg);
}

TEST(scriptmath_angle_around_axis_on_axis) {
  // a on the axis projects to zero: dot 0, cross 0, which counts as positive
  CHECK_NEAR(AngleDegAroundAxis({0, 0, 1}, {1, 0, 0}, {0, 0, 1}), 90, kDeg);
}

TEST(scriptmath_mul_order) {
  // 90 degrees about X times 90 degrees about Z, worked by the Hamilton product: the two orders differ
  const float h = 0.70710678f;
  Quat x{h, 0, 0, h}, z{0, 0, h, h};
  Quat xz = Mul(x, z), zx = Mul(z, x);
  CHECK_NEAR(xz.i, 0.5, 1e-6);
  CHECK_NEAR(xz.j, -0.5, 1e-6);
  CHECK_NEAR(xz.k, 0.5, 1e-6);
  CHECK_NEAR(xz.r, 0.5, 1e-6);
  CHECK_NEAR(zx.i, 0.5, 1e-6);
  CHECK_NEAR(zx.j, 0.5, 1e-6);
  CHECK_NEAR(zx.k, 0.5, 1e-6);
  CHECK_NEAR(zx.r, 0.5, 1e-6);
  // a * b rotates by b first, then a, as Rotate composes
  Vec3 v{0.3f, -0.7f, 2.0f};
  Vec3 once = Rotate(xz, v), twice = Rotate(x, Rotate(z, v));
  CHECK_NEAR(once.x, twice.x, 1e-6);
  CHECK_NEAR(once.y, twice.y, 1e-6);
  CHECK_NEAR(once.z, twice.z, 1e-6);
  // MulInverse undoes the second rotation
  Quat back = MulInverse(xz, z);
  CHECK_NEAR(back.i, x.i, 1e-6);
  CHECK_NEAR(back.j, x.j, 1e-6);
  CHECK_NEAR(back.k, x.k, 1e-6);
  CHECK_NEAR(back.r, x.r, 1e-6);
}

TEST(scriptmath_rotate_axis) {
  const float halfPi = 1.57079637f;
  // right-handed: Z turned about Y goes to +X, Z turned about X goes to -Y
  CheckVec(RotateAxis({0, 0, 1}, {0, 1, 0}, halfPi), 1, 0, 0);
  CheckVec(RotateAxis({0, 0, 1}, {1, 0, 0}, halfPi), 0, -1, 0);
  // angle 0 is exact
  Vec3 v{0.3f, -0.2f, 0.9f};
  Vec3 same = RotateAxis(v, {0, 1, 0}, 0.0f);
  CHECK(same.x == v.x && same.y == v.y && same.z == v.z);
  // sin 0.15 and cos 0.15
  CheckVec(RotateAxis({0, 0, 1}, {0, 1, 0}, 0.15f), 0.149438, 0, 0.988771);
}

TEST(scriptmath_interpolate_lerp) {
  CheckVec(Interpolate({0, 0, 1}, {1, 0, 0}, 0.25f), 0.25, 0, 0.75);
  Vec3 a{0.3f, -0.2f, 0.9f};
  Vec3 at0 = Interpolate(a, {5, 6, 7}, 0.0f);
  CHECK(at0.x == a.x && at0.y == a.y && at0.z == a.z);
  CHECK_NEAR(Lerp(0.25f, 2, 6), 3, 1e-6);
  CHECK_NEAR(Lerp(1.0f, 2, 6), 6, 1e-6);
}

TEST(scriptmath_rotate_axis_diagonal) {
  // Rodrigues: about n = (1, 1, 0) / sqrt 2, Z is perpendicular, so it turns to Z cos + (n x Z) sin, and
  // n x Z = (h, -h, 0) with h = 1 / sqrt 2; a quarter turn either way
  const float h = 0.70710678f, quarter = 1.57079633f;
  Vec3 n{h, h, 0};
  Vec3 a = RotateAxis({0, 0, 1}, n, quarter);
  CHECK_NEAR(a.x, h, 1e-6);
  CHECK_NEAR(a.y, -h, 1e-6);
  CHECK_NEAR(a.z, 0, 1e-6);
  Vec3 b = RotateAxis({0, 0, 1}, n, -quarter);
  CHECK_NEAR(b.x, -h, 1e-6);
  CHECK_NEAR(b.y, h, 1e-6);
  CHECK_NEAR(b.z, 0, 1e-6);
  // a vector along the axis does not move
  Vec3 c = RotateAxis({2 * h, 2 * h, 0}, n, 1.0f);
  CHECK_NEAR(c.x, 2 * h, 1e-6);
  CHECK_NEAR(c.y, 2 * h, 1e-6);
  CHECK_NEAR(c.z, 0, 1e-6);
}
