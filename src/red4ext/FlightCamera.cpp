#include "FlightCamera.hpp"
#include "FlightController.hpp"
#include "FlightSystem.hpp"
#include "FlightSettings.hpp"
#include <RED4ext/Scripting/Natives/vehiclePhysics.hpp>
#include <spdlog/spdlog.h>
#include "Addresses.hpp"

// Treat flying vehicles as being on the ground (for TPP camera)

namespace vehicle {
namespace flight {

REGISTER_FLIGHT_MODULE(Camera);

decltype(&Camera::TPPCameraStatsUpdate) TPPCameraStatsUpdate_Original;

float defaultSlopeCorrectionOnGroundStrength = 0.0;

uintptr_t Camera::TPPCameraStatsUpdate(RED4ext::vehicle::TPPCameraComponent *camera, uintptr_t data) {
  uintptr_t result = TPPCameraStatsUpdate_Original(camera, data);
  
  auto fc = FlightController::FlightController::GetInstance();
  bool resetSlope = false;
  if (fc->active) {
    camera->isInAir = false;
    camera->drivingDirectionCompensationAngleSmooth =
        FlightSettings::GetProperty<float>("drivingDirectionCompensationAngleSmooth");
    camera->drivingDirectionCompensationSpeedCoef =
        FlightSettings::GetProperty<float>("drivingDirectionCompensationSpeedCoef");

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
      //camera->lockedCamera = true;
      //camera->pitch = 30.0;
      //camera->yaw = 0.0;
      //if (camera->slopeCorrectionOnGroundStrength != 0.0) {
      //  defaultSlopeCorrectionOnGroundStrength = camera->slopeCorrectionOnGroundStrength;
      //}
      //camera->slopeCorrectionOnGroundStrength = 0.0;
    } else {
      //resetSlope = true;
    }
    camera->slopeCorrectionOnGroundPitchMax = 90.0;
    camera->slopeCorrectionOnGroundPitchMin = -90.0;
    camera->slopeCorrectionInAirPitchMax = 90.0;
    camera->slopeCorrectionInAirPitchMin = -90.0;
  } else {
    //resetSlope = true;
  }

  if (resetSlope && camera->slopeCorrectionOnGroundStrength == 0.0 && defaultSlopeCorrectionOnGroundStrength != 0.0) {
    camera->slopeCorrectionOnGroundStrength = defaultSlopeCorrectionOnGroundStrength;
    defaultSlopeCorrectionOnGroundStrength = 0.0;
  }

  return result;
}

decltype(&Camera::FPPCameraUpdate) FPPCameraUpdate_Original;

char __fastcall Camera::FPPCameraUpdate(RED4ext::game::FPPCameraComponent *fpp, float deltaTime, float deltaYaw, float deltaPitch,
                                        float deltaYawExternal,
                                 float deltaPitchExternal, char a7) {
  // a1 is a shifted pointer - this gets the whole struct
  //auto fpp = reinterpret_cast<RED4ext::game::FPPCameraComponent *>(a1 - 0x120);

  auto fc = FlightController::FlightController::GetInstance();
  bool lockCamera = false;
  if (fc->active) {
    auto rtti = RED4ext::CRTTISystem::Get();
    auto fcc = rtti->GetClass("FlightSystem");
    auto fs = FlightSystem::FlightSystem::GetInstance();
    lockCamera |= fs->cameraIndex == 1;
    auto pcp = fcc->GetProperty("playerComponent");
    auto fcomp = pcp->GetValue<RED4ext::Handle<RED4ext::IScriptable>>(fs);
    auto fcompc = rtti->GetClass("FlightComponent");
    RED4ext::Handle<RED4ext::IScriptable> mode;
    auto flightModeCls = rtti->GetClass("FlightMode");
    auto result = RED4ext::CStackType(flightModeCls, &mode);
    auto stack = RED4ext::CStack(fcomp, nullptr, 0, &result, 0);
    fcompc->GetFunction("GetFlightMode")->Execute(&stack);
    lockCamera |= flightModeCls->GetProperty("usesRightStickInput")->GetValue<bool>(mode);
  }
  // once we find it
  //if (usesRightStickInput && !fpp->isUsingMouse) {
  if (lockCamera) {
    //fpp->pitchInput = 0.0;
    //fpp->yawInput = 0.0;
    fpp->yawOffset = 0.0;
    //fpp->headingLocked = true;
    //fpp->sensitivityMultX = 0.0;
    //fpp->sensitivityMultY = 0.0;
    deltaYaw = 0.0;
    deltaPitch = 0.0;
    //fpp->unk484 = 0.0;
  } else {
    //fpp->headingLocked = false;
    //fpp->sensitivityMultX = 1.0;
    //fpp->sensitivityMultY = 1.0;
  }

  auto og = FPPCameraUpdate_Original(fpp, deltaTime, deltaYaw, deltaPitch, deltaYawExternal, deltaPitchExternal, a7);

  if (lockCamera) {
    //fpp->animFeature->additiveCameraMovementsWeight = 1.0 - FlightSettings::GetFloat("lockFPPCameraForDrone");
    fpp->animFeature->vehicleProceduralCameraWeight = 1.0 - FlightSettings::GetFloat("lockFPPCameraForDrone");
    //fpp->animFeature->vehicleOffsetWeight = 1.0 - FlightSettings::GetFloat("lockFPPCameraForDrone");
    //fpp->animFeature->gameplayCameraPoseWeight = 1.0 - FlightSettings::GetFloat("lockFPPCameraForDrone");
    //fpp->pitchOffset = 0.0;
    //fpp->yawOffset = 0.0;
    auto vehicle = *(RED4ext::vehicle::BaseObject **)&fpp->entity;
    //if (vehicle->physics) {
    //  vehicle->physics->customTiltTarget = 0.0;
    //}

  } else {
    fpp->animFeature->vehicleProceduralCameraWeight = 1.0;
  }
  return og;
}

void Camera::Load(const RED4ext::Sdk *aSdk, RED4ext::PluginHandle aHandle) {
  while (!aSdk->hooking->Attach(aHandle, RED4EXT_OFFSET_TO_ADDR(TPPCameraStatsUpdateAddr), &TPPCameraStatsUpdate,
                                reinterpret_cast<void **>(&TPPCameraStatsUpdate_Original)))
    ;
  while (!aSdk->hooking->Attach(aHandle, RED4EXT_OFFSET_TO_ADDR(FPPCameraUpdateAddr), &FPPCameraUpdate,
                                reinterpret_cast<void **>(&FPPCameraUpdate_Original)))
    ;
}

void Camera::Unload(const RED4ext::Sdk *aSdk, RED4ext::PluginHandle aHandle) {
  aSdk->hooking->Detach(aHandle, RED4EXT_OFFSET_TO_ADDR(TPPCameraStatsUpdateAddr));
  aSdk->hooking->Detach(aHandle, RED4EXT_OFFSET_TO_ADDR(FPPCameraUpdateAddr));
}
} // namespace flight
} // namespace vehicle