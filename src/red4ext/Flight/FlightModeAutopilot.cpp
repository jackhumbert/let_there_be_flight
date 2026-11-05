// The Autopilot's flight mode (FlightModeAutopilot.hpp): the frame's gathering, Step, the outputs, the redscript glue
// and the telemetry. The script mode it replaces is FlightModeAutopilot.reds at c3e24ab; line numbers below are its.

#include "FlightModeAutopilot.hpp"

#include "Flight/Component.hpp"
#include "Flight/DeveloperSettings.hpp"
#include "Flight/FlightModeControl.hpp"
#include "Flight/FlightPID.hpp"
#include "Flight/FlightStats.hpp"
#include "Flight/Settings.hpp"
#include "Flight/Telemetry.hpp"
#include "FlightSettings.hpp"
#include "Navigation/AutopilotTelemetry.hpp"
#include "Navigation/FlightNavMap.hpp"
#include "Utils/Utils.hpp"
#include "stdafx.hpp"

#include <RED4ext/RTTISystem.hpp>

#include <array>
#include <chrono>
#include <cmath>
#include <string>
#include <unordered_map>

namespace {

using RED4ext::CProperty;

// The script fields the mode reads and writes, looked up once.
struct Props {
  // FlightMode
  CProperty *component = nullptr, *gravityFactor = nullptr, *collisionPenalty = nullptr;
  // FlightComponent: the input fields (surge, lift, sway, yaw, roll, pitch, linearBrake, angularBrake), the rest
  CProperty *controls[8] = {};
  CProperty *tensor = nullptr, *isPlayerMounted = nullptr, *collisionTimer = nullptr, *pitchPid = nullptr,
            *rollPid = nullptr, *modes = nullptr;
  // FlightModeStandard's Mod Settings: surge, yaw, sway, pitch, lift, hover, pitch input angle, roll, roll input angle
  RED4ext::CClass *standard = nullptr;
  CProperty *standardFields[9] = {};
  // FlightSettings' instance fields (the script read FlightSettings.GetInstance().<name>)
  CProperty *physx = nullptr, *brakeLinear = nullptr, *brakeAngular = nullptr, *dampAngular = nullptr,
            *yawDamp = nullptr, *dampMax = nullptr, *rollWithYaw = nullptr;
  bool ok = false;
};

const Props &GetProps() {
  static const Props p = [] {
    Props p;
    auto rtti = RED4ext::CRTTISystem::Get();
    auto mode = rtti->GetClass("FlightMode");
    auto component = rtti->GetClass("FlightComponent");
    auto settings = rtti->GetClass("FlightSettings");
    p.standard = rtti->GetClass("FlightModeStandard");
    if (!mode || !component) {
      return p;
    }
    p.component = mode->GetProperty("component");
    p.gravityFactor = mode->GetProperty("gravityFactor");
    p.collisionPenalty = mode->GetProperty("collisionPenalty");
    const char *controls[8] = {"surge", "lift", "sway", "yaw", "roll", "pitch", "linearBrake", "angularBrake"};
    bool all = true;
    for (int i = 0; i < 8; ++i) {
      all &= (p.controls[i] = component->GetProperty(controls[i])) != nullptr;
    }
    p.tensor = component->GetProperty("thrusterTensor");
    p.isPlayerMounted = component->GetProperty("isPlayerMounted");
    p.collisionTimer = component->GetProperty("collisionTimer");
    p.pitchPid = component->GetProperty("pitchPID");
    p.rollPid = component->GetProperty("rollPID");
    p.modes = component->GetProperty("modes");
    if (p.standard) {
      const char *names[9] = {"standardModeSurgeFactor", "standardModeYawFactor",       "standardModeSwayFactor",
                              "standardModePitchFactor", "standardModeLiftFactor",      "standardModeHoverFactor",
                              "standardModePitchInputAngle", "standardModeRollFactor", "standardModeRollInputAngle"};
      for (int i = 0; i < 9; ++i) {
        p.standardFields[i] = p.standard->GetProperty(names[i]);
      }
    }
    if (settings) {
      p.physx = settings->GetProperty("physxFlightPhysics");
      p.brakeLinear = settings->GetProperty("brakeFactorLinear");
      p.brakeAngular = settings->GetProperty("brakeFactorAngular");
      p.dampAngular = settings->GetProperty("generalDampFactorAngular");
      p.yawDamp = settings->GetProperty("generalYawDampFactor");
      p.dampMax = settings->GetProperty("generalDampFactorAngularMax");
      p.rollWithYaw = settings->GetProperty("generalRollWithYaw");
    }
    p.ok = all && p.component && p.gravityFactor && p.tensor && p.isPlayerMounted && p.collisionTimer && p.modes;
    return p;
  }();
  return p;
}

template <typename T> T &Field(CProperty *aProp, void *aOwner) { return *aProp->GetValuePtr<T>(aOwner); }

template <typename T> T FieldOr(CProperty *aProp, void *aOwner, T aFallback) {
  return aProp && aOwner ? *aProp->GetValuePtr<T>(aOwner) : aFallback;
}

Navigation::Vec3 N(const RED4ext::Vector4 &aV) { return {aV.X, aV.Y, aV.Z}; }
RED4ext::Vector4 P(const Navigation::Vec3 &aV) { return {aV.x, aV.y, aV.z, 1.0f}; }
RED4ext::Vector4 D(const Navigation::Vec3 &aV) { return {aV.x, aV.y, aV.z, 0.0f}; }
RED4ext::Vector4 D(const Flight::Vec3 &aV) { return {aV.x, aV.y, aV.z, 0.0f}; }
bool Same(const RED4ext::Vector4 &a, const RED4ext::Vector4 &b) { return a.X == b.X && a.Y == b.Y && a.Z == b.Z; }

// The script functions on FlightModeAutopilot the C++ calls (FlightModeAutopilot.reds), by index.
constexpr std::array<const char *, 7> kGlue = {"TrackedDestination", "AutodriveEngaged", "PublishProgress",
                                               "PublishFreeRoamUI", "ShowNotification",  "ExitAutopilot",
                                               "RequestFlightOff"};
enum { kTrackedDestination, kAutodriveEngaged, kPublishProgress, kPublishFreeRoamUI, kShowNotification, kExitAutopilot,
       kRequestFlightOff };

// the object's class's glue function, found once per class; nullptr (warned once) when the script lacks it
RED4ext::CBaseFunction *GlueFunction(RED4ext::IScriptable *aMode, int aIndex) {
  static std::unordered_map<const RED4ext::CClass *, std::array<RED4ext::CBaseFunction *, kGlue.size()>> found;
  const RED4ext::CClass *cls = aMode->GetType();
  auto it = found.find(cls);
  if (it == found.end()) {
    std::array<RED4ext::CBaseFunction *, kGlue.size()> f{};
    for (size_t i = 0; i < kGlue.size(); ++i) {
      f[i] = cls->GetFunction(kGlue[i]);
      if (!f[i]) {
        spdlog::warn("[FlightModeAutopilot] the script function {} was not found: that part of the Autopilot's glue "
                     "does nothing",
                     kGlue[i]);
      }
    }
    it = found.emplace(cls, f).first;
  }
  return it->second[aIndex];
}

} // namespace

