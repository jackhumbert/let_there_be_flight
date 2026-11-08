#include "FlightModeStandard.hpp"

#include "Flight/Component.hpp"

#include "Flight/FlightGround.hpp"
#include "Flight/FlightModeControl.hpp"
#include "Flight/FlightStats.hpp"
#include "Flight/Handling.hpp"
#include "Flight/Settings.hpp"
#include "FlightSettings.hpp"
#include "Utils/Utils.hpp"
#include "stdafx.hpp"

#include <RED4ext/RTTISystem.hpp>
#include <algorithm>

namespace {

// The script fields the mode reads and writes, looked up once: the mode's own component (FlightMode.component) and the
// component's controls, thruster tensor and hover height.
struct Fields {
  RED4ext::CProperty *component = nullptr;
  RED4ext::CProperty *controls[8] = {};
  RED4ext::CProperty *tensor = nullptr, *hoverHeight = nullptr;
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
    const char *names[8] = {"surge", "lift", "sway", "yaw", "roll", "pitch", "linearBrake", "angularBrake"};
    bool all = true;
    for (int i = 0; i < 8; ++i)
      all &= (f.controls[i] = component->GetProperty(names[i])) != nullptr;
    f.tensor = component->GetProperty("thrusterTensor");
    f.hoverHeight = component->GetProperty("hoverHeight");
    f.ok = all && f.component && f.tensor && f.hoverHeight;
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
    LTBF_WARN_ONCE("[FlightModeStandard] a FlightMode or FlightComponent field was not found: the mode does nothing");
    return {};
  }
  auto handle = f.component->GetValuePtr<RED4ext::Handle<RED4ext::IScriptable>>(aMode);
  if (!handle || !*handle)
    return {};
  return {handle->GetPtr(), FlightStats::Of(handle->GetPtr())};
}

Flight::StandardParams Params(const Flight::StatsStatic &aStatic) {
  auto rtti = RED4ext::CRTTISystem::Get();
  Flight::StandardParams p;
  p.minHoverHeight = GetFlightSettingsFloat(rtti, "hoverModeMinHoverHeight", p.minHoverHeight);
  p.maxHoverHeight = GetFlightSettingsFloat(rtti, "hoverModeMaxHoverHeight", p.maxHoverHeight);
  // the brake times Brake Strength, as FlightSettings.LinearBrake gives it
  p.brakeFactorLinear = GetFlightSettingsFloat(rtti, "brakeFactorLinear", p.brakeFactorLinear) *
                        GetFlightSettingsFloat(rtti, "brakeStrength", 1.0f);
  const float thrust = FlightModeControl::Thrust(aStatic, p.surgeFactor);
  p.surgeFactor *= thrust;
  p.swayFactor *= thrust;
  return p;
}

} // namespace

void FlightModeStandard::OnActivateNative() {
  const Frame fr = Gather(this);
  if (!fr.component || !fr.stats)
    return;
  const Flight::StandardParams p = Params(fr.stats->Static());
  // hold the height the car is at, hover at the ground's distance inside the band
  state = {};
  state.referenceZ = fr.stats->d_position.Z;
  const Flight::Ground g = FlightGroundNative::Find(fr.component, p.maxHoverHeight);
  float &hoverHeight = *Resolve().hoverHeight->GetValuePtr<float>(fr.component);
  const float distance = g.found ? g.distance : hoverHeight;
  hoverHeight = state.hoverHeight = std::clamp(distance, p.minHoverHeight, p.maxHoverHeight);
}

void FlightModeStandard::OnUpdateNative(float aTimeDelta) {
  const Frame fr = Gather(this);
  if (!fr.component || !fr.stats)
    return;
  const Fields &f = Resolve();
  Flight::Controls c;
  float *controls[8] = {&c.surge, &c.lift, &c.sway, &c.yaw, &c.roll, &c.pitch, &c.linearBrake, &c.angularBrake};
  for (int i = 0; i < 8; ++i)
    *controls[i] = *f.controls[i]->GetValuePtr<float>(fr.component);
  const auto &t = *f.tensor->GetValuePtr<RED4ext::Vector4>(fr.component);
  const Flight::StatsStatic st = fr.stats->Static();
  const Flight::StandardParams p = Params(st);

  Flight::StandardGains a;
  a.maxYawRate = Flight::Player().standardTurnRate * 3.14159265f / 180.0f;
  a.maxClimbRate = Flight::Player().standardClimbRate;

  const Flight::Ground g = FlightGroundNative::Find(fr.component, p.maxHoverHeight);
  const Flight::Vec3 response = Flight::TorqueResponse(st, {t.X, t.Y, t.Z});
  a = Flight::WithHandling(a, FlightSettingsHandling());
  const Flight::ModeOutput out = Flight::Standard(state, fr.stats->State(), st, c, p, a, g, response, aTimeDelta, Band());
  force = {out.force.x, out.force.y, out.force.z, 0.0f};
  torque = {out.torque.x, out.torque.y, out.torque.z, 0.0f};
  *f.hoverHeight->GetValuePtr<float>(fr.component) = state.hoverHeight;
  hovering = state.hovering;
  referenceZ = state.referenceZ;
  holdingAltitude = state.holdingAltitude;
}

int32_t FlightSettingsHandling() {
  // FlightSettings.handling on the instance Mod Settings writes (a redscript enum: its size from its type)
  static RED4ext::CProperty *field = [] {
    auto cls = RED4ext::CRTTISystem::Get()->GetClass("FlightSettings");
    return cls ? cls->GetProperty("handling") : nullptr;
  }();
  static FlightSettings *settings = FlightSettings::GetInstance().GetPtr();
  const int32_t fallback = static_cast<int32_t>(Flight::Handling::Default);
  if (!field || !field->type || !settings)
    return fallback;
  const void *data = field->GetValuePtr<void>(settings);
  switch (field->type->GetSize()) {
  case 1:
    return *static_cast<const int8_t *>(data);
  case 4:
    return *static_cast<const int32_t *>(data);
  case 8:
    return static_cast<int32_t>(*static_cast<const int64_t *>(data));
  default:
    return fallback;
  }
}

bool ExtraFlightModesEnabled() {
  return GetFlightSettingsBool(RED4ext::CRTTISystem::Get(), "extraModesEnabled", false);
}
