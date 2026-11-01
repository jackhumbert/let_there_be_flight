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

namespace {
// The weight HoverFly and Automatic compute: 1 at minHoverHeight, 0 at maxHoverHeight, 0 without ground.
float HoverWeight(const Ground &g, const StandardParams &p) {
  if (!g.found)
    return 0.0f;
  float w = 1.0f - (g.distance - p.minHoverHeight) / (p.maxHoverHeight - p.minHoverHeight);
  return std::fmin(std::fmax(w, 0.0f), 1.0f);
}
} // namespace

Ground GroundFromHits(const Hit *hits, int count, float lastDistance) {
  Ground g;
  g.distance = lastDistance;
  if (count <= 0)
    return g;
  for (int i = 0; i < count; ++i)
    if (!hits[i].valid)
      return g;

  float distanceSum = 0.0f;
  Vec3 normalSum;
  for (int i = 0; i < count; ++i) {
    distanceSum += Length(Sub(hits[i].position, hits[i].tire));
    normalSum = Add(normalSum, Normalize(hits[i].normal));
  }
  float n = static_cast<float>(count);
  g.found = true;
  g.distance = distanceSum / n;
  g.normal = Normalize({normalSum.x / n, normalSum.y / n, normalSum.z / n});
  return g;
}

ModeOutput Fly(const StatsState &s, const StatsStatic &st, const Controls &c, const StandardParams &p, Pid &pitchPid,
               Pid &rollPid, float dt) {
  float liftForce = p.liftFactor * c.lift * dt + kGravity * p.gravityFactor;
  return StandardLift(s, st, c, p, pitchPid, rollPid, kUp, liftForce, dt);
}

ModeOutput Hover(float &hoverHeight, const StatsState &s, const StatsStatic &st, const Controls &c,
                 const StandardParams &p, const Ground &g, Pid &hoverPid, Pid &pitchPid, Pid &rollPid, float dt) {
  hoverHeight = std::fmax(p.minHoverHeight, hoverHeight);
  float heightDifference = 0.0f;
  Vec3 normal = kUp;
  if (g.found) {
    heightDifference = hoverHeight - g.distance;
    normal = g.normal;
  }
  return StandardDistance(s, st, c, p, hoverPid, pitchPid, rollPid, normal, heightDifference, dt);
}

ModeOutput HoverFly(HoverFlyState &m, const StatsState &s, const StatsStatic &st, const Controls &c,
                    const StandardParams &p, const HoverFlyParams &h, const Ground &g, Pid &hoverPid, Pid &pitchPid,
                    Pid &rollPid, float dt) {
  float lastHovering = m.hovering;
  m.hovering = HoverWeight(g, p);

  if (lastHovering == 0.0f && m.hovering > 0.0f) {
    m.hoverHeight = g.distance;
    PidReset(hoverPid.state);
  }

  m.hoverHeight =
      std::fmin(std::fmax(m.hoverHeight + c.lift * h.heightCorrectionFactor * dt * 60.0f, p.minHoverHeight),
                p.maxHoverHeight);

  float heightDifference = m.hoverHeight - g.distance;
  Vec3 idealNormal = Interpolate(kUp, g.normal, m.hovering);

  float hoverCorrection = 0.0f;
  if (m.hovering > 0.0f)
    hoverCorrection = PidCorrectionClamped(hoverPid.state, hoverPid.gains, heightDifference, dt, p.hoverClamp);

  float z = s.d_position.z;
  float vz = s.d_velocity.z;
  if (std::fabs(c.lift) > 0.01f || m.hovering > 0.0f) {
    m.holdingAltitude = false;
  } else if (!m.holdingAltitude) {
    m.holdingAltitude = true;
    m.referenceZ = z;
    if (h.heightDampening > 0.0f)
      m.referenceZ += vz / h.heightDampening;
  }

  float flyCorrection = c.lift * p.liftFactor - vz * h.heightDampening;
  if (m.holdingAltitude)
    flyCorrection += std::fmin(std::fmax((m.referenceZ - z) * h.altitudeHoldFactor, -p.liftFactor), p.liftFactor);
  float liftFactor = Lerp(m.hovering, flyCorrection, hoverCorrection * p.hoverFactor);
  liftFactor *= (1.0f - c.linearBrake);

  return StandardLift(s, st, c, p, pitchPid, rollPid, idealNormal, liftFactor + kGravity * p.gravityFactor, dt);
}

ModeOutput Automatic(AutomaticState &m, const StatsState &s, const StatsStatic &st, const Controls &c,
                     const StandardParams &p, const AutomaticParams &a, const Ground &g, Pid &hoverPid, Pid &pitchPid,
                     Pid &rollPid, float dt) {
  float lastHovering = m.hovering;
  m.hovering = HoverWeight(g, p);

  if (lastHovering == 0.0f && m.hovering > 0.0f)
    m.hoverHeight = std::fmax(g.distance + c.lift * dt * a.liftFactor, p.minHoverHeight);
  else
    m.hoverHeight = std::fmax(m.hoverHeight + c.lift * dt * a.liftFactor, p.minHoverHeight);

  float heightDifference = m.hoverHeight - g.distance;
  Vec3 idealNormal = Interpolate(kUp, g.normal, m.hovering);

  float hoverCorrection = PidCorrectionClamped(hoverPid.state, hoverPid.gains, heightDifference, dt, p.hoverClamp);
  float liftFactor = Lerp(m.hovering, c.lift - s.d_velocity.z * 0.1f, hoverCorrection);

  ModeOutput out = StandardLift(s, st, c, p, pitchPid, rollPid, idealNormal,
                                liftFactor * a.hoverFactor + kGravity * p.gravityFactor, dt);

  float aeroFactor = Dot(s.d_forward, s.d_direction);
  float yawDirectionality = s.d_speedRatio * a.yawDirectionality;
  float directionFactor = std::fabs(Dot(Sub(s.d_forward, s.d_direction), s.d_right));

  out.force = Add(out.force, Mul(Mul(Mul(kForward, directionFactor), yawDirectionality), aeroFactor));
  out.force = Add(out.force,
                  Mul(Mul(Mul(Neg(s.d_localDirection), directionFactor), yawDirectionality), std::fabs(aeroFactor)));

  if (std::fabs(c.surge) < 1.0f) {
    Vec3 velocityDamp =
        Mul(s.d_localVelocity, (1.0f - std::fabs(c.surge)) * a.autoBrakingFactor * st.brakingFrictionFactor);
    out.force.x -= velocityDamp.x;
    out.force.y -= velocityDamp.y;
  }
  return out;
}

} // namespace Flight
