#include "Flight/Modes.hpp"

namespace Flight {

ModeOutput AttitudeLift(const StatsState &s, const StatsStatic &st, const Controls &c, const StandardParams &p,
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

float DroneSkid(float aClearance, float aVerticalSpeed, float aBodyUpZ, float aLift, const DroneSkidParams &k) {
  if (k.clearance <= 0.0f || aBodyUpZ < k.minUp || !std::isfinite(aClearance))
    return 0.0f;
  const float w = std::fmin(std::fmax((k.clearance + k.band - aClearance) / k.band, 0.0f), 1.0f);
  if (w <= 0.0f)
    return 0.0f;
  const float hold = k.floor ? 1.0f : 1.0f - std::fmin(std::fmax(-aLift, 0.0f), 1.0f);
  const float a = w * kGravity + k.p * (k.clearance - aClearance) - k.d * aVerticalSpeed;
  return std::fmin(std::fmax(a, 0.0f), k.maxAccel) * hold;
}

float GroundedLift(float aLiftAccel, float aClearance, float aBodyUpZ, const DroneSkidParams &k) {
  if (aLiftAccel >= -k.press || !std::isfinite(aClearance) || aBodyUpZ < k.minUp)
    return aLiftAccel;
  const float w = std::fmin(std::fmax((k.clearance + k.band - aClearance) / k.band, 0.0f), 1.0f);
  return aLiftAccel + w * (-k.press - aLiftAccel);
}

float ClosingSpeed(Vec3 aVelocity, Vec3 aGroundNormal) {
  return aGroundNormal.z > 0.1f ? Dot(aVelocity, aGroundNormal) / aGroundNormal.z : aVelocity.z;
}

ModeOutput DroneSkidPoints(const SkidPoint *aPoints, int aCount, const StatsState &s, const StatsStatic &st,
                           Vec3 aResponse, float aLift, const DroneSkidParams &k, float *aLoads) {
  ModeOutput out;
  if (aCount <= 0)
    return out;
  const Quat toBody = Conjugate(s.d_orientation);
  const Vec3 up = Rotate(toBody, kUp); // world up in the body frame
  const float share = 1.0f / static_cast<float>(aCount);
  float lift = 0.0f;
  Vec3 drag;   // the points' friction (body frame)
  Vec3 moment; // the points' moment about the centre of mass per unit of the car's mass (body frame)
  for (int i = 0; i < aCount; ++i) {
    const SkidPoint &p = aPoints[i];
    const Vec3 r = p.offset;
    const Vec3 pointVelocity = Add(s.d_velocity, Rotate(s.d_orientation, Cross(s.d_angularVelocity, r)));
    const float a = DroneSkid(p.clearance, ClosingSpeed(pointVelocity, p.normal), s.d_up.z, aLift, k) * share;
    lift += a;
    moment = Add(moment, Cross(r, Mul(up, a)));
    if (aLoads)
      aLoads[i] = a / kGravity;
    if (k.floor || p.friction <= 0.0f || a <= 0.0f || k.touch <= k.clearance)
      continue;
    const float touching = std::fmin(std::fmax((k.touch - p.clearance) / (k.touch - k.clearance), 0.0f), 1.0f);
    const Vec3 along = Sub(pointVelocity, Mul(p.normal, Dot(pointVelocity, p.normal)));
    const float slide = Length(along);
    if (touching <= 0.0f || !(slide > 1e-4f))
      continue;
    const Vec3 f = Rotate(toBody, Mul(along, -p.friction * a * touching / std::fmax(slide, k.softSlide)));
    drag = Add(drag, f);
    moment = Add(moment, Cross(r, f));
  }
  out.force = Add(Mul(up, lift), drag);
  auto axis = [&](float m, float inertia, float response) {
    return inertia > 0.0f && response > 0.0f ? k.turn * st.mass * m / inertia / response : 0.0f;
  };
  out.torque = {axis(moment.x, st.momentOfInertia.x, aResponse.x), axis(moment.y, st.momentOfInertia.y, aResponse.y),
                axis(moment.z, st.momentOfInertia.z, aResponse.z)};
  if (k.floor && aResponse.y > 0.0f && s.d_up.z >= k.minUp) {
    // upright while within the band: the roll about the body's forward (+ is the top to the right) back to level
    float closeness = 0.0f;
    for (int i = 0; i < aCount; ++i)
      closeness = std::fmax(closeness, std::fmin(std::fmax((k.clearance + k.band - aPoints[i].clearance) / k.band, 0.0f), 1.0f));
    const float roll = std::atan2(-up.x, up.z);
    out.torque.y += closeness * -(k.uprightP * roll + k.uprightD * s.d_angularVelocity.y) / aResponse.y;
  }
  return out;
}

namespace {
// The hover weight (HoverBand): Blend 1 at minHoverHeight, 0 at maxHoverHeight; Always 1 on any ground found; Never and
// without ground 0.
float HoverWeight(const Ground &g, const StandardParams &p, HoverBand band) {
  if (!g.found || band == HoverBand::Never)
    return 0.0f;
  if (band == HoverBand::Always)
    return 1.0f;
  float w = 1.0f - (g.distance - p.minHoverHeight) / (p.maxHoverHeight - p.minHoverHeight);
  return std::fmin(std::fmax(w, 0.0f), 1.0f);
}
} // namespace

Vec3 GroundRayStart(Vec3 aTire) { return {aTire.x, aTire.y, aTire.z + kGroundRayRise}; }

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
    // the rays are straight down, so the height over the hit is the ray's length when the ground is under the emitter
    distanceSum += hits[i].tire.z - hits[i].position.z;
    normalSum = Add(normalSum, Normalize(hits[i].normal));
  }
  float n = static_cast<float>(count);
  g.found = true;
  g.distance = distanceSum / n;
  g.normal = Normalize({normalSum.x / n, normalSum.y / n, normalSum.z / n});
  return g;
}

