#include "Utils/FlightModule.hpp"
#include <RED4ext/Common.hpp>
#include <RED4ext/Scripting/Natives/vehiclePhysics.hpp>
#include <RED4ext/Scripting/Natives/vehicleEffects.hpp>
#include <RED4ext/Scripting/Natives/Generated/vehicle/BaseObject.hpp>
#include <RED4ext/Scripting/Natives/Generated/vehicle/CarBaseObject.hpp>
#include <spdlog/spdlog.h>
#include "Flight/Component.hpp"
#include "Addresses.hpp"
#include <queue>

// using namespace RED4ext;

/// @hash 2081691750
RED4ext::vehicle::MaterialFx * __fastcall GetFxForMaterial(RED4ext::vehicle::Effects *effects, RED4ext::CName material, bool isBackWheel);

/// @hash 3590925758
bool __fastcall TireTrackEffectStart(RED4ext::vehicle::Effects *effects, RED4ext::vehicle::Effects::Unk30 *a2, RED4ext::vehicle::MaterialFx *fxLookup, RED4ext::Transform *a4, RED4ext::Transform *a5, bool physicalMaterialChange, bool conditionChange, bool condition);

/// @hash 2693082452
bool __fastcall SkidMarkEffectStart(RED4ext::vehicle::Effects *effects, RED4ext::vehicle::Effects::Unk30 *unk30, RED4ext::vehicle::MaterialFx *fxLookup, RED4ext::Transform *a4, RED4ext::Transform *a5, bool physicalMaterialChange, bool conditionChange, bool condition);

struct FlightStatus {
  uint8_t isActive : 1;
  uint8_t hasChanged : 1;
};

// replace tire tracks & skid marks with our own effects
REGISTER_FLIGHT_HOOK(RED4ext::vehicle::MaterialFx * __fastcall, GetFxForMaterial, RED4ext::vehicle::Effects *effects,
                     RED4ext::CName material, bool isBackWheel) {
  auto og = GetFxForMaterial_Original(effects, material, isBackWheel);
  RED4ext::vehicle::MaterialFx * fx = og;
  auto fc = FlightComponent::Get(effects->vehicle);
  if (fc) {
    auto status = (FlightStatus*)&og->normal.particle.extra_byte_for_fun;
    status->hasChanged = status->isActive ^ fc->active;
    status->isActive = fc->active;
    if (fc->active && fc->configuration) {
      fx = new RED4ext::vehicle::MaterialFx();
      // *fx = fc->configuration->ExecuteFunction<RED4ext::vehicle::MaterialFx>("GetEffectForMaterial", material, *og).value();
      RED4ext::ExecuteFunction(
        (RED4ext::ScriptInstance*)fc->configuration.instance, 
        fc->configuration->nativeType->GetFunction("GetEffectForMaterial"), 
        (void*)fx,
        material, 
        *og
      );

    }
  }
  return fx;
}

// trigger resource change when flight is changed
REGISTER_FLIGHT_HOOK(bool __fastcall, TireTrackEffectStart, RED4ext::vehicle::Effects *effects,
                     RED4ext::vehicle::Effects::Unk30 *unk30, RED4ext::vehicle::MaterialFx *fxLookup,
                     RED4ext::Transform *a4, RED4ext::Transform *a5, bool physicalMaterialChange, bool conditionChange,
                     bool condition) {
  auto modeChanged =  ((FlightStatus*)&fxLookup->normal.particle.extra_byte_for_fun)->hasChanged;
  return TireTrackEffectStart_Original(effects, unk30, fxLookup, a4, a5,
                                                     physicalMaterialChange || modeChanged, conditionChange, condition);
}

// trigger resource change when flight is changed
REGISTER_FLIGHT_HOOK(bool __fastcall, SkidMarkEffectStart, RED4ext::vehicle::Effects *effects,
                     RED4ext::vehicle::Effects::Unk30 *unk30, RED4ext::vehicle::MaterialFx *fxLookup,
                     RED4ext::Transform *a4, RED4ext::Transform *a5, bool physicalMaterialChange, bool conditionChange,
                     bool condition) {
  auto modeChanged = ((FlightStatus*)&fxLookup->normal.particle.extra_byte_for_fun)->hasChanged;
  return SkidMarkEffectStart_Original(effects, unk30, fxLookup, a4, a5, physicalMaterialChange || modeChanged, conditionChange, condition);
}