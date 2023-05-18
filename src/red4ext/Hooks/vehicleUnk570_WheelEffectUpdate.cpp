#include "Addresses.hpp"
#include "Flight/Component.hpp"
#include "Utils/FlightModule.hpp"
#include <RED4ext/Common.hpp>
#include <RED4ext/Scripting/Natives/Generated/vehicle/BaseObject.hpp>
#include <RED4ext/Scripting/Natives/Generated/vehicle/CarBaseObject.hpp>
#include <RED4ext/Scripting/Natives/vehiclePhysics.hpp>


// make tire always skidding
REGISTER_FLIGHT_HOOK(void __fastcall, vehicleUnk570_WheelEffectUpdate, RED4ext::vehicle::Unk570 *unk570, unsigned int wheelIndex,
                     RED4ext::vehicle::Unk570::Unk40 *unk40, float deltaTime) {
  auto fc = FlightComponent::Get(unk570->vehicle);
  if (fc && fc->active) {
    unk40->wheelLongSlip = 2.0;
  }
  vehicleUnk570_WheelEffectUpdate_Original(unk570, wheelIndex, unk40, deltaTime);
}