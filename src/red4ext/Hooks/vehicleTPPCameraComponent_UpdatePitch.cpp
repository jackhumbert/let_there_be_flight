
#include "Utils/FlightModule.hpp"
#include "Flight/Component.hpp"
#include "FlightSettings.hpp"
#include <RED4ext/Common.hpp>
#include <RED4ext/Scripting/Natives/Generated/vehicle/TPPCameraComponent.hpp>
#ifdef __APPLE__
#include "MacLayout.hpp"
#endif

// ignores pitch slope adjustment
// updates cameraDirection
// updates cameraPitch from slopeAdjustment
/// @hash 2719293980
// void __fastcall UpdatePitch(RED4ext::vehicle::TPPCameraComponent *camera, RED4ext::Vector4 *localPosition, RED4ext::Vector3 *cameraPosition, RED4ext::vehicle::TPPCameraUpdate *update);

// void vehicle::TPPCameraComponent::UpdateSlopeCompensation(Vector4 const &, Vector4 const &, vehicle::TPPCameraComponent::UpdateContext const &)
#ifdef __APPLE__
// LTBF R41: macOS offsets (R41b): vehicle Handle instance 0x318, cameraPitch 0x4D8, cameraDirection 0x4DC; the update context is the
// fork's TPPCameraUpdate (LtbfMac::Layout::TppCameraUpdate). Arguments x0..x3 as on Windows rcx, rdx, r8, r9.
REGISTER_FLIGHT_HOOK_HASH(void, 2719293980, UpdatePitch,
                     RED4ext::vehicle::TPPCameraComponent *camera, RED4ext::Vector4 *localPosition,
                     RED4ext::Vector3 *cameraPosition, LtbfMac::Layout::TppCameraUpdate *update) {
  auto vehicle = LtbfMac::Layout::At<RED4ext::vehicle::BaseObject *>(camera, LtbfMac::Layout::TPPCamera_vehicle);
  auto fc = FlightComponent::Get(vehicle);
  if (fc && fc->active) {
    LtbfMac::Layout::At<RED4ext::Vector3>(camera, LtbfMac::Layout::TPPCamera_cameraDirection) = update->locationFromOffset.AsVector3();
    LtbfMac::Layout::At<float>(camera, LtbfMac::Layout::TPPCamera_cameraPitch) =
        FlightSettings::GetProperty<float>("tppCameraPitchOffset");
  } else {
    UpdatePitch_Original(camera, localPosition, cameraPosition, update);
  }
}
#else
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
#endif