#include "Addresses.hpp"
#include "Utils/FlightModule.hpp"
#include "Flight/Component.hpp"
#include "FlightSettings.hpp"
#include <RED4ext/Common.hpp>
#include <RED4ext/Scripting/Natives/Generated/vehicle/TPPCameraComponent.hpp>

// ignores pitch slope adjustment
// updates cameraDirection
// updates cameraPitch from slopeAdjustment
/// @hash 2719293980
// void __fastcall UpdatePitch(RED4ext::vehicle::TPPCameraComponent *camera, RED4ext::Vector4 *localPosition, RED4ext::Vector3 *cameraPosition, RED4ext::vehicle::TPPCameraUpdate *update);

// void vehicle::TPPCameraComponent::UpdateSlopeCompensation(Vector4 const &, Vector4 const &, vehicle::TPPCameraComponent::UpdateContext const &)
REGISTER_FLIGHT_HOOK_HASH(void __fastcall, 2719293980, UpdatePitch,
                     RED4ext::vehicle::TPPCameraComponent *camera, RED4ext::Vector4 *localPosition,
                     RED4ext::Vector3 *cameraPosition, RED4ext::vehicle::TPPCameraUpdate *update) {
  auto vehicle = camera->vehicle;
  auto fc = FlightComponent::Get(vehicle);
  if (fc && fc->active) {
    camera->cameraDirection = update->locationFromOffset.AsVector3();
    camera->cameraPitch = FlightSettings::GetProperty<float>("tppCameraPitchOffset");
  } else {
    UpdatePitch_Original(camera, localPosition, cameraPosition, update);
  }
}