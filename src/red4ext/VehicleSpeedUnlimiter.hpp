#pragma once

#include <RED4ext/RED4ext.hpp>
#include <RED4ext/Scripting/Natives/Generated/vehicle/BaseObject.hpp>
#include "FlightModule.hpp"

namespace vehicle {
struct SpeedUnlimiter : FlightModule {
  void Load(const RED4ext::Sdk *aSdk, RED4ext::PluginHandle aHandle);
  void Unload(const RED4ext::Sdk *aSdk, RED4ext::PluginHandle aHandle);
  static short PhysicsStructUpdate(RED4ext::physics::VehiclePhysicsStruct *ps);
};
} // namespace vehicle
