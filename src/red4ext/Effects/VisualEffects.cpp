#include "Utils/FlightModule.hpp"
#include <RED4ext/RED4ext.hpp>
#include <RED4ext/Scripting/Natives/vehiclePhysics.hpp>
#include <RED4ext/Scripting/Natives/Generated/vehicle/BaseObject.hpp>
#include <RED4ext/Scripting/Natives/Generated/vehicle/CarBaseObject.hpp>
#include <spdlog/spdlog.h>
#include "FlightComponent.hpp"
#include "Addresses.hpp"
#include <queue>

using namespace RED4ext;

struct FlightStatus {
  uint8_t isActive : 1;
  uint8_t hasChanged : 1;
};

// make tire always skidding
REGISTER_FLIGHT_HOOK(void __fastcall, vehicleUnk570_WheelEffectUpdate, vehicle::Unk570 *unk570,
                     unsigned int wheelIndex, vehicle::Unk570::Unk40 *unk40, float deltaTime) {
  auto fc = FlightComponent::Get(unk570->vehicle);
  if (fc && fc->active) {
    unk40->wheelLongSlip = 2.0;
  }
  vehicleUnk570_WheelEffectUpdate_Original(unk570, wheelIndex, unk40, deltaTime);
}

// replace tire tracks & skid marks with our own effects
REGISTER_FLIGHT_HOOK(vehicle::MaterialFx * __fastcall, vehicleUnk570_GetFxForMaterial, vehicle::Unk570 *unk570,
                     CName material, char isBackWheel) {
  auto og = vehicleUnk570_GetFxForMaterial_Original(unk570, material, isBackWheel);
  vehicle::MaterialFx * fx = og;
  auto fc = FlightComponent::Get(unk570->vehicle);
  if (fc) {
    auto status = (FlightStatus*)&og->normal.particle.extra_byte_for_fun;
    status->hasChanged = status->isActive ^ fc->active;
    status->isActive = fc->active;
    if (fc->active && fc->configuration) {
      fx = new vehicle::MaterialFx();
      *fx = fc->configuration->ExecuteFunction<vehicle::MaterialFx>("GetEffectForMaterial", material, *og).value();
    }
  }
  return fx;
}

// trigger resource change when flight is changed
REGISTER_FLIGHT_HOOK(bool __fastcall, vehicleUnk570_TireTrackEffectStart, vehicle::Unk570 *unk570,
                     vehicle::Unk570::Unk30 *unk30, vehicle::MaterialFx *fxLookup,
                     Transform *a4, Transform *a5, bool physicalMaterialChange, bool conditionChange,
                     bool condition) {
  auto modeChanged =  ((FlightStatus*)&fxLookup->normal.particle.extra_byte_for_fun)->hasChanged;
  return vehicleUnk570_TireTrackEffectStart_Original(unk570, unk30, fxLookup, a4, a5,
                                                     physicalMaterialChange || modeChanged, conditionChange, condition);
}

// trigger resource change when flight is changed
REGISTER_FLIGHT_HOOK(bool __fastcall, vehicleUnk570_SkidMarkEffectStart, vehicle::Unk570 *unk570,
                     vehicle::Unk570::Unk30 *unk30, vehicle::MaterialFx *fxLookup,
                     Transform *a4, Transform *a5, bool physicalMaterialChange, bool conditionChange,
                     bool condition) {
  auto modeChanged = ((FlightStatus*)&fxLookup->normal.particle.extra_byte_for_fun)->hasChanged;
  return vehicleUnk570_SkidMarkEffectStart_Original(unk570, unk30, fxLookup, a4, a5, physicalMaterialChange || modeChanged, conditionChange, condition);
}