// What a frame needs beside the AutopilotInput.
struct FlightModeAutopilot::Frame {
  RED4ext::IScriptable *component = nullptr;
  FlightStats *stats = nullptr;
  float collisionTimer = 0.0f;
};

FlightModeAutopilot::FlightModeAutopilot() : ap(world, FlightNavMap::Queue(), params, rng) {
  // the free roam picks: a seed per mode, so legs differ between runs as the script's RandRangeF did
  rng.state = uint64_t(std::chrono::steady_clock::now().time_since_epoch().count()) ^ uint64_t(uintptr_t(this));
  status = "idle";
}

// Initialize (425): set up, with the Mod Settings applied; the standard modes' hit penalty (FlightModeStandard did it)
void FlightModeAutopilot::OnInitializeNative() {
  const Props &p = GetProps();
  if (p.ok) {
    auto handle = p.component->GetValuePtr<RED4ext::Handle<RED4ext::IScriptable>>(this);
    world.component = handle && *handle ? handle->GetPtr() : nullptr;
  }
  if (p.collisionPenalty) {
    Field<float>(p.collisionPenalty, this) = 0.5f;
  }
  for (int i = 0; i < int(kGlue.size()); ++i) {
    GlueFunction(this, i);
  }
  ApplySettings(true);
  PullState();
}

// The Mod Settings (enabled, brakeAccel, landAtDestination, padRange) and the developer overrides (autopilot.<name>)
// written into the tuning: when the mode is set up and engaged (aForce), and when either changes. A field a script set
// meanwhile keeps its value until then, as with the script mode.
void FlightModeAutopilot::ApplySettings(bool aForce) {
  const uint32_t settings = Flight::AutopilotSettingsGeneration();
  const auto &dev = Flight::Developer();
  if (!aForce && settingsApplied && settings == settingsSeen && dev.autopilotGeneration == developerSeen) {
    return;
  }
  settingsApplied = true;
  settingsSeen = settings;
  developerSeen = dev.autopilotGeneration;
  const auto &player = Flight::AutopilotPlayer();
  enabled = player.enabled;
  params.brakeAccel = player.brakeAccel;
  params.landAtDestination = player.landAtDestination;
  params.padRange = player.padRange;
  for (const auto &[name, value] : dev.autopilot) {
    Navigation::SetAutopilotParam(params, name, value);
  }
}

