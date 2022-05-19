#include "FlightCamera.hpp"
#include "FlightController.hpp"

// Treat flying vehicles as being on the ground (for TPP camera)

namespace vehicle {
namespace flight {

REGISTER_FLIGHT_MODULE(Camera);

// F3 0F 10 42 04 8B 02 F3 0F 10 4A 08 F3 0F 11 81 B4 02 00 00 F3 0F 11 89 B8 02 00 00 89 81 B0 02
constexpr uintptr_t TPPCameraStatsUpdateAddr = 0x141CC7560 - RED4ext::Addresses::ImageBase;
decltype(&Camera::TPPCameraStatsUpdate) TPPCameraStatsUpdate_Original;

uintptr_t Camera::TPPCameraStatsUpdate(RED4ext::vehicle::TPPCameraComponent *camera, uintptr_t data) {
  uintptr_t result = TPPCameraStatsUpdate_Original(camera, data);
  
  if (FlightController::FlightController::GetInstance()->active) {
    camera->isInAir = false;
    camera->drivingDirectionCompensationAngleSmooth = 120.0;
    camera->drivingDirectionCompensationSpeedCoef = 0.1;
  }

  return result;
}

void Camera::Load(const RED4ext::Sdk *aSdk, RED4ext::PluginHandle aHandle) {
  aSdk->hooking->Attach(aHandle, RED4EXT_OFFSET_TO_ADDR(TPPCameraStatsUpdateAddr), &TPPCameraStatsUpdate,
                        reinterpret_cast<void **>(&TPPCameraStatsUpdate_Original));
}

void Camera::Unload(const RED4ext::Sdk *aSdk, RED4ext::PluginHandle aHandle) {
  aSdk->hooking->Detach(aHandle, RED4EXT_OFFSET_TO_ADDR(TPPCameraStatsUpdateAddr));
}
} // namespace flight
} // namespace vehicle