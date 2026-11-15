#include "FlightModeHoverFlyClassic.hpp"

#include "Flight/Component.hpp"
#include "Flight/FlightGround.hpp"
#include "Flight/FlightModeControl.hpp"
#include "Flight/FlightModeStandard.hpp"
#include "Flight/FlightPID.hpp"
#include "Flight/FlightStats.hpp"
#include "Flight/Handling.hpp"
#include "Flight/Settings.hpp"
#include "Utils/Utils.hpp"
#include "stdafx.hpp"

#include <RED4ext/RTTISystem.hpp>

namespace {

// The script fields the mode reads and writes, looked up once: the mode's component and gravityFactor
// (FlightMode.component, FlightMode.gravityFactor), and the component's controls, distance, hover height and PIDs.
struct Fields {
  RED4ext::CProperty *component = nullptr, *gravityFactor = nullptr;
  RED4ext::CProperty *controls[8] = {};
  RED4ext::CProperty *distance = nullptr, *hoverHeight = nullptr;
  RED4ext::CProperty *pitchPid = nullptr, *rollPid = nullptr, *hoverPid = nullptr;
  bool ok = false;
};

const Fields &Resolve() {
  static Fields f = [] {
    Fields f;
    auto rtti = RED4ext::CRTTISystem::Get();
    auto mode = rtti->GetClass("FlightMode");
    auto component = rtti->GetClass("FlightComponent");
    if (!mode || !component)
      return f;
    f.component = mode->GetProperty("component");
    f.gravityFactor = mode->GetProperty("gravityFactor");
    const char *names[8] = {"surge", "lift", "sway", "yaw", "roll", "pitch", "linearBrake", "angularBrake"};
    bool all = true;
    for (int i = 0; i < 8; ++i)
      all &= (f.controls[i] = component->GetProperty(names[i])) != nullptr;
    f.distance = component->GetProperty("distance");
    f.hoverHeight = component->GetProperty("hoverHeight");
    f.pitchPid = component->GetProperty("pitchPID");
    f.rollPid = component->GetProperty("rollPID");
    f.hoverPid = component->GetProperty("hoverGroundPID");
    f.ok = all && f.component && f.gravityFactor && f.distance && f.hoverHeight && f.pitchPid && f.rollPid &&
           f.hoverPid;
    return f;
  }();
  return f;
}

struct Frame {
  RED4ext::IScriptable *component = nullptr;
  FlightStats *stats = nullptr;
};

Frame Gather(FlightMode *aMode) {
  const Fields &f = Resolve();
  if (!f.ok) {
    LTBF_WARN_ONCE("[FlightModeHoverFlyClassic] a FlightMode or FlightComponent field was not found: the mode does nothing");
    return {};
  }
  auto handle = f.component->GetValuePtr<RED4ext::Handle<RED4ext::IScriptable>>(aMode);
  if (!handle || !*handle)
    return {};
  return {handle->GetPtr(), FlightStats::Of(handle->GetPtr())};
}

FlightPID *PidOf(RED4ext::CProperty *aField, RED4ext::IScriptable *aComponent) {
  auto h = aField->GetValuePtr<RED4ext::Handle<RED4ext::IScriptable>>(aComponent);
  return h && *h ? static_cast<FlightPID *>(h->GetPtr()) : nullptr;
}

// The standard factors at the code's defaults (the old script FlightModeStandard's), with the FlightSettings values the
// script read each frame, the player's Handling (the roll with yaw it sets) and Brake Strength, and surge and sway
// scaled by the car's thrust as the Autopilot's manual flight has them.
Flight::StandardParams Params(const Flight::StatsStatic &aStatic, float aGravityFactor) {
  auto rtti = RED4ext::CRTTISystem::Get();
  Flight::StandardParams p;
  p.minHoverHeight = GetFlightSettingsFloat(rtti, "hoverModeMinHoverHeight", p.minHoverHeight);
  p.maxHoverHeight = GetFlightSettingsFloat(rtti, "hoverModeMaxHoverHeight", p.maxHoverHeight);
  p.hoverClamp = GetFlightSettingsFloat(rtti, "hoverClamp", p.hoverClamp);
  p.pitchWithLift = GetFlightSettingsFloat(rtti, "pitchWithLift", p.pitchWithLift);
  p.pitchWithSurge = GetFlightSettingsFloat(rtti, "pitchWithSurge", p.pitchWithSurge);
  Flight::HandlingFactors d;
  d.generalRollWithYaw = p.rollWithYaw;
  p.rollWithYaw = Flight::HandlingSet(FlightSettingsHandling(), d).generalRollWithYaw;
  p.brakeFactorLinear = GetFlightSettingsFloat(rtti, "brakeFactorLinear", p.brakeFactorLinear) *
                        GetFlightSettingsFloat(rtti, "brakeStrength", 1.0f);
  p.gravityFactor = aGravityFactor;
  const float thrust = FlightModeControl::Thrust(aStatic, p.surgeFactor);
  p.surgeFactor *= thrust;
  p.swayFactor *= thrust;
  return p;
}

} // namespace

