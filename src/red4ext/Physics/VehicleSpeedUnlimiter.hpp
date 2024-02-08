#pragma once

#include <RED4ext/Common.hpp>
#include <RED4ext/Scripting/Natives/Generated/vehicle/BaseObject.hpp>
#include <RED4ext/Scripting/Natives/vehiclePhysicsData.hpp>
#include "Utils/FlightModule.hpp"
#include "Addresses.hpp"

// Vehicle Speed Unlimiter

namespace vehicle {

/// @hash 712710137
short PhysicsStructUpdate(RED4ext::vehicle::PhysicsData *ps);
constexpr const uintptr_t PhysicsStructUpdate_Addr = vehicle_PhysicsStructUpdate_Addr;

}