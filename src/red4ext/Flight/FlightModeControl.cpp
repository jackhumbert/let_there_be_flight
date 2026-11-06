#include "FlightModeControl.hpp"

#include "Flight/Component.hpp"
#include "Flight/DeveloperSettings.hpp"
#include "Flight/FlightGround.hpp"
#include "FlightConfiguration.hpp"

#include <RED4ext/RTTISystem.hpp>

float GetFlightSettingsFloat(RED4ext::CRTTISystem *rtti, RED4ext::CName aProperty, float aFallback);

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

// Drone's skid (Flight::DroneSkid) for the car of aComponent, as a body-frame acceleration: the car's clearance is the
// ground's distance under the wheel emitters (their rays' mean) less the emitters' height over its lowest point (the
// configuration's, in the vehicle's frame: the chassis shape's, or LTBF's thruster spheres', which the car rests on).
// The shortest ray in place of the mean made the car bob 0.6 to 1.5 m at 40 km/h (2026-11-06). Nothing before the
// chassis shape is measured, or without ground.
Flight::Vec3 Skid(const Flight::StatsState &aState, float aLift, FlightComponent *aComponent) {
  const float clearance = Flight::Developer().droneSkid;
  if (clearance <= 0.0f || !aComponent || !aComponent->configuration || !aComponent->configuration->dragAreaMeasured)
    return {};
  auto vehicle = reinterpret_cast<RED4ext::vehicle::BaseObject *>(aComponent->entity);
  if (!vehicle)
    return {};
  const float maxHover = GetFlightSettingsFloat(RED4ext::CRTTISystem::Get(), "hoverModeMaxHoverHeight", 7.0f);
  Flight::Vec3 emitters;
  const Flight::Ground g = FlightGroundNative::Find(aComponent, maxHover, &emitters);
  if (!g.found)
    return {};
  const auto origin = vehicle->worldTransform.Position.AsVector4();
  const auto &o = vehicle->worldTransform.Orientation;
  const Flight::Vec3 up = Flight::Rotate(Flight::Quat{o.i, o.j, o.k, o.r}, {0.0f, 0.0f, 1.0f});
  const float emittersOverBottom =
      Flight::Dot(Flight::Sub(emitters, {origin.X, origin.Y, origin.Z}), up) -
      std::min(aComponent->configuration->chassisBottom, aComponent->configuration->colliderBottom);
  Flight::DroneSkidParams k;
  k.clearance = clearance;
  const float a = Flight::DroneSkid(g.distance - emittersOverBottom, aState.d_velocity.z, aState.d_up.z, aLift, k);
  return Flight::Rotate(Flight::Conjugate(aState.d_orientation), {0.0f, 0.0f, a});
}
} // namespace

float FlightModeControl::Thrust(const Flight::StatsStatic &aStatic, float aReference) {
  const auto &dev = Flight::Developer();
  return Flight::ThrustScale(aStatic.thrusters, aStatic.mass, dev.thrustPerThruster, aReference, dev.thrustStrength);
}

Flight::StandardParams FlightModeControl::Standard(const Flight::StatsStatic &aStatic) const {
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
  const float thrust = Thrust(aStatic, standardModeSurgeFactor);
  p.surgeFactor *= thrust;
  p.swayFactor *= thrust;
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
  SetOutput(Flight::StandardLift(aStats->State(), aStats->Static(), Controls(), Standard(aStats->Static()), pitchPid, rollPid,
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
  SetOutput(Flight::StandardDistance(aStats->State(), aStats->Static(), Controls(), Standard(aStats->Static()), hoverPid, pitchPid,
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
  SetOutput(Flight::HoverFly(m, aStats->State(), aStats->Static(), Controls(), Standard(aStats->Static()), h, GroundOf(), hoverPid,
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
  SetOutput(Flight::Automatic(m, aStats->State(), aStats->Static(), Controls(), Standard(aStats->Static()), a, GroundOf(), hoverPid,
                              pitchPid, rollPid, dt));
  hovering = m.hovering;
  hoverHeight = m.hoverHeight;
  Store(aHoverPid, hoverPid);
  Store(aPitchPid, pitchPid);
  Store(aRollPid, rollPid);
}

void FlightModeControl::Drone(RED4ext::Handle<FlightStats> aStats, RED4ext::Handle<RED4ext::IScriptable> aComponent) {
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
  const float thrust = Thrust(aStats->Static(), droneModeSurgeFactor);
  p.surgeFactor *= thrust;
  p.swayFactor *= thrust;
  p.brakeFactorLinear = brakeFactorLinear;
  p.brakeFactorAngular = brakeFactorAngular;
  Flight::ModeOutput out = Flight::Drone(aStats->State(), aStats->Static(), Controls(), p);
  out.force = Flight::Add(out.force, Skid(aStats->State(), lift, static_cast<FlightComponent *>(aComponent.GetPtr())));
  SetOutput(out);
}

RED4ext::Vector4 FlightModeControl::RotateAxis(RED4ext::Vector4 aV, RED4ext::Vector4 aAxis, float aAngle) {
  return ToDirection(Flight::RotateAxis(ToVec(aV), ToVec(aAxis), aAngle));
}

RED4ext::Vector4 FlightModeControl::Interpolate(RED4ext::Vector4 aA, RED4ext::Vector4 aB, float aT) {
  return ToDirection(Flight::Interpolate(ToVec(aA), ToVec(aB), aT));
}

float FlightModeControl::Lerp(float aAlpha, float aA, float aB) { return Flight::Lerp(aAlpha, aA, aB); }