bool FlightModeHoverFlyClassic::IsEnabledNative() { return Flight::Player().hoverFlyClassicEnabled; }

void FlightModeHoverFlyClassic::OnActivateNative() {
  const Frame fr = Gather(this);
  if (!fr.component || !fr.stats)
    return;
  const Fields &f = Resolve();
  const Flight::StandardParams p = Params(fr.stats->Static(), *f.gravityFactor->GetValuePtr<float>(this));
  // FindGround writes the component's distance when it finds ground; the height held is the distance it leaves
  FlightGroundNative::Find(fr.component, p.maxHoverHeight);
  Flight::HoverFlyClassicActivate(state, *f.distance->GetValuePtr<float>(fr.component), p);
  *f.hoverHeight->GetValuePtr<float>(fr.component) = state.hoverHeight;
}

void FlightModeHoverFlyClassic::OnUpdateNative(float aTimeDelta) {
  const Frame fr = Gather(this);
  if (!fr.component || !fr.stats)
    return;
  const Fields &f = Resolve();
  Flight::Controls c;
  float *controls[8] = {&c.surge, &c.lift, &c.sway, &c.yaw, &c.roll, &c.pitch, &c.linearBrake, &c.angularBrake};
  for (int i = 0; i < 8; ++i)
    *controls[i] = *f.controls[i]->GetValuePtr<float>(fr.component);
  const Flight::StatsStatic st = fr.stats->Static();
  const Flight::StandardParams p = Params(st, *f.gravityFactor->GetValuePtr<float>(this));

  // the hover height the component holds (the HUD and a mode switched to next read it)
  state.hoverHeight = *f.hoverHeight->GetValuePtr<float>(fr.component);
  Flight::Ground g = FlightGroundNative::Find(fr.component, p.maxHoverHeight);
  g.distance = *f.distance->GetValuePtr<float>(fr.component);

  FlightPID *handles[3] = {PidOf(f.pitchPid, fr.component), PidOf(f.rollPid, fr.component),
                           PidOf(f.hoverPid, fr.component)};
  Flight::Pid pids[3];
  for (int i = 0; i < 3; ++i) {
    if (handles[i]) {
      pids[i].state = handles[i]->State();
      pids[i].gains = handles[i]->Gains();
    }
  }
  const Flight::ModeOutput out = Flight::HoverFlyClassic(state, fr.stats->State(), st, c, p, {}, pids[0], pids[1],
                                                         pids[2], g, aTimeDelta);
  for (int i = 0; i < 3; ++i) {
    if (handles[i])
      handles[i]->SetState(pids[i].state);
  }
  force = {out.force.x, out.force.y, out.force.z, 0.0f};
  torque = {out.torque.x, out.torque.y, out.torque.z, 0.0f};
  *f.hoverHeight->GetValuePtr<float>(fr.component) = state.hoverHeight;
  hovering = state.hovering;
}
