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

Vec3 GroundRayEnd(Vec3 aTire, float aMaxHoverHeight) { return {aTire.x, aTire.y, aTire.z - aMaxHoverHeight - 10.0f}; }

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

Vec3 TorqueResponse(const StatsStatic &st, Vec3 t) {
  auto axis = [&](float gain, float tensor, float inertia) { return inertia > 0.0f ? gain * st.mass * tensor / inertia : 0.0f; };
  return {axis(kEngineTorqueGain.x, t.x, st.momentOfInertia.x), axis(kEngineTorqueGain.y, std::fmax(t.y, 1.0f), st.momentOfInertia.y),
          axis(kEngineTorqueGain.z, t.z, st.momentOfInertia.z)};
}

ModeOutput HoverFlyAssist(HoverFlyState &m, const StatsState &s, const StatsStatic &st, const Controls &c,
                          const StandardParams &p, const HoverFlyAssistParams &a, const Ground &g, Vec3 response,
                          float dt) {
  auto clamp = [](float x, float lo, float hi) { return std::fmin(std::fmax(x, lo), hi); };
  Quat toBody = Conjugate(s.d_orientation);

  // the hover band, as HoverFly enters it
  float lastHovering = m.hovering;
  m.hovering = HoverWeight(g, p);
  if (lastHovering == 0.0f && m.hovering > 0.0f)
    m.hoverHeight = g.distance;
  m.hoverHeight = clamp(m.hoverHeight + c.lift * a.maxClimbRate * dt, p.minHoverHeight, p.maxHoverHeight);

  // attitude: the rotation (body frame) taking the body's up to the target up
  Vec3 targetUp = Normalize(Interpolate(kUp, g.found ? g.normal : kUp, m.hovering));
  Vec3 t = Rotate(toBody, targetUp);
  Vec3 axis{-t.y, t.x, 0.0f}; // (0, 0, 1) x t
  float sinAngle = Length(axis);
  float angle = std::atan2(sinAngle, t.z);
  Vec3 tilt = sinAngle > 1e-6f ? Mul(axis, std::fmin(angle, a.attitudeLimit) / sinAngle) : Vec3{};
  // the inputs tilt the target the way StandardLift's input angles do (pitch about -X, roll about +Y)
  tilt.x -= c.pitch * p.pitchInputAngle * (3.14159265f / 180.0f);
  tilt.y += c.roll * p.rollInputAngle * (3.14159265f / 180.0f);
  Vec3 w = s.d_angularVelocity;
  Vec3 alpha;
  alpha.x = (a.attitudeP * tilt.x - a.attitudeD * w.x) * (1.0f - c.angularBrake);
  alpha.y = (a.attitudeP * tilt.y - a.attitudeD * w.y) * (1.0f - c.angularBrake);

  // yaw: a rate about world up (StandardLift's sign: positive yaw input turns the body negatively about up)
  Vec3 up = s.d_localUp;
  float yawRate = Dot(w, up);
  // at speed, no faster than the sideways grip can turn the velocity with the nose (80% of it)
  float yawLimit = std::fmin(a.maxYawRate, 0.8f * a.maxSideGrip / std::fmax(s.d_speed2D, 1.0f));
  alpha = Add(alpha, Mul(up, a.yawP * (-c.yaw * yawLimit - yawRate)));

  // the angular acceleration as mode torque, axis by axis
  ModeOutput out;
  out.torque = {response.x > 0.0f ? alpha.x / response.x : 0.0f, response.y > 0.0f ? alpha.y / response.y : 0.0f,
                response.z > 0.0f ? alpha.z / response.z : 0.0f};

  // travel: along the level forward and right
  Vec3 fwd = s.d_forward2D;
  Vec3 right{fwd.y, -fwd.x, 0.0f};
  float vf = Dot(s.d_velocity, fwd), vs = Dot(s.d_velocity, right);
  float af = c.surge * p.surgeFactor - (1.0f - std::fabs(c.surge)) * clamp(a.brake * vf, -a.maxBrake, a.maxBrake);
  float as = c.sway * p.swayFactor - (1.0f - std::fabs(c.sway)) * clamp(a.sideBrake * vs, -a.maxSideGrip, a.maxSideGrip);

  // height
  float z = s.d_position.z, vz = s.d_velocity.z;
  float fly;
  if (std::fabs(c.lift) > 0.01f || m.hovering > 0.0f) {
    m.holdingAltitude = false;
    fly = a.climbP * (c.lift * a.maxClimbRate - vz);
  } else if (!m.holdingAltitude) {
    // let go: stop the climb or sink, then hold where it stopped
    fly = -a.climbP * vz;
    if (std::fabs(vz) < 0.3f) {
      m.holdingAltitude = true;
      m.referenceZ = z;
    }
  } else {
    fly = a.holdP * (m.referenceZ - z) - a.holdD * vz;
  }
  fly = clamp(fly, -p.liftFactor, p.liftFactor);
  float hover = clamp(a.hoverP * (m.hoverHeight - g.distance) - a.hoverD * vz, -p.liftFactor, p.liftFactor);
  float az = Lerp(m.hovering, fly, hover);

  // the brake scales the travel the mode asks for; the height stays held (HoverFly's brake scales its height terms
  // too, so in game the car rose while braking: whatever the game's vertical balance lacks, nothing corrected it)
  Vec3 accel = Add(Mul(Add(Mul(fwd, af), Mul(right, as)), 1.0f - c.linearBrake), Mul(kUp, az));
  accel.z += kGravity * p.gravityFactor;
  out.force = Rotate(toBody, accel);
  out.force = Sub(out.force, Mul(s.d_localVelocity, c.linearBrake * p.brakeFactorLinear * st.brakingFrictionFactor));
  return out;
}

ModeOutput DroneAssist(const StatsState &s, const StatsStatic &st, const Controls &c, const DroneParams &p,
                       const DroneAssistParams &a, Vec3 response) {
  ModeOutput out = Drone(s, st, c, p);
  // the rate each input asks for, with Drone's signs: pitch and yaw turn the body negatively about X and Z
  float keep = 1.0f - c.angularBrake;
  Vec3 target{-c.pitch * a.maxRate.x * keep, c.roll * a.maxRate.y * keep, -c.yaw * a.maxRate.z * keep};
  Vec3 alpha = Mul(Sub(target, s.d_angularVelocity), a.rateP);
  out.torque = {response.x > 0.0f ? alpha.x / response.x : 0.0f, response.y > 0.0f ? alpha.y / response.y : 0.0f,
                response.z > 0.0f ? alpha.z / response.z : 0.0f};
  return out;
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
