#include "Utils/FlightModule.hpp"
#include <RED4ext/Common.hpp>
#include <RED4ext/Scripting/Natives/vehiclePhysics.hpp>
#include <RED4ext/Scripting/Natives/Generated/vehicle/BaseObject.hpp>
#include <RED4ext/GameEngine.hpp>
#include <RED4ext/Scripting/Natives/audioThing.hpp>
#include <RED4ext/Scripting/Natives/Generated/vehicle/CarBaseObject.hpp>
#include <spdlog/spdlog.h>
#include "Flight/Component.hpp"
#include "Addresses.hpp"
#include <queue>

using namespace RED4ext;

// stop tire skid sfx
REGISTER_FLIGHT_HOOK(void __fastcall, vehicleUnk580_UpdateTireParameters,
                     vehicle::TireParameterUpdate *tpu, vehicle::TireUpdate * tireUpdate) {
  auto fc = FlightComponent::Get(tpu->unk580->vehicle);
  if (fc && fc->active) {
//    float skidValue = tireUpdate->skidValue2;
//    if (tpu->unk580->unk3D0) {
//      skidValue = tireUpdate->skidValue;
//    }
//    auto phys = (vehicle::WheeledPhysics*)tpu->unk580->vehicle->physics;
//    spdlog::info("{}: {}", tpu->variables->wheelIndex, phys->insert2[tpu->variables->wheelIndex].dampedSpringForce);
//    fc->ExecuteFunction("HandleScraping", skidValue, tpu->variables->wheelIndex, tpu->variables->emitterName);
//    CGameEngine::Get()->audioThing->GetEmitterPosition(tpu->unk580->vehicle->entityID, tpu->variables->emitterName);

    tireUpdate->skidValue = 0.0;
    tireUpdate->skidValue2 = 0.0;
  }
  vehicleUnk580_UpdateTireParameters_Original(tpu, tireUpdate);
}