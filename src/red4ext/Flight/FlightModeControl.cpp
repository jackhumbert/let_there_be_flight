#include "FlightModeControl.hpp"

#include "Flight/Component.hpp"
#include "Flight/DeveloperSettings.hpp"
#include "Flight/FlightGround.hpp"
#include "Physics/ScrapeEffects.hpp"
#include "FlightConfiguration.hpp"

#include <RED4ext/RTTISystem.hpp>

#include <limits>

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

// Drone's skid for the car of aComponent, as a body-frame force and mode torque. A clearance is a ray's distance to
// the ground less its wheel emitter's height over the car's lowest point (the configuration's, in the vehicle's frame:
// the chassis shape's, or LTBF's thruster spheres', which the car rests on). By default each emitter, where a thruster
// is, is a skid point of its own (Flight::DroneSkidPoints); with droneSkidPerThruster off, the rays' mean holds the car
// once through its centre of mass (Flight::DroneSkid). Holding the shortest ray instead of the mean made the car bob
// 0.6 to 1.5 m at 40 km/h. Nothing before the chassis shape is measured, or without ground. aClearance gets the
// clearance (the points' mean), infinity without one.
Flight::ModeOutput Skid(const Flight::StatsState &aState, const Flight::StatsStatic &aStatic, float aLift,
                        FlightComponent *aComponent, float &aClearance) {
  aClearance = std::numeric_limits<float>::infinity();
  const float clearance = Flight::Developer().droneSkid;
  if (clearance <= 0.0f || !aComponent || !aComponent->configuration || !aComponent->configuration->dragAreaMeasured)
    return {};
  auto vehicle = reinterpret_cast<RED4ext::vehicle::BaseObject *>(aComponent->entity);
  if (!vehicle)
    return {};
  const float maxHover = GetFlightSettingsFloat(RED4ext::CRTTISystem::Get(), "hoverModeMaxHoverHeight", 7.0f);
  Flight::Vec3 emitters;
  Flight::Hit hits[4];
  const Flight::Ground g = FlightGroundNative::Find(aComponent, maxHover, &emitters, hits);
  if (!g.found)
    return {};
  const auto origin = vehicle->worldTransform.Position.AsVector4();
  const auto &o = vehicle->worldTransform.Orientation;
  const Flight::Vec3 up = Flight::Rotate(Flight::Quat{o.i, o.j, o.k, o.r}, {0.0f, 0.0f, 1.0f});
  const Flight::Vec3 o3{origin.X, origin.Y, origin.Z};
  const float bottom = std::min(aComponent->configuration->chassisBottom, aComponent->configuration->colliderBottom);
  Flight::DroneSkidParams k;
  k.clearance = clearance;
  static RED4ext::CClass *bikeClass = RED4ext::CRTTISystem::Get()->GetClass("vehicleBikeBaseObject");
  k.floor = bikeClass && vehicle->GetType()->IsA(bikeClass);
  Flight::ModeOutput out;
  static RED4ext::CProperty *tensorField = [] {
    auto component = RED4ext::CRTTISystem::Get()->GetClass("FlightComponent");
    return component ? component->GetProperty("thrusterTensor") : nullptr;
  }();
  if (Flight::Developer().droneSkidPerThruster && tensorField) {
    Flight::SkidPoint points[4];
    if (!FlightGroundNative::Touches(aComponent, aState, hits, points))
      return {};
    // the ground's grip on each: Ground Friction times the material's (as the chassis's against PhysX contacts)
    for (int i = 0; i < 4; ++i)
      points[i].friction = aComponent->groundFriction * ScrapeEffects::FrictionOfMaterial(hits[i].material);
    aComponent->skidRanAge = 0.0f;
    static bool frictionLogged = false;
    if (!frictionLogged && points[0].friction > 0.0f) {
      frictionLogged = true;
      spdlog::info("[Skid] the ground grips the thrusters: Ground Friction {:.2f}, x{:.2f} on this ground",
                   aComponent->groundFriction, points[0].friction / aComponent->groundFriction);
    }
    aClearance = 0.25f * (points[0].clearance + points[1].clearance + points[2].clearance + points[3].clearance);
    const auto &t = *tensorField->GetValuePtr<RED4ext::Vector4>(aComponent);
    float loads[4] = {};
    const Flight::ModeOutput skid = Flight::DroneSkidPoints(
        points, 4, aState, aStatic, Flight::TorqueResponse(aStatic, {t.X, t.Y, t.Z}), aLift, k, loads);
    // how hard each thruster presses, for its scrape's loudness
    for (int i = 0; i < 4; ++i)
      aComponent->skidContacts[i].load = loads[i];
    return skid;
  }
  const float emittersOverBottom = Flight::Dot(Flight::Sub(emitters, o3), up) - bottom;
  aClearance = g.distance - emittersOverBottom;
  const float a = Flight::DroneSkid(g.distance - emittersOverBottom, Flight::ClosingSpeed(aState.d_velocity, g.normal),
                                    aState.d_up.z, aLift, k);
  out.force = Flight::Rotate(Flight::Conjugate(aState.d_orientation), {0.0f, 0.0f, a});
  return out;
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
  float clearance;
  const Flight::ModeOutput skid =
      Skid(aStats->State(), aStats->Static(), lift, static_cast<FlightComponent *>(aComponent.GetPtr()), clearance);
  out.force = Flight::Add(out.force, skid.force);
  out.torque = Flight::Add(out.torque, skid.torque);
  // lift down at the ground presses lightly, so the car slides (Flight::GroundedLift)
  Flight::DroneSkidParams k;
  k.clearance = Flight::Developer().droneSkid;
  const float push = Controls().lift * p.liftFactor;
  out.force.z += Flight::GroundedLift(push, clearance, aStats->State().d_up.z, k) - push;
  SetOutput(out);
}

RED4ext::Vector4 FlightModeControl::RotateAxis(RED4ext::Vector4 aV, RED4ext::Vector4 aAxis, float aAngle) {
  return ToDirection(Flight::RotateAxis(ToVec(aV), ToVec(aAxis), aAngle));
}

RED4ext::Vector4 FlightModeControl::Interpolate(RED4ext::Vector4 aA, RED4ext::Vector4 aB, float aT) {
  return ToDirection(Flight::Interpolate(ToVec(aA), ToVec(aB), aT));
}

float FlightModeControl::Lerp(float aAlpha, float aA, float aB) { return Flight::Lerp(aAlpha, aA, aB); }
