#include <RED4ext/RED4ext.hpp>
#include <RED4ext/Scripting/Natives/Generated/vehicle/TPPCameraComponent.hpp>
#include <RED4ext/Scripting/Natives/Generated/game/FPPCameraComponent.hpp>
#include <RED4ext/Scripting/Natives/Generated/anim/AnimFeature_FPPCamera.hpp>
#include "FlightModule.hpp"

namespace vehicle {
namespace flight {
struct Camera : FlightModule {
  void Load(const RED4ext::Sdk *aSdk, RED4ext::PluginHandle aHandle);
  void Unload(const RED4ext::Sdk *aSdk, RED4ext::PluginHandle aHandle);
  static uintptr_t TPPCameraStatsUpdate(RED4ext::vehicle::TPPCameraComponent *, uintptr_t);
  static char __fastcall FPPCameraUpdate(RED4ext::game::FPPCameraComponent *a1, float deltaTime, float deltaYaw,
                                                 float deltaPitch, float deltaYawExternal, float deltaPitchExternal, char a7);
};
} // namespace flight
} // namespace vehicle