bool FlightModeAutopilot::Gather(Navigation::AutopilotInput &aIn, Frame &aFrame) {
  const Props &p = GetProps();
  if (!p.ok) {
    LTBF_WARN_ONCE("[FlightModeAutopilot] a FlightMode or FlightComponent field was not found: the Autopilot does "
                   "nothing");
    return false;
  }
  auto handle = p.component->GetValuePtr<RED4ext::Handle<RED4ext::IScriptable>>(this);
  if (!handle || !*handle) {
    return false;
  }
  auto *c = handle->GetPtr();
  auto *stats = FlightStats::Of(c);
  if (!stats) {
    return false;
  }
  aFrame.component = c;
  aFrame.stats = stats;
  world.component = c;

  aIn.stats = stats->State();
  aIn.statics = stats->Static();
  // the input fields as FlightComponent wrote them this frame: nothing here has written them yet
  float *controls[8] = {&aIn.controls.surge, &aIn.controls.lift,  &aIn.controls.sway,        &aIn.controls.yaw,
                        &aIn.controls.roll,  &aIn.controls.pitch, &aIn.controls.linearBrake, &aIn.controls.angularBrake};
  for (int i = 0; i < 8; ++i) {
    *controls[i] = Field<float>(p.controls[i], c);
  }
  aIn.isPlayerMounted = Field<bool>(p.isPlayerMounted, c);
  aFrame.collisionTimer = aIn.collisionTimer = Field<float>(p.collisionTimer, c);
  const auto &t = Field<RED4ext::Vector4>(p.tensor, c);
  aIn.thrusterTensor = {t.X, t.Y, t.Z};
  // the world inertia PhysX applies the flight torque with (the script's GetWorldInertiaRow, W 1 when valid)
  auto *fc = static_cast<FlightComponent *>(c);
  const RED4ext::Vector4 rows[3] = {fc->GetWorldInertiaRow(0), fc->GetWorldInertiaRow(1), fc->GetWorldInertiaRow(2)};
  aIn.inertia.valid = rows[0].W > 0.5f;
  aIn.inertia.x = {rows[0].X, rows[0].Y, rows[0].Z};
  aIn.inertia.y = {rows[1].X, rows[1].Y, rows[1].Z};
  aIn.inertia.z = {rows[2].X, rows[2].Y, rows[2].Z};

  // FlightSettings as the script read them: the instance's fields, and the named floats (GetFloatN). The instance is
  // FlightSettings.cpp's static, kept for the game's life (GetInstance adds a reference on each call: taken once).
  static FlightSettings *settings = FlightSettings::GetInstance().GetPtr();
  auto &f = aIn.settings;
  f.physxFlightPhysics = FieldOr<bool>(p.physx, settings, f.physxFlightPhysics);
  f.brakeFactorLinear = FieldOr<float>(p.brakeLinear, settings, f.brakeFactorLinear);
  f.brakeFactorAngular = FieldOr<float>(p.brakeAngular, settings, f.brakeFactorAngular);
  f.generalDampFactorAngular = FieldOr<float>(p.dampAngular, settings, f.generalDampFactorAngular);
  f.generalYawDampFactor = FieldOr<float>(p.yawDamp, settings, f.generalYawDampFactor);
  f.generalDampFactorAngularMax = FieldOr<float>(p.dampMax, settings, f.generalDampFactorAngularMax);
  f.dampStabilityK = FlightSettings::GetFloatN("dampStabilityK");
  f.dampMaxPerFrame = FlightSettings::GetFloatN("dampMaxPerFrame");
  f.collisionRecoveryDelay = FlightSettings::GetFloatN("collisionRecoveryDelay");
  f.shapeDrag = Flight::Developer().shapeDrag;
  f.dragK = Flight::Developer().dragK;

  // the Standard factors: the script mode inherited FlightModeStandard's Mod Settings, so they are read from a
  // FlightModeStandard in this car's list (Mod Settings writes them on each); the defaults without one
  auto &s = aIn.standard;
  s = {};
  if (p.standard) {
    const auto &modes = Field<RED4ext::DynArray<RED4ext::Handle<RED4ext::IScriptable>>>(p.modes, c);
    for (uint32_t i = 0; i < modes.size; ++i) {
      auto *m = modes.entries[i].GetPtr();
      if (m && m->GetType()->IsA(p.standard)) {
        float *fields[9] = {&s.surgeFactor, &s.yawFactor,  &s.swayFactor,      &s.pitchFactor,   &s.liftFactor,
                            &s.hoverFactor, &s.pitchInputAngle, &s.rollFactor, &s.rollInputAngle};
        for (int k = 0; k < 9; ++k) {
          *fields[k] = FieldOr<float>(p.standardFields[k], m, *fields[k]);
        }
        break;
      }
    }
  }
  s.rollWithYaw = FieldOr<float>(p.rollWithYaw, settings, s.rollWithYaw);
  s.pitchWithLift = FlightSettings::GetFloatN("pitchWithLift");
  s.pitchWithSurge = FlightSettings::GetFloatN("pitchWithSurge");
  s.brakeFactorLinear = f.brakeFactorLinear;
  s.hoverClamp = FlightSettings::GetFloatN("hoverClamp");
  s.minHoverHeight = FlightSettings::GetFloatN("hoverModeMinHoverHeight");
  s.maxHoverHeight = FlightSettings::GetFloatN("hoverModeMaxHoverHeight");
  s.gravityFactor = Field<float>(p.gravityFactor, this);
  standardModeLiftFactor = s.liftFactor;

  // the destination the game tracks: a summon's own goal wins and needs none (CurrentDestination, 868)
  aIn.tracked = {};
  if (!ap.hasExternalDestination) {
    if (auto *fn = GlueFunction(this, kTrackedDestination)) {
      RED4ext::Vector4 target{};
      if (RED4ext::ExecuteFunction(this, fn, &target)) {
        aIn.tracked = {target.W > 0.5f, N(target)};
      }
    }
  }
  // the PIDs are not passed: a frame the player has taken over is flown below with the scaled Standard factors
  aIn.pitchPid = aIn.rollPid = nullptr;
  ap.planBase = FlightNavMap::PlanBase();
  return true;
}

