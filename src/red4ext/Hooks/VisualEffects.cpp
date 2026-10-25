#include "Utils/FlightModule.hpp"
#include <RED4ext/Common.hpp>
#include <RED4ext/Scripting/Natives/vehiclePhysics.hpp>
#include <RED4ext/Scripting/Natives/vehicleEffects.hpp>
#include <RED4ext/Scripting/Natives/Generated/vehicle/BaseObject.hpp>
#include <RED4ext/Scripting/Natives/Generated/vehicle/CarBaseObject.hpp>
#include <spdlog/spdlog.h>
#include "Flight/Component.hpp"

#include <queue>

// using namespace RED4ext;

/// @hash 2081691750
// RED4ext::vehicle::MaterialFx * __fastcall GetFxForMaterial(RED4ext::vehicle::Effects *effects, RED4ext::CName material, bool isBackWheel);

/// @hash 3590925758
// bool __fastcall TireTrackEffectStart(RED4ext::vehicle::Effects *effects, RED4ext::vehicle::Effects::Unk30 *a2, RED4ext::vehicle::MaterialFx *fxLookup, RED4ext::Transform *a4, RED4ext::Transform *a5, bool physicalMaterialChange, bool conditionChange, bool condition);

/// @hash 2693082452
// bool __fastcall SkidMarkEffectStart(RED4ext::vehicle::Effects *effects, RED4ext::vehicle::Effects::Unk30 *unk30, RED4ext::vehicle::MaterialFx *fxLookup, RED4ext::Transform *a4, RED4ext::Transform *a5, bool physicalMaterialChange, bool conditionChange, bool condition);

struct FlightStatus {
  uint8_t isActive : 1;
  uint8_t hasChanged : 1;
};

// doesn't seem to affect audio bug

#ifdef __APPLE__
// LTBF R41: GetFxForMaterial is inlined on macOS (R40: no function to hook), and SkidMarkEffectStart / TireTrackEffectStart only
// restart the effect when GetFxForMaterial flagged a flight mode change in the MaterialFx's spare byte; they do not get the material
// name the replacement effect is looked up by. So on macOS the flight trail effects are not replaced and none of the three hooks is
// registered (macos/NOT_YET.md).
#else

// replace tire tracks & skid marks with our own effects
REGISTER_FLIGHT_HOOK_HASH(RED4ext::vehicle::MaterialFx * __fastcall, 2081691750, GetFxForMaterial, RED4ext::vehicle::Effects *effects,
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
#endif
