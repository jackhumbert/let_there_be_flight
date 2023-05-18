#pragma once

#include <RED4ext/Common.hpp>
#include <RED4ext/Scripting/Natives/Generated/vehicle/BaseObject.hpp>
#include <RED4ext/Scripting/Natives/vehiclePhysicsData.hpp>
#include "Utils/FlightModule.hpp"
#include "Addresses.hpp"

// Vehicle Speed Unlimiter

namespace vehicle {

// 1.6  RVA: 0x1D0E180 / 30466432
// 1.61 RVA: 0x1D0E540
/// @pattern 40 53 48 81 EC 80 00 00 00 F3 0F 10 41 40 48 8B D9 F3 0F 10 51 08 0F 28 C8 F3 0F 59 09 0F 29 74
short PhysicsStructUpdate(RED4ext::vehicle::PhysicsData *ps);
constexpr const uintptr_t PhysicsStructUpdate_Addr = vehicle_PhysicsStructUpdate_Addr;

}