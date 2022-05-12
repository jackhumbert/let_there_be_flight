#include <RED4ext/RED4ext.hpp>
#include <RED4ext/Scripting/Natives/Generated/vehicle/TPPCameraComponent.hpp>
#include "FlightModule.hpp"

namespace vehicle {
namespace flight {
struct Camera : FlightModule {
  void Load(const RED4ext::Sdk *aSdk, RED4ext::PluginHandle aHandle);
  void Unload(const RED4ext::Sdk *aSdk, RED4ext::PluginHandle aHandle);
  static uintptr_t TPPCameraStatsUpdate(RED4ext::vehicle::TPPCameraComponent *, uintptr_t);
};
} // namespace flight
} // namespace vehicle