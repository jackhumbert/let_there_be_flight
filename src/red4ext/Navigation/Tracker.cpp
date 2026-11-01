#include "Tracker.hpp"

#include "Vec3Ops.hpp"

#include <algorithm>
#include <cmath>

namespace Navigation {

namespace {
float Clamp(float v, float lo, float hi) { return std::min(std::max(v, lo), hi); }

bool Valid(const Trajectory *aTrajectory) { return aTrajectory && aTrajectory->samples.size() >= 2; }

// The predicted path from + v t + accel t^2 / 2 for t up to the horizon, in three pieces, is blocked in
// aSpace; aTravelled is the length flown before the first blocked point, aHit that point.
bool PredictBlocked(const TrackerParams &aParams, const Space &aSpace, const Vec3 &aFrom, const Vec3 &aV,
                    const Vec3 &aAccel, float &aTravelled, Vec3 &aHit) {
  Vec3 prev = aFrom;
  aTravelled = 0.0f;
  for (int k = 1; k <= 3; ++k) {
    const float t = aParams.collisionHorizon * float(k) / 3.0f;
    const Vec3 q = V::Add(V::Add(aFrom, V::Mul(aV, t)), V::Mul(aAccel, 0.5f * t * t));
    const float length = V::Len(V::Sub(q, prev));
    const float clear = aSpace.SegmentClear(prev, q);
    if (clear < 1.0f) {
      aTravelled += clear * length;
      aHit = V::Add(prev, V::Mul(V::Sub(q, prev), clear));
      return true;
    }
    aTravelled += length;
    prev = q;
  }
  return false;
}
} // namespace

TrackOutput Track(TrackerState &aState, const TrackerParams &aParams, const VehicleSpec &aSpec, const TrackInput &aInput,
                  const Route &aRoute, const Trajectory *aTrajectory, const Space &aSpace) {
  TrackOutput out;
  const Vec3 &p = aInput.position;
  const Vec3 &v = aInput.velocity;
  const float speed = aInput.speed;
  const bool planned = aInput.planned && Valid(aTrajectory);

  aState.segment = aRoute.Nearest(p, aState.segment, 12);
  aState.closestNow = aRoute.Project(p, aState.segment);
  const Vec3 closest = aState.closestNow;
  const float off = V::Len(V::Sub(p, closest));
  aState.offRouteNow = off;
  aState.closestS = aRoute.Distance(aState.segment) + V::Len(V::Sub(aRoute.Point(aState.segment), closest));
  aState.remaining = aRoute.Length() - aState.closestS;

  if (aInput.routeLine) {
    out.replanLine = off > aParams.replanOffLine && !aInput.recovering && aInput.replanTimer <= 2.0f &&
                     !aInput.requestPending;
  } else {
    out.requestRoute = off > aParams.replanDistance && aInput.replanTimer <= 0.0f && !aInput.requestPending;
  }

  float ahead = Clamp(0.7f * speed + 12.0f, 15.0f, 90.0f);
  // not far past the next sharp bend: a long lookahead cuts inside it
  if (aState.segment + 2 < aRoute.Size()) {
    ahead = std::min(ahead, aRoute.SharpBend(aState.segment, closest, aParams.cornerSpan) + aParams.cornerOvershoot);
  }
  Vec3 target = aRoute.Ahead(aState.segment, closest, ahead);
  // the straight line to the aim point must be free, or the car cuts through what the route bends
  // around: pulled back along the route until it is, checked from the car or, inside the map's margin, the route
  const Vec3 from = aSpace.IsFree(p) ? p : closest;
  int tries = 0;
  while (tries < 6 && ahead > 4.0f && aSpace.SegmentClear(from, target) < 1.0f) {
    ahead *= 0.6f;
    target = aRoute.Ahead(aState.segment, closest, ahead);
    ++tries;
  }
  // pulled back at once, grown back smoothly
  if (ahead < aState.aheadSmooth) {
    aState.aheadSmooth = ahead;
  } else {
    aState.aheadSmooth += (ahead - aState.aheadSmooth) * Clamp(aInput.dt / 0.6f, 0.0f, 1.0f);
    ahead = aState.aheadSmooth;
    target = aRoute.Ahead(aState.segment, closest, ahead);
  }
  aState.curvatureAhead = aRoute.Turn(aState.segment, closest, ahead);
  // smoothed: it steps as route points pass
  const Vec3 curvature = aRoute.Curvature(aState.segment, closest, ahead);
  aState.pathCurvature = V::Add(aState.pathCurvature,
                                V::Mul(V::Sub(curvature, aState.pathCurvature), Clamp(aInput.dt / 0.25f, 0.0f, 1.0f)));
  out.target = V::Add(target, aInput.aimOffset);

  // the trajectory's speed and its rate at the car's place on the line, a moment ahead
  const float sRef = aState.closestS + speed * aParams.lineLead;
  TrajectoryPoint ref;
  if (planned) {
    ref = SampleAt(*aTrajectory, sRef);
  }
  aState.targetSpeed = planned ? ref.speed : 0.0f;

  // where the velocity is taking the car must be free; brake to stop short of the first blocked point
  // otherwise, only when the car is leaving a route that is clear over the same distance
  aState.blockedAhead = false;
  if (speed > 3.0f) {
    float travelled = 0.0f;
    Vec3 hit{};
    const bool blocked = PredictBlocked(aParams, aSpace, from, v, aState.accelCmd, travelled, hit);
    const float reachLength = V::Len(v) * aParams.collisionHorizon;
    const bool lineHeld = aInput.routeLine && off < aParams.raceOffLine;
    if (blocked && !lineHeld && travelled > 10.0f &&
        aRoute.DistanceTo(hit, aState.segment, speed * aParams.collisionHorizon + 50.0f) > 2.0f &&
        aRoute.ClearAhead(aState.segment, closest, reachLength, aSpace)) {
      const float room = std::max(travelled - 5.0f, 0.0f);
      aState.targetSpeed = std::min(aState.targetSpeed, std::sqrt(2.0f * aSpec.margin * Decel(aSpec, 0.0f) * room));
      aState.blockedAhead = true;
    }
  }
  aState.speedCmd = aState.targetSpeed;
  aState.speedRate = planned ? ref.along : -Decel(aSpec, speed);
  if (aState.blockedAhead) {
    // the gap from the car's speed to the blocked one closed at 2 speedGain, at most the decel
    const float brake =
        std::min(std::max(speed - aState.targetSpeed, 0.0f) * 2.0f * aParams.speedGain, Decel(aSpec, speed));
    aState.speedRate = std::min(aState.speedRate, -brake);
  }
  // braked for like any other limit while there is room to stop at brakeMax, hard only when there is
  // not and it has been blocked for a moment
  aState.emergencyBrake = false;
  // once clear again, carry on from the speed the car has
  if (!aState.blockedAhead && aState.blockedTimer > 0.2f) {
    aState.speedCmd = std::max(aState.speedCmd, speed);
    aState.speedRate = std::max(aState.speedRate, 0.0f);
  }
  aState.blockedTimer = aState.blockedAhead ? aState.blockedTimer + aInput.dt : 0.0f;
  if (aState.blockedAhead && aState.blockedTimer > 0.2f && speed > aState.targetSpeed) {
    const float room = std::max(aState.targetSpeed * aState.targetSpeed / (2.0f * aParams.brakeAccel), 0.5f);
    if (speed * speed / (2.0f * room) > aParams.brakeMax) {
      aState.speedCmd = aState.targetSpeed;
      aState.speedRate = std::min(aState.speedRate, 0.0f);
      aState.emergencyBrake = true;
    }
  }
  return out;
}

CommandOutput Command(TrackerState &aState, const TrackerParams &aParams, const CommandInput &aInput, const Route &aRoute,
                      const Trajectory *aTrajectory) {
  CommandOutput out;
  const Vec3 &p = aInput.position;
  const Vec3 &v = aInput.velocity;
  const float speed = aInput.speed;
  const bool planned = aInput.planned && Valid(aTrajectory);
  const bool following = aInput.following;
  out.onLine = following && aInput.routeLine;

  const Vec3 toTarget = V::Sub(aInput.target, p);
  Vec3 desired{};
  Vec3 accel{};
  float jerk = aParams.accelJerk;
  out.lookahead = aInput.target;
  if (following) {
    desired = V::Add(V::Mul(V::Normalize(toTarget), aState.speedCmd), aInput.overlayVelocity);
    accel = V::Mul(V::Sub(desired, v), aParams.velocityGain);
  } else {
    // the script clears these at the top of every frame; Track does it when following, here the rest
    aState.targetSpeed = 0.0f;
    aState.emergencyBrake = false;
    // the next stretch of route is followed from the speed the car has
    aState.speedCmd = speed;
    aState.speedRate = 0.0f;
    aState.aheadSmooth = 15.0f;
    desired = aInput.rawDesiredVelocity;
    accel = aInput.rawAccel;
    jerk = aInput.rawJerk;
  }
  if (out.onLine) {
    Vec3 tangent = aRoute.Tangent(aState.closestS, aParams.lineSpan);
    if (V::Len(tangent) < 0.5f) {
      tangent = aInput.forward;
    }
    // the bend at the lead point, as the script's second LineFrame call leaves it
    const Vec3 bend = aRoute.Bend(aState.closestS + speed * aParams.lineLead, aParams.lineSpan);
    Vec3 across = V::Sub(aState.closestNow, p);
    across = V::Sub(across, V::Mul(tangent, V::Dot(across, tangent)));
    Vec3 back = V::Mul(across, aParams.lineSpring);
    if (V::Len(back) > 15.0f) {
      back = V::Mul(V::Normalize(back), 15.0f);
    }
    desired = V::Add(V::Add(V::Mul(tangent, aState.speedCmd), back), aInput.overlayVelocity);
    // the bend at the lower of the speed flown and the trajectory's
    const float bendSpeed = planned ? std::min(speed, aState.speedCmd) : aState.speedCmd;
    Vec3 correct = V::Mul(V::Sub(desired, v), aParams.lineVelocityGain);
    // on a trajectory its own acceleration goes in whole and the correction is capped
    if (planned && V::Len(correct) > aParams.trajectoryCorrectionMax) {
      correct = V::Mul(V::Normalize(correct), aParams.trajectoryCorrectionMax);
    }
    accel = V::Add(V::Add(correct, V::Mul(bend, bendSpeed * bendSpeed)), V::Mul(tangent, aState.speedRate));
    out.lookahead = aRoute.At(aState.closestS + 30.0f);
    jerk = aParams.raceJerk;
  } else if (following) {
    // the bend's own acceleration ahead of time, and the trajectory's rate along the route
    accel = V::Add(accel, V::Mul(aState.pathCurvature, aState.speedCmd * aState.speedCmd));
    if (V::Len(toTarget) > 0.1f) {
      accel = V::Add(accel, V::Mul(V::Normalize(toTarget), aState.speedRate));
    }
    jerk = aParams.raceJerk;
  }
  // after a hit, while the controls are damped: hold the velocity the car has
  if (aInput.recovering && following) {
    accel = Vec3{0.0f, 0.0f, 0.0f};
    desired = v;
  }
  // rate-limit the command, except in an emergency stop for blocked space
  Vec3 change = V::Sub(accel, aState.accelCmd);
  const float maxChange = jerk * aInput.dt;
  if (!aState.emergencyBrake && V::Len(change) > maxChange) {
    change = V::Mul(V::Normalize(change), maxChange);
  }
  aState.accelCmd = V::Add(aState.accelCmd, change);
  out.accel = aState.accelCmd;
  out.desiredVelocity = desired;

  // the surge hold, where the speed is the car's to set
  const bool free = aInput.surgeFree && out.onLine && !aState.blockedAhead && !aState.emergencyBrake;
  if (Valid(aTrajectory)) {
    const TrajectoryPoint ref = SampleAt(*aTrajectory, aState.closestS + speed * aParams.lineLead);
    UpdateSurge(aState.surge, aParams.surge, free, true, ref.limit, ref.speed, speed, aInput.dt);
  } else {
    UpdateSurge(aState.surge, aParams.surge, free, false, TrajectorySample::Limit::End, 0.0f, speed, aInput.dt);
  }
  return out;
}

} // namespace Navigation
