// The Autopilot's hold, descent and yaw (docs/design/autopilot.md, Descent): the velocity wanted
// off the route (holding, the descent's profile onto the landing spot, settling onto the wheels), the heading turned
// with the height lost, and the landing turn (LandSlewYaw) the nose makes into the descent.
// FlightModeAutopilot.reds line numbers (at c3e24ab) name what each part ports.

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

namespace {
// m the ground the wheel rays hit may be off the nav map's floor under the spot and still be landed on: further off it
// is something the map lacks (a canopy over the spot, a hole), and the car comes down onto the map's floor as it does
// touching down (held up, it hovers: "arrived")
constexpr float kWheelGroundMatch = 1.0f;
} // namespace

// The height the descent's profile ends at. Setting down: the rest height plus wheelClearance over the ground under
// the wheels (the nav map's floor under the spot until the rays find it); touching down: the spot (where the map has a
// resting car's centre), passed at touchdownRate.
float Autopilot::DescentTarget() {
  if (!setDown) {
    return landingSpot.z;
  }
  const float floor = landingSpot.z - world.CarHalfHeight();
  if (in->wheelGroundValid && std::fabs(in->wheelGroundZ - floor) < kWheelGroundMatch &&
      Distance2D(N(in->stats.d_position), landingSpot) < params.landingOverSpot) {
    if (in->wheelGroundSpread > params.wheelGroundSpread) {
      setDown = false;
      spdlog::info("[Autopilot] the ground under the wheels is {:.2f} m uneven: touching down", in->wheelGroundSpread);
      return landingSpot.z;
    }
    if (!setDownGround) {
      spdlog::info("[Autopilot] ground under the wheels at {:.2f} ({:+.2f} m off the map's floor, {:.2f} m uneven)",
                   in->wheelGroundZ, in->wheelGroundZ - floor, in->wheelGroundSpread);
    }
    setDownGround = true;
    setDownTarget = in->wheelGroundZ + in->restHeight + params.wheelClearance;
  } else if (!setDownGround) {
    setDownTarget = floor + in->restHeight + params.wheelClearance;
  }
  return setDownTarget;
}

// The descent's profile, a step: the reference height sinks speeding up at descentAccel and braking at it to end still
// on the target (setting down) or at touchdownRate (touching down, sinking on at it until the floor stops the car),
// capped at descentRate. From rest it is half speeding up, half braking. The target moving (the rays finding the
// ground, a moved spot) reshapes what is left: braking harder than 2 x descentAccel is not asked for, so a target
// come up too close is reached late and the hold catches it.
void Autopilot::DescentProfile(const Vec3 &aP, float aDt) {
  const float target = DescentTarget();
  const float a = std::max(params.descentAccel, 0.1f);
  const float vEnd = setDown ? 0.0f : params.touchdownRate;
  if (!descentProfile) {
    descentProfile = true;
    descentZ = aP.z;
    descentV = std::min(in->stats.d_velocity.z, 0.0f);
    descentA = 0.0f;
  }
  const float d = descentZ - target;
  const float sink = -descentV;
  float next = vEnd;
  if (d > 0.0f) {
    next = std::min({sink + a * aDt, std::sqrt(vEnd * vEnd + 2.0f * a * d), params.descentRate});
    next = std::max({next, sink - 2.0f * a * aDt, 0.0f});
  }
  descentA = aDt > 0.0f ? -(next - sink) / aDt : 0.0f;
  descentV = -next;
  descentZ += descentV * aDt;
  if (setDown && descentZ <= target) {
    descentZ = target;
    descentV = 0.0f;
    descentA = 0.0f;
  } else if (!setDown && d <= 0.0f) {
    // past the spot at touchdownRate: the speed alone, so the floor stopping the car does not wind up a push into it
    descentZ = aP.z;
  }
}

// Update 1852-1873: not following, approach the hold point, slowing down near it; descending, over the
// spot first at the height the descent began, then the descent's profile; landed, settling onto the wheels
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
    if (!descentProfile && Distance2D(aP, landingSpot) > params.landingOverSpot) {
      desired.z = Clamp(0.5f * (landingStartZ - aP.z), -2.0f, 2.0f);
    } else {
      DescentProfile(aP, in->dt);
      desired.z = descentV;
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
