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
  return AimAtCarLead(camera, com, com, orient);
}

float StopLimit(float maxDistanceFromCamera, float reach) { return std::max(maxDistanceFromCamera, kReachMargin * reach); }

bool DistanceStop(const F::Vec3 &vehicle, const F::Vec3 &camera, float maxDistanceFromCamera, float reach) {
  return F::Length(F::Sub(vehicle, camera)) >= StopLimit(maxDistanceFromCamera, reach);
}

namespace {
// v made level and unit; zero when v is (nearly) vertical
F::Vec3 Level(const F::Vec3 &v, float aMin) {
  const float l = F::Length2D(v);
  return l > aMin ? F::Vec3{v.x / l, v.y / l, 0.0f} : F::Vec3{0.0f, 0.0f, 0.0f};
}
} // namespace

F::Vec3 LevelHeading(const F::Vec3 &prev, const F::Quat &car, const F::Vec3 &velocity, float aDt) {
  const F::Vec3 forward = F::Rotate(car, {0.0f, 1.0f, 0.0f});
  F::Vec3 target = Level(forward, 0.3f);
  if (F::Length2D(target) == 0.0f)
    target = Level(velocity, 1.0f);
  const F::Vec3 held = Level(prev, 0.5f);
  if (F::Length2D(target) == 0.0f)
    return held;
  if (F::Length2D(held) == 0.0f)
    return target;
  const float k = 1.0f - std::exp(-std::max(aDt, 0.0f) / kHeadingTime);
  const F::Vec3 eased = Level(F::Add(held, F::Mul(F::Sub(target, held), k)), 0.01f);
  // turned right round in one step (a reversal): nothing to ease through
  return F::Length2D(eased) == 0.0f ? target : eased;
}

F::Vec3 AimPoint(const F::Vec3 &camera, const F::Vec3 &centre, const F::Vec3 &velocity) {
  const F::Vec3 toCar = F::Sub(centre, camera);
  const float distance = F::Length(toCar);
  if (distance < 0.01f)
    return centre;
  const F::Vec3 n = F::Mul(toCar, 1.0f / distance);
  const F::Vec3 across = F::Sub(velocity, F::Mul(n, F::Dot(velocity, n)));
  const float speed = F::Length(across);
  if (speed < 0.5f)
    return centre;
  const float angle = kLeadAngle * std::min(speed / kLeadSpeed, 1.0f);
  return F::Add(centre, F::Mul(across, distance * std::tan(angle) / speed));
}

Pose AimAtCarLead(Pose camera, const F::Vec3 &centre, const F::Vec3 &aim, const OrientFromDir &orient,
                  float minDistance) {
  const F::Vec3 toCar = F::Sub(centre, camera.position);
  const float distance = F::Length(toCar);
  if (distance < 0.01f)
    return camera;
  if (distance < minDistance)
    camera.position = F::Sub(centre, F::Mul(toCar, minDistance / distance));
  const F::Vec3 look = F::Sub(aim, camera.position);
  const float l = F::Length(look);
  if (l > 0.01f && std::fabs(look.z) / l < 0.98f)
    camera.orientation = orient(look);
  return camera;
}

float FitDistance(const F::Vec3 &camera, const F::Vec3 &centre, const F::Quat &car, const F::Vec3 &half,
                  float tanHalfV) {
  const F::Vec3 away = F::Sub(camera, centre);
  const float l = F::Length(away);
  if (l < 0.01f || F::Length(half) < 0.01f || tanHalfV <= 0.0f)
    return kMinDistance;
  const F::Vec3 dir = F::Mul(away, 1.0f / l);
  const F::Vec3 forward = F::Neg(dir);
  if (std::fabs(forward.z) > 0.98f)
    return kMinDistance;
  const F::Vec3 right = F::Normalize(F::Cross(forward, {0.0f, 0.0f, 1.0f}));
  const F::Vec3 up = F::Cross(right, forward);
  const float tanHalfH = tanHalfV * kAspect;
  const auto fits = [&](float d) {
    const F::Vec3 eye = F::Add(centre, F::Mul(dir, d));
    for (int k = 0; k < 8; ++k) {
      const F::Vec3 local{k & 4 ? half.x : -half.x, k & 2 ? half.y : -half.y, k & 1 ? half.z : -half.z};
      const F::Vec3 rel = F::Sub(F::Add(centre, F::Rotate(car, local)), eye);
      const float depth = F::Dot(rel, forward);
      if (depth < 0.3f)
        return false;
      if (std::fabs(F::Dot(rel, up)) > kFillHeight * depth * tanHalfV ||
          std::fabs(F::Dot(rel, right)) > kFillWidth * depth * tanHalfH)
        return false;
    }
    return true;
  };
  if (fits(kMinDistance))
    return kMinDistance;
  if (!fits(kMaxFitDistance))
    return kMaxFitDistance;
  float lo = kMinDistance, hi = kMaxFitDistance;
  for (int i = 0; i < 24; ++i) {
    const float mid = 0.5f * (lo + hi);
    (fits(mid) ? hi : lo) = mid;
  }
  return hi;
}

