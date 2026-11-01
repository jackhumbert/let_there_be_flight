#include "Flight/Thruster.hpp"

#include <algorithm>

namespace Flight {

namespace {
// ClampF(v, lo, hi).
float Clamp(float v, float lo, float hi) { return std::min(std::max(v, lo), hi); }

// FlightUtils.Up, Forward and Right.
constexpr Vec3 kUp{0.0f, 0.0f, 1.0f};
constexpr Vec3 kForward{0.0f, 1.0f, 0.0f};
constexpr Vec3 kRight{1.0f, 0.0f, 0.0f};

// The window test every GetPitch / GetYaw shares: outside [lo - allowance, hi + allowance] the angle is dropped.
float Windowed(float angle, float lo, float hi, float allowance) {
  if (angle < (lo - allowance) || angle > (hi + allowance))
    return 0.0f;
  return angle;
}
} // namespace

Vec3 ThrusterNormalize(const Vec3 &v, float threshold) {
  if (Length(v) > threshold)
    return Normalize(v);
  return {v.x / threshold, v.y / threshold, v.z / threshold};
}

float ThrusterPitch(const ThrusterParams &p, const Vec3 &f, const Vec3 &t) {
  float angle = AngleDegAroundAxis(kUp, f, kRight);
  angle = Windowed(angle, -p.maxThrusterAnglePitch, p.maxThrusterAnglePitch, p.thrusterAngleAllowance);
  angle *= (1.0f - std::fabs(t.y) * 0.5f);
  float c = Clamp(angle, -p.maxThrusterAnglePitch, p.maxThrusterAnglePitch);
  if (p.shape == ThrusterShape::Car) {
    float dir = p.isRight ? 1.0f : -1.0f;
    return c * dir;
  }
  return -c;
}

float ThrusterRoll(const ThrusterParams &p) { return p.isRight ? 0.0f : 180.0f; }

float ThrusterYaw(const ThrusterParams &p, const Vec3 &f, const Vec3 &t, bool active) {
  if (!active)
    return 180.0f;
  float angle = AngleDegAroundAxis(kUp, f, kForward);
  if (p.shape == ThrusterShape::Car) {
    float dir, outside, inside;
    if (p.isRight) {
      dir = 1.0f;
      outside = p.maxThrusterAngleInside;
      inside = p.maxThrusterAngleOutside;
    } else {
      dir = -1.0f;
      outside = p.maxThrusterAngleOutside;
      inside = p.maxThrusterAngleInside;
    }
    angle = Windowed(angle, -inside, outside, p.thrusterAngleAllowance);
    float tor = (p.isFront != p.isRight) ? t.z : -t.z; // FL, BR : FR, BL
    return tor * p.mainThrusterYawFactor + Clamp(angle, -inside, outside) * dir;
  }
  float outside = p.maxThrusterAngleOutside;
  float inside = p.maxThrusterAngleOutside;
  angle = Windowed(angle, -inside, outside, p.thrusterAngleAllowance);
  if (p.shape == ThrusterShape::BikeFront)
    return t.z * 30.0f + Clamp(angle, -inside, outside);
  return t.z * -30.0f - Clamp(angle, -inside, outside);
}

ThrusterAngles ThrusterEuler(const ThrusterParams &p, const Vec3 &f, const Vec3 &t, bool active) {
  return {ThrusterPitch(p, f, t), ThrusterRoll(p), ThrusterYaw(p, f, t, active)};
}

Vec3 ThrusterScale(const Vec3 &current, bool active) {
  Vec3 target = active ? Vec3{1.0f, 1.0f, 1.0f} : Vec3{0.0f, 0.0f, 0.0f};
  return Interpolate(current, target, kThrusterScaleLerp);
}

float ThrusterFxDisplacement(ThrusterFxKind k, const Vec3 &up, const Vec3 &f, const Vec3 &t, bool isFront,
                             bool isRight, float &aBlackboard) {
  switch (k) {
  case ThrusterFxKind::Regular: {
    float amount = Dot(up, f);
    float x = t.x;
    if (!isFront)
      x *= -1.0f;
    amount += x + t.y;
    aBlackboard = Clamp(amount + t.z, -1.0f, 1.0f);
    return Clamp(amount, -1.0f, 1.0f);
  }
  case ThrusterFxKind::Main: {
    float amount = Dot(up, f);
    float x = t.x;
    float y = t.y;
    if (!isFront)
      x *= -1.0f;
    if (isRight)
      y *= -1.0f;
    aBlackboard = Clamp(amount + x + y, -1.0f, 1.0f);
    return Clamp(amount, -1.0f, 1.0f);
  }
  case ThrusterFxKind::Side: {
    float tor = t.z;
    Vec3 vec{1.0f, 0.0f, 0.0f};
    if (isRight) {
      tor *= -1.0f;
      vec = Mul(vec, -1.0f);
    }
    if (!isFront)
      vec = Mul(vec, -1.0f);
    float amount = (Dot(vec, f) + tor) * 0.1f;
    aBlackboard = Clamp(amount, -1.0f, 1.0f);
    return 0.0f;
  }
  default:
    aBlackboard = Clamp(0.0f, -1.0f, 1.0f);
    return 0.0f;
  }
}

float ThrusterDeviation(const ThrusterParams &p, float deviation, float amount) {
  return Lerp(p.boneLerpAmount, deviation, kAnimDeviationCenter + amount * kAnimDeviationScale);
}

} // namespace Flight
