#include "Flight/Modes.hpp"

namespace Flight {

ModeOutput StandardLift(const StatsState &s, const StatsStatic &st, const Controls &c, const StandardParams &p,
                        Pid &pitchPid, Pid &rollPid, Vec3 normal, float liftForce, float dt) {
  normal = RotateAxis(normal, s.d_forward, c.yaw * p.rollWithYaw);
  normal = RotateAxis(normal, s.d_right, c.lift * p.pitchWithLift * Dot(s.d_forward, s.d_direction));
  normal = RotateAxis(normal, s.d_right, c.surge * p.pitchWithSurge);

  float pitchOffNormal = 90.0f - std::fabs(AngleDegAroundAxis(normal, s.d_forward, s.d_right));
  float pitchOffInput = c.pitch * p.pitchInputAngle;
  float pitchOff = pitchOffNormal + pitchOffInput;
  float rollOffNormal = 90.0f - std::fabs(AngleDegAroundAxis(normal, s.d_right, s.d_forward));
  float rollOffInput = c.roll * p.rollInputAngle;
  float rollOff = rollOffNormal + rollOffInput;

  float pitchCorrection = 0.0f;
  float rollCorrection = 0.0f;
  if (std::fabs(pitchOff) < 120.0f)
    pitchCorrection = PidCorrectionClamped(pitchPid.state, pitchPid.gains, pitchOff / 90.0f, dt, 10.0f);
  if (std::fabs(rollOff) < 120.0f)
    rollCorrection = PidCorrectionClamped(rollPid.state, rollPid.gains, rollOff / 90.0f, dt, 10.0f);
  pitchCorrection *= p.pitchFactor;
  rollCorrection *= p.rollFactor;

  Vec3 velocityDamp = Mul(s.d_localVelocity, c.linearBrake * p.brakeFactorLinear * st.brakingFrictionFactor);
  float surgeForce = c.surge * p.surgeFactor;

  ModeOutput out;
  out.force = Add(out.force, Mul(s.d_localUp, liftForce));
  out.force = Add(out.force, Mul(kForward, surgeForce));
  out.force = Add(out.force, Mul(Mul(kRight, c.sway), p.swayFactor));
  out.force = Sub(out.force, velocityDamp);

  out.torque.x = -(pitchCorrection * (1.0f - c.angularBrake));
  out.torque.y = rollCorrection * (1.0f - c.angularBrake);
  out.torque.z = -((c.yaw - (rollOffInput * pitchOffInput / 1800.0f)) * p.yawFactor);
  return out;
}

ModeOutput StandardDistance(const StatsState &s, const StatsStatic &st, const Controls &c, const StandardParams &p,
                            Pid &hoverPid, Pid &pitchPid, Pid &rollPid, Vec3 normal, float heightDifference, float dt) {
  float hoverCorrection = PidCorrectionClamped(hoverPid.state, hoverPid.gains, heightDifference, dt, p.hoverClamp);
  float liftForce = hoverCorrection * p.liftFactor + kGravity * p.gravityFactor;
  return StandardLift(s, st, c, p, pitchPid, rollPid, normal, liftForce, dt);
}

ModeOutput Drone(const StatsState &s, const StatsStatic &st, const Controls &c, const DroneParams &p) {
  Vec3 velocityDamp = Mul(Mul(Mul(s.d_localVelocity, c.linearBrake), p.brakeFactorLinear), st.brakingFrictionFactor);
  Vec3 angularDamp = Mul(Mul(Mul(s.d_angularVelocity, c.angularBrake), p.brakeFactorAngular), st.brakingFrictionFactor);

  ModeOutput out;
  out.force = Add(out.force, Mul(Mul(kUp, c.lift), p.liftFactor));
  out.force = Add(out.force, Mul(Mul(kForward, c.surge), p.surgeFactor));
  out.force = Add(out.force, Mul(Mul(kRight, c.sway), p.swayFactor));
  out.force = Sub(out.force, velocityDamp);

  out.torque.x = -(c.pitch * p.pitchFactor + angularDamp.x);
  out.torque.y = c.roll * p.rollFactor - angularDamp.y;
  out.torque.z = -(c.yaw * p.yawFactor + angularDamp.z);
  return out;
}

} // namespace Flight
