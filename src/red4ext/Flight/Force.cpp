// FlightMode.ApplyPhysics and the tail of FlightComponent.OnUpdate as pure code (Flight/Force.hpp).
#include "Flight/Force.hpp"

#include <algorithm>

namespace Flight {

float CapDamping(float damp, float angularVelocity, float tensor, float inertia, float mass, const ForceParams &p) {
  if (std::fabs(angularVelocity) < 0.0001f || inertia <= 0.0f || tensor <= 0.0f)
    return damp;
  float response = p.dampStabilityK * mass * tensor / inertia;
  if (response <= 0.0f)
    return damp;
  float maxGain = p.dampMaxPerFrame / response;
  float gain = damp / angularVelocity;
  if (gain > maxGain)
    return angularVelocity * maxGain;
  return damp;
}

void ApplyPhysics(const ForceParams &p, const StatsStatic &st, const StatsState &s, const ForceInput &in,
                  ForceOutput &out) {
  Vec3 velocityDamp;
  if (p.physxFlightPhysics && st.mass > 0.0f)
    velocityDamp = Mul(s.d_localVelocity, s.d_speed * 1.2f * st.airResistanceFactor / st.mass);

  // only the spin that did not come from the world is damped
  Vec3 spin = Sub(s.d_angularVelocity, Rotate(Conjugate(s.d_orientation), in.contactSpin));
  Vec3 damp = Mul(spin, p.generalDampFactorAngular + in.angularBrake * p.brakeFactorAngular * st.brakingFrictionFactor);
  damp.x *= 1.0f - std::fabs(in.pitch);
  damp.y *= 1.0f - std::fabs(in.roll);
  damp.z *= 1.0f - std::fabs(in.yaw);
  damp.z *= p.generalYawDampFactor;

  damp.x = CapDamping(damp.x, spin.x, in.thrusterTensor.x, st.momentOfInertia.x, st.mass, p);
  damp.y = CapDamping(damp.y, spin.y, std::max(in.thrusterTensor.y, 1.0f), st.momentOfInertia.y, st.mass, p);
  damp.z = CapDamping(damp.z, spin.z, in.thrusterTensor.z, st.momentOfInertia.z, st.mass, p);

  if (Length(damp) > p.generalDampFactorAngularMax)
    damp = Mul(Normalize(damp), p.generalDampFactorAngularMax);

  out.linearDamp = p.generalDampFactorLinear + in.linearBrake * p.brakeFactorLinear;

  Vec3 direction = s.d_direction;
  if (Dot(s.d_direction, s.d_forward) < 0.0f)
    direction = Neg(s.d_direction);
  float yawAngle = AngleDegAroundAxis(direction, s.d_forward, s.d_up);
  float pitchAngle = AngleDegAroundAxis(direction, s.d_forward, s.d_right);
  float aeroYaw = yawAngle * s.d_speedRatio;
  float aeroPitch = pitchAngle * s.d_speedRatio;

  float yawDir = (s.d_speedRatio + std::fabs(in.yaw) * p.generalSwayWithYaw) * p.generalYawDirectionalityFactor;
  float pitchDir = s.d_speedRatio * p.generalPitchDirectionalityFactor;
  float aeroFactor = Dot(s.d_forward, s.d_direction);
  Vec3 off = Sub(s.d_forward, s.d_direction);
  float sideY = std::fabs(Dot(off, s.d_right));
  float sideP = std::fabs(Dot(off, s.d_up));
  const Vec3 fwd{0.0f, 1.0f, 0.0f};
  Vec3 back = Neg(s.d_localDirection);

  Vec3 force = Neg(velocityDamp);
  force = Add(force, Mul(fwd, sideY * yawDir * aeroFactor));
  force = Add(force, Mul(back, sideY * yawDir * std::fabs(aeroFactor)));
  force = Add(force, Mul(fwd, sideP * pitchDir * aeroFactor));
  force = Add(force, Mul(back, sideP * pitchDir * std::fabs(aeroFactor)));
  out.physicsForce = force;

  Vec3 torque = Neg(damp);
  torque.z -= aeroYaw * p.generalYawAeroFactor;
  torque.x -= aeroPitch * p.generalPitchAeroFactor;
  out.physicsTorque = torque;
}

ForceOutput ForceTail(const ForceParams &p, const StatsStatic &st, const StatsState &s, const ForceInput &in) {
  ForceOutput out;
  out.collisionTimer = in.collisionTimer;
  Vec3 f = in.force, t = in.torque;
  if (in.applyPhysics) {
    ApplyPhysics(p, st, s, in, out);
    f = Add(f, out.physicsForce);
    t = Add(t, out.physicsTorque);
  }

  f = Rotate(s.d_orientation, Mul(f, in.dt * st.mass));
  t = Mul(t, in.dt * st.mass);
  t.x *= in.thrusterTensor.x;
  t.y *= std::max(in.thrusterTensor.y, 1.0f); // prevents bikes from rolling excessively
  t.z *= in.thrusterTensor.z;
  t = Rotate(s.d_orientation, t);

  if (out.collisionTimer < p.collisionRecoveryDelay + p.collisionRecoveryDuration) {
    float k = std::min(std::max(0.0f, (out.collisionTimer - p.collisionRecoveryDelay) / p.collisionRecoveryDuration), 1.0f);
    f = Mul(f, k);
    t = Mul(t, k);
    out.collisionTimer += in.dt;
  }
  out.worldForce = f;
  out.worldTorque = t;
  return out;
}

} // namespace Flight