// The summon's fields script wrote since the last call (FlightSummonSystem sets them after Activate), into the
// Autopilot.
void FlightModeAutopilot::PushScriptWrites() {
  if (!Same(externalDestination, shadowExternal)) {
    ap.externalDestination = N(externalDestination);
  }
  if (!Same(padLandingOffset, shadowPadOffset)) {
    ap.padLandingOffset = N(padLandingOffset);
  }
  if (!Same(landingKeepClear, shadowKeepClear)) {
    ap.landingKeepClear = N(landingKeepClear);
  }
}

// The Autopilot's String and Vector4 fields copied out for script (the rest are properties over the C++ members).
void FlightModeAutopilot::PullState() {
  status = ap.status.c_str();
  shadowExternal = externalDestination = P(ap.externalDestination);
  shadowPadOffset = padLandingOffset = D(ap.padLandingOffset);
  shadowKeepClear = landingKeepClear = P(ap.landingKeepClear);
  destination = P(ap.destination);
  holdPosition = P(ap.holdPosition);
  routeGoal = P(ap.routeGoal);
  landingSpot = P(ap.landingSpot);
  landingHeading0 = D(ap.landingHeading0);
  landingHeading = D(ap.landingHeading);
  lastPosition = P(ap.lastPosition);
  freeRoamCentre = P(ap.freeRoamCentre);
  joinPoint = P(ap.joinPoint);
  gateEntry = P(ap.gateEntry);
  routeGateEntry = P(ap.routeGateEntry);
  landSlewHeading = D(ap.landSlewHeading);
  thrustCmd = D(ap.thrustCmd);
  closestNow = P(ap.tracker.closestNow);
  pathCurvature = D(ap.tracker.pathCurvature);
  accelCmd = D(ap.tracker.accelCmd);
  headingRef = D(ap.attitude.headingRef);
  attAccel = D(ap.attitude.attAccel);
  attRef = D(ap.attitude.attRef);
  targetRight = D(ap.control.targetRight);
  targetForward = D(ap.control.targetForward);
  targetUp = D(ap.control.targetUp);
  omegaRef = D(ap.control.omegaRef);
}

// What the events ask of the game, in the script's order within a frame (the panel, the toast, then the controller).
void FlightModeAutopilot::Glue(const Navigation::AutopilotEvents &aEvents, bool aMounted) {
  if (aEvents.progress) {
    if (auto *fn = GlueFunction(this, kPublishProgress)) {
      float length = aEvents.remainingLength, time = aEvents.remainingTime;
      RED4ext::ExecuteFunction(this, fn, nullptr, length, time);
    }
  }
  if (aEvents.freeRoamUI && aMounted) {
    if (auto *fn = GlueFunction(this, kPublishFreeRoamUI)) {
      bool on = aEvents.freeRoamEnabled;
      RED4ext::ExecuteFunction(this, fn, nullptr, on);
    }
  }
  for (const auto &key : aEvents.notifications) {
    if (auto *fn = GlueFunction(this, kShowNotification)) {
      RED4ext::CString text(key.c_str());
      RED4ext::ExecuteFunction(this, fn, nullptr, text);
    }
  }
  if (aEvents.flightOff) {
    if (auto *fn = GlueFunction(this, kRequestFlightOff)) {
      RED4ext::ExecuteFunction(this, fn, nullptr);
    }
  }
  if (aEvents.exitAutopilot) {
    if (auto *fn = GlueFunction(this, kExitAutopilot)) {
      RED4ext::ExecuteFunction(this, fn, nullptr);
    }
  }
}

// Activate (455): the Autopilot engaged, then, with the player in the car, vanilla autodrive's facts and panel
void FlightModeAutopilot::OnActivateNative() {
  Navigation::AutopilotInput in;
  Frame fr;
  if (!Gather(in, fr)) {
    return;
  }
  ApplySettings(true);
  PushScriptWrites();
  ap.Activate(in);
  if (in.isPlayerMounted) {
    if (auto *fn = GlueFunction(this, kAutodriveEngaged)) {
      bool on = true;
      RED4ext::ExecuteFunction(this, fn, nullptr, on);
    }
  }
  Glue(ap.TakeEvents(), in.isPlayerMounted);
  PullState();
}

