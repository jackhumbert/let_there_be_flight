#include "FlightDroneAttitude.hpp"

namespace {
Navigation::Vec3 ToVec(const RED4ext::Vector4 &aV) { return {aV.X, aV.Y, aV.Z}; }
RED4ext::Vector4 ToDirection(const Navigation::Vec3 &aV) { return {aV.x, aV.y, aV.z, 0.0f}; }
} // namespace

Navigation::AttitudeParams FlightDroneAttitude::Params() const {
  Navigation::AttitudeParams p;
  p.droneBodyShare = droneBodyShare;
  p.droneTiltTime = droneTiltTime;
  p.attitudeLead = attitudeLead;
  p.droneYawLead = droneYawLead;
  p.lineSpan = lineSpan;
  p.droneYawFrequency = droneYawFrequency;
  p.droneYawRateMax = droneYawRateMax;
  p.droneYawAccelMax = droneYawAccelMax;
  p.racePitchFrequency = racePitchFrequency;
  p.racePitchRateMax = racePitchRateMax;
  p.racePitchAccelMax = racePitchAccelMax;
  p.raceRollFrequency = raceRollFrequency;
  p.raceRollRateMax = raceRollRateMax;
  p.raceRollAccelMax = raceRollAccelMax;
  p.trajectoryAttitudeLead = trajectoryAttitudeLead;
  p.droneTakeoffTilt = droneTakeoffTilt;
  p.droneTakeoffIn = droneTakeoffIn;
  p.droneTakeoffFull = droneTakeoffFull;
  p.droneTakeoffOut = droneTakeoffOut;
  p.droneTakeoffLength = droneTakeoffLength;
  p.dragFeedForward = dragFeedForward;
  return p;
}

Navigation::AttitudeState FlightDroneAttitude::State() const {
  Navigation::AttitudeState s;
  s.droneYaw = droneYaw;
  s.droneYawRate = droneYawRate;
  s.dronePitch = dronePitch;
  s.dronePitchRate = dronePitchRate;
  s.droneRoll = droneRoll;
  s.droneRollRate = droneRollRate;
  s.attAccel = ToVec(attAccel);
  s.attRef = ToVec(attRef);
  s.headingRef = ToVec(headingRef);
  return s;
}

Navigation::AttitudeInput FlightDroneAttitude::Input() const {
  Navigation::AttitudeInput i;
  i.dt = dt;
  i.accel = ToVec(accel);
  i.blend = blend;
  i.live = live;
  i.velocity = ToVec(velocity);
  i.speed = speed;
  i.forward = ToVec(forward);
  i.onLine = onLine;
  i.closestS = closestS;
  i.following = following;
  i.descending = descending;
  return i;
}

float FlightDroneAttitude::HeadingYaw(RED4ext::Handle<FlightRoute> aRoute, RED4ext::Handle<FlightTrajectory> aTrajectory) {
  const Navigation::Route *route = aRoute ? &aRoute->route : nullptr;
  const Navigation::Trajectory *trajectory = aTrajectory && aTrajectory->IsValid() ? &aTrajectory->trajectory : nullptr;
  return Navigation::DroneHeadingYaw(State(), Params(), Input(), route, trajectory);
}

void FlightDroneAttitude::Step(float aWantedYaw, RED4ext::Handle<FlightRoute> aRoute,
                               RED4ext::Handle<FlightTrajectory> aTrajectory, RED4ext::Handle<FlightVehicleModel> aVehicle) {
  const Navigation::Route *route = aRoute ? &aRoute->route : nullptr;
  const Navigation::Trajectory *trajectory = aTrajectory && aTrajectory->IsValid() ? &aTrajectory->trajectory : nullptr;
  const Navigation::VehicleSpec spec = aVehicle ? aVehicle->Spec() : Navigation::VehicleSpec{};
  Navigation::AttitudeState state = State();
  const Navigation::AttitudeOutput out =
      Navigation::DroneAttitude(state, Params(), spec, Input(), aWantedYaw, route, trajectory);
  droneYaw = state.droneYaw;
  droneYawRate = state.droneYawRate;
  dronePitch = state.dronePitch;
  dronePitchRate = state.dronePitchRate;
  droneRoll = state.droneRoll;
  droneRollRate = state.droneRollRate;
  attAccel = ToDirection(state.attAccel);
  attRef = ToDirection(state.attRef);
  headingRef = ToDirection(state.headingRef);
  fwd = ToDirection(out.forward);
  up = ToDirection(out.up);
}
