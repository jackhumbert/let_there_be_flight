#include <RED4ext/RED4ext.hpp>
#include <RED4ext/Scripting/Natives/Generated/vehicle/TPPCameraComponent.hpp>
#include <RED4ext/Scripting/Natives/Generated/game/FPPCameraComponent.hpp>
#include "FlightModule.hpp"

namespace vehicle {
namespace flight {
struct Camera : FlightModule {
  void Load(const RED4ext::Sdk *aSdk, RED4ext::PluginHandle aHandle);
  void Unload(const RED4ext::Sdk *aSdk, RED4ext::PluginHandle aHandle);
  static uintptr_t TPPCameraStatsUpdate(RED4ext::vehicle::TPPCameraComponent *, uintptr_t);
  static char __fastcall FPPCameraUpdate(RED4ext::game::FPPCameraComponent *a1, float xmm1_4_0, float a3,
                                                 float a4, int a5, int a6, char a7);
};
} // namespace flight
} // namespace vehicle