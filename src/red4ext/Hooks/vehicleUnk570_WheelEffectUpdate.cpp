#include "Addresses.hpp"
#include "Flight/Component.hpp"
#include "Utils/FlightModule.hpp"
#include <RED4ext/Common.hpp>
#include <RED4ext/Scripting/Natives/Generated/vehicle/BaseObject.hpp>
#include <RED4ext/Scripting/Natives/Generated/vehicle/CarBaseObject.hpp>
#include <RED4ext/Scripting/Natives/vehiclePhysics.hpp>

/// @hash 2932936864
// void __fastcall WheelEffectUpdate(RED4ext::vehicle::Effects *effects, unsigned int wheelIndex, RED4ext::vehicle::Effects::Unk40 *unk40, float deltaTime);

// make tire always skidding
REGISTER_FLIGHT_HOOK_HASH(void __fastcall, 2932936864, WheelEffectUpdate, RED4ext::vehicle::Effects *effects, unsigned int wheelIndex,
                     RED4ext::vehicle::Effects::Unk40 *unk40, float deltaTime) {
  auto fc = FlightComponent::Get(effects->vehicle);
  if (fc && fc->active) {
    unk40->wheelLongSlip = 2.0;
  }
  WheelEffectUpdate_Original(effects, wheelIndex, unk40, deltaTime);
}