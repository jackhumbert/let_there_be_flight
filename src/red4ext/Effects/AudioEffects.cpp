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

// stop tire skid sfx
REGISTER_FLIGHT_HOOK(void __fastcall, vehicleUnk580_UpdateTireParameters,
                     vehicle::TireParameterUpdate *tpu, vehicle::TireUpdate * tireUpdate) {
  auto fc = FlightComponent::Get(tpu->unk580->vehicle);
  if (fc && fc->active) {
    float skidValue = tireUpdate->skidValue2;
    if (tpu->unk580->unk3D0) {
      skidValue = tireUpdate->skidValue;
    }
    fc->ExecuteFunction("HandleScraping", skidValue, tpu->variables->wheelIndex, tpu->variables->emitterName);

    tireUpdate->skidValue = 0.0;
    tireUpdate->skidValue2 = 0.0;
  }
  vehicleUnk580_UpdateTireParameters_Original(tpu, tireUpdate);
}