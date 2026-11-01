#include "FlightTracker.hpp"

#include "MapSpace.hpp"
#include "NavMap.hpp"

namespace {
Navigation::Vec3 ToVec(const RED4ext::Vector4 &aV) { return {aV.X, aV.Y, aV.Z}; }
RED4ext::Vector4 ToDirection(const Navigation::Vec3 &aV) { return {aV.x, aV.y, aV.z, 0.0f}; }
RED4ext::Vector4 ToPoint(const Navigation::Vec3 &aV) { return {aV.x, aV.y, aV.z, 1.0f}; }

const Navigation::Route &RouteOf(const RED4ext::Handle<FlightRoute> &aRoute) {
  static const Navigation::Route empty;
  return aRoute ? aRoute->route : empty;
}

const Navigation::Trajectory *TrajectoryOf(const RED4ext::Handle<FlightTrajectory> &aTrajectory) {
  return aTrajectory && aTrajectory->IsValid() ? &aTrajectory->trajectory : nullptr;
}
} // namespace

Navigation::TrackerParams FlightTracker::Params() const {
  Navigation::TrackerParams p;
  p.replanOffLine = replanOffLine;
  p.replanDistance = replanDistance;
  p.cornerSpan = cornerSpan;
  p.cornerOvershoot = cornerOvershoot;
  p.collisionHorizon = collisionHorizon;
  p.raceOffLine = raceOffLine;
  p.lineLead = lineLead;
  p.speedGain = speedGain;
  p.brakeAccel = brakeAccel;
  p.brakeMax = brakeMax;
  p.velocityGain = velocityGain;
  p.accelJerk = accelJerk;
  p.raceJerk = raceJerk;
  p.lineSpring = lineSpring;
  p.lineVelocityGain = lineVelocityGain;
  p.lineSpan = lineSpan;
  p.trajectoryCorrectionMax = trajectoryCorrectionMax;
  p.surge.fullSurge = fullSurge;
  p.surge.slack = slack;
  p.surge.dwell = dwell;
  return p;
}

Navigation::TrackerState FlightTracker::State() const {
  Navigation::TrackerState s;
  s.segment = segment;
  s.closestS = closestS;
  s.offRouteNow = offRouteNow;
  s.remaining = remaining;
  s.closestNow = ToVec(closestNow);
  s.aheadSmooth = aheadSmooth;
  s.curvatureAhead = curvatureAhead;
  s.pathCurvature = ToVec(pathCurvature);
  s.blockedTimer = blockedTimer;
  s.blockedAhead = blockedAhead;
  s.emergencyBrake = emergencyBrake;
  s.targetSpeed = targetSpeed;
  s.speedCmd = speedCmd;
  s.speedRate = speedRate;
  s.accelCmd = ToVec(accelCmd);
  s.surge.held = surgeHeld;
  s.surge.changing = surgeChanging;
  return s;
}

void FlightTracker::SetState(const Navigation::TrackerState &aState) {
  segment = aState.segment;
  closestS = aState.closestS;
  offRouteNow = aState.offRouteNow;
  remaining = aState.remaining;
  closestNow = ToPoint(aState.closestNow);
  aheadSmooth = aState.aheadSmooth;
  curvatureAhead = aState.curvatureAhead;
  pathCurvature = ToDirection(aState.pathCurvature);
  blockedTimer = aState.blockedTimer;
  blockedAhead = aState.blockedAhead;
  emergencyBrake = aState.emergencyBrake;
  targetSpeed = aState.targetSpeed;
  speedCmd = aState.speedCmd;
  speedRate = aState.speedRate;
  accelCmd = ToDirection(aState.accelCmd);
  surgeHeld = aState.surge.held;
  surgeChanging = aState.surge.changing;
}

void FlightTracker::Track(RED4ext::Handle<FlightRoute> aRoute, RED4ext::Handle<FlightTrajectory> aTrajectory,
                          RED4ext::Handle<FlightVehicleModel> aVehicle) {
  const Navigation::VehicleSpec spec = aVehicle ? aVehicle->Spec() : Navigation::VehicleSpec{};
  Navigation::TrackInput in;
  in.dt = dt;
  in.position = ToVec(position);
  in.velocity = ToVec(velocity);
  in.speed = speed;
  in.routeLine = routeLine;
  in.planned = planned;
  in.recovering = recovering;
  in.replanTimer = replanTimer;
  in.requestPending = requestPending;
  in.aimOffset = ToVec(aimOffset);
  Navigation::TrackerState state = State();
  const Navigation::TrackOutput out = Navigation::Track(state, Params(), spec, in, RouteOf(aRoute),
                                                         TrajectoryOf(aTrajectory), Navigation::MapSpace(Navigation::NavMap::Get()));
  SetState(state);
  target = ToPoint(out.target);
  lookahead = target;
  replanLine = out.replanLine;
  requestRoute = out.requestRoute;
}

void FlightTracker::Command(RED4ext::Handle<FlightRoute> aRoute, RED4ext::Handle<FlightTrajectory> aTrajectory) {
  Navigation::CommandInput in;
  in.dt = dt;
  in.position = ToVec(position);
  in.velocity = ToVec(velocity);
  in.speed = speed;
  in.forward = ToVec(forward);
  in.following = following;
  in.routeLine = routeLine;
  in.planned = planned;
  in.recovering = recovering;
  in.target = ToVec(target);
  in.overlayVelocity = ToVec(overlayVelocity);
  in.rawAccel = ToVec(rawAccel);
  in.rawDesiredVelocity = ToVec(rawDesiredVelocity);
  in.rawJerk = rawJerk;
  in.surgeFree = surgeFree;
  Navigation::TrackerState state = State();
  const Navigation::CommandOutput out = Navigation::Command(state, Params(), in, RouteOf(aRoute), TrajectoryOf(aTrajectory));
  SetState(state);
  accel = ToDirection(out.accel);
  desiredVelocity = ToDirection(out.desiredVelocity);
  lookahead = ToPoint(out.lookahead);
  onLine = out.onLine;
}