// Deactivate (740): the route and its plan dropped, and the facts and the panel off with the player in the car
void FlightModeAutopilot::OnDeactivateNative() {
  const Props &p = GetProps();
  RED4ext::IScriptable *c = nullptr;
  if (p.ok) {
    auto handle = p.component->GetValuePtr<RED4ext::Handle<RED4ext::IScriptable>>(this);
    c = handle && *handle ? handle->GetPtr() : nullptr;
  }
  const bool mounted = c && Field<bool>(p.isPlayerMounted, c);
  PushScriptWrites();
  ap.Deactivate();
  if (mounted) {
    if (auto *fn = GlueFunction(this, kAutodriveEngaged)) {
      bool on = false;
      RED4ext::ExecuteFunction(this, fn, nullptr, on);
    }
  }
  Glue(ap.TakeEvents(), mounted);
  PullState();
}

// Teleported (1392): hold where the car is now, the route planned again from there if there was one
void FlightModeAutopilot::OnTeleportedNative() {
  Navigation::AutopilotInput in;
  Frame fr;
  if (!Gather(in, fr)) {
    return;
  }
  PushScriptWrites();
  ap.Teleported(in);
  Glue(ap.TakeEvents(), in.isPlayerMounted);
  PullState();
}

void FlightModeAutopilot::CloseTelemetry() {
  if (telemetryFile != 0) {
    FlightTelemetry::End(telemetryFile);
    telemetryFile = 0;
  }
}

// WriteTelemetry (1442): with telemetry on, a row a frame before Step, from the position the frame will fly from
void FlightModeAutopilot::WriteTelemetry(const Navigation::AutopilotInput &aIn, float aCollisionTimer) {
  if (!ap.telemetry) {
    CloseTelemetry();
    return;
  }
  if (telemetryFile == 0) {
    telemetryFile = FlightTelemetry::Begin("autopilot", Navigation::kAutopilotTelemetryHeader);
    telemetryClock = 0.0f;
    if (telemetryFile == 0) {
      return;
    }
  }
  telemetryClock += aIn.dt;
  const Navigation::Vec3 position{aIn.stats.d_position.x, aIn.stats.d_position.y, aIn.stats.d_position.z};
  const Navigation::Vec3 p = ap.JumpHeldOff(position, aIn.stats.d_speed, aIn.dt) ? ap.lastPosition : position;
  const auto row = Navigation::AutopilotTelemetryRow(ap, telemetryClock, p, aIn.stats, aCollisionTimer);
  RED4ext::DynArray<float> values;
  values.Reserve(uint32_t(row.size()));
  for (const float v : row) {
    values.EmplaceBack(v);
  }
  FlightTelemetry::Row(telemetryFile, values);
}

// Update (1552)
void FlightModeAutopilot::OnUpdateNative(float aTimeDelta) {
  Navigation::AutopilotInput in;
  Frame fr;
  in.dt = aTimeDelta;
  if (!Gather(in, fr)) {
    return;
  }
  ApplySettings(false);
  PushScriptWrites();
  WriteTelemetry(in, fr.collisionTimer);

  const Navigation::AutopilotOutput out = ap.Step(in);

  if (out.manual) {
    // the player has the controls: the Standard mode's UpdateWithNormalLift on the native path (FlightModeControl's
    // StandardLift with StandardControl's params: surge and sway scaled by the car's thrust) and the component's PIDs
    const Props &p = GetProps();
    Flight::StandardParams scaled = in.standard;
    const float thrust = FlightModeControl::Thrust(in.statics, scaled.surgeFactor);
    scaled.surgeFactor *= thrust;
    scaled.swayFactor *= thrust;
    auto pid = [&](CProperty *aProp) -> FlightPID * {
      auto h = aProp ? aProp->GetValuePtr<RED4ext::Handle<RED4ext::IScriptable>>(fr.component) : nullptr;
      return h && *h ? static_cast<FlightPID *>(h->GetPtr()) : nullptr;
    };
    FlightPID *pitchHandle = pid(p.pitchPid), *rollHandle = pid(p.rollPid);
    Flight::Pid pitchPid, rollPid;
    if (pitchHandle) {
      pitchPid.state = pitchHandle->State();
      pitchPid.gains = pitchHandle->Gains();
    }
    if (rollHandle) {
      rollPid.state = rollHandle->State();
      rollPid.gains = rollHandle->Gains();
    }
    const Flight::ModeOutput manual = Flight::StandardLift(in.stats, in.statics, in.controls, scaled, pitchPid, rollPid,
                                                           out.manualNormal, out.manualLift, aTimeDelta);
    if (pitchHandle) {
      pitchHandle->SetState(pitchPid.state);
    }
    if (rollHandle) {
      rollHandle->SetState(rollPid.state);
    }
    force = D(manual.force);
    torque = D(manual.torque);
  } else {
    force = D(out.force);
    torque = D(out.torque);
  }
  if (out.writeControls) {
    // the commands into the input fields, for the thrusters' look, the engine sound and the force tail's damping (no
    // yaw input: it damps less on an axis with input, and the attitude control's give-back assumes full damping)
    const Props &p = GetProps();
    Field<float>(p.controls[0], fr.component) = out.controls.surge;
    Field<float>(p.controls[1], fr.component) = out.controls.lift;
    Field<float>(p.controls[2], fr.component) = out.controls.sway;
    Field<float>(p.controls[3], fr.component) = out.controls.yaw;
    Field<float>(p.controls[6], fr.component) = out.controls.linearBrake;
    Field<float>(p.controls[7], fr.component) = out.controls.angularBrake;
  }
  Glue(out.events, in.isPlayerMounted);
  PullState();
}

