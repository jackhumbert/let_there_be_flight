
#include "Utils/FlightModule.hpp"
#include "Flight/Component.hpp"
#include "FlightSettings.hpp"
#include <RED4ext/Common.hpp>
#include <RED4ext/Scripting/Natives/Generated/vehicle/TPPCameraComponent.hpp>
#include <RED4ext/Scripting/Natives/vehiclePhysics.hpp>

using namespace RED4ext;

static bool custom_mode = false;
static bool has_changed_mode = false;
static Vector4 og;
static Vector4 last;
static LARGE_INTEGER start;
static LARGE_INTEGER now;

// 500ms
#define FADE_DURATION (5000000 / 2)

// gets the location for the camera to look at
/// @hash 283779224
// Vector4 *__fastcall GetLocationFromOffset(vehicle::TPPCameraComponent *camera, Vector4 *location, Vector3 *lookAtOffset);

// uses preset->linearVelocity
// Vector4 vehicle::TPPCameraComponent::CalculateLookAtPosition(vehicle::TPPCameraComponent::CameraPreset const &) const
REGISTER_FLIGHT_HOOK_HASH(Vector4 *__fastcall, 283779224, GetLocationFromOffset,
                     vehicle::TPPCameraComponent *camera, Vector4 *offset,
                     vehicle::TPPCameraComponent::CameraPreset *preset) {
  auto vehicle = camera->vehicle;
  auto fc = FlightComponent::Get(vehicle);
  if (fc && fc->active) {
    // ignore pitch adjustments
    // camera->pitch = camera->cameraPitch;
    camera->data.isInAir = false;
    if (FlightSettings::GetProperty<bool>("tppCameraCenterOnMass") && !FlightSettings::GetBool("inTPPDriverCombat")) {
      has_changed_mode = !custom_mode;
      custom_mode = true;
      *offset = vehicle->worldTransform.Position.AsVector4() +
             (vehicle->worldTransform.Orientation * vehicle->physicsData->centerOfMass);
    } else {
      has_changed_mode = custom_mode;
      custom_mode = false;
      *offset = *GetLocationFromOffset_Original(camera, offset, preset);
    }
  } else {
    has_changed_mode = custom_mode;
    custom_mode = false;
    *offset = *GetLocationFromOffset_Original(camera, offset, preset);
  }

  
  if (has_changed_mode) {
    QueryPerformanceCounter(&start);
    og = last;
  }

  QueryPerformanceCounter(&now);

  auto diff = now.QuadPart - start.QuadPart;

  if (diff < FADE_DURATION) {
    auto ratio = (double)diff / FADE_DURATION;
    // basic easing
    auto remaining = pow(1.0 - ratio, 2);
    offset->X = (offset->X * (1.0 - remaining)) + (og.X * remaining);
    offset->Y = (offset->Y * (1.0 - remaining)) + (og.Y * remaining);
    offset->Z = (offset->Z * (1.0 - remaining)) + (og.Z * remaining);
  }

  last = *offset;

  return offset;
}