#include "Addresses.hpp"
#include "Utils/FlightModule.hpp"
#include "Flight/Component.hpp"
#include "FlightSettings.hpp"
#include <RED4ext/Common.hpp>
#include <RED4ext/Scripting/Natives/Generated/vehicle/TPPCameraComponent.hpp>
#include <RED4ext/Scripting/Natives/vehiclePhysics.hpp>

// gets the location for the camera to look at
/// @hash 283779224
// RED4ext::Vector4 *__fastcall GetLocationFromOffset(RED4ext::vehicle::TPPCameraComponent *camera, RED4ext::Vector4 *location, RED4ext::Vector3 *lookAtOffset);

// uses preset->linearVelocity
// Vector4 vehicle::TPPCameraComponent::CalculateLookAtPosition(vehicle::TPPCameraComponent::CameraPreset const &) const
REGISTER_FLIGHT_HOOK_HASH(RED4ext::Vector4 *__fastcall, 283779224, GetLocationFromOffset,
                     RED4ext::vehicle::TPPCameraComponent *camera, RED4ext::Vector4 *offset,
                     RED4ext::vehicle::TPPCameraComponent::CameraPreset *preset) {
  auto v = new RED4ext::Vector4();
  auto vehicle = camera->vehicle;
  auto fc = FlightComponent::Get(vehicle);
  if (fc && fc->active) {
    // ignore pitch adjustments
    // camera->pitch = camera->cameraPitch;
    camera->data.isInAir = false;
    if (FlightSettings::GetProperty<bool>("tppCameraCenterOnMass") && !FlightSettings::GetBool("inTPPDriverCombat")) {
      *v = vehicle->worldTransform.Position.AsVector4() +
             (vehicle->worldTransform.Orientation * vehicle->physicsData->centerOfMass);
    } else {
      *v = *GetLocationFromOffset_Original(camera, offset, preset);
    }
    return v;
  } else {
    return GetLocationFromOffset_Original(camera, offset, preset);
  }
}