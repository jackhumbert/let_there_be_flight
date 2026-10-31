#include "DroneAttitude.hpp"

#include "Vec3Ops.hpp"

#include <algorithm>
#include <cmath>

namespace Navigation {

namespace {
constexpr float kPi = 3.1415927f;

float Clamp(float v, float lo, float hi) { return std::min(std::max(v, lo), hi); }

bool Follows(const AttitudeInput &aInput, const Trajectory *aTrajectory) {
  return aInput.following && aTrajectory && aTrajectory->samples.size() >= 2;
}

// the body's pitch and roll for a commanded acceleration, as the script works them out: the vertical
// floored, the body's share of the thrust's angle, at most aCap
struct Tilt {
  float pitch, roll;
};
Tilt TiltFor(const VehicleSpec &aSpec, const AttitudeParams &aParams, const Vec3 &aAccel, const Vec3 &aDrag,
             const Vec3 &aH, const Vec3 &aSide, float aG, float aSpeed, float aCap) {
  Vec3 thrust = V::Add(V::Add(aAccel, aDrag), Vec3{0.0f, 0.0f, aG});
  thrust.z = std::max(thrust.z, aSpec.tiltFloor * aG);
  float size = V::Len(thrust);
  float ahead = V::Dot(thrust, aH);
  float along = ahead >= 0.0f ? aSpec.surge : Decel(aSpec, aSpeed) - Drag(aSpec, aSpeed);
  Tilt tilt;
  tilt.pitch = Clamp(DroneBodyAngle(aSpec, std::atan2(ahead, thrust.z), size, along,
                                    ahead >= 0.0f ? aParams.droneBodyShare : aSpec.brakeShare),
                     -aCap, aCap);
  tilt.roll = Clamp(DroneBodyAngle(aSpec, std::atan2(V::Dot(thrust, aSide), thrust.z), size, aSpec.sway,
                                   aParams.droneBodyShare),
                    -aCap, aCap);
  return tilt;
}
} // namespace

float WrapAngle(float aAngle) {
  float x = aAngle;
  while (x > 3.1415927f) {
    x -= 6.2831853f;
  }
  while (x < -3.1415927f) {
    x += 6.2831853f;
  }
  return x;
}

float EaseAngle(float aDt, float aWanted, float aAngle, float aRateIn, float aF, float aRateMax, float aAccelMax,
                float &aRateOut) {
  float accel = Clamp(aF * aF * (aWanted - aAngle) - 2.0f * aF * aRateIn, -aAccelMax, aAccelMax);
  aRateOut = Clamp(aRateIn + accel * aDt, -aRateMax, aRateMax);
  return aAngle + aRateOut * aDt;
}

Vec3 DragFeedForward(const VehicleSpec &aSpec, const Vec3 &aVelocity, bool aOn) {
  float speed = V::Len(aVelocity);
  if (!aOn || speed < 0.5f) {
    return {0.0f, 0.0f, 0.0f};
  }
  return V::Mul(aVelocity, Drag(aSpec, speed) / speed);
}

float DroneHeadingYaw(const AttitudeState &aState, const AttitudeParams &aParams, const AttitudeInput &aInput,
                      const Route *aRoute, const Trajectory *aTrajectory) {
  // the horizontal part with its length is taken by hand: Flat returns a unit vector
  Vec3 ahead3 = V::Add(aInput.velocity, V::Mul(aInput.accel, aParams.attitudeLead));
  Vec3 path{ahead3.x, ahead3.y, 0.0f};
  float pathWeight = Clamp(V::Len(path) / 6.0f, 0.0f, 1.0f);
  if (aInput.onLine && aRoute) {
    float s = aInput.closestS + aInput.speed * aParams.droneYawLead;
    Vec3 tangent = aRoute->Tangent(s, aParams.lineSpan);
    if (V::Len(tangent) < 0.5f) {
      tangent = aInput.forward;
    }
    path = V::Flat(tangent);
    // turned into the bend as the trajectory plans (its drift), so the surge pushes round it
    if (Follows(aInput, aTrajectory)) {
      float drift = SampleAt(*aTrajectory, s).drift;
      path = {path.x * std::cos(drift) - path.y * std::sin(drift), path.x * std::sin(drift) + path.y * std::cos(drift),
              0.0f};
    }
    pathWeight = Clamp((V::Len(path) - 0.15f) / 0.35f, 0.0f, 1.0f);
  }
  // coming down onto the spot the nose keeps to the hover heading, not the drift
  if (aInput.descending) {
    pathWeight = 0.0f;
  }
  Vec3 previous = aInput.live ? Vec3{std::cos(aState.droneYaw), std::sin(aState.droneYaw), 0.0f} : aState.headingRef;
  Vec3 h = V::Mul(aState.headingRef, 1.0f - aInput.blend);
  if (V::Len(path) > 0.01f) {
    Vec3 mix = V::Add(V::Mul(V::Normalize(path), pathWeight), V::Mul(previous, 1.0f - pathWeight));
    h = V::Add(h, V::Mul(mix, aInput.blend));
  }
  h = V::Len(h) > 0.1f ? V::Normalize(h) : aState.headingRef;
  return std::atan2(h.y, h.x);
}

AttitudeOutput DroneAttitude(AttitudeState &aState, const AttitudeParams &aParams, const VehicleSpec &aSpec,
                             const AttitudeInput &aInput, float aWantedYaw, const Route *,
                             const Trajectory *aTrajectory) {
  const Vec3 z{0.0f, 0.0f, 1.0f};
  float dt = aInput.dt;
  float g = 9.81f * aSpec.gravityFactor;
  bool following = Follows(aInput, aTrajectory);

  // the heading eased like the tilt: rate and acceleration limited
  if (!aInput.live) {
    aState.droneYaw = aWantedYaw;
    aState.droneYawRate = 0.0f;
  }
  float rate = 0.0f;
  aState.droneYaw = WrapAngle(EaseAngle(dt, aState.droneYaw + WrapAngle(aWantedYaw - aState.droneYaw), aState.droneYaw,
                                        aState.droneYawRate, aParams.droneYawFrequency, aParams.droneYawRateMax,
                                        aParams.droneYawAccelMax, rate));
  aState.droneYawRate = rate;
  Vec3 h{std::cos(aState.droneYaw), std::sin(aState.droneYaw), 0.0f};
  // the hover heading starts from the nose
  aState.headingRef = h;
  Vec3 side = V::Normalize(V::Cross(h, z));

  float smooth = Clamp(dt / aParams.droneTiltTime, 0.0f, 1.0f);
  aState.attAccel = V::Add(aState.attAccel, V::Mul(V::Sub(aInput.accel, aState.attAccel), smooth));
  Vec3 drag = DragFeedForward(aSpec, aInput.velocity, aParams.dragFeedForward);
  float cap = aSpec.maxTilt * kPi / 180.0f;
  Tilt tilt = TiltFor(aSpec, aParams, aState.attAccel, drag, h, side, g, aInput.speed, cap);
  float pitch = tilt.pitch;
  float roll = tilt.roll;

  if (following) {
    // the planned tilt, plus what the follower's correction adds to the trajectory's own acceleration
    // (both worked out the same way, so on the trajectory that is 0)
    aState.attRef = V::Add(aState.attRef,
                           V::Mul(V::Sub(SampleAt(*aTrajectory, aInput.closestS).acceleration, aState.attRef), smooth));
    Tilt ref = TiltFor(aSpec, aParams, aState.attRef, drag, h, side, g, aInput.speed, cap);
    TrajectoryPoint plan = SampleAt(*aTrajectory, aInput.closestS + aInput.speed * aParams.trajectoryAttitudeLead);
    pitch = Clamp(plan.pitch + pitch - ref.pitch, -cap, cap);
    // the plan's roll is towards (h.y, -h.x), the heading turned right; side may point the other way
    float flip = V::Dot(side, Vec3{h.y, -h.x, 0.0f}) >= 0.0f ? 1.0f : -1.0f;
    roll = Clamp(flip * plan.roll + roll - ref.roll, -cap, cap);
  } else {
    aState.attRef = aState.attAccel;
  }

  // taking off: a little lean forward, as a player tips the nose into the surge
  if (aParams.droneTakeoffTilt > 0.0f && following && aInput.closestS < aParams.droneTakeoffLength &&
      SampleAt(*aTrajectory, aInput.closestS).along > 0.0f) {
    float into = Clamp(aInput.speed / std::max(aParams.droneTakeoffIn, 0.1f), 0.0f, 1.0f);
    float out = Clamp((aParams.droneTakeoffOut - aInput.speed) /
                          std::max(aParams.droneTakeoffOut - aParams.droneTakeoffFull, 0.1f),
                      0.0f, 1.0f);
    float w = into * into * (3.0f - 2.0f * into) * out * out * (3.0f - 2.0f * out);
    pitch = std::max(pitch, aParams.droneTakeoffTilt * kPi / 180.0f * w);
  }

  if (!aInput.live) {
    aState.dronePitch = pitch;
    aState.droneRoll = roll;
    aState.dronePitchRate = 0.0f;
    aState.droneRollRate = 0.0f;
  }
  aState.dronePitch = EaseAngle(dt, pitch, aState.dronePitch, aState.dronePitchRate, aParams.racePitchFrequency,
                                aParams.racePitchRateMax, aParams.racePitchAccelMax, rate);
  aState.dronePitchRate = rate;
  aState.droneRoll = EaseAngle(dt, roll, aState.droneRoll, aState.droneRollRate, aParams.raceRollFrequency,
                               aParams.raceRollRateMax, aParams.raceRollAccelMax, rate);
  aState.droneRollRate = rate;

  // top towards the thrust: tilted forward is nose down, as a quadcopter speeding up
  AttitudeOutput result;
  Vec3 lean = V::Add(V::Mul(h, std::tan(aState.dronePitch)), V::Mul(side, std::tan(aState.droneRoll)));
  result.up = V::Normalize(V::Add(z, lean));
  result.forward = V::Normalize(V::Across(h, result.up));
  return result;
}

} // namespace Navigation
