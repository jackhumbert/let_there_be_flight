#include "Aerobatics.hpp"

#include "Flight/Force.hpp"
#include "Navigation/DroneAttitude.hpp"

#include <algorithm>
#include <cmath>

namespace Navigation {

namespace F = Flight;

namespace {
float ClampF(float v, float lo, float hi) { return std::min(std::max(v, lo), hi); }

constexpr F::Vec3 kUp{0.0f, 0.0f, 1.0f};
} // namespace

F::Vec3 RotationError(const F::Vec3 &r, const F::Vec3 &f, const F::Vec3 &u, const F::Vec3 &tr, const F::Vec3 &tf,
                      const F::Vec3 &tu) {
  // m[row][col] = dot(axis row of the first frame, axis col of the second)
  const float m00 = F::Dot(r, tr);
  const float m01 = F::Dot(r, tf);
  const float m02 = F::Dot(r, tu);
  const float m10 = F::Dot(f, tr);
  const float m11 = F::Dot(f, tf);
  const float m12 = F::Dot(f, tu);
  const float m20 = F::Dot(u, tr);
  const float m21 = F::Dot(u, tf);
  const float m22 = F::Dot(u, tu);
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
  const float sinHalf = std::sqrt(x * x + y * y + z * z);
  if (sinHalf < 0.000001f)
    return {0.0f, 0.0f, 0.0f};
  const float angle = 2.0f * std::atan2(sinHalf, w);
  return {x * angle / sinHalf, y * angle / sinHalf, z * angle / sinHalf};
}

F::Vec3 Across(const F::Vec3 &u, const F::Vec3 &f) { return F::Sub(u, F::Mul(f, F::Dot(u, f))); }

Maneuver CreateManeuver(ManeuverKind aKind, float aSide, const F::Vec3 &aPosition, const F::Vec3 &aVelocity,
                        float aAccel, float aRollTime) {
  Maneuver m;
  m.kind = aKind;
  m.side = aSide >= 0.0f ? 1.0f : -1.0f;
  m.origin = aPosition;
  m.dir = F::Normalize2D(aVelocity);
  m.right = F::Cross(m.dir, kUp);
  m.speed = std::max(F::Length2D(aVelocity), 10.0f);
  m.rollTime = aRollTime;
  if (aKind == ManeuverKind::AileronRoll) {
    m.duration = 2.2f;
  } else if (aKind == ManeuverKind::BarrelRoll) {
    m.radius = 5.0f;
    m.duration = 3.5f;
  } else {
    // tight enough for the city: v^2 / r at most accel, within 14 to 35 m
    m.radius = ClampF(m.speed * m.speed / aAccel, 14.0f, 35.0f);
    const float half = 3.1415927f * m.radius / m.speed;
    m.duration = aKind == ManeuverKind::Loop ? 2.0f * half : half + aRollTime;
  }
  return m;
}

bool IsOverlay(const Maneuver &aManeuver) {
  return aManeuver.kind == ManeuverKind::AileronRoll || aManeuver.kind == ManeuverKind::BarrelRoll;
}

bool IsDone(const Maneuver &aManeuver, float aTime) { return aTime >= aManeuver.duration; }

float ManeuverEase(float aT, float aD, float &aRate) {
  const float x = ClampF(aT / aD, 0.0f, 1.0f);
  aRate = 0.5f * 3.1415927f / aD * std::sin(3.1415927f * x);
  return 0.5f * (1.0f - std::cos(3.1415927f * x));
}

F::Vec3 RollUp(const Maneuver &aManeuver, float aT, const F::Vec3 &aF, const F::Vec3 &aU) {
  float rate;
  const float phi = 6.2831853f * ManeuverEase(aT, aManeuver.duration, rate) * aManeuver.side;
  return F::Add(F::Mul(aU, std::cos(phi)), F::Mul(F::Cross(aF, aU), std::sin(phi)));
}

void BarrelOffset(const Maneuver &aManeuver, float aT, F::Vec3 &aOffset, F::Vec3 &aOffsetVelocity, F::Vec3 &aUp) {
  float rate;
  const float theta = 6.2831853f * ManeuverEase(aT, aManeuver.duration, rate);
  const float thetaRate = 6.2831853f * rate;
  const F::Vec3 z = kUp;
  const F::Vec3 s = F::Mul(aManeuver.right, aManeuver.side);
  aOffset = F::Mul(F::Add(F::Mul(z, 1.0f - std::cos(theta)), F::Mul(s, std::sin(theta))), aManeuver.radius);
  aOffsetVelocity =
      F::Mul(F::Add(F::Mul(z, std::sin(theta)), F::Mul(s, std::cos(theta))), aManeuver.radius * thetaRate);
  aUp = F::Sub(F::Mul(z, aManeuver.radius), aOffset);
}

void SampleManeuver(const Maneuver &aManeuver, float aT, F::Vec3 &aPos, F::Vec3 &aVel, F::Vec3 &aAcc, F::Vec3 &aFwd,
                    F::Vec3 &aUp) {
  const F::Vec3 z = kUp;
  const F::Vec3 h = aManeuver.dir;
  const float r = aManeuver.radius;
  const float v = aManeuver.speed;
  const float w = v / r;
  const float half = 3.1415927f * r / v;
  const F::Vec3 s = F::Mul(aManeuver.right, aManeuver.side);
  const ManeuverKind kind = aManeuver.kind;
  float rate;
  aAcc = {0.0f, 0.0f, 0.0f};
  if (kind == ManeuverKind::Loop || (kind == ManeuverKind::Immelmann && aT < half)) {
    // up and over a circle of radius r, the car's top towards its centre
    const float a = w * ClampF(aT, 0.0f, 2.0f * half);
    aPos = F::Add(aManeuver.origin, F::Mul(F::Add(F::Mul(h, std::sin(a)), F::Mul(z, 1.0f - std::cos(a))), r));
    aVel = F::Mul(F::Add(F::Mul(h, std::cos(a)), F::Mul(z, std::sin(a))), v);
    aAcc = F::Mul(F::Sub(F::Mul(z, std::cos(a)), F::Mul(h, std::sin(a))), v * w);
    aUp = F::Sub(F::Mul(z, std::cos(a)), F::Mul(h, std::sin(a)));
  } else if (kind == ManeuverKind::Immelmann) {
    // inverted at the top heading back, rolling upright
    const float psi = 3.1415927f * ManeuverEase(aT - half, aManeuver.rollTime, rate);
    aPos = F::Sub(F::Add(aManeuver.origin, F::Mul(z, 2.0f * r)), F::Mul(h, v * (aT - half)));
    aVel = F::Mul(h, -v);
    aUp = F::Add(F::Mul(z, -std::cos(psi)), F::Mul(s, std::sin(psi)));
  } else if (kind == ManeuverKind::SplitS && aT < aManeuver.rollTime) {
    // rolling inverted, straight on
    const float psi = 3.1415927f * ManeuverEase(aT, aManeuver.rollTime, rate);
    aPos = F::Add(aManeuver.origin, F::Mul(h, v * std::max(aT, 0.0f)));
    aVel = F::Mul(h, v);
    aUp = F::Add(F::Mul(z, std::cos(psi)), F::Mul(s, std::sin(psi)));
  } else if (kind == ManeuverKind::SplitS) {
    // down and round a circle under the top, the car's top towards its centre
    const float b = w * std::min(aT - aManeuver.rollTime, half);
    aPos = F::Add(F::Add(aManeuver.origin, F::Mul(h, v * aManeuver.rollTime)),
                  F::Mul(F::Sub(F::Mul(h, std::sin(b)), F::Mul(z, 1.0f - std::cos(b))), r));
    aVel = F::Mul(F::Sub(F::Mul(h, std::cos(b)), F::Mul(z, std::sin(b))), v);
    aAcc = F::Mul(F::Sub(F::Mul(z, -std::cos(b)), F::Mul(h, std::sin(b))), v * w);
    aUp = F::Sub(F::Mul(z, -std::cos(b)), F::Mul(h, std::sin(b)));
  } else {
    aPos = F::Add(aManeuver.origin, F::Mul(h, v * aT));
    aVel = F::Mul(h, v);
    aUp = z;
  }
  aFwd = F::Normalize(aVel);
}

bool BankFrame(const F::Vec3 &aFwd, F::Vec3 &aLevel, F::Vec3 &aSide) {
  aLevel = Across(kUp, aFwd);
  if (F::Length(aLevel) < 0.2f)
    return false;
  aLevel = F::Normalize(aLevel);
  aSide = F::Cross(aFwd, aLevel);
  return true;
}

F::Vec3 Unsteep(const F::Vec3 &aFwd, const F::Vec3 &aHeadingRef) {
  const float steep = ClampF((std::fabs(aFwd.z) - 0.6f) / 0.3f, 0.0f, 1.0f);
  if (steep > 0.0f)
    return F::Normalize(F::Add(F::Mul(aFwd, 1.0f - 0.6f * steep), F::Mul(aHeadingRef, 0.6f * steep)));
  return aFwd;
}

F::Vec3 EaseBank(EaseState &aState, const EaseParams &aParams, float aDt, const F::Vec3 &aFwd, const F::Vec3 &aUp) {
  F::Vec3 level, side;
  if (!BankFrame(aFwd, level, side)) {
    aState.bankRateNow = 0.0f;
    return aUp;
  }
  const float wanted = std::atan2(F::Dot(aUp, side), F::Dot(aUp, level));
  const float error = WrapAngle(wanted - aState.bankAngle);
  const float f = aParams.raceRollFrequency;
  const float accelMax = aParams.raceRollAccelMax;
  const float rateMax = aParams.raceRollRateMax;
  const float rollAccel = ClampF(f * f * error - 2.0f * f * aState.bankRateNow, -accelMax, accelMax);
  aState.bankRateNow = ClampF(aState.bankRateNow + rollAccel * aDt, -rateMax, rateMax);
  aState.bankAngle = WrapAngle(aState.bankAngle + aState.bankRateNow * aDt);
  return F::Add(F::Mul(level, std::cos(aState.bankAngle)), F::Mul(side, std::sin(aState.bankAngle)));
}

F::Vec3 EasePitch(EaseState &aState, const EaseParams &aParams, float aDt, const F::Vec3 &aFwd, bool aOnLine,
                  const F::Vec3 &aLineTangent, const F::Vec3 &aHeadingRef) {
  const F::Vec3 level = F::Normalize2D(aFwd);
  if (F::Length(level) < 0.5f) {
    SyncPitch(aState, aFwd);
    return aFwd;
  }
  float wanted = std::asin(ClampF(aFwd.z, -1.0f, 1.0f));
  if (aOnLine) {
    // the same steep-path rule as the nose off the line
    const float slope = std::asin(ClampF(aLineTangent.z, -1.0f, 1.0f));
    const F::Vec3 along =
        Unsteep(F::Add(F::Mul(level, std::cos(slope)), F::Mul(kUp, std::sin(slope))), aHeadingRef);
    wanted = std::asin(ClampF(along.z, -1.0f, 1.0f));
  }
  const float f = aParams.racePitchFrequency;
  const float accel = ClampF(f * f * (wanted - aState.pitchAngle) - 2.0f * f * aState.pitchRateNow,
                             -aParams.racePitchAccelMax, aParams.racePitchAccelMax);
  aState.pitchRateNow = ClampF(aState.pitchRateNow + accel * aDt, -aParams.racePitchRateMax, aParams.racePitchRateMax);
  aState.pitchAngle = ClampF(aState.pitchAngle + aState.pitchRateNow * aDt, -1.5f, 1.5f);
  return F::Add(F::Mul(level, std::cos(aState.pitchAngle)), F::Mul(kUp, std::sin(aState.pitchAngle)));
}

void SyncPitch(EaseState &aState, const F::Vec3 &aFwd) {
  aState.pitchAngle = std::asin(ClampF(aFwd.z, -1.0f, 1.0f));
  aState.pitchRateNow = 0.0f;
}

void SyncBank(float &aBankAngle, float &aBankRateNow, const F::Vec3 &aFwd, const F::Vec3 &aUp) {
  F::Vec3 level, side;
  if (BankFrame(aFwd, level, side))
    aBankAngle = std::atan2(F::Dot(aUp, side), F::Dot(aUp, level));
  aBankRateNow = 0.0f;
}

F::Vec3 AngularImpulse(const WorldInertia &aInertia, const F::Vec3 &aA, bool &aValid) {
  aValid = aInertia.valid;
  if (!aInertia.valid)
    return {0.0f, 0.0f, 0.0f};
  const WorldInertia &m = aInertia;
  return {m.x.x * aA.x + m.y.x * aA.y + m.z.x * aA.z, m.x.y * aA.x + m.y.y * aA.y + m.z.y * aA.z,
          m.x.z * aA.x + m.y.z * aA.y + m.z.z * aA.z};
}

AttitudeControlOutput AttitudeControl(AttitudeControlState &aState, const AttitudeControlParams &aParams,
                                      const F::StatsStatic &aStatic, const F::StatsState &aStats,
                                      const AttitudeControlInput &aInput) {
  AttitudeControlOutput out;
  // FlyAerobatic's own floor on the frame time
  const float dt = std::max(aInput.dt, 0.001f);
  const F::Vec3 fwd = aInput.fwd;
  const F::Vec3 right = F::Normalize(F::Cross(fwd, aInput.up));
  const F::Vec3 up = F::Cross(right, fwd);
  if (aInput.shape) {
    // a shape sets its own attitude: carry on the bank easing from where it leaves the car
    SyncBank(aState.bankAngle, aState.bankRateNow, fwd, up);
  }

  // rate of the target, smoothed
  const F::Vec3 step = RotationError(aState.targetRight, aState.targetForward, aState.targetUp, right, fwd, up);
  const F::Vec3 omega =
      F::Mul(F::Add(F::Add(F::Mul(aState.targetRight, step.x), F::Mul(aState.targetForward, step.y)),
                    F::Mul(aState.targetUp, step.z)),
             1.0f / dt);
  aState.omegaRef = F::Add(aState.omegaRef, F::Mul(F::Sub(omega, aState.omegaRef), ClampF(dt / 0.05f, 0.0f, 1.0f)));
  aState.targetRight = right;
  aState.targetForward = fwd;
  aState.targetUp = up;

  // angular acceleration from the error and the rate error, then the torque that gives it on each axis
  const F::Vec3 error = RotationError(aStats.d_right, aStats.d_forward, aStats.d_up, right, fwd, up);
  out.attitudeError = F::Length(error);
  out.rateRef = {F::Dot(aState.omegaRef, aStats.d_right), F::Dot(aState.omegaRef, aStats.d_forward),
                 F::Dot(aState.omegaRef, aStats.d_up)};
  const F::Vec3 rate = aStats.d_angularVelocity;
  const float freq = aParams.attitudeFrequency;
  F::Vec3 alpha =
      F::Add(F::Mul(error, freq * freq), F::Mul(F::Sub(out.rateRef, rate), 2.0f * aParams.attitudeDamping * freq));
  if (F::Length(alpha) > aParams.maxAngularAccel)
    alpha = F::Mul(F::Normalize(alpha), aParams.maxAngularAccel);
  const float mass = aStatic.mass;
  const F::Vec3 inertia = aStatic.momentOfInertia;
  const F::Vec3 tensor = aInput.thrusterTensor;
  bool valid;
  const F::Vec3 impulse = AngularImpulse(aInput.inertia, F::Rotate(aStats.d_orientation, alpha), valid);
  F::Vec3 torque;
  if (valid) {
    const F::Vec3 local = F::Rotate(F::Conjugate(aStats.d_orientation), impulse);
    torque = {local.x / std::max(mass * tensor.x, 0.001f), local.y / std::max(mass * std::max(tensor.y, 1.0f), 0.001f),
              local.z / std::max(mass * tensor.z, 0.001f)};
  } else {
    torque = {alpha.x * inertia.x / std::max(aParams.torqueGainPitch * mass * tensor.x, 0.001f),
              alpha.y * inertia.y / std::max(aParams.torqueGainRoll * mass * std::max(tensor.y, 1.0f), 0.001f),
              alpha.z * inertia.z / std::max(aParams.torqueGainYaw * mass * tensor.z, 0.001f)};
  }
  // ApplyPhysics damps the rotation: give back exactly what it takes from the rotation wanted, its CapDamping per
  // axis included (without it, on the cars the cap cuts, the give-back outweighed the damping and the attitude
  // overshot by up to 20 deg). No inputs and no angular brake while the Autopilot flies. A hit is
  // untouched: ApplyPhysics still leaves the contact spin undamped and the recovery still fades this torque.
  F::ForceParams damping;
  damping.generalDampFactorAngular = aParams.generalDampFactorAngular;
  damping.generalYawDampFactor = aParams.generalYawDampFactor;
  damping.generalDampFactorAngularMax = aParams.generalDampFactorAngularMax;
  damping.dampStabilityK = aParams.dampStabilityK;
  damping.dampMaxPerFrame = aParams.dampMaxPerFrame;
  const F::Vec3 giveBack = F::AngularDamping(damping, aStatic, out.rateRef, 0.0f, 0.0f, 0.0f, 0.0f, tensor);
  out.torque = F::Add(torque, giveBack);

  // thrust: the commanded acceleration, the drag it holds off and gravity, in the car's frame
  const float g = 9.81f * aParams.gravityFactor;
  F::Vec3 local =
      F::Rotate(F::Conjugate(aStats.d_orientation), F::Add(F::Add(aInput.accel, aInput.drag), F::Mul(kUp, g)));
  local.x = ClampF(local.x, -aParams.standardModeSwayFactor, aParams.standardModeSwayFactor);
  local.y = ClampF(local.y, aInput.dragAccel - aInput.raceDecel, aParams.standardModeSurgeFactor);
  local.z = ClampF(local.z, g - aParams.standardModeLiftFactor, g + aParams.standardModeLiftFactor);
  out.thrust = local;
  return out;
}

} // namespace Navigation
