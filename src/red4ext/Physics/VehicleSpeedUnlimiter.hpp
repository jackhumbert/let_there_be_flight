#pragma once

#include <RED4ext/Common.hpp>
#include <RED4ext/Scripting/Natives/Generated/vehicle/BaseObject.hpp>
#include <RED4ext/Scripting/Natives/vehiclePhysicsData.hpp>
#include "Utils/FlightModule.hpp"


// Vehicle Speed Unlimiter

namespace vehicle {

// void vehicle::RigidBody::Recalculate(void)
/// @hash 593628708
short RigidBody_Recalculate(RED4ext::vehicle::PhysicsData *ps);
// constexpr const uintptr_t RigidBody_Recalculate_Addr = vehicle_RigidBody_Recalculate_Addr;

}