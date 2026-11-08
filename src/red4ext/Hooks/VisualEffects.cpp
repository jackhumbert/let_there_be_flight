#include "Utils/FlightModule.hpp"
#include <RED4ext/Common.hpp>
#include <RED4ext/Scripting/Natives/vehiclePhysics.hpp>
#include <RED4ext/Scripting/Natives/vehicleEffects.hpp>
#include <RED4ext/Scripting/Natives/Generated/vehicle/BaseObject.hpp>
#include <RED4ext/Scripting/Natives/Generated/vehicle/CarBaseObject.hpp>
#include <spdlog/spdlog.h>
#include "Flight/Component.hpp"

#include <mutex>
#include <unordered_map>

// using namespace RED4ext;

/// @hash 2081691750
// RED4ext::vehicle::MaterialFx * __fastcall FX_FindWheelsMaterial(RED4ext::vehicle::Effects *effects, RED4ext::CName material, bool isBackWheel);

/// @hash 3590925758
// bool __fastcall TireTrackEffectStart(RED4ext::vehicle::Effects *effects, RED4ext::vehicle::Effects::Unk30 *a2, RED4ext::vehicle::MaterialFx *fxLookup, RED4ext::Transform *a4, RED4ext::Transform *a5, bool physicalMaterialChange, bool conditionChange, bool condition);

/// @hash 2693082452
// bool __fastcall SkidMarkEffectStart(RED4ext::vehicle::Effects *effects, RED4ext::vehicle::Effects::Unk30 *unk30, RED4ext::vehicle::MaterialFx *fxLookup, RED4ext::Transform *a4, RED4ext::Transform *a5, bool physicalMaterialChange, bool conditionChange, bool condition);

struct FlightStatus {
  uint8_t isActive : 1;
  uint8_t hasChanged : 1;
};

// While flight is on the wheels leave no skid marks or tyre tracks: the scrapes have their own sparks and trails
// (Physics/ScrapeEffects.cpp). The wheels' effects had been swapped for thruster_sparks.effect and thruster_mark.effect,
// whose sound (the engine damage loop) played at every wheel at once and combed. The lookup the game gets back while
// flying is a copy of the material's with its effects emptied, one per material, kept for as long as the game holds it.
// vehicle::FX::WheelsMaterial const * vehicle::FX::FindWheelsMaterial(CName, bool) const
REGISTER_FLIGHT_HOOK_HASH(RED4ext::vehicle::MaterialFx * __fastcall, 2081691750, FX_FindWheelsMaterial, RED4ext::vehicle::Effects *effects,
                     RED4ext::CName material, bool isBackWheel) {
  auto og = FX_FindWheelsMaterial_Original(effects, material, isBackWheel);
  auto fc = og ? FlightComponent::Get(effects->vehicle) : nullptr;
  if (!fc) {
    return og;
  }
  auto status = (FlightStatus *)&og->normal.particle.extra_byte_for_fun;
  status->hasChanged = status->isActive ^ fc->active;
  status->isActive = fc->active;
  if (!fc->active) {
    return og;
  }
  static std::mutex lock;
  static std::unordered_map<uint64_t, RED4ext::vehicle::MaterialFx> empty; // nodes stay put: the game keeps pointers
  std::lock_guard<std::mutex> _(lock);
  auto &fx = empty[material.hash];
  fx = *og;
  for (auto *condition : {&fx.normal, &fx.wet, &fx.rain}) {
    for (auto *resource : {&condition->particle, &condition->decal}) {
      resource->skidMarks = {};
      resource->tireTracks = {};
    }
  }
  return &fx;
}

// trigger resource change when flight is changed
REGISTER_FLIGHT_HOOK_HASH(bool __fastcall, 3590925758, TireTrackEffectStart, RED4ext::vehicle::Effects *effects,
                     RED4ext::vehicle::Effects::Unk30 *unk30, RED4ext::vehicle::MaterialFx *fxLookup,
                     RED4ext::Transform *a4, RED4ext::Transform *a5, bool physicalMaterialChange, bool conditionChange,
                     bool condition) {
  auto modeChanged =  ((FlightStatus*)&fxLookup->normal.particle.extra_byte_for_fun)->hasChanged;
  return TireTrackEffectStart_Original(effects, unk30, fxLookup, a4, a5,
                                                     physicalMaterialChange || modeChanged, conditionChange, condition);
}

// trigger resource change when flight is changed
REGISTER_FLIGHT_HOOK_HASH(bool __fastcall, 2693082452, SkidMarkEffectStart, RED4ext::vehicle::Effects *effects,
                     RED4ext::vehicle::Effects::Unk30 *unk30, RED4ext::vehicle::MaterialFx *fxLookup,
                     RED4ext::Transform *a4, RED4ext::Transform *a5, bool physicalMaterialChange, bool conditionChange,
                     bool condition) {
  auto modeChanged = ((FlightStatus*)&fxLookup->normal.particle.extra_byte_for_fun)->hasChanged;
  return SkidMarkEffectStart_Original(effects, unk30, fxLookup, a4, a5, physicalMaterialChange || modeChanged, conditionChange, condition);
}