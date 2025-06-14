#include "Addresses.hpp"
#include "Utils/FlightModule.hpp"
#include "Flight/Component.hpp"
#include "FlightSettings.hpp"
#include <RED4ext/Common.hpp>
#include <RED4ext/Scripting/Natives/Generated/vehicle/TPPCameraComponent.hpp>
#include <RED4ext/Scripting/Natives/vehiclePhysics.hpp>

static bool custom_mode = false;
static bool has_changed_mode = false;
static RED4ext::Vector4 og;
static RED4ext::Vector4 last;
static LARGE_INTEGER start;
static LARGE_INTEGER now;

// 500ms
#define FADE_DURATION (5000000 / 2)

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
      has_changed_mode = !custom_mode;
      custom_mode = true;
      *v = vehicle->worldTransform.Position.AsVector4() +
             (vehicle->worldTransform.Orientation * vehicle->physicsData->centerOfMass);
    } else {
      has_changed_mode = custom_mode;
      custom_mode = false;
      *v = *GetLocationFromOffset_Original(camera, offset, preset);
    }
  } else {
    has_changed_mode = custom_mode;
    custom_mode = false;
    *v = *GetLocationFromOffset_Original(camera, offset, preset);
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
    v->X = (v->X * (1.0 - remaining)) + (og.X * remaining);
    v->Y = (v->Y * (1.0 - remaining)) + (og.Y * remaining);
    v->Z = (v->Z * (1.0 - remaining)) + (og.Z * remaining);
  }

  last = *v;

  return v;
}