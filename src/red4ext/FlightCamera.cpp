#include "FlightCamera.hpp"
#include "FlightController.hpp"
#include "FlightSystem.hpp"
#include <spdlog/spdlog.h>

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
    auto flightModeCls = rtti->GetClass("FlightMode");
    auto result = RED4ext::CStackType(flightModeCls, &mode);
    auto stack = RED4ext::CStack(fcomp, nullptr, 0, &result, 0);
    fcompc->GetFunction("GetFlightMode")->Execute(&stack);
    auto usesRightStickInput = flightModeCls->GetProperty("usesRightStickInput")->GetValue<bool>(mode);

    if (usesRightStickInput && !camera->isUsingMouse) {
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

//48 8B C4 F3 0F 11 48 10  53 48 81 EC 30 01 00 00 80 B9 A0 04 00 00 00 48 8B D9 0F 29 70 D8 0F 28
constexpr uintptr_t FPPCameraUpdateAddr = 0x16FC2A0 + 0x140000C00 - RED4ext::Addresses::ImageBase;
decltype(&Camera::FPPCameraUpdate) FPPCameraUpdate_Original;
char __fastcall Camera::FPPCameraUpdate(RED4ext::game::FPPCameraComponent *fpp, float a2, float a3, float a4,
                                        int a5,
                                 int a6, char a7) {
  // a1 is a shifted pointer - this gets the whole struct
  //auto fpp = reinterpret_cast<RED4ext::game::FPPCameraComponent *>(a1 - 0x120);

  auto fc = FlightController::FlightController::GetInstance();
  bool usesRightStickInput = false;
  if (fc->active) {
    auto rtti = RED4ext::CRTTISystem::Get();
    auto fcc = rtti->GetClass("FlightSystem");
    auto fs = FlightSystem::FlightSystem::GetInstance();
    auto pcp = fcc->GetProperty("playerComponent");
    auto fcomp = pcp->GetValue<RED4ext::Handle<RED4ext::IScriptable>>(fs);
    auto fcompc = rtti->GetClass("FlightComponent");
    RED4ext::Handle<RED4ext::IScriptable> mode;
    auto flightModeCls = rtti->GetClass("FlightMode");
    auto result = RED4ext::CStackType(flightModeCls, &mode);
    auto stack = RED4ext::CStack(fcomp, nullptr, 0, &result, 0);
    fcompc->GetFunction("GetFlightMode")->Execute(&stack);
    usesRightStickInput = flightModeCls->GetProperty("usesRightStickInput")->GetValue<bool>(mode);
  }
  // once we find it
  //if (usesRightStickInput && !fpp->isUsingMouse) {
  if (usesRightStickInput) {
    //fpp->pitchInput = 0.0;
    //fpp->yawInput = 0.0;
    fpp->yawOffset = 0.0;
    //fpp->headingLocked = true;
    //fpp->sensitivityMultX = 0.0;
    //fpp->sensitivityMultY = 0.0;
    a3 = 0.0;
    a4 = 0.0;
  } else {
    //fpp->headingLocked = false;
    //fpp->sensitivityMultX = 1.0;
    //fpp->sensitivityMultY = 1.0;
  }

  return FPPCameraUpdate_Original(fpp, a2, a3, a4, a5, a6, a7);
}

void Camera::Load(const RED4ext::Sdk *aSdk, RED4ext::PluginHandle aHandle) {
  if (!aSdk->hooking->Attach(aHandle, RED4EXT_OFFSET_TO_ADDR(TPPCameraStatsUpdateAddr), &TPPCameraStatsUpdate,
                             reinterpret_cast<void **>(&TPPCameraStatsUpdate_Original))) {
    spdlog::error("TPP Camera hook could not be attached");
  }
  if (!aSdk->hooking->Attach(aHandle, RED4EXT_OFFSET_TO_ADDR(FPPCameraUpdateAddr), &FPPCameraUpdate,
    reinterpret_cast<void**>(&FPPCameraUpdate_Original))) {
    spdlog::error("FPP Camera hook could not be attached");
  }
}

void Camera::Unload(const RED4ext::Sdk *aSdk, RED4ext::PluginHandle aHandle) {
  aSdk->hooking->Detach(aHandle, RED4EXT_OFFSET_TO_ADDR(TPPCameraStatsUpdateAddr));
  aSdk->hooking->Detach(aHandle, RED4EXT_OFFSET_TO_ADDR(FPPCameraUpdateAddr));
}
} // namespace flight
} // namespace vehicle