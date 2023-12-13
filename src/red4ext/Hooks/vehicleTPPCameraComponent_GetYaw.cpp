#include "Addresses.hpp"
#include "Utils/FlightModule.hpp"
#include "Flight/Component.hpp"
#include <RED4ext/Common.hpp>
#include <RED4ext/Scripting/Natives/Generated/vehicle/TPPCameraComponent.hpp>

// uses driving direction variables, a3 is unkTransform340 & update->locationFromOffset, and X from unkWorldPosition470

// pre 2.0
// 1.6 RVA: 0x1CF57B0 / 30365616
/// @pattern 48 8B C4 F3 0F 11 58 20 53 56 57 48 81 EC 10 01 00 00 F3 0F 10 81 D8 04 00 00 49 8B F8 F3 0F 10

// post 2.0
/// @pattern 48 8B C4 48 89 58 08 48 89 70 10 48 89 78 18 55 48 8D 68 A9 48 81 EC 00 01 00 00 F3 0F 10 81 70
void __fastcall GetYaw(RED4ext::vehicle::TPPCameraComponent *camera, float *yaw, RED4ext::Vector4 *a3, float isInAir);

// ignores driving direction correction
REGISTER_FLIGHT_HOOK(void __fastcall, GetYaw, RED4ext::vehicle::TPPCameraComponent *camera,
                     float *yaw, RED4ext::Vector4 *position, float isInAir) {
  auto vehicle = camera->vehicle;
  auto fc = FlightComponent::Get(vehicle);
  if (fc && fc->active) {
    *yaw = 0.0;
  } else {
    GetYaw_Original(camera, yaw, position, isInAir);
  }
}
