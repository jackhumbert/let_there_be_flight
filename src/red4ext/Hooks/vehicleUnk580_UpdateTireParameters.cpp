#include "Utils/FlightModule.hpp"
#include <RED4ext/Common.hpp>
#include <RED4ext/Scripting/Natives/vehiclePhysics.hpp>
#include <RED4ext/Scripting/Natives/vehicleAcoustics.hpp>
#include <RED4ext/Scripting/Natives/Generated/vehicle/BaseObject.hpp>
#include <RED4ext/GameEngine.hpp>
#include <RED4ext/Scripting/Natives/audioThing.hpp>
#include <RED4ext/Scripting/Natives/Generated/vehicle/CarBaseObject.hpp>
#include <spdlog/spdlog.h>
#include "Flight/Component.hpp"
#include "Addresses.hpp"
#include <queue>

using namespace RED4ext;

/// @hash 1554324956
// void __fastcall UpdateTireParameters(vehicle::TireParameterUpdate * tpu, vehicle::TireUpdate *tireUpdate, CName emitter);

// stop tire skid sfx
REGISTER_FLIGHT_HOOK_HASH(void __fastcall, 1554324956, UpdateTireParameters,
                     vehicle::TireParameterUpdate *tpu, vehicle::TireUpdate * tireUpdate, CName emitter) {
  auto fc = FlightComponent::Get(tpu->acoustics->vehicle);
  if (fc && fc->active) {
//    float skidValue = tireUpdate->skidValue2;
//    if (tpu->acoustics->unk3D0) {
//      skidValue = tireUpdate->skidValue;
//    }
//    auto phys = (vehicle::WheeledPhysics*)tpu->acoustics->vehicle->physics;
//    spdlog::info("{}: {}", tpu->variables->wheelIndex, phys->insert2[tpu->variables->wheelIndex].dampedSpringForce);
//    fc->ExecuteFunction("HandleScraping", skidValue, tpu->variables->wheelIndex, tpu->variables->emitterName);
//    CGameEngine::Get()->audioThing->GetEmitterPosition(tpu->acoustics->vehicle->entityID, tpu->variables->emitterName);

    tireUpdate->skidValue = 0.0;
    tireUpdate->skidValue2 = 0.0;
  }
  UpdateTireParameters_Original(tpu, tireUpdate, emitter);
}