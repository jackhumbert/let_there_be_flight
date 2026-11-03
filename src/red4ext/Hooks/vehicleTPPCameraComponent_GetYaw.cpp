
#include "Utils/FlightModule.hpp"
#include "Flight/Component.hpp"
#include <RED4ext/Common.hpp>
#include <RED4ext/Scripting/Natives/Generated/vehicle/TPPCameraComponent.hpp>
#ifdef __APPLE__
#include "MacLayout.hpp"
#endif

// uses driving direction variables, a3 is unkTransform340 & update->locationFromOffset, and X from unkWorldPosition470
/// @hash 4124321726
// void __fastcall GetYaw(RED4ext::vehicle::TPPCameraComponent *camera, float *yaw, RED4ext::Vector4 *a3, float isInAir);

// called from vehicle::TPPCameraComponent::UpdateCameraState(vehicle::TPPCameraComponent::UpdateContext const &)
// ignores driving direction correction
REGISTER_FLIGHT_HOOK_HASH(void __fastcall, 4124321726, GetYaw, RED4ext::vehicle::TPPCameraComponent *camera,
                     float *yaw, RED4ext::Vector4 *position, float isInAir) {
#ifdef __APPLE__
  // LTBF R41: the vehicle Handle's instance is at 0x318 on macOS (R41b; Windows 0x320)
  auto vehicle = LtbfMac::Layout::At<RED4ext::vehicle::BaseObject *>(camera, LtbfMac::Layout::TPPCamera_vehicle);
#else
  auto vehicle = camera->vehicle;
#endif
  auto fc = FlightComponent::Get(vehicle);
  if (fc && fc->active) {
    *yaw = 0.0;
  } else {
    GetYaw_Original(camera, yaw, position, isInAir);
  }
}