int32_t FlightModeAutopilot::RouteSize() { return ap.RouteSize(); }

RED4ext::Vector4 FlightModeAutopilot::RoutePoint(int32_t aIndex) {
  return aIndex >= 0 && aIndex < ap.RouteSize() ? P(ap.route.Point(aIndex)) : RED4ext::Vector4{};
}

float FlightModeAutopilot::RouteDistance(int32_t aIndex) {
  return aIndex >= 0 && aIndex < ap.RouteSize() ? ap.route.Distance(aIndex) : 0.0f;
}

float FlightModeAutopilot::RouteLength() { return ap.route.Length(); }

bool FlightModeAutopilot::HasTrajectory() { return ap.FollowsTrajectory(); }

float FlightModeAutopilot::PlannedSpeed(float aS) {
  return ap.FollowsTrajectory() ? Navigation::SampleAt(ap.trajectory, aS).speed : 0.0f;
}

RED4ext::Vector4 FlightModeAutopilot::GroundSpot(RED4ext::Vector4 aTarget, RED4ext::Vector4 aKeepClear,
                                                 float aKeepRadius) {
  const Props &p = GetProps();
  if (p.ok) {
    auto handle = p.component->GetValuePtr<RED4ext::Handle<RED4ext::IScriptable>>(this);
    if (handle && *handle) {
      world.component = handle->GetPtr();
    }
  }
  Navigation::Vec3 spot;
  if (!ap.FindGroundSpot(N(aTarget), N(aKeepClear), aKeepRadius, spot)) {
    return {};
  }
  return P(spot);
}

// ToggleFreeRoam (1023)
void FlightModeAutopilot::ToggleFreeRoam() {
  Navigation::AutopilotInput in;
  Frame fr;
  if (!Gather(in, fr)) {
    return;
  }
  PushScriptWrites();
  ap.ToggleFreeRoam(in);
  Glue(ap.TakeEvents(), in.isPlayerMounted);
  PullState();
}

void FlightModeAutopilot::BuildTrajectory() {
  Navigation::AutopilotInput in;
  Frame fr;
  if (!Gather(in, fr)) {
    return;
  }
  ApplySettings(false);
  PushScriptWrites();
  ap.BuildTrajectory(in);
  PullState();
}

// RefreshVehicle (2013): the car's limits again, and FlightVehicleModel filled as the script filled it (the raw
// numbers; its Spec is the effective car the Autopilot plans with)
void FlightModeAutopilot::RefreshVehicle() {
  Navigation::AutopilotInput in;
  Frame fr;
  if (!Gather(in, fr)) {
    return;
  }
  ApplySettings(false);
  ap.RefreshVehicle(in);
  if (!vehicleModel) {
    vehicleModel = RED4ext::MakeHandle<FlightVehicleModel>();
  }
  auto &m = *vehicleModel;
  m.mass = in.statics.mass;
  m.airResistance = in.statics.airResistanceFactor;
  m.brakingFriction = in.statics.brakingFrictionFactor;
  m.surge = in.standard.surgeFactor;
  m.sway = in.standard.swayFactor;
  m.lift = in.standard.liftFactor;
  m.gravityFactor = in.standard.gravityFactor;
  m.brakeLinear = in.settings.brakeFactorLinear;
  m.drag = in.settings.physxFlightPhysics;
  m.drone = true;
  m.maxTilt = params.droneMaxTilt;
  m.brakeShare = params.droneBrakeShare;
  m.tiltFloor = params.droneTiltFloor;
  m.driftMax = params.raceDriftMax;
  m.speedCap = ap.freeRoam ? params.freeRoamTopSpeed : params.raceSpeedCap;
  m.margin = params.raceMargin;
  m.planDecel = params.racePlanDecel;
  m.pushOver = params.dronePushOver;
  m.cdaFront = in.statics.dragArea.y;
  m.cdaSide = in.statics.dragArea.x;
  m.cdaTop = in.statics.dragArea.z;
  m.thrusters = in.statics.thrusters;
}

RED4ext::Handle<FlightVehicleModel> FlightModeAutopilot::Vehicle() {
  if (!vehicleModel) {
    RefreshVehicle();
  }
  if (!vehicleModel) {
    vehicleModel = RED4ext::MakeHandle<FlightVehicleModel>();
  }
  return vehicleModel;
}

