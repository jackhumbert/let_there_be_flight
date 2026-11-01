#include "FlightModeControl.hpp"

namespace {
Flight::Vec3 ToVec(const RED4ext::Vector4 &aV) { return {aV.X, aV.Y, aV.Z}; }
RED4ext::Vector4 ToDirection(const Flight::Vec3 &aV) { return {aV.x, aV.y, aV.z, 0.0f}; }

// A PID as the pure code takes it; a null handle is a zero PID (correction 0), which is not written back.
Flight::Pid Load(const RED4ext::Handle<FlightPID> &aPid) {
  Flight::Pid p;
  if (aPid) {
    p.state = aPid->State();
    p.gains = aPid->Gains();
  }
  return p;
}

void Store(const RED4ext::Handle<FlightPID> &aPid, const Flight::Pid &aValue) {
  if (aPid)
    aPid->SetState(aValue.state);
}
} // namespace

Flight::StandardParams FlightModeControl::Standard() const {
  Flight::StandardParams p;
  p.surgeFactor = standardModeSurgeFactor;
  p.yawFactor = standardModeYawFactor;
  p.swayFactor = standardModeSwayFactor;
  p.pitchFactor = standardModePitchFactor;
  p.liftFactor = standardModeLiftFactor;
  p.hoverFactor = standardModeHoverFactor;
  p.pitchInputAngle = standardModePitchInputAngle;
  p.rollFactor = standardModeRollFactor;
  p.rollInputAngle = standardModeRollInputAngle;
  p.rollWithYaw = generalRollWithYaw;
  p.pitchWithLift = pitchWithLift;
  p.pitchWithSurge = pitchWithSurge;
  p.brakeFactorLinear = brakeFactorLinear;
  p.hoverClamp = hoverClamp;
  p.minHoverHeight = hoverModeMinHoverHeight;
  p.maxHoverHeight = hoverModeMaxHoverHeight;
  p.gravityFactor = gravityFactor;
  return p;
}

Flight::Controls FlightModeControl::Controls() const {
  Flight::Controls c;
  c.surge = surge;
  c.lift = lift;
  c.sway = sway;
  c.yaw = yaw;
  c.roll = roll;
  c.pitch = pitch;
  c.linearBrake = linearBrake;
  c.angularBrake = angularBrake;
  return c;
}

Flight::Ground FlightModeControl::GroundOf() const {
  Flight::Ground g;
  g.found = foundGround;
  g.distance = distance;
  g.normal = ToVec(normal);
  return g;
}

void FlightModeControl::SetOutput(const Flight::ModeOutput &aOut) {
  force = ToDirection(aOut.force);
  torque = ToDirection(aOut.torque);
}

void FlightModeControl::StandardLift(RED4ext::Handle<FlightStats> aStats, RED4ext::Handle<FlightPID> aPitchPid,
                                     RED4ext::Handle<FlightPID> aRollPid, RED4ext::Vector4 aNormal, float aLiftForce) {
  if (!aStats) {
    SetOutput({});
    return;
  }
  Flight::Pid pitchPid = Load(aPitchPid), rollPid = Load(aRollPid);
  SetOutput(Flight::StandardLift(aStats->State(), aStats->Static(), Controls(), Standard(), pitchPid, rollPid,
                                 ToVec(aNormal), aLiftForce, dt));
  Store(aPitchPid, pitchPid);
  Store(aRollPid, rollPid);
}

void FlightModeControl::StandardDistance(RED4ext::Handle<FlightStats> aStats, RED4ext::Handle<FlightPID> aHoverPid,
                                         RED4ext::Handle<FlightPID> aPitchPid, RED4ext::Handle<FlightPID> aRollPid,
                                         RED4ext::Vector4 aNormal, float aHeightDifference) {
  if (!aStats) {
    SetOutput({});
    return;
  }
  Flight::Pid hoverPid = Load(aHoverPid), pitchPid = Load(aPitchPid), rollPid = Load(aRollPid);
  SetOutput(Flight::StandardDistance(aStats->State(), aStats->Static(), Controls(), Standard(), hoverPid, pitchPid,
                                     rollPid, ToVec(aNormal), aHeightDifference, dt));
  Store(aHoverPid, hoverPid);
  Store(aPitchPid, pitchPid);
  Store(aRollPid, rollPid);
}

