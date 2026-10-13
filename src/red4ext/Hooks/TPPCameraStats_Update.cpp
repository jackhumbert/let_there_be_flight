
#include "Utils/FlightModule.hpp"
#include "FlightController.hpp"
#include "FlightSystem.hpp"
#include "FlightSettings.hpp"
#include <RED4ext/Common.hpp>
#include <RED4ext/Scripting/Natives/Generated/vehicle/TPPCameraComponent.hpp>
#ifdef __APPLE__
#include "MacLayout.hpp"
#endif

using namespace RED4ext;

// adjusts TPP camera based on flight mode
// void vehicle::TPPCameraComponent::SetVehicleData(vehicle::CameraParentData const &)
REGISTER_FLIGHT_HOOK_HASH(uintptr_t, 4125170300, TPPCameraStats_Update, vehicle::TPPCameraComponent *camera, uintptr_t data) {
  uintptr_t result = TPPCameraStats_Update_Original(camera, data);
  
  auto fc = FlightController::FlightController::GetInstance();
  //bool resetSlope = false;
  if (fc->active) {
#ifdef __APPLE__
    // LTBF R41: macOS offsets (R41b): the CameraPreset block at 0x330, the compensation floats at 0x580 / 0x578 (same as Windows;
    // the macOS SDK's TPPCameraComponent has only the reflected fields).
    auto &cameraData = LtbfMac::Layout::At<LtbfMac::Layout::TppCameraPreset>(camera, LtbfMac::Layout::TPPCamera_data);
    cameraData.isInAir = false;
    LtbfMac::Layout::At<float>(camera, LtbfMac::Layout::TPPCamera_angleSmooth) =
        FlightSettings::GetProperty<float>("drivingDirectionCompensationAngleSmooth");
    LtbfMac::Layout::At<float>(camera, LtbfMac::Layout::TPPCamera_speedCoef) =
        FlightSettings::GetProperty<float>("drivingDirectionCompensationSpeedCoef");
#else
    auto &cameraData = camera->data;
    cameraData.isInAir = false;
    camera->drivingDirectionCompensationAngleSmooth =
        FlightSettings::GetProperty<float>("drivingDirectionCompensationAngleSmooth");
    camera->drivingDirectionCompensationSpeedCoef =
        FlightSettings::GetProperty<float>("drivingDirectionCompensationSpeedCoef");
#endif

    auto rtti = CRTTISystem::Get();
    auto fcc = rtti->GetClass("FlightSystem");
    auto fs = FlightSystem::FlightSystem::GetInstance();
    auto pcp = fcc->GetProperty("playerComponent");
    auto fcomp = pcp->GetValue<Handle<IScriptable>>(fs);
    auto fcompc = rtti->GetClass("FlightComponent");
    Handle<IScriptable> mode;
    auto flightModeCls = rtti->GetClass("FlightMode");
    auto result = CStackType(flightModeCls, &mode);
    auto stack = CStack(fcomp, nullptr, 0, &result);
    fcompc->GetFunction("GetFlightMode")->Execute(&stack);

    // in case a mode is removed
    if (mode) {
      auto usesRightStickInput = flightModeCls->GetProperty("usesRightStickInput")->GetValue<bool>(mode);

      if (usesRightStickInput && !cameraData.isUsingMouse) {
        cameraData.pitchDelta = 0.0;
        cameraData.yawDelta = 0.0;
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
    }
    //camera->slopeCorrectionOnGroundPitchMax = 90.0;
    //camera->slopeCorrectionOnGroundPitchMin = -90.0;
    //camera->slopeCorrectionInAirPitchMax = 90.0;
    //camera->slopeCorrectionInAirPitchMin = -90.0;
  } else {
    //resetSlope = true;
  }

  //if (resetSlope && camera->slopeCorrectionOnGroundStrength == 0.0 && defaultSlopeCorrectionOnGroundStrength != 0.0) {
  //  camera->slopeCorrectionOnGroundStrength = defaultSlopeCorrectionOnGroundStrength;
  //  defaultSlopeCorrectionOnGroundStrength = 0.0;
  //}

  return result;
}