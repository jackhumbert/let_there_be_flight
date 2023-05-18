#include "Addresses.hpp"
#include "Utils/FlightModule.hpp"
#include "Flight/Component.hpp"
#include "FlightSettings.hpp"
#include <RED4ext/Common.hpp>
#include <RED4ext/Scripting/Natives/Generated/vehicle/TPPCameraComponent.hpp>

// ignores pitch slope adjustment
REGISTER_FLIGHT_HOOK(void __fastcall, vehicleTPPCameraComponent_UpdatePitch,
                     RED4ext::vehicle::TPPCameraComponent *camera, RED4ext::Vector4 *localPosition,
                     RED4ext::Vector3 *cameraPosition, RED4ext::vehicle::TPPCameraUpdate *update) {
  auto vehicle = camera->vehicle;
  auto fc = FlightComponent::Get(vehicle);
  if (fc && fc->active) {
    camera->cameraDirection = update->locationFromOffset;
    camera->cameraPitch = FlightSettings::GetProperty<float>("tppCameraPitchOffset");
  } else {
    vehicleTPPCameraComponent_UpdatePitch_Original(camera, localPosition, cameraPosition, update);
  }
}