void FlightModeControl::HoverFly(RED4ext::Handle<FlightStats> aStats, RED4ext::Handle<FlightPID> aHoverPid,
                                 RED4ext::Handle<FlightPID> aPitchPid, RED4ext::Handle<FlightPID> aRollPid) {
  if (!aStats) {
    SetOutput({});
    return;
  }
  Flight::HoverFlyParams h;
  h.altitudeHoldFactor = altitudeHoldFactor;
  h.heightDampening = heightDampening;
  h.heightCorrectionFactor = heightCorrectionFactor;
  Flight::HoverFlyState m;
  m.hovering = hovering;
  m.referenceZ = referenceZ;
  m.hoverHeight = hoverHeight;
  m.holdingAltitude = holdingAltitude;
  Flight::Pid hoverPid = Load(aHoverPid), pitchPid = Load(aPitchPid), rollPid = Load(aRollPid);
  SetOutput(Flight::HoverFly(m, aStats->State(), aStats->Static(), Controls(), Standard(), h, GroundOf(), hoverPid,
                             pitchPid, rollPid, dt));
  hovering = m.hovering;
  referenceZ = m.referenceZ;
  hoverHeight = m.hoverHeight;
  holdingAltitude = m.holdingAltitude;
  Store(aHoverPid, hoverPid);
  Store(aPitchPid, pitchPid);
  Store(aRollPid, rollPid);
}

void FlightModeControl::Automatic(RED4ext::Handle<FlightStats> aStats, RED4ext::Handle<FlightPID> aHoverPid,
                                  RED4ext::Handle<FlightPID> aPitchPid, RED4ext::Handle<FlightPID> aRollPid) {
  if (!aStats) {
    SetOutput({});
    return;
  }
  Flight::AutomaticParams a;
  a.autoBrakingFactor = automaticModeAutoBrakingFactor;
  a.yawDirectionality = automaticModeYawDirectionality;
  a.liftFactor = automaticModeLiftFactor;
  a.hoverFactor = automaticModeHoverFactor;
  Flight::AutomaticState m;
  m.hovering = hovering;
  m.hoverHeight = hoverHeight;
  Flight::Pid hoverPid = Load(aHoverPid), pitchPid = Load(aPitchPid), rollPid = Load(aRollPid);
  SetOutput(Flight::Automatic(m, aStats->State(), aStats->Static(), Controls(), Standard(), a, GroundOf(), hoverPid,
                              pitchPid, rollPid, dt));
  hovering = m.hovering;
  hoverHeight = m.hoverHeight;
  Store(aHoverPid, hoverPid);
  Store(aPitchPid, pitchPid);
  Store(aRollPid, rollPid);
}

void FlightModeControl::Drone(RED4ext::Handle<FlightStats> aStats) {
  if (!aStats) {
    SetOutput({});
    return;
  }
  Flight::DroneParams p;
  p.liftFactor = droneModeLiftFactor;
  p.pitchFactor = droneModePitchFactor;
  p.rollFactor = droneModeRollFactor;
  p.surgeFactor = droneModeSurgeFactor;
  p.yawFactor = droneModeYawFactor;
  p.swayFactor = droneModeSwayFactor;
  p.brakeFactorLinear = brakeFactorLinear;
  p.brakeFactorAngular = brakeFactorAngular;
  SetOutput(Flight::Drone(aStats->State(), aStats->Static(), Controls(), p));
}

RED4ext::Vector4 FlightModeControl::RotateAxis(RED4ext::Vector4 aV, RED4ext::Vector4 aAxis, float aAngle) {
  return ToDirection(Flight::RotateAxis(ToVec(aV), ToVec(aAxis), aAngle));
}

RED4ext::Vector4 FlightModeControl::Interpolate(RED4ext::Vector4 aA, RED4ext::Vector4 aB, float aT) {
  return ToDirection(Flight::Interpolate(ToVec(aA), ToVec(aB), aT));
}

float FlightModeControl::Lerp(float aAlpha, float aA, float aB) { return Flight::Lerp(aAlpha, aA, aB); }
