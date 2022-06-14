#pragma once

#include <RED4ext/Scripting/Natives/Generated/physics/VehiclePhysics.hpp>
#include <RED4ext/Scripting/Natives/Generated/vehicle/BaseObject.hpp>
#include <RED4ext/RED4ext.hpp>
#include <spdlog/spdlog.h>

RED4ext::ent::IComponent *GetFlightComponent(RED4ext::vehicle::BaseObject *v);