#include "follow.hpp"

#include "Navigation/Vec3Ops.hpp"

#include <algorithm>
#include <cmath>

namespace Navigation {
namespace {

float Clamp(float aX, float aLo, float aHi) { return std::min(std::max(aX, aLo), aHi); }

} // namespace

float Percentile(const std::vector<float> &aSorted, double aQ) {
  if (aSorted.empty()) {
    return 0.0f;
  }
  const double n = double(aSorted.size());
  const auto rank = int64_t(std::ceil(aQ * n - 1e-9)); // 1-based; the epsilon keeps 0.95 x 20 at 19
  return aSorted[size_t(std::min<int64_t>(std::max<int64_t>(rank, 1), int64_t(aSorted.size())) - 1)];
}

FollowResult Follow(const VehicleSpec &aSpec, const Route &aRoute, const Trajectory &aTrajectory, const Space &aSpace,
                    const FollowParams &aParams) {
  using namespace V;
  FollowResult result;
  if (aRoute.Size() < 2 || aTrajectory.samples.size() < 2) {
    return result;
  }
  const float dt = aParams.dt, g = 9.81f * aSpec.gravityFactor;
  const float maxTime = aParams.maxTime > 0.0f ? aParams.maxTime : 2.0f * aTrajectory.Duration() + 30.0f;

  Vec3 p = aRoute.points[0], v = aTrajectory.samples[0].velocity;
  TrackerState state;
  state.speedCmd = Len(v);
  AttitudeState att;
  att.headingRef = Flat(aRoute.Tangent(0.0f, aParams.attitude.lineSpan));
  if (Len(att.headingRef) == 0.0f) {
    att.headingRef = {1.0f, 0.0f, 0.0f};
  }
  att.droneYaw = std::atan2(att.headingRef.y, att.headingRef.x);
  Vec3 forward = att.headingRef;

  for (int32_t k = 0; float(k) * dt <= maxTime; ++k) {
    const float t = float(k) * dt, speed = Len(v);

    TrackInput ti;
    ti.dt = dt;
    ti.position = p;
    ti.velocity = v;
    ti.speed = speed;
    ti.routeLine = true;
    ti.planned = true;
    const TrackOutput tr = Track(state, aParams.tracker, aSpec, ti, aRoute, &aTrajectory, aSpace);
    result.replans += tr.replanLine ? 1 : 0;

    CommandInput ci;
    ci.dt = dt;
    ci.position = p;
    ci.velocity = v;
    ci.speed = speed;
    ci.forward = forward;
    ci.following = true;
    ci.routeLine = true;
    ci.planned = true;
    ci.target = tr.target;
    ci.surgeFree = speed > 1.0f;
    const Vec3 aCmd = Command(state, aParams.tracker, ci, aRoute, &aTrajectory).accel;

    float blend = Clamp((speed - 4.0f) / 8.0f, 0.0f, 1.0f);
    blend = blend * blend * (3.0f - 2.0f * blend);
    AttitudeInput ai;
    ai.dt = dt;
    ai.accel = aCmd;
    ai.blend = blend;
    ai.live = k > 0;
    ai.velocity = v;
    ai.speed = speed;
    ai.forward = forward;
    ai.onLine = true;
    ai.closestS = state.closestS;
    ai.following = true;
    ai.descending = false;
    const float yaw = DroneHeadingYaw(att, aParams.attitude, ai, &aRoute, &aTrajectory);
    const AttitudeOutput out = DroneAttitude(att, aParams.attitude, aSpec, ai, yaw, &aRoute, &aTrajectory);
    forward = out.forward;

    // the thrust in the target's frame, inside what the car's controls give (FlyAerobatic)
    const Vec3 fwd = out.forward, up0 = out.up;
    const Vec3 right = Normalize(Cross(fwd, up0)), up = Cross(right, fwd);
    const Vec3 w = Add(Add(aCmd, DragFeedForward(aSpec, v, aParams.attitude.dragFeedForward)), Vec3{0.0f, 0.0f, g});
    auto local = [&](const Vec3 &aVec) { return Vec3{Dot(aVec, right), Dot(aVec, fwd), Dot(aVec, up)}; };
    Vec3 th = local(w);
    th.x = Clamp(th.x, -aSpec.sway, aSpec.sway);
    th.y = Clamp(th.y, Navigation::Drag(aSpec, speed) - Decel(aSpec, speed), aSpec.surge);
    th.z = Clamp(th.z, g - aSpec.lift, g + aSpec.lift);
    if (state.surge.held) {
      // AutopilotSurge.Full: more along the path until the surge is full
      const Vec3 path = local(Normalize(v));
      if (path.y >= 0.2f && th.y < aSpec.surge) {
        th = Add(th, Mul(path, (aSpec.surge - th.y) / path.y));
      }
      th.x = Clamp(th.x, -aSpec.sway, aSpec.sway);
      th.z = Clamp(th.z, g - aSpec.lift, g + aSpec.lift);
    }

    const Vec3 thrust = Add(Add(Mul(right, th.x), Mul(fwd, th.y)), Mul(up, th.z));
    Vec3 a = Sub(thrust, Vec3{0.0f, 0.0f, g});
    if (aSpec.drag && speed > 1e-3f) {
      a = Sub(a, Mul(Mul(v, 1.0f / speed), Navigation::Drag(aSpec, speed)));
    }

    FollowFrame f;
    f.t = t;
    f.s = state.closestS;
    f.off = state.offRouteNow;
    f.speed = speed;
    f.refSpeed = SampleAt(aTrajectory, state.closestS).speed;
    f.leadSpeed = SampleAt(aTrajectory, state.closestS + speed * aParams.tracker.lineLead).speed;
    f.pitch = att.dronePitch;
    f.roll = att.droneRoll;
    f.yaw = att.droneYaw;
    f.p = p;
    f.v = v;
    f.accel = a;
    f.thrust = thrust;
    f.held = state.surge.held;
    f.blocked = state.blockedAhead;
    f.emergency = state.emergencyBrake;
    result.frames.push_back(f);

    result.arrived = state.remaining < aParams.arriveDistance && speed < aParams.arriveSpeed;
    result.ended = state.remaining < aParams.endDistance;
    if (result.arrived || result.ended) {
      break;
    }
    v = Add(v, Mul(a, dt));
    p = Add(p, Mul(v, dt));
  }

  if (result.frames.empty()) {
    return result;
  }
  result.time = result.frames.back().t;
  std::vector<float> off, err;
  bool inLost = false;
  double errSum = 0.0, biasSum = 0.0;
  for (const auto &f : result.frames) {
    off.push_back(f.off);
    err.push_back(std::fabs(f.speed - f.leadSpeed));
    errSum += err.back();
    biasSum += f.speed - f.leadSpeed;
    const bool lost = f.off > aParams.tracker.raceOffLine;
    if (lost && !inLost) {
      ++result.lost;
      result.lostAt.push_back(f.s);
    }
    inLost = lost;
    result.held += f.held ? 1 : 0;
    result.blocked += f.blocked ? 1 : 0;
    result.emergency += f.emergency ? 1 : 0;
  }
  std::sort(off.begin(), off.end());
  std::sort(err.begin(), err.end());
  result.offP50 = Percentile(off, 0.50);
  result.offP95 = Percentile(off, 0.95);
  result.offP99 = Percentile(off, 0.99);
  result.offMax = off.back();
  result.speedErrMean = float(errSum / double(err.size()));
  result.speedBiasMean = float(biasSum / double(err.size()));
  result.speedErrP95 = Percentile(err, 0.95);
  return result;
}

} // namespace Navigation
