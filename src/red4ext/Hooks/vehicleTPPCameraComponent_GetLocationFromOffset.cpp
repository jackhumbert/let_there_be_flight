#include "Addresses.hpp"
#include "Utils/FlightModule.hpp"
#include "Flight/Component.hpp"
#include "FlightSettings.hpp"
#include <RED4ext/Common.hpp>
#include <RED4ext/Scripting/Natives/Generated/vehicle/TPPCameraComponent.hpp>

 // gets the location for the camera to look at
REGISTER_FLIGHT_HOOK(RED4ext::Vector4 *__fastcall, vehicleTPPCameraComponent_GetLocationFromOffset,
                     RED4ext::vehicle::TPPCameraComponent *camera, RED4ext::Vector4 *location,
                     RED4ext::Vector3 *offset) {
  auto v = new RED4ext::Vector4();
  auto vehicle = camera->vehicle;
  auto fc = FlightComponent::Get(vehicle);
  if (fc && fc->active) {
    // ignore pitch adjustments
    // camera->pitch = camera->cameraPitch;
    camera->data.isInAir = false;
    if (FlightSettings::GetProperty<bool>("tppCameraCenterOnMass")) {
      *v = *vehicle->worldTransform.Position.ToVector4() +
             (vehicle->worldTransform.Orientation * vehicle->physicsData->centerOfMass);
    } else {
      *v = *vehicleTPPCameraComponent_GetLocationFromOffset_Original(camera, location, offset);
    }
    return v;
  } else {
    return vehicleTPPCameraComponent_GetLocationFromOffset_Original(camera, location, offset);
  }
}