Vec3 TorqueResponse(const StatsStatic &st, Vec3 t) {
  auto axis = [&](float gain, float tensor, float inertia) { return inertia > 0.0f ? gain * st.mass * tensor / inertia : 0.0f; };
  return {axis(kEngineTorqueGain.x, t.x, st.momentOfInertia.x), axis(kEngineTorqueGain.y, std::fmax(t.y, 1.0f), st.momentOfInertia.y),
          axis(kEngineTorqueGain.z, t.z, st.momentOfInertia.z)};
}

ModeOutput Standard(StandardState &m, const StatsState &s, const StatsStatic &st, const Controls &c,
                    const StandardParams &p, const StandardGains &a, const Ground &g, Vec3 response, float dt,
                    HoverBand band) {
  auto clamp = [](float x, float lo, float hi) { return std::fmin(std::fmax(x, lo), hi); };
  Quat toBody = Conjugate(s.d_orientation);

  // the hover band; a moment without ground (a ray through a seam in the collision, off a
  // kerb's edge) is not leaving it: the hover height crept up 0.1 to 0.25 m a time on such frames, taken again at
  // the next frame's distance (in game, 2026-11-06)
  m.hovering = HoverWeight(g, p, band);
  if (m.hovering > 0.0f) {
    if (m.outOfBand >= kRegainHoverHeight)
      m.hoverHeight = g.distance;
    m.outOfBand = 0.0f;
  } else {
    m.outOfBand += dt;
  }
  m.hoverHeight = clamp(m.hoverHeight + c.lift * a.maxClimbRate * dt, p.minHoverHeight, p.maxHoverHeight);

  // attitude: the rotation (body frame) taking the body's up to the target up
  Vec3 targetUp = Normalize(Interpolate(kUp, g.found ? g.normal : kUp, m.hovering));
  Vec3 t = Rotate(toBody, targetUp);
  Vec3 axis{-t.y, t.x, 0.0f}; // (0, 0, 1) x t
  float sinAngle = Length(axis);
  float angle = std::atan2(sinAngle, t.z);
  Vec3 tilt = sinAngle > 1e-6f ? Mul(axis, std::fmin(angle, a.attitudeLimit) / sinAngle) : Vec3{};
  // the inputs tilt the target the way AttitudeLift's input angles do (pitch about -X, roll about +Y)
  tilt.x -= c.pitch * p.pitchInputAngle * (3.14159265f / 180.0f);
  tilt.y += c.roll * p.rollInputAngle * (3.14159265f / 180.0f);
  Vec3 w = s.d_angularVelocity;
  Vec3 alpha;
  alpha.x = (a.attitudeP * tilt.x - a.attitudeD * w.x) * (1.0f - c.angularBrake);
  alpha.y = (a.attitudeP * tilt.y - a.attitudeD * w.y) * (1.0f - c.angularBrake);

  // yaw: a rate about world up (AttitudeLift's sign: positive yaw input turns the body negatively about up)
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

  // the brake scales the travel the mode asks for; the height stays held (the old Hover & Fly's brake scaled its
  // height terms too, so in game the car rose while braking: whatever the game's vertical balance lacked, nothing
  // corrected it)
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

} // namespace Flight
