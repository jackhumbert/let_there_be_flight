#include "Images.hpp"

#include <cmath>

namespace Acoustics {

namespace {

constexpr float kSpeedOfSound = 343.0f;

float Dot(const Vec3 &a, const Vec3 &b) {
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

Vec3 Minus(const Vec3 &a, const Vec3 &b) {
  return {a.x - b.x, a.y - b.y, a.z - b.z};
}

float Length(const Vec3 &a, const Vec3 &b) {
  const Vec3 d = Minus(a, b);
  return std::sqrt(Dot(d, d));
}

} // namespace

Vec3 Mirror(const Vec3 &aPoint, const Plane &aSurface) {
  const float k = 2 * Dot(Minus(aPoint, aSurface.point), aSurface.normal);
  return {aPoint.x - k * aSurface.normal.x, aPoint.y - k * aSurface.normal.y, aPoint.z - k * aSurface.normal.z};
}

Echo EchoFor(const Plane &aSurface, const Vec3 &aSource, const Vec3 &aListener, const Vec3 &aVelocity,
             float aReflectivity, const Tuning &aTuning, bool aInRange) {
  const Vec3 image = Mirror(aSource, aSurface);
  const float via = Length(aListener, image), direct = Length(aListener, aSource);
  Echo e;
  e.delay = (via - direct) / kSpeedOfSound;
  // the car and listener moving at v past a still surface: the image moves at v - 2 (v.n) n, so
  // listener minus image changes at 2 (v.n) n; the direct path stays as it is
  const Vec3 outward{-aSurface.normal.x, -aSurface.normal.y, -aSurface.normal.z};
  const Vec3 toListener = Minus(aListener, image);
  e.rate = via > 1e-3f ? 2 * Dot(aVelocity, outward) * Dot(toListener, outward) / via / kSpeedOfSound : 0.0f;
  // spreading relative to the dry sound heard at a few metres; nothing beyond the probe's range
  e.gain = aInRange ? aReflectivity * aTuning.gain * (direct + aTuning.nearDistance) / (via + aTuning.nearDistance)
                    : 0.0f;
  return e;
}

Plane SquareOn(const Vec3 &aSource, const Vec3 &aOutward, float aDistance) {
  return {{aSource.x + aDistance * aOutward.x, aSource.y + aDistance * aOutward.y, aSource.z + aDistance * aOutward.z},
          {-aOutward.x, -aOutward.y, -aOutward.z}};
}

} // namespace Acoustics
