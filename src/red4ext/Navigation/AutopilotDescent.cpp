// The Autopilot's hold, descent and yaw (docs/design/native-ports.md, M6 step 4d): the velocity wanted
// off the route (holding, sinking onto the landing spot, settling onto the wheels), the heading turned
// with the height lost, and the landing turn (LandSlewYaw) the nose makes into the descent.
// FlightModeAutopilot.reds line numbers name what each part ports.

#include "Autopilot.hpp"

#include "AutopilotDetail.hpp"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <cmath>

namespace Navigation {

using namespace AutopilotDetail;

// BlendHeading (1255)
Vec3 BlendHeading(const Vec3 &aA, const Vec3 &aB, float aF) {
  const float a0 = std::atan2(aA.y, aA.x);
  float delta = std::atan2(aB.y, aB.x) - a0;
  while (delta > kHalfTurn) {
    delta -= kTurn;
  }
  while (delta < -kHalfTurn) {
    delta += kTurn;
  }
  const float angle = a0 + delta * aF;
  return {std::cos(angle), std::sin(angle), 0.0f};
}

// Update 1852-1873: not following, approach the hold point, slowing down near it; descending, over the
// spot first at the height the descent began, then down at a capped rate slowing in the last metres;
// landed, settling onto the wheels
Vec3 Autopilot::HoldVelocity(const Vec3 &aP, const Vec3 &aToTarget) {
  // the next stretch of route is followed from the speed the car has
  tracker.speedCmd = in->stats.d_speed;
  tracker.speedRate = 0.0f;
  tracker.aheadSmooth = 15.0f;
  Vec3 desired = Mul(aToTarget, 0.8f);
  if (Length(desired) > 10.0f) {
    desired = Mul(Normalize(desired), 10.0f);
  }
  if (status == "descending") {
    // sliding across while sinking, the car can hit what is over the neighbouring columns
    if (Distance2D(aP, landingSpot) > params.landingOverSpot) {
      desired.z = Clamp(0.5f * (landingStartZ - aP.z), -2.0f, 2.0f);
    } else {
      desired.z = -Clamp(0.5f * (aP.z - landingSpot.z), params.touchdownRate, params.descentRate);
    }
  } else if (status == "landed") {
    // flight is being switched off: keep settling onto the wheels
    desired = {0.0f, 0.0f, -params.touchdownRate};
  }
  return desired;
}

// Update 1998-2002: along the desired velocity; descending, from the arrival heading to the landing heading
// in step with the height lost, so the turn and the descent end together
Vec3 Autopilot::Heading(const Vec3 &aP, const Vec3 &aDesired) const {
  Vec3 heading{aDesired.x, aDesired.y, 0.0f};
  if (status == "descending" || status == "landed") {
    const float f =
        Clamp((landingStartZ - aP.z) / std::max(landingStartZ - landingSpot.z, 0.5f), 0.0f, 1.0f);
    heading = Mul(BlendHeading(landingHeading0, landingHeading, f * f * (3.0f - 2.0f * f)), 10.0f);
  }
  return heading;
}

// TimeToDescent (2238): seconds the trajectory takes from here to where the descent takes over; 1000
// without one, so the landing turn does not start early
float Autopilot::TimeToDescent() const {
  if (!FollowsTrajectory()) {
    return 1000.0f;
  }
  const float total = trajectory.samples.back().s;
  return std::max(SampleAt(trajectory, total - params.landingEntry).t - SampleAt(trajectory, tracker.closestS).t, 0.0f);
}

// LandSlewYaw (2252): one turn of the nose from where it was when the turn began to the heading it lands on
// (the spot's most open line or the pad's, the way round nearest droneLandSlewYaw), carried on over the time
// left so a moved end stretches what is left of it. Returns the nose yaw wanted.
float Autopilot::LandSlewYaw(float aTimeLeft, float aDt, float aYaw) {
  if (!landSlewOn) {
    landSlewOn = true;
    landSlewP = 0.0f;
    landSlewStartYaw = aYaw;
    const float start = landSlewStartYaw;
    const Vec3 startHeading{std::cos(start), std::sin(start), 0.0f};
    const Vec3 base = landingPad >= 0 ? PadHeading(world.Pads()[size_t(landingPad)], startHeading)
                                      : LandingHeading(world, landingSpot, startHeading);
    // either way round (direction 1: the yaw rising, the nose turning left)
    const float want = Deg2Rad(params.droneLandSlewYaw);
    float best = -1.0f;
    float bestDirection = 1.0f;
    for (int d = 0; d < 2; ++d) {
      const float direction = d == 0 ? 1.0f : -1.0f;
      for (int i = 0; i < (landingPad >= 0 ? 1 : 2); ++i) {
        const Vec3 heading = i == 0 ? base : Mul(base, -1.0f);
        float turn = WrapAngle(std::atan2(heading.y, heading.x) - start) * direction;
        if (turn < 0.0f) {
          turn += kTurn;
        }
        if (best < 0.0f || std::fabs(turn - want) < std::fabs(best - want)) {
          best = turn;
          bestDirection = direction;
          landSlewHeading = heading;
        }
      }
    }
    landSlewDelta = best * bestDirection;
    spdlog::info("[Autopilot]   landing turn {:.0f} deg", Rad2Deg(landSlewDelta));
  }
  // over time, not distance: over distance most of the turn is done early in a long brake
  if (aDt > 0.0f) {
    landSlewP = Clamp(landSlewP + (1.0f - landSlewP) * aDt / std::max(aTimeLeft, aDt), 0.0f, 1.0f);
  }
  const float p = landSlewP;
  return landSlewStartYaw + landSlewDelta * p * p * (3.0f - 2.0f * p);
}

// ChooseYaw (2605): with droneBrakeTurn the nose keeps to the line until droneLandSlewBefore before the
// descent by the trajectory's clock, then turns to the landing heading, on droneLandSlewDuring into it
float Autopilot::ChooseYaw(float aDt, bool aLive, float aWantedYaw) {
  float wantedYaw = aWantedYaw;
  if (params.droneBrakeTurn && landSlewOn && (status == "descending" || status == "landed")) {
    landSlewDescent += aDt;
    wantedYaw = LandSlewYaw(std::max(params.droneLandSlewDuring - landSlewDescent, 0.0f), aDt, attitude.droneYaw);
  } else if (params.droneBrakeTurn && FollowsTrajectory() && status == "following") {
    const float timeLeft = landing ? TimeToDescent() : 1000.0f;
    if (landSlewOn || timeLeft <= params.droneLandSlewBefore) {
      wantedYaw = LandSlewYaw(timeLeft + params.droneLandSlewDuring, aDt, aLive ? attitude.droneYaw : wantedYaw);
    }
  }
  return wantedYaw;
}

} // namespace Navigation