RED4ext::Vector4 FlightModeAutopilot::Flat(RED4ext::Vector4 aV) {
  const float length = std::sqrt(aV.X * aV.X + aV.Y * aV.Y);
  return length > 0.0001f ? RED4ext::Vector4{aV.X / length, aV.Y / length, 0.0f, 0.0f} : RED4ext::Vector4{};
}

// --- the fields script reads, straight over the C++ members ---

namespace {

// a member's offset in FlightModeAutopilot, through any number of nested members (measured on an object at a
// non-null address, as offsetof does it)
#define LTBF_AP_OFFSET(path)                                                                                          \
  (reinterpret_cast<size_t>(&(reinterpret_cast<FlightModeAutopilot *>(0x1000)->path)) - size_t(0x1000))

template <typename T> RED4ext::CBaseRTTIType *TypeOf();
template <> RED4ext::CBaseRTTIType *TypeOf<float>() { return RED4ext::CRTTISystem::Get()->GetType("Float"); }
template <> RED4ext::CBaseRTTIType *TypeOf<int32_t>() { return RED4ext::CRTTISystem::Get()->GetType("Int32"); }
template <> RED4ext::CBaseRTTIType *TypeOf<bool>() { return RED4ext::CRTTISystem::Get()->GetType("Bool"); }

struct Added {
  RED4ext::CClass *type;
  std::vector<std::string> names;

  template <typename T> void Add(const char *aName, size_t aOffset) {
    for (const auto &n : names) {
      if (n == aName) {
        return;
      }
    }
    names.emplace_back(aName);
    type->props.PushBack(RED4ext::CProperty::Create(TypeOf<T>(), aName, nullptr, uint32_t(aOffset)));
  }
};

} // namespace