float FitDistanceAnyBank(const F::Vec3 &camera, const F::Vec3 &centre, const F::Quat &car, const F::Vec3 &half,
                         float tanHalfV) {
  const F::Vec3 level = Level(F::Rotate(car, {0.0f, 1.0f, 0.0f}), 0.1f);
  const F::Quat heading = F::Length2D(level) > 0.0f ? OrientationFromDir(level) : car;
  const float round = std::sqrt(half.x * half.x + half.z * half.z);
  return FitDistance(camera, centre, heading, {round, half.y, round}, tanHalfV);
}

float EaseDistance(float prev, float target, float aDt) {
  if (prev <= 0.0f)
    return target;
  const float time = target > prev ? kFitOutTime : kFitInTime;
  return prev + (target - prev) * (1.0f - std::exp(-std::max(aDt, 0.0f) / time));
}

namespace {
// every point from a to b a metre apart (half a nav map cell), `blocked` false for each (the last aSkip metres
// before b left out)
bool ClearLine(const F::Vec3 &a, const F::Vec3 &b, float aSkip, const std::function<bool(const F::Vec3 &)> &blocked) {
  const F::Vec3 d = F::Sub(b, a);
  const float length = F::Length(d) - aSkip;
  for (float s = 0.0f; s <= length; s += 1.0f)
    if (blocked(F::Add(a, F::Mul(d, s / (length + aSkip)))))
      return false;
  return true;
}
} // namespace

Flyby PlanFlyby(const Ahead &ahead, const F::Vec3 &offset, float speed, int side,
                const std::function<bool(const F::Vec3 &)> &blocked) {
  Flyby out;
  speed = std::max(speed, 1.0f);
  // the pass as late as kFlybyPassTime, sooner when the route ends before (and 5 m short of its end)
  float pass = kFlybyPassTime;
  while (pass >= kFlybyMinPassTime && ahead.distanceAfter(pass) > ahead.end - 5.0f)
    pass -= 0.5f;
  if (pass < kFlybyMinPassTime)
    return out;
  // the pass at pass seconds: each spot tried in turn, the first clear one taken
  const auto tryAt = [&](float aPass) {
    const float passS = ahead.distanceAfter(aPass);
    const float planned = ahead.speedAt(passS);
    const float passSpeed = std::max(planned > 0.5f ? planned : speed, 1.0f);
    const float relative = std::min(passSpeed, kFlybyRelSpeed);
    const F::Vec3 tangent = F::Normalize(ahead.tangent(passS));
    const F::Vec3 right = Level(F::Cross(tangent, {0.0f, 0.0f, 1.0f}), 0.05f);
    if (F::Length2D(right) == 0.0f)
      return Flyby{};
    const F::Vec3 drift = F::Mul(tangent, passSpeed - relative);
    const float across =
        std::clamp(std::max(std::fabs(offset.x), relative / kFlybyMaxPan), kFlybyMinSide, kFlybyMaxSide);
    const F::Vec3 car = ahead.at(passS);
    for (int attempt = 0; attempt < 4; ++attempt) {
      const int s = attempt < 2 ? side : -side;
      const float up = offset.z + (attempt % 2 == 1 ? kFlybyRaise : 0.0f);
      const F::Vec3 atPass = F::Add(F::Add(car, F::Mul(right, float(s) * across)), {0.0f, 0.0f, up});
      Flyby f{true, F::Sub(atPass, F::Mul(drift, aPass)), drift, aPass, s};
      // the camera's own path over the shot (twice the pass time)
      if (!ClearLine(FlybyAt(f, 0.0f), FlybyAt(f, 2.0f * aPass), 0.0f, blocked) || blocked(f.start))
        continue;
      // its view of the car at the start, half way, the pass and half again after
      bool clear = true;
      for (float k : {0.0f, 0.5f, 1.0f, 1.5f}) {
        const float t = k * aPass;
        if (!ClearLine(FlybyAt(f, t), ahead.at(ahead.distanceAfter(t)), kFlybySightSkip, blocked)) {
          clear = false;
          break;
        }
      }
      if (clear)
        return f;
    }
    return Flyby{};
  };
  out = tryAt(pass);
  // a second try kFlybyCloser sooner (a nearer pass, a shorter shot), if the route allows it
  if (!out.ok && pass - kFlybyCloser >= kFlybyMinPassTime)
    out = tryAt(pass - kFlybyCloser);
  return out;
}

F::Vec3 FlybyAt(const Flyby &flyby, float aT) { return F::Add(flyby.start, F::Mul(flyby.velocity, aT)); }

Ahead StraightAhead(const F::Vec3 &position, const F::Vec3 &velocity) {
  const float speed = F::Length(velocity);
  const F::Vec3 dir = speed > 0.5f ? F::Mul(velocity, 1.0f / speed) : F::Vec3{0.0f, 1.0f, 0.0f};
  Ahead a;
  a.distanceAfter = [speed](float aT) { return speed * aT; };
  a.at = [position, dir](float aS) { return F::Add(position, F::Mul(dir, aS)); };
  a.tangent = [dir](float) { return dir; };
  a.speedAt = [speed](float) { return speed; };
  a.end = 1.0e6f;
  return a;
}

} // namespace Cinematic
