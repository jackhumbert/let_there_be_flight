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

// post 2.0
/// @pattern 48 89 5C 24 08 48 89 54 24 10 57 48 83 EC 50 48 8B F9 45 84 C0 75 09 44 38 81 60 01 00 00 75 6A
RED4ext::vehicle::MaterialFx * __fastcall GetFxForMaterial(RED4ext::vehicle::Effects *effects, RED4ext::CName material, bool isBackWheel);

// post 2.0
/// @pattern 48 8B C4 48 89 58 08 48 89 70 10 48 89 78 18 4C 89 48 20 55 41 54 41 55 41 56 41 57 48 8D 68 A8
/// @nth 0/2
bool __fastcall TireTrackEffectStart(RED4ext::vehicle::Effects *effects, RED4ext::vehicle::Effects::Unk30 *a2, RED4ext::vehicle::MaterialFx *fxLookup, RED4ext::Transform *a4, RED4ext::Transform *a5, bool physicalMaterialChange, bool conditionChange, bool condition);

// post 2.0
/// @pattern 48 8B C4 48 89 58 08 48 89 70 10 48 89 78 18 4C 89 48 20 55 41 54 41 55 41 56 41 57 48 8D 68 A8
/// @nth 1/2
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