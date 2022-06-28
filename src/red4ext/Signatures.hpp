#pragma once
#include <RED4ext/RED4ext.hpp>
#include <RED4ext/Scripting/Natives/Generated/ent/Entity.hpp>
#include <RED4ext/Scripting/Natives/Generated/vehicle/BaseObject.hpp>
#include "Addresses.hpp"

// right before components are processed for entites, and an appropriate time to insert our own
// can also look for string "Entity/InitializeComponents"
/// @pattern 48 89 54 24 10 55 53 56 57 41 54 41 55 41 56 41 57 48 8D AC 24 ? FB FF FF 48 81 EC ? 05 00 00
/// @nth 0/2
using Entity_InitializeComponents = void (RED4ext::ent::Entity * entity, void * a2, void * a3);

// processes weapon firing for vehicles - we can check the cycleTimer value after to see if something was fired
// 1.5 added a byte in the middle of this pattern, which makes it hard to match with ?
/// @pattern 48 8B C4 55 56 41 54 41 55 41 56 41 57 48 8D A8
using VehicleProcessWeapons = void (RED4ext::vehicle::BaseObject *vehicle, float timeDelta, unsigned int shootIndex);