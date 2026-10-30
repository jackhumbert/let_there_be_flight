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

#include <atomic>
#include <queue>

using namespace RED4ext;

/// @hash 1554324956
// void __fastcall UpdateTireParameters(vehicle::TireParameterUpdate *tpu, vehicle::TireUpdate *tireUpdate, CName emitter,
//                                      float suspensionPressure);
// (the 4th argument, in xmm3, was left out before 2026-10-30: the original got whatever xmm3 held)

namespace VehicleAudio {
extern std::atomic<uint64_t> flyingVehicle;
}

// stop tire skid sfx; and which vehicle is flying, for SetSoundParameter
REGISTER_FLIGHT_HOOK_HASH(void __fastcall, 1554324956, UpdateTireParameters,
                     vehicle::TireParameterUpdate *tpu, vehicle::TireUpdate * tireUpdate, CName emitter, float suspensionPressure) {
  auto fc = FlightComponent::Get(tpu->acoustics->vehicle);
  const uint64_t entity = *reinterpret_cast<const uint64_t *>(&tpu->entityID);
  if (fc && fc->active) {
    VehicleAudio::flyingVehicle = entity;
    tireUpdate->skidValue = 0.0;
    tireUpdate->skidValue2 = 0.0;
  } else if (VehicleAudio::flyingVehicle == entity) {
    VehicleAudio::flyingVehicle = 0;
  }
  UpdateTireParameters_Original(tpu, tireUpdate, emitter, suspensionPressure);
}
