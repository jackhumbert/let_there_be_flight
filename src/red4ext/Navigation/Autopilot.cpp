// The Autopilot's lifecycle and its frame: Activate, Deactivate, Teleported, the car's limits, Step (the
// script's Update: the override, the follower, the state machine) and FlyAerobatic's drone path.
// FlightModeAutopilot.reds line numbers (at c3e24ab, the last commit with the script mode) name what each part ports.

#include "Autopilot.hpp"

#include "AutopilotDetail.hpp"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <cmath>
#include <utility>

namespace Navigation {

using namespace AutopilotDetail;

uint64_t Rng::Next() {
  // splitmix64
  uint64_t z = (state += 0x9E3779B97F4A7C15ull);
  z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
  z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
  return z ^ (z >> 31);
}

float Rng::Unit() { return float(Next() >> 40) / float(1ull << 24); }

float Rng::RangeF(float aMin, float aMax) { return aMin + (aMax - aMin) * Unit(); }

int32_t Rng::Range(int32_t aMin, int32_t aMax) {
  if (aMax <= aMin) {
    return aMin;
  }
  return aMin + int32_t(Next() % uint64_t(aMax - aMin));
}

Autopilot::Autopilot(const World &aWorld, PlanQueue &aPlans, const AutopilotParams &aParams, Rng &aRng)
    : world(aWorld), plans(aPlans), params(aParams), rng(aRng) {}

const Flight::DeveloperSettings &Autopilot::Dev() const {
  return in && in->developer ? *in->developer : Flight::Developer();
}

AutopilotEvents Autopilot::TakeEvents() { return std::exchange(pending, AutopilotEvents{}); }

// CurrentDestination (868): a summon's goal for this flight wins over the tracked one
bool Autopilot::CurrentDestination(Vec3 &aTarget) const {
  if (hasExternalDestination) {
    aTarget = externalDestination;
    return true;
  }
  if (in && in->tracked.has) {
    aTarget = in->tracked.at;
    return true;
  }
  return false;
}

// PlayerInput (1385)
bool Autopilot::PlayerInput() const {
  const auto &c = in->controls;
  return std::fabs(c.surge) > 0.15f || std::fabs(c.yaw) > 0.15f || std::fabs(c.lift) > 0.15f ||
         std::fabs(c.sway) > 0.15f || c.linearBrake > 0.15f;
}

bool Autopilot::JumpHeldOff(const Vec3 &aPosition, float aSpeed, float aDt) const {
  const float reach = std::max(aSpeed, 20.0f) * std::max(aDt, 0.02f) * 3.0f + 10.0f;
  return Distance(aPosition, lastPosition) > reach && positionJumpTimer < params.positionJumpHold;
}

// CheckedPosition (1410): a jump further than the car could have flown held off for up to positionJumpHold
Vec3 Autopilot::CheckedPosition(const Vec3 &aPosition, float aDt) {
  if (JumpHeldOff(aPosition, in->stats.d_speed, aDt)) {
    if (positionJumpTimer == 0.0f) {
      spdlog::warn("[Autopilot] position jumped {:.0f} m to {:.1f}, {:.1f}, {:.1f}: held off",
                   Distance(aPosition, lastPosition), aPosition.x, aPosition.y, aPosition.z);
    }
    positionJumpTimer += aDt;
    return lastPosition;
  }
  positionJumpTimer = 0.0f;
  lastPosition = aPosition;
  return aPosition;
}

// RefreshVehicle (2013) and FlightVehicleModel.Spec: the car's limits from its stats and the tuning, the
// effective car (its thrust scaled as the developer settings say), surge and sway as it flies them
void Autopilot::RefreshVehicle() {
  const auto &st = in->statics;
  const auto &dev = Dev();
  VehicleSpec s;
  s.mass = st.mass;
  s.airResistance = st.airResistanceFactor;
  s.brakingFriction = st.brakingFrictionFactor;
  s.surge = in->standard.surgeFactor;
  s.sway = in->standard.swayFactor;
  s.lift = in->standard.liftFactor;
  s.gravityFactor = in->standard.gravityFactor;
  s.brakeLinear = in->settings.brakeFactorLinear;
  s.drag = in->settings.physxFlightPhysics;
  s.drone = true;
  s.maxTilt = params.droneMaxTilt;
  s.brakeShare = params.droneBrakeShare;
  s.tiltFloor = params.droneTiltFloor;
  s.driftMax = params.raceDriftMax;
  s.speedCap = freeRoam ? params.freeRoamTopSpeed : params.raceSpeedCap;
  s.margin = params.raceMargin;
  s.planDecel = params.racePlanDecel;
  s.pushOver = params.dronePushOver;
  s.cdaFront = dev.shapeDrag ? st.dragArea.y : 0.0f;
  s.cdaTop = dev.shapeDrag ? st.dragArea.z : 0.0f;
  s.dragK = dev.dragK;
  s.thrusters = st.thrusters;
  s.thrustStrength = dev.thrustStrength;
  s.thrustPerThruster = dev.thrustPerThruster;
  vehicle = Effective(s);
  surgeMax = vehicle.surge;
  swayMax = vehicle.sway;
}

// AutopilotSurge.Drag and FlightVehicleModel.DragVector: the drag the air takes from the car, to thrust back
Vec3 Autopilot::SurgeDrag(const Vec3 &aVelocity) const {
  if (!surgeHold.dragFeedForward) {
    return {0.0f, 0.0f, 0.0f};
  }
  const Flight::Vec3 v = F(aVelocity);
  const float speed = Flight::Length(v);
  if (speed < 0.5f) {
    return {0.0f, 0.0f, 0.0f};
  }
  const auto &dev = Dev();
  const auto &st = in->statics;
  if (dev.shapeDrag && st.dragArea.y > 0.0f && st.mass > 0.0f) {
    const Flight::Quat q = in->stats.d_orientation;
    const Flight::Vec3 area = Flight::Mul(st.dragArea, 0.5f * 1.2f * dev.dragK);
    const Flight::Vec3 local = Flight::ShapeDragAccel(Flight::Rotate(Flight::Conjugate(q), v), area, st.mass);
    return N(Flight::Neg(Flight::Rotate(q, local)));
  }
  return N(Flight::Mul(v, Drag(vehicle, speed) / speed));
}

void Autopilot::BuildTrajectory(const AutopilotInput &aIn) {
  in = &aIn;
  BuildTrajectory();
  in = nullptr;
}

void Autopilot::RefreshVehicle(const AutopilotInput &aIn) {
  in = &aIn;
  RefreshVehicle();
  in = nullptr;
}

KeepClear Autopilot::Keep() const { return {landingKeepClear, landingKeepClearRadius}; }

bool Autopilot::FindGroundSpot(const Vec3 &aTarget, const Vec3 &aKeepClear, float aKeepRadius, Vec3 &aSpot) const {
  GroundSpot found;
  if (!FindLandingSpot(world, params, aTarget, {aKeepClear, aKeepRadius}, found)) {
    return false;
  }
  aSpot = found.spot;
  return true;
}

// Activate (455)
void Autopilot::Activate(const AutopilotInput &aIn) {
  in = &aIn;
  const auto &stats = aIn.stats;
  AheadReset();
  Vec3 target;
  freeRoamChosen = !CurrentDestination(target);
  holdPosition = N(stats.d_position);
  ClearRoute();
  DropRequest();
  hasExternalDestination = false;
  padLandingOffset = {0.0f, 0.0f, 0.0f};
  landingKeepClearRadius = 0.0f;
  hasDestination = false;
  freeRoam = false;
  requestJoin = false;
  status = "idle";
  replanTimer = 0.0f;
  tracker.remaining = 0.0f;
  landing = false;
  landingPad = -1;
  descentProfile = false;
  setDown = false;
  tracker.speedCmd = stats.d_speed;
  tracker.speedRate = 0.0f;
  tracker.accelCmd = {0.0f, 0.0f, 0.0f};
  lastPosition = N(stats.d_position);
  positionJumpTimer = 0.0f;
  gateEndS = 0.0f;
  gateTail.clear();
  // the take-off is part of the plan: the route starts at the car
  tracker.aheadSmooth = 15.0f;
  attitude.headingRef = N(stats.d_forward2D);
  attitude.attAccel = {0.0f, 0.0f, 0.0f};
  control.targetRight = stats.d_right;
  control.targetForward = stats.d_forward;
  control.targetUp = stats.d_up;
  control.omegaRef = {0.0f, 0.0f, 0.0f};
  control.bankAngle = 0.0f;
  control.bankRateNow = 0.0f;
  tracker.pathCurvature = {0.0f, 0.0f, 0.0f};
  in = nullptr;
}

// Deactivate (740)
void Autopilot::Deactivate() {
  ClearRoute();
  DropRequest();
  freeRoam = false;
  requestJoin = false;
  status = "idle";
}

// Teleported (1392)
void Autopilot::Teleported(const AutopilotInput &aIn) {
  in = &aIn;
  const Vec3 p = N(aIn.stats.d_position);
  holdPosition = p;
  lastPosition = p;
  positionJumpTimer = 0.0f;
  tracker.speedCmd = 0.0f;
  tracker.speedRate = 0.0f;
  if (RouteSize() > 0 || request != 0) {
    ClearRoute();
    DropRequest();
    RequestRoute(p);
  }
  in = nullptr;
}

// Update (1552)
AutopilotOutput Autopilot::Step(const AutopilotInput &aIn) {
  in = &aIn;
  AutopilotOutput out;
  const float timeDelta = aIn.dt;
  const auto &stats = aIn.stats;
  RefreshVehicle();
  const Vec3 p = CheckedPosition(N(stats.d_position), timeDelta);
  const Vec3 v = N(stats.d_velocity);

  UpdateRoute(timeDelta);
  WatchHits(timeDelta, p, v);

  // an unmounted flight reads the drive sim's inputs as a player's: the summon's own flight ignores them
  if (PlayerInput() && !hasExternalDestination) {
    overrideTimer = params.overrideHold;
  }
  if (overrideTimer > 0.0f) {
    // manual: the player's inputs as given, the altitude held by damping the vertical speed
    overrideTimer -= timeDelta;
    holdPosition = p;
    tracker.speedCmd = stats.d_speed;
    tracker.speedRate = 0.0f;
    tracker.accelCmd = {0.0f, 0.0f, 0.0f};
    attitude.headingRef = N(stats.d_forward2D);
    out.manual = true;
    out.manualNormal = Flight::kUp;
    out.manualLift = 9.81f * aIn.standard.gravityFactor + aIn.controls.lift * aIn.standard.liftFactor - v.z;
    if (aIn.pitchPid && aIn.rollPid) {
      const auto manual = Flight::AttitudeLift(stats, aIn.statics, aIn.controls, aIn.standard, *aIn.pitchPid,
                                               *aIn.rollPid, out.manualNormal, out.manualLift, timeDelta);
      out.force = manual.force;
      out.torque = manual.torque;
    }
    out.commandSurge = commandSurge;
    out.commandSway = commandSway;
    out.status = status;
    out.events = TakeEvents();
    in = nullptr;
    return out;
  }

  // where to go
  Vec3 target = holdPosition;
  tracker.targetSpeed = 0.0f;
  tracker.emergencyBrake = false;
  const bool following = status == "following" && RouteSize() >= 2;
  const Trajectory *planned = FollowsTrajectory() ? &trajectory : nullptr;
  TrackerParams trackerParams;
  trackerParams.replanOffLine = params.replanOffLine;
  trackerParams.replanDistance = params.replanDistance;
  trackerParams.cornerSpan = params.cornerSpan;
  trackerParams.cornerOvershoot = params.cornerOvershoot;
  trackerParams.collisionHorizon = params.collisionHorizon;
  trackerParams.raceOffLine = params.raceOffLine;
  trackerParams.lineLead = params.lineLead;
  trackerParams.speedGain = params.speedGain;
  trackerParams.brakeAccel = params.brakeAccel;
  trackerParams.brakeMax = params.brakeMax;
  trackerParams.velocityGain = params.velocityGain;
  trackerParams.accelJerk = params.accelJerk;
  trackerParams.raceJerk = params.raceJerk;
  trackerParams.lineSpring = params.lineSpring;
  trackerParams.lineVelocityGain = params.lineVelocityGain;
  trackerParams.lineSpan = params.lineSpan;
  trackerParams.trajectoryCorrectionMax = params.trajectoryCorrectionMax;
  trackerParams.surge.fullSurge = surgeHold.fullSurge;
  trackerParams.surge.slack = surgeHold.slack;
  trackerParams.surge.dwell = surgeHold.dwell;
  trackerParams.surge.rate = surgeHold.rate;
  if (following) {
    // the follower (Navigation::Track): the replans and the route request are the Autopilot's
    TrackInput track;
    track.dt = timeDelta;
    track.position = p;
    track.velocity = v;
    track.speed = stats.d_speed;
    track.routeLine = routeLine;
    track.planned = FollowsTrajectory();
    track.recovering = recovering > 0.0f;
    track.replanTimer = replanTimer;
    track.requestPending = request != 0;
    const TrackOutput tracked = Track(tracker, trackerParams, vehicle, track, route, planned, world);
    target = tracked.target;
    if (tracked.replanLine) {
      spdlog::info("[Autopilot] {:.1f} m off the line at {:.1f} m/s: replanning from here", tracker.offRouteNow,
                   stats.d_speed);
      Replan(p, v);
    }
    if (tracked.requestRoute) {
      RequestRoute(p);
    }
    // barely moving while it should fly: wedged against something the map does not know about; out to the
    // nearest free space first, and a new route from there
    if (stats.d_speed < 1.5f && tracker.targetSpeed > 5.0f) {
      stuckTimer += timeDelta;
    } else {
      stuckTimer = 0.0f;
    }
    if (stuckTimer > 4.0f) {
      const Vec3 free = NearestFree(p);
      spdlog::warn("[Autopilot] wedged at {:.1f}, {:.1f}, {:.1f}: out to {:.1f}, {:.1f}, {:.1f} and replanning", p.x, p.y,
                   p.z, free.x, free.y, free.z);
      stuckTimer = 0.0f;
      ClearRoute();
      RequestRoute(free);
    }
    if (aIn.isPlayerMounted && !freeRoam) {
      // PublishProgress
      const float speed = std::max(std::max(stats.d_speed, tracker.targetSpeed), 10.0f);
      pending.progress = true;
      pending.remainingLength = tracker.remaining;
      pending.remainingTime = tracker.remaining / speed;
    }
    if (RouteSize() >= 2) {
      RecheckLandingSpot(timeDelta);
    }
    Arrive(p);
  } else if (status == "descending") {
    Descend(p, timeDelta);
  } else if (status == "arrived" && aIn.isPlayerMounted) {
    // hold on the destination until the car is still, then hand the controls back like vanilla autodrive
    settleTimer += timeDelta;
    if ((stats.d_speed < 0.5f && Distance(p, holdPosition) < 2.0f) || settleTimer > 5.0f) {
      status = "idle";
      spdlog::info("[Autopilot] settled after {:.1f} s, handing back", settleTimer);
      pending.notifications.push_back("LocKey#96672");
      pending.exitAutopilot = true;
    }
  }

  // desired velocity and acceleration (world)
  const Vec3 toTarget = Sub(target, p);
  Vec3 desired{0.0f, 0.0f, 0.0f};
  if (!following) {
    desired = HoldVelocity(p, toTarget);
  }
  Vec3 accel = Mul(Sub(desired, v), params.velocityGain);
  const float jerk = params.accelJerk;
  onLine = following && routeLine;
  if (!following) {
    // hovering: the acceleration capped
    Vec3 horizontal{accel.x, accel.y, 0.0f};
    if (Length(horizontal) > params.maxAccel) {
      horizontal = Mul(Normalize(horizontal), params.maxAccel);
    }
    accel.x = horizontal.x;
    accel.y = horizontal.y;
    accel.z = Clamp(accel.z, -params.maxAccel, params.maxAccel);
  }
  if (status == "descending" && descentProfile) {
    // the descent's profile: its acceleration, and a critically damped spring onto its height and speed
    const float w = params.descentFrequency;
    accel.z = Clamp(descentA + w * w * (descentZ - p.z) + 2.0f * w * (descentV - v.z), -params.maxAccel, params.maxAccel);
  }
  // the follower's acceleration (or, not following, the one above) rate limited, and the surge hold
  CommandInput command;
  command.dt = timeDelta;
  command.position = p;
  command.velocity = v;
  command.speed = stats.d_speed;
  command.forward = N(stats.d_forward);
  command.following = following;
  command.routeLine = routeLine;
  command.planned = FollowsTrajectory();
  command.recovering = recovering > 0.0f;
  command.target = target;
  command.rawAccel = accel;
  command.rawJerk = jerk;
  command.rawDesiredVelocity = desired;
  command.surgeFree = recovering <= 0.0f && stats.d_speed > 1.0f && FollowsTrajectory() && status == "following";
  const CommandOutput commanded =
      Command(tracker, trackerParams, command, route, FollowsTrajectory() ? &trajectory : nullptr);
  accel = tracker.accelCmd;
  if (following) {
    desired = commanded.desiredVelocity;
  }

  FlyAerobatic(timeDelta, accel, Heading(p, desired), out);
  out.status = status;
  out.events = TakeEvents();
  in = nullptr;
  return out;
}

// The arrival chain of the following branch (1750-1798)
void Autopilot::Arrive(const Vec3 &aP) {
  const auto &stats = in->stats;
  if (RouteSize() < 2) {
    // the route was just cleared (wedged, replanning): nothing to arrive at this frame
    return;
  }
  if (freeRoam && tracker.remaining < params.arriveDistance && stats.d_speed < 3.0f) {
    // the end of the route before the next leg joined: on from here (ChainFreeRoam)
    holdPosition = route.Point(route.Size() - 1);
    ClearRoute();
    status = "free roam";
    spdlog::info("[Autopilot] free roam: end of the route, planning on from here");
    return;
  }
  if (ahead.firstLeg) {
    // the end of a first leg, not the trip's: the rest is on its way (or asked for again)
    return;
  }
  const bool arrived = landing ? tracker.remaining < params.landingEntry && stats.d_speed < params.landingEntrySpeed
                               : tracker.remaining < params.arriveDistance && stats.d_speed < 3.0f;
  if (!arrived) {
    return;
  }
  // the spot again now the cars around it have streamed in: taken, the nearest free one with room
  if (landing && landingPad < 0 && SpotTaken(world, landingSpot)) {
    GroundSpot found;
    if (FindLandingSpot(world, params, destination, Keep(), found)) {
      spdlog::info("[Autopilot] landing spot taken by a vehicle, moving to {:.1f}, {:.1f} ({:.1f} m over)", found.spot.x,
                   found.spot.y, Distance2D(found.spot, landingSpot));
      landingSpot = found.spot;
    } else {
      spdlog::info("[Autopilot] landing spot taken by a vehicle and no free one near: hovering");
      landing = false;
    }
  }
  status = landing ? "descending" : "arrived";
  holdPosition = landing ? landingSpot : route.Point(route.Size() - 1);
  if (landing) {
    const Vec3 forward2D = N(stats.d_forward2D);
    landingStartZ = aP.z;
    landingHeading0 = forward2D;
    landingHeading = landingPad >= 0 ? PadHeading(world.Pads()[size_t(landingPad)], forward2D)
                                     : LandingHeading(world, landingSpot, forward2D);
    // a landing turn under way ends on its own heading, droneLandSlewDuring into the descent
    if (landSlewOn) {
      landingHeading = landSlewHeading;
      landSlewDescent = 0.0f;
    }
  } else {
    landSlewOn = false;
  }
  ClearRoute();
  settleTimer = 0.0f;
  touchdownTimer = 0.0f;
  // the descent's profile starts over the spot; setting down needs the car's rest height
  descentProfile = false;
  setDown = landing && params.landingMode == kLandWheels && in->restHeight > 0.0f;
  setDownGround = false;
  setDownTimer = 0.0f;
  setDownWait = 0.0f;
  setDownTarget = landingSpot.z - world.CarHalfHeight() + in->restHeight + params.wheelClearance;
  spdlog::info("[Autopilot] arrived{}", !landing ? "" : setDown ? ", setting down on the wheels" : ", descending");
}

namespace {
// s the car may hold at the set-down height without settling (not still, not level) before it touches down instead
constexpr float kSetDownWaitMax = 4.0f;
} // namespace

// The descending branch (1799-1829): set down once held still on the wheels' height (setDown), or touched down once the
// car stops sinking close to the floor
void Autopilot::Descend(const Vec3 &aP, float aDt) {
  const float height = aP.z - landingSpot.z;
  settleTimer += aDt; // time since the descent began
  if (Distance2D(aP, landingSpot) > params.landingOverSpot) {
    settleTimer = 0.0f; // still moving over the spot: not sinking yet
  }
  // stopped sinking: no new low point for a while
  if (settleTimer < 2.0f || aP.z < touchdownLow - 0.1f) {
    touchdownLow = aP.z;
    touchdownTimer = 0.0f;
  } else {
    touchdownTimer += aDt;
  }
  // set down: held at the target, still and level, for wheelSettle; not settling there in time, it touches down
  if (setDown && descentProfile && descentZ <= setDownTarget + 0.001f) {
    setDownWait += aDt;
    const bool there = std::fabs(aP.z - setDownTarget) < params.wheelTolerance;
    const bool still = in->stats.d_speed < params.wheelStill;
    const bool level = in->stats.d_up.z > 0.996f; // within 5 degrees
    setDownTimer = there && still && level ? setDownTimer + aDt : 0.0f;
    if (setDownTimer >= params.wheelSettle) {
      status = "landed";
      // how far over its height resting on the wheels, over the ground under them (or the map's floor)
      spdlog::info("[Autopilot] set down {:.2f} m over its rest on the wheels (over the {}, rest height {:.2f} m), {:.1f} m "
                   "from the spot",
                   aP.z - (setDownTarget - params.wheelClearance), setDownGround ? "ground under them" : "map's floor",
                   in->restHeight, Distance2D(aP, landingSpot));
      if (in->isPlayerMounted) {
        pending.notifications.push_back("LocKey#96672");
        pending.flightOff = true;
      }
      return;
    }
    if (setDownWait > kSetDownWaitMax) {
      // touching down from here: it sinks onto the floor first (the hold made no new low point either)
      setDown = false;
      touchdownLow = aP.z;
      touchdownTimer = 0.0f;
      spdlog::warn("[Autopilot] not settled {:.2f} m from the set-down height in {:.0f} s: touching down",
                   aP.z - setDownTarget, kSetDownWaitMax);
    }
  }
  if (!setDown && touchdownTimer > 0.7f && height < 3.0f) {
    status = "landed";
    spdlog::info("[Autopilot] touched down {:.2f} m above the floor, {:.1f} m from the spot", height,
                 Distance2D(aP, landingSpot));
    if (in->isPlayerMounted) {
      pending.notifications.push_back("LocKey#96672");
      pending.flightOff = true;
    }
  } else if (touchdownTimer > 1.5f && height >= 3.0f) {
    // held up well above the floor (something the map does not have): stay in the air
    status = "arrived";
    holdPosition = aP;
    settleTimer = 0.0f;
    spdlog::warn("[Autopilot] descent stopped {:.1f} m above the floor, hovering", height);
  }
}

// FlyAerobatic (2911), drone style with the native attitude and attitude control: the heading at hover
// speeds, the drone's attitude target, the torque and the thrust in the car's frame, the surge held full,
// the commands for the engine sound and the logs
void Autopilot::FlyAerobatic(float aTimeDelta, const Vec3 &aAccel, const Vec3 &aHeading, AutopilotOutput &aOut) {
  const auto &stats = in->stats;
  const Vec3 v = N(stats.d_velocity);
  const float dt = std::max(aTimeDelta, 0.001f);

  // heading at hover speeds: towards the wanted one, eased and at most maxHeadingRate
  if (Length(aHeading) > 2.0f) {
    const Vec3 want = Normalize2D(aHeading);
    const float angle = std::acos(Clamp(Dot(want, attitude.headingRef), -1.0f, 1.0f));
    if (angle > 0.0001f) {
      const float f = std::min(Clamp(dt / params.yawSmoothing, 0.0f, 1.0f), params.maxHeadingRate * dt / angle);
      attitude.headingRef = BlendHeading(attitude.headingRef, want, f);
    }
  }

  // the eased tilt starts from the wanted one when it was not flown last frame
  const bool live = droneEaseLive;
  droneEaseLive = false;
  float blend = Clamp((stats.d_speed - 4.0f) / 8.0f, 0.0f, 1.0f);
  blend = blend * blend * (3.0f - 2.0f * blend);

  // DroneAttitudeNative (2824)
  AttitudeParams ap;
  ap.droneBodyShare = params.droneBodyShare;
  ap.droneTiltTime = params.droneTiltTime;
  ap.attitudeLead = params.attitudeLead;
  ap.droneYawLead = params.droneYawLead;
  ap.lineSpan = params.lineSpan;
  ap.droneYawFrequency = params.droneYawFrequency;
  ap.droneYawRateMax = params.droneYawRateMax;
  ap.droneYawAccelMax = params.droneYawAccelMax;
  ap.racePitchFrequency = params.racePitchFrequency;
  ap.racePitchRateMax = params.racePitchRateMax;
  ap.racePitchAccelMax = params.racePitchAccelMax;
  ap.raceRollFrequency = params.raceRollFrequency;
  ap.raceRollRateMax = params.raceRollRateMax;
  ap.raceRollAccelMax = params.raceRollAccelMax;
  ap.trajectoryAttitudeLead = params.trajectoryAttitudeLead;
  ap.droneTakeoffTilt = params.droneTakeoffTilt;
  ap.droneTakeoffIn = params.droneTakeoffIn;
  ap.droneTakeoffFull = params.droneTakeoffFull;
  ap.droneTakeoffOut = params.droneTakeoffOut;
  ap.droneTakeoffLength = params.droneTakeoffLength;
  ap.dragFeedForward = surgeHold.dragFeedForward;
  AttitudeInput ai;
  ai.dt = dt;
  ai.accel = aAccel;
  ai.blend = blend;
  ai.live = live;
  ai.velocity = v;
  ai.speed = stats.d_speed;
  ai.forward = N(stats.d_forward);
  ai.onLine = onLine;
  ai.closestS = tracker.closestS;
  ai.following = FollowsTrajectory() && status == "following";
  ai.descending = status == "descending" || status == "landed";
  const Trajectory *planned = FollowsTrajectory() ? &trajectory : nullptr;
  const float wantedYaw = ChooseYaw(dt, live, DroneHeadingYaw(attitude, ap, ai, &route, planned));
  const AttitudeOutput wanted = DroneAttitude(attitude, ap, vehicle, ai, wantedYaw, &route, planned);
  droneEaseLive = true;

  // the target frame, the torque and the thrust (Navigation::AttitudeControl)
  AttitudeControlParams cp;
  cp.attitudeFrequency = params.attitudeFrequency;
  cp.attitudeDamping = params.attitudeDamping;
  cp.maxAngularAccel = params.maxAngularAccel;
  cp.torqueGainPitch = params.torqueGainPitch;
  cp.torqueGainRoll = params.torqueGainRoll;
  cp.torqueGainYaw = params.torqueGainYaw;
  cp.generalDampFactorAngular = in->settings.generalDampFactorAngular;
  cp.generalYawDampFactor = in->settings.generalYawDampFactor;
  cp.generalDampFactorAngularMax = in->settings.generalDampFactorAngularMax;
  cp.dampStabilityK = in->settings.dampStabilityK;
  cp.dampMaxPerFrame = in->settings.dampMaxPerFrame;
  cp.standardModeSurgeFactor = surgeMax;
  cp.standardModeSwayFactor = swayMax;
  cp.standardModeLiftFactor = in->standard.liftFactor;
  cp.gravityFactor = in->standard.gravityFactor;
  AttitudeControlInput ci;
  ci.dt = dt;
  ci.fwd = F(wanted.forward);
  ci.up = F(wanted.up);
  ci.shape = false;
  ci.accel = F(aAccel);
  ci.drag = F(SurgeDrag(v));
  ci.dragAccel = Drag(vehicle, stats.d_speed);
  ci.raceDecel = Decel(vehicle, stats.d_speed);
  ci.thrusterTensor = in->thrusterTensor;
  ci.inertia = in->inertia;
  const AttitudeControlOutput c = AttitudeControl(control, cp, in->statics, stats, ci);
  attitudeError = c.attitudeError;
  Flight::Vec3 local = c.thrust;
  const float g = 9.81f * in->standard.gravityFactor;
  // where the speed is the car's to set, the surge pushed toward full along the path by the hold's weight
  // (Navigation::Command decided it)
  const float surgeWeight = tracker.surge.weight;
  if (surgeWeight > 0.0f) {
    const Flight::Vec3 path = Flight::Rotate(Flight::Conjugate(stats.d_orientation), Flight::Normalize(stats.d_velocity));
    if (!(path.y < 0.2f || local.y >= surgeMax)) {
      local = Flight::Add(local, Flight::Mul(path, surgeWeight * (surgeMax - local.y) / path.y));
    }
    local.x = Clamp(local.x, -swayMax, swayMax);
    local.z = Clamp(local.z, g - in->standard.liftFactor, g + in->standard.liftFactor);
  }
  aOut.force = local;
  thrustCmd = local;
  aOut.torque = c.torque;
  // inputs for audio, the thrust as a share of full, not eased; no yaw input, ApplyPhysics damps less on an
  // axis with input
  commandSurge = Clamp(local.y / surgeMax, -1.0f, 1.0f);
  commandSway = Clamp(local.x / swayMax, -1.0f, 1.0f);
  aOut.commandSurge = commandSurge;
  aOut.commandSway = commandSway;
  aOut.writeControls = true;
  aOut.controls = in->controls;
  aOut.controls.surge = commandSurge;
  aOut.controls.sway = commandSway;
  aOut.controls.yaw = 0.0f;
  aOut.controls.lift = 0.0f;
  aOut.controls.linearBrake = 0.0f;
  aOut.controls.angularBrake = 0.0f;

  if (telemetry) {
    telemetryTime += aTimeDelta;
    const Flight::Vec3 &pos = stats.d_position;
    const Flight::Vec3 &rate = stats.d_angularVelocity;
    const Flight::Vec3 &up = control.targetUp;
    spdlog::info("[AutopilotA] {:.3f} {:.2f} {:.2f} {:.2f} {:.2f} {:.2f} {:.2f} {:.2f} {:.3f} {:.3f} {:.3f} {:.3f} "
                 "{:.3f} {:.3f} {:.3f} {:.3f} {:.3f} {:.3f} {:.3f} {:.3f} {:.2f} {:.2f} {:.2f} 0 0 {}",
                 telemetryTime, pos.x, pos.y, pos.z, v.x, v.y, v.z, Rad2Deg(attitudeError), rate.x, rate.y, rate.z,
                 c.rateRef.x, c.rateRef.y, c.rateRef.z, up.x, up.y, up.z, stats.d_up.x, stats.d_up.y, stats.d_up.z,
                 aAccel.x, aAccel.y, aAccel.z, status);
  }
  logTimer -= aTimeDelta;
  if (logTimer <= 0.0f) {
    logTimer = 1.0f;
    spdlog::info("[Autopilot] {} seg {}/{} speed {:.1f} target {:.1f} remaining {:.0f} attitude error {:.1f} up {:.2f} "
                 "z {:.1f}",
                 status, tracker.segment, RouteSize(), stats.d_speed, tracker.targetSpeed, tracker.remaining,
                 Rad2Deg(attitudeError), stats.d_up.z, stats.d_position.z);
  }
}

} // namespace Navigation
