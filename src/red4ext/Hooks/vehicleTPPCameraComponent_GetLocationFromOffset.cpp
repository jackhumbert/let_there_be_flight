#include "Addresses.hpp"
#include "Utils/FlightModule.hpp"
#include "Flight/Component.hpp"
#include "FlightSettings.hpp"
#include <RED4ext/Common.hpp>
#include <RED4ext/Scripting/Natives/Generated/vehicle/TPPCameraComponent.hpp>

// gets the location for the camera to look at

// pre 2.0
/// @pattern 48 8B C4 48 89 58 18 48 89 78 20 55 48 8D 68 D8 48 81 EC 20 01 00 00 0F 28 05 ? ? ? 01 0F 57

// post 2.0
/// @pattern 48 8B C4 48 89 58 08 48 89 78 10 55 48 8D A8 58 FF FF FF 48 81 EC A0 01 00 00 0F 28 05 EB 6A B9
RED4ext::Vector4 *__fastcall GetLocationFromOffset(RED4ext::vehicle::TPPCameraComponent *camera, RED4ext::Vector4 *location, RED4ext::Vector3 *lookAtOffset);

REGISTER_FLIGHT_HOOK(RED4ext::Vector4 *__fastcall, GetLocationFromOffset,
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
      *v = *GetLocationFromOffset_Original(camera, location, offset);
    }
    return v;
  } else {
    return GetLocationFromOffset_Original(camera, location, offset);
  }
}