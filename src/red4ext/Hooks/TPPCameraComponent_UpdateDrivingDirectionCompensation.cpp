
#include "Utils/FlightModule.hpp"
#include "Flight/Component.hpp"
#include <RED4ext/Common.hpp>
#include <RED4ext/Scripting/Natives/Generated/vehicle/TPPCameraComponent.hpp>

// uses driving direction variables, a3 is unkTransform340 & update->locationFromOffset, and X from unkWorldPosition470
/// @hash 4124321726
// void __fastcall TPPCameraComponent_UpdateDrivingDirectionCompensation(RED4ext::vehicle::TPPCameraComponent *camera, float *yaw, RED4ext::Vector4 *a3, float isInAir);

// called from vehicle::TPPCameraComponent::UpdateCameraState(vehicle::TPPCameraComponent::UpdateContext const &)
// ignores driving direction correction
// void vehicle::TPPCameraComponent::UpdateDrivingDirectionCompensation(float &, Vector4 const &, float, vehicle::TPPCameraComponent::UpdateContext const &)
REGISTER_FLIGHT_HOOK_HASH(void __fastcall, 4124321726, TPPCameraComponent_UpdateDrivingDirectionCompensation, RED4ext::vehicle::TPPCameraComponent *camera,
                     float *yaw, RED4ext::Vector4 *position, float isInAir) {
  auto vehicle = camera->vehicle;
  auto fc = FlightComponent::Get(vehicle);
  if (fc && fc->active) {
    *yaw = 0.0;
  } else {
    TPPCameraComponent_UpdateDrivingDirectionCompensation_Original(camera, yaw, position, isInAir);
  }
}