void FlightModeAutopilot::DescribeFields(RED4ext::CClass *aType) {
  Added a{aType, {}};
  // the tuning, every AutopilotParams field by its script name
  const size_t params = LTBF_AP_OFFSET(params);
  for (const auto &r : Navigation::AutopilotParamTable()) {
    auto *base = reinterpret_cast<Navigation::AutopilotParams *>(0x1000);
    if (r.f) {
      a.Add<float>(r.name, params + reinterpret_cast<size_t>(&(base->*r.f)) - 0x1000);
    } else if (r.i) {
      a.Add<int32_t>(r.name, params + reinterpret_cast<size_t>(&(base->*r.i)) - 0x1000);
    } else {
      a.Add<bool>(r.name, params + reinterpret_cast<size_t>(&(base->*r.b)) - 0x1000);
    }
  }
  // the state's numbers and flags under the script's names
#define LTBF_AP_FIELD(T, name, path) a.Add<T>(name, LTBF_AP_OFFSET(path))
  LTBF_AP_FIELD(int32_t, "request", ap.request);
  LTBF_AP_FIELD(bool, "hasDestination", ap.hasDestination);
  LTBF_AP_FIELD(bool, "hasExternalDestination", ap.hasExternalDestination);
  LTBF_AP_FIELD(float, "landingKeepClearRadius", ap.landingKeepClearRadius);
  LTBF_AP_FIELD(float, "overrideTimer", ap.overrideTimer);
  LTBF_AP_FIELD(float, "replanTimer", ap.replanTimer);
  LTBF_AP_FIELD(float, "logTimer", ap.logTimer);
  LTBF_AP_FIELD(float, "stuckTimer", ap.stuckTimer);
  LTBF_AP_FIELD(float, "positionJumpTimer", ap.positionJumpTimer);
  LTBF_AP_FIELD(float, "settleTimer", ap.settleTimer);
  LTBF_AP_FIELD(bool, "landing", ap.landing);
  LTBF_AP_FIELD(int32_t, "landingPad", ap.landingPad);
  LTBF_AP_FIELD(float, "touchdownTimer", ap.touchdownTimer);
  LTBF_AP_FIELD(float, "landingStartZ", ap.landingStartZ);
  LTBF_AP_FIELD(float, "touchdownLow", ap.touchdownLow);
  LTBF_AP_FIELD(float, "spotCheckTimer", ap.spotCheckTimer);
  LTBF_AP_FIELD(bool, "requestRace", ap.requestRace);
  LTBF_AP_FIELD(bool, "requestRejoin", ap.requestRejoin);
  LTBF_AP_FIELD(bool, "routeLine", ap.routeLine);
  LTBF_AP_FIELD(bool, "onLine", ap.onLine);
  LTBF_AP_FIELD(bool, "freeRoam", ap.freeRoam);
  LTBF_AP_FIELD(bool, "freeRoamChosen", ap.freeRoamChosen);
  LTBF_AP_FIELD(bool, "requestJoin", ap.requestJoin);
  LTBF_AP_FIELD(float, "joinS", ap.joinS);
  LTBF_AP_FIELD(float, "gateEndS", ap.gateEndS);
  LTBF_AP_FIELD(bool, "spliceGate", ap.spliceGate);
  LTBF_AP_FIELD(float, "clock", ap.clock);
  LTBF_AP_FIELD(float, "lastCollisionTimer", ap.lastCollisionTimer);
  LTBF_AP_FIELD(float, "recovering", ap.recovering);
  LTBF_AP_FIELD(bool, "landSlewOn", ap.landSlewOn);
  LTBF_AP_FIELD(float, "landSlewP", ap.landSlewP);
  LTBF_AP_FIELD(float, "landSlewDescent", ap.landSlewDescent);
  LTBF_AP_FIELD(float, "landSlewStartYaw", ap.landSlewStartYaw);
  LTBF_AP_FIELD(float, "landSlewDelta", ap.landSlewDelta);
  LTBF_AP_FIELD(bool, "droneEaseLive", ap.droneEaseLive);
  LTBF_AP_FIELD(float, "attitudeError", ap.attitudeError);
  LTBF_AP_FIELD(float, "commandSurge", ap.commandSurge);
  LTBF_AP_FIELD(float, "commandSway", ap.commandSway);
  LTBF_AP_FIELD(float, "surgeMax", ap.surgeMax);
  LTBF_AP_FIELD(float, "swayMax", ap.swayMax);
  LTBF_AP_FIELD(bool, "telemetry", ap.telemetry);
  LTBF_AP_FIELD(float, "telemetryTime", ap.telemetryTime);
  // the follower's (FlightTracker's, in the script's helper)
  LTBF_AP_FIELD(int32_t, "segment", ap.tracker.segment);
  LTBF_AP_FIELD(float, "closestS", ap.tracker.closestS);
  LTBF_AP_FIELD(float, "offRouteNow", ap.tracker.offRouteNow);
  LTBF_AP_FIELD(float, "remaining", ap.tracker.remaining);
  LTBF_AP_FIELD(float, "aheadSmooth", ap.tracker.aheadSmooth);
  LTBF_AP_FIELD(float, "curvatureAhead", ap.tracker.curvatureAhead);
  LTBF_AP_FIELD(float, "blockedTimer", ap.tracker.blockedTimer);
  LTBF_AP_FIELD(bool, "blockedAhead", ap.tracker.blockedAhead);
  LTBF_AP_FIELD(bool, "emergencyBrake", ap.tracker.emergencyBrake);
  LTBF_AP_FIELD(float, "targetSpeed", ap.tracker.targetSpeed);
  LTBF_AP_FIELD(float, "speedCmd", ap.tracker.speedCmd);
  LTBF_AP_FIELD(float, "speedRate", ap.tracker.speedRate);
  LTBF_AP_FIELD(bool, "surgeHeld", ap.tracker.surge.held);
  LTBF_AP_FIELD(float, "surgeChanging", ap.tracker.surge.changing);
  // the drone attitude's and the attitude control's
  LTBF_AP_FIELD(float, "droneYaw", ap.attitude.droneYaw);
  LTBF_AP_FIELD(float, "droneYawRate", ap.attitude.droneYawRate);
  LTBF_AP_FIELD(float, "dronePitch", ap.attitude.dronePitch);
  LTBF_AP_FIELD(float, "dronePitchRate", ap.attitude.dronePitchRate);
  LTBF_AP_FIELD(float, "droneRoll", ap.attitude.droneRoll);
  LTBF_AP_FIELD(float, "droneRollRate", ap.attitude.droneRollRate);
  LTBF_AP_FIELD(float, "bankAngle", ap.control.bankAngle);
  LTBF_AP_FIELD(float, "bankRateNow", ap.control.bankRateNow);
  // AutopilotSurge's tunables (the script's surgeHold.<name>)
  LTBF_AP_FIELD(bool, "surgeDragFeedForward", ap.surgeHold.dragFeedForward);
  LTBF_AP_FIELD(bool, "surgeFullSurge", ap.surgeHold.fullSurge);
  LTBF_AP_FIELD(float, "surgeSlack", ap.surgeHold.slack);
  LTBF_AP_FIELD(float, "surgeDwell", ap.surgeHold.dwell);
  LTBF_AP_FIELD(float, "surgeAudioWindow", ap.surgeHold.audioWindow);
  LTBF_AP_FIELD(float, "surgeAudioJump", ap.surgeHold.audioJump);
  LTBF_AP_FIELD(float, "surgeAudioTime", ap.surgeHold.audioTime);
  // AutopilotPlanAhead's (the script's ahead.<name>)
  LTBF_AP_FIELD(bool, "aheadEnabled", ap.ahead.enabled);
  LTBF_AP_FIELD(float, "aheadHorizon", ap.ahead.horizon);
  LTBF_AP_FIELD(float, "aheadMinTrip", ap.ahead.minTrip);
  LTBF_AP_FIELD(float, "aheadMargin", ap.ahead.margin);
  LTBF_AP_FIELD(bool, "aheadFirstLeg", ap.ahead.firstLeg);
  LTBF_AP_FIELD(bool, "aheadWaiting", ap.ahead.waiting);
  LTBF_AP_FIELD(int32_t, "aheadTrip", ap.ahead.trip);
#undef LTBF_AP_FIELD
}
