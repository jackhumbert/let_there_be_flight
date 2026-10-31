
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
static bool fading = false;
// look-at height above the vanilla look-at point, last frame
static float last_shift = 0.0;

// offsets in game 2.5 (vehicle::TPPCameraComponent::UpdateCameraState)
static_assert(offsetof(vehicle::TPPCameraComponent, pitch) == 0x400);
static_assert(offsetof(vehicle::TPPCameraComponent, boomLength) == 0x4D4);

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
  Vector4 vanilla;
  if (fc && fc->active) {
    // ignore pitch adjustments
    // camera->pitch = camera->cameraPitch;
    camera->data.isInAir = false;
    if (FlightSettings::GetProperty<bool>("tppCameraCenterOnMass") && !FlightSettings::GetBool("inTPPDriverCombat")) {
      has_changed_mode = !custom_mode;
      custom_mode = true;
      vanilla = *GetLocationFromOffset_Original(camera, &vanilla, preset);
      *offset = vehicle->worldTransform.Position.AsVector4() +
             (vehicle->worldTransform.Orientation * vehicle->physicsData->centerOfMass);
    } else {
      has_changed_mode = custom_mode;
      custom_mode = false;
      *offset = *GetLocationFromOffset_Original(camera, offset, preset);
      vanilla = *offset;
    }
  } else {
    has_changed_mode = custom_mode;
    custom_mode = false;
    *offset = *GetLocationFromOffset_Original(camera, offset, preset);
    vanilla = *offset;
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

  // The camera sits at the look-at point plus the boom pitched up by `pitch`. While the look-at point
  // moves between the vanilla one and the centre of mass, move the pitch with it so the camera keeps
  // its height: vanilla re-centres the pitch at a rate scaled by speed, so a parked car would keep the
  // dropped camera until it drove off.
  float shift = offset->Z - vanilla.Z;
  bool was_fading = fading;
  fading = diff < FADE_DURATION;
  if ((fading || was_fading) && camera->boomLength > 0.1f) {
    constexpr float kDegToRad = 0.017453292f;
    float height = sinf(camera->pitch * kDegToRad) - (shift - last_shift) / camera->boomLength;
    camera->pitch = asinf(fminf(fmaxf(height, -0.99f), 0.99f)) / kDegToRad;
  }
  last_shift = shift;

  last = *offset;

  return offset;
}