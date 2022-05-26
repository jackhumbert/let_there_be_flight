#include "FlightCamera.hpp"
#include "FlightController.hpp"
#include "FlightSystem.hpp"

// Treat flying vehicles as being on the ground (for TPP camera)

namespace vehicle {
namespace flight {

REGISTER_FLIGHT_MODULE(Camera);

// F3 0F 10 42 04 8B 02 F3 0F 10 4A 08 F3 0F 11 81 B4 02 00 00 F3 0F 11 89 B8 02 00 00 89 81 B0 02
constexpr uintptr_t TPPCameraStatsUpdateAddr = 0x141CC7560 - RED4ext::Addresses::ImageBase;
decltype(&Camera::TPPCameraStatsUpdate) TPPCameraStatsUpdate_Original;

float defaultSlopeCorrectionOnGroundStrength = 0.0;

uintptr_t Camera::TPPCameraStatsUpdate(RED4ext::vehicle::TPPCameraComponent *camera, uintptr_t data) {
  uintptr_t result = TPPCameraStatsUpdate_Original(camera, data);
  
  auto fc = FlightController::FlightController::GetInstance();
  bool resetSlope = false;
  if (fc->active) {
    camera->isInAir = false;
    camera->drivingDirectionCompensationAngleSmooth = 120.0;
    camera->drivingDirectionCompensationSpeedCoef = 0.1;

    auto rtti = RED4ext::CRTTISystem::Get();
    auto fcc = rtti->GetClass("FlightSystem");
    auto fs = FlightSystem::FlightSystem::GetInstance();
    auto pcp = fcc->GetProperty("playerComponent");
    auto fcomp = pcp->GetValue<RED4ext::Handle<RED4ext::IScriptable>>(fs);
    auto fcompc = rtti->GetClass("FlightComponent");
    RED4ext::Handle<RED4ext::IScriptable> mode;
    auto result = RED4ext::CStackType(rtti->GetClass("FlightMode"), &mode);
    auto stack = RED4ext::CStack(fcomp, nullptr, 0, &result, 0);
    fcompc->GetFunction("GetFlightMode")->Execute(&stack);

    if (mode->GetType() == rtti->GetClass("FlightModeDrone")) {
      camera->pitchDelta = 0.0;
      camera->yawDelta = 0.0;
      if (camera->slopeCorrectionOnGroundStrength != 0.0) {
        defaultSlopeCorrectionOnGroundStrength = camera->slopeCorrectionOnGroundStrength;
      }
      camera->slopeCorrectionOnGroundStrength = 0.0;
    } else {
      resetSlope = true;
    }
  } else {
    resetSlope = true;
  }

  if (resetSlope && camera->slopeCorrectionOnGroundStrength == 0.0 && defaultSlopeCorrectionOnGroundStrength != 0.0) {
    camera->slopeCorrectionOnGroundStrength = defaultSlopeCorrectionOnGroundStrength;
    defaultSlopeCorrectionOnGroundStrength = 0.0;
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