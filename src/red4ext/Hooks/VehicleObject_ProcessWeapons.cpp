#include "Addresses.hpp"
#include "Flight/Component.hpp"
#include "Utils/FlightModule.hpp"
#include <RED4ext/Common.hpp>
#include <RED4ext/Scripting/Natives/Generated/ent/PlaceholderComponent.hpp>
#include <RED4ext/Scripting/Natives/vehicleWeapon.hpp>


// processes weapon firing for vehicles - we can check the cycleTimer value after to see if something was fired
// 1.5 added a byte in the middle of this pattern, which makes it hard to match with ?
/// @pattern 48 8B C4 55 56 41 54 41 55 41 56 41 57 48 8D A8
// void VehicleObject_ProcessWeapons(RED4ext::vehicle::BaseObject *vehicle, float timeDelta, unsigned int shootIndex);

// REGISTER_FLIGHT_HOOK(void __fastcall, VehicleObject_ProcessWeapons, RED4ext::vehicle::BaseObject *vehicle,
//                      float timeDelta, unsigned int shootIndex) {
//   VehicleObject_ProcessWeapons_Original(vehicle, timeDelta, shootIndex);
//   if (vehicle->weapons[shootIndex].cycleTimer == 0.0) {
//     RED4ext::Quaternion quat = {0.0, 0.0, 0.0, 1.0};
//     auto ph = (RED4ext::ent::PlaceholderComponent *)vehicle->weapons[shootIndex].weaponObject.GetPtr()->placeholder;
//     if (ph) {
//       quat = ph->worldTransform.Orientation;
//     }

//     auto rtti = RED4ext::CRTTISystem::Get();
//     auto fcc = rtti->GetClass("FlightComponent");
//     auto fc = FlightComponent::Get(vehicle);
//     auto onFireWeapon = fcc->GetFunction("OnFireWeapon");
//     RED4ext::CStackType args[3];
//     args[0] = RED4ext::CStackType(rtti->GetType("Quaternion"), &quat);
//     args[1] = RED4ext::CStackType(rtti->GetType("TweakDBID"), &vehicle->weapons[shootIndex].item);
//     args[2] = RED4ext::CStackType(rtti->GetType("TweakDBID"), &vehicle->weapons[shootIndex].slot);
//     auto stack = RED4ext::CStack(fc, args, 3, nullptr, 0);
//     onFireWeapon->Execute(&stack);
//   }
// }