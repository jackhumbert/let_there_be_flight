#include "Camera/Cinematic.hpp"

#include <algorithm>
#include <cmath>

namespace Cinematic {

namespace F = Flight;

F::Quat OrientationFromDir(const F::Vec3 &dir) {
  const F::Vec3 f = F::Normalize(dir);
  const F::Vec3 r = F::Normalize(F::Cross(f, {0.0f, 0.0f, 1.0f}));
  const F::Vec3 u = F::Cross(r, f);
  // the matrix with r, f, u as columns: m[row][col]
  const float m00 = r.x, m01 = f.x, m02 = u.x;
  const float m10 = r.y, m11 = f.y, m12 = u.y;
  const float m20 = r.z, m21 = f.z, m22 = u.z;
  // to a quaternion (Shepperd: divide by the largest of the four candidates)
  float w, x, y, z;
  const float trace = m00 + m11 + m22;
  if (trace > 0.0f) {
    const float s = std::sqrt(trace + 1.0f) * 2.0f;
    w = 0.25f * s;
    x = (m21 - m12) / s;
    y = (m02 - m20) / s;
    z = (m10 - m01) / s;
  } else if (m00 > m11 && m00 > m22) {
    const float s = std::sqrt(std::max(1.0f + m00 - m11 - m22, 0.000001f)) * 2.0f;
    w = (m21 - m12) / s;
    x = 0.25f * s;
    y = (m01 + m10) / s;
    z = (m02 + m20) / s;
  } else if (m11 > m22) {
    const float s = std::sqrt(std::max(1.0f + m11 - m00 - m22, 0.000001f)) * 2.0f;
    w = (m02 - m20) / s;
    x = (m01 + m10) / s;
    y = 0.25f * s;
    z = (m12 + m21) / s;
  } else {
    const float s = std::sqrt(std::max(1.0f + m22 - m00 - m11, 0.000001f)) * 2.0f;
    w = (m10 - m01) / s;
    x = (m02 + m20) / s;
    y = (m12 + m21) / s;
    z = 0.25f * s;
  }
  if (w < 0.0f) {
    w = -w;
    x = -x;
    y = -y;
    z = -z;
  }
  return {x, y, z, w};
}

F::Vec3 TransformPoint(const Pose &p, const F::Vec3 &v) { return F::Add(p.position, F::Rotate(p.orientation, v)); }

Pose AimAtCar(Pose camera, const F::Vec3 &com, const OrientFromDir &orient) {
  const F::Vec3 toCar = F::Sub(com, camera.position);
  const float distance = F::Length(toCar);
  if (distance < 0.01f)
    return camera;
  if (distance < kMinDistance)
    camera.position = F::Sub(com, F::Mul(toCar, kMinDistance / distance));
  // the line as it was before the camera moved out: the same direction either way
  if (std::fabs(toCar.z) / distance < 0.98f)
    camera.orientation = orient(toCar);
  return camera;
}

float InitialScale(float speed) { return std::clamp(std::fabs(speed) / kRoadSpeed, 1.0f, kMaxScale); }

float FitAhead(const Pose &shot, const F::Vec3 &offset, float scale, const std::function<bool(const F::Vec3 &)> &blocked,
               const TransformPointFn &transform) {
  while (scale > 1.0f && blocked(transform(shot, {offset.x, offset.y * scale, offset.z})))
    scale = std::max(1.0f, scale * kShrink);
  return scale;
}

float StopLimit(float maxDistanceFromCamera, float reach) { return std::max(maxDistanceFromCamera, kReachMargin * reach); }

bool DistanceStop(const F::Vec3 &vehicle, const F::Vec3 &camera, float maxDistanceFromCamera, float reach) {
  return F::Length(F::Sub(vehicle, camera)) >= StopLimit(maxDistanceFromCamera, reach);
}

} // namespace Cinematic
