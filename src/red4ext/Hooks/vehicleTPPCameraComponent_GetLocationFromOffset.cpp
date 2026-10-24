
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
#ifdef __APPLE__
#include <chrono>
#include "MacLayout.hpp"
// LTBF R41: QueryPerformanceCounter's 10 MHz ticks, from the steady clock (the fade below is in those ticks).
struct LARGE_INTEGER_MAC {
  int64_t QuadPart = 0;
};
static LARGE_INTEGER_MAC start;
static LARGE_INTEGER_MAC now;
static void QueryPerformanceCounter(LARGE_INTEGER_MAC *aOut) {
  aOut->QuadPart = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count() / 100;
}
#else
static LARGE_INTEGER start;
static LARGE_INTEGER now;
#endif

// 500ms
#define FADE_DURATION (5000000 / 2)

// gets the location for the camera to look at
/// @hash 283779224
// Vector4 *__fastcall GetLocationFromOffset(vehicle::TPPCameraComponent *camera, Vector4 *location, Vector3 *lookAtOffset);

// uses preset->linearVelocity
// Vector4 vehicle::TPPCameraComponent::CalculateLookAtPosition(vehicle::TPPCameraComponent::CameraPreset const &) const
#ifdef __APPLE__
// LTBF R41: on macOS the Vector4 comes back through x8 (the indirect result register), with the camera in x0 and the preset in x1;
// Windows passes the result buffer as the second argument and returns it. A Vector4 of four floats would come back in s0..s3, so the
// hook returns a type with a user-provided destructor, which clang returns through x8 (R40, "GetLocationFromOffset").
struct MacVector4Result {
  float X, Y, Z, W;
  MacVector4Result() = default;
  MacVector4Result(const Vector4 &aValue) : X(aValue.X), Y(aValue.Y), Z(aValue.Z), W(aValue.W) {}
  MacVector4Result(const MacVector4Result &aOther) = default;
  ~MacVector4Result() {}
  Vector4 AsVector4() const { return {X, Y, Z, W}; }
};
static_assert(sizeof(MacVector4Result) == 16, "Vector4 result");

REGISTER_FLIGHT_HOOK_HASH(MacVector4Result, 283779224, GetLocationFromOffset,
                     vehicle::TPPCameraComponent *camera, void *preset) {
  Vector4 location;
  Vector4 *offset = &location;
  auto vehicle = LtbfMac::Layout::At<vehicle::BaseObject *>(camera, LtbfMac::Layout::TPPCamera_vehicle);
  auto fc = FlightComponent::Get(vehicle);
  if (fc && fc->active) {
    LtbfMac::Layout::At<LtbfMac::Layout::TppCameraPreset>(camera, LtbfMac::Layout::TPPCamera_data).isInAir = false;
    if (FlightSettings::GetProperty<bool>("tppCameraCenterOnMass") && !FlightSettings::GetBool("inTPPDriverCombat")) {
      has_changed_mode = !custom_mode;
      custom_mode = true;
      *offset = vehicle->worldTransform.Position.AsVector4() +
             (vehicle->worldTransform.Orientation * vehicle->physicsData->centerOfMass);
    } else {
      has_changed_mode = custom_mode;
      custom_mode = false;
      *offset = GetLocationFromOffset_Original(camera, preset).AsVector4();
    }
  } else {
    has_changed_mode = custom_mode;
    custom_mode = false;
    *offset = GetLocationFromOffset_Original(camera, preset).AsVector4();
  }
#else
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
#endif

  
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

#ifdef __APPLE__
  return MacVector4Result(*offset);
#else
  return offset;
#endif
}