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

#include <queue>

using namespace RED4ext;

/// @hash 1554324956
// void __fastcall UpdateTireParameters(vehicle::TireParameterUpdate * tpu, vehicle::TireUpdate *tireUpdate, CName emitter);

// doesn't affect audio bug
// stop tire skid sfx
#ifdef __APPLE__
// LTBF R41: the macOS function takes a fourth argument in s0 (`fmov s9, s0` at entry; Windows has it in xmm3 too). It is declared
// here so it reaches the original unchanged. The fork offsets this hook uses match the macOS body: TireParameterUpdate acoustics +0,
// TireUpdate skidValue +0x1C / skidValue2 +0x20 (both twins read [acoustics+0x414] to choose), Acoustics.vehicle +0x138 inferred.
REGISTER_FLIGHT_HOOK_HASH(void, 1554324956, UpdateTireParameters,
                     vehicle::TireParameterUpdate *tpu, vehicle::TireUpdate * tireUpdate, CName emitter, float a4) {
#else
REGISTER_FLIGHT_HOOK_HASH(void __fastcall, 1554324956, UpdateTireParameters,
                     vehicle::TireParameterUpdate *tpu, vehicle::TireUpdate * tireUpdate, CName emitter) {
#endif
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
#ifdef __APPLE__
  UpdateTireParameters_Original(tpu, tireUpdate, emitter, a4);
#else
  UpdateTireParameters_Original(tpu, tireUpdate, emitter);
#endif
}