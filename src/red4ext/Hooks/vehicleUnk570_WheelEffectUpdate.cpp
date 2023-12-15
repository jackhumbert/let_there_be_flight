#include "Addresses.hpp"
#include "Flight/Component.hpp"
#include "Utils/FlightModule.hpp"
#include <RED4ext/Common.hpp>
#include <RED4ext/Scripting/Natives/Generated/vehicle/BaseObject.hpp>
#include <RED4ext/Scripting/Natives/Generated/vehicle/CarBaseObject.hpp>
#include <RED4ext/Scripting/Natives/vehiclePhysics.hpp>

// pre 2.0
// 1.6  RVA: 0x1D2A340 / 30581568
/// @pattern 48 8B C4 89 50 10 55 48 8D A8 78 FE FF FF 48 81 EC 80 02 00 00 41 80 78 7C 00 48 89 58 20 48 89

// post 2.0
/// @pattern 48 8B C4 48 89 58 20 55 56 57 41 54 41 55 41 56 41 57 48 8D A8 28 FF FF FF 48 81 EC A0 01 00 00
/// @nth 1/2
void __fastcall WheelEffectUpdate(RED4ext::vehicle::Unk570 *unk570, unsigned int wheelIndex, RED4ext::vehicle::Unk570::Unk40 *unk40, float deltaTime);

// make tire always skidding
REGISTER_FLIGHT_HOOK(void __fastcall, WheelEffectUpdate, RED4ext::vehicle::Unk570 *unk570, unsigned int wheelIndex,
                     RED4ext::vehicle::Unk570::Unk40 *unk40, float deltaTime) {
  auto fc = FlightComponent::Get(unk570->vehicle);
  if (fc && fc->active) {
    unk40->wheelLongSlip = 2.0;
  }
  WheelEffectUpdate_Original(unk570, wheelIndex, unk40, deltaTime);
}