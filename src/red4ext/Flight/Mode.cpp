#include "Mode.hpp"

#include "stdafx.hpp"

#include <RED4ext/RTTISystem.hpp>
#include <RED4ext/TweakDB.hpp>
#include <array>
#include <mutex>
#include <string>
#include <unordered_map>

namespace {
// the hooks FlightMode.reds declares, by index
constexpr std::array<const char *, 4> kHooks = {"OnActivate", "OnDeactivate", "OnUpdate", "Description"};
enum { kActivate, kDeactivate, kUpdate, kDescription };
} // namespace

RED4ext::CClassFunction *FlightMode::Hook(int aIndex) {
  static std::mutex lock;
  static std::unordered_map<const RED4ext::CClass *, std::array<RED4ext::CClassFunction *, kHooks.size()>> hooks;
  static const RED4ext::CClass *base = RED4ext::CRTTISystem::Get()->GetClass("FlightMode");
  const RED4ext::CClass *cls = GetType();
  std::lock_guard guard(lock);
  auto it = hooks.find(cls);
  if (it == hooks.end()) {
    std::array<RED4ext::CClassFunction *, kHooks.size()> found{};
    for (size_t i = 0; i < kHooks.size(); ++i) {
      auto *f = cls->GetFunction(kHooks[i]);
      // FlightMode's own (empty) hook is no override
      found[i] = f && f->parent != base ? f : nullptr;
    }
    it = hooks.emplace(cls, found).first;
  }
  return it->second[aIndex];
}

void FlightMode::NativeActivate() {
  OnActivateNative();
  if (auto *f = Hook(kActivate))
    RED4ext::ExecuteFunction(this, f, nullptr);
}

void FlightMode::NativeDeactivate() {
  OnDeactivateNative();
  if (auto *f = Hook(kDeactivate))
    RED4ext::ExecuteFunction(this, f, nullptr);
}

void FlightMode::NativeUpdate(float aTimeDelta) {
  OnUpdateNative(aTimeDelta);
  if (auto *f = Hook(kUpdate))
    RED4ext::ExecuteFunction(this, f, nullptr, aTimeDelta);
}

RED4ext::CString FlightMode::NativeDescription() {
  if (auto *f = Hook(kDescription)) {
    RED4ext::CString out;
    if (RED4ext::ExecuteFunction(this, f, &out))
      return out;
  }
  return DescriptionNative();
}

RED4ext::DynArray<RED4ext::CName> FlightMode::ModeClasses() {
  RED4ext::DynArray<RED4ext::CName> classes;
  auto *tweakDB = RED4ext::TweakDB::Get();
  if (!tweakDB || !tweakDB->TryGetValue(RED4ext::TweakDBID("FlightModes.classes"), classes))
    spdlog::warn("[FlightMode] no FlightModes.classes in TweakDB: no flight modes");
  return classes;
}

RED4ext::Handle<FlightMode> FlightMode::Instantiate(RED4ext::CName aClass) {
  auto *rtti = RED4ext::CRTTISystem::Get();
  static auto *base = rtti->GetClass("FlightMode");
  auto *cls = rtti->GetClassByScriptName(aClass);
  if (!cls)
    cls = rtti->GetClass(aClass);
  if (!cls || !base || !cls->IsA(base) || cls->flags.isAbstract) {
    spdlog::warn("[FlightMode] {} is not a concrete FlightMode class", aClass.ToString());
    return {};
  }
  auto *mode = cls->CreateInstance<FlightMode *>(true);
  if (!mode) {
    spdlog::warn("[FlightMode] could not make a {}", aClass.ToString());
    return {};
  }
  cls->ConstructCls(mode);
  // the instance's own class, for a redscript one (its overrides and fields), as `new` in redscript sets it
  mode->nativeType = cls;
  return RED4ext::Handle<FlightMode>(mode);
}

void FlightModeNativeProbe::OnActivateNative() {
  if (activations++ == 0)
    spdlog::info("[FlightModeNativeProbe] Activate reached the C++ mode");
}

void FlightModeNativeProbe::OnUpdateNative(float aTimeDelta) {
  if (updates++ == 0)
    spdlog::info("[FlightModeNativeProbe] Update reached the C++ mode (dt {:.4f})", aTimeDelta);
  // a marker a redscript hook after this step can read (native_mode_probe.reds)
  force = {0.25f, 0.0f, 0.0f, 0.0f};
}

RED4ext::CString FlightModeNativeProbe::Calls() {
  return ("activations " + std::to_string(activations) + " updates " + std::to_string(updates)).c_str();
}
