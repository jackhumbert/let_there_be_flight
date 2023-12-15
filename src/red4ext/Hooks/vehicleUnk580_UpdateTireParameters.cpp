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

// pre 2.0
// 1.6 RVA: 0x1C62AE0 / 29764320
/// @pattern 48 89 5C 24 10 48 89 6C 24 18 56 57 41 56 48 83 EC 70 48 8B 69 08 49 8B D8 4C 8B 01 48 8B F2 0F

// post 2.0
/// @pattern 48 8B C4 48 89 58 10 48 89 70 18 48 89 78 20 55 48 8B EC 48 83 EC 70 0F 29 70 E8 49 8B D8 4C 8B
void __fastcall UpdateTireParameters(vehicle::TireParameterUpdate * tpu, vehicle::TireUpdate *tireUpdate, CName emitter);

// stop tire skid sfx
REGISTER_FLIGHT_HOOK(void __fastcall, UpdateTireParameters,
                     vehicle::TireParameterUpdate *tpu, vehicle::TireUpdate * tireUpdate, CName emitter) {
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
  UpdateTireParameters_Original(tpu, tireUpdate, emitter);
}