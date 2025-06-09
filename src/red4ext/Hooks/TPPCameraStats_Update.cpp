#include "Addresses.hpp"
#include "Utils/FlightModule.hpp"
#include "FlightController.hpp"
#include "FlightSystem.hpp"
#include "FlightSettings.hpp"
#include <RED4ext/Common.hpp>
#include <RED4ext/Scripting/Natives/Generated/vehicle/TPPCameraComponent.hpp>

// adjusts TPP camera based on flight mode
/// @hash 4125170300
// uintptr_t TPPCameraStats_Update(RED4ext::vehicle::TPPCameraComponent *camera, uintptr_t data);

// void vehicle::TPPCameraComponent::SetVehicleData(vehicle::CameraParentData const &)
REGISTER_FLIGHT_HOOK_HASH(uintptr_t, 4125170300, TPPCameraStats_Update, RED4ext::vehicle::TPPCameraComponent *camera, uintptr_t data) {
  uintptr_t result = TPPCameraStats_Update_Original(camera, data);
  
  auto fc = FlightController::FlightController::GetInstance();
  //bool resetSlope = false;
  if (fc->active) {
    camera->data.isInAir = false;
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
    auto stack = RED4ext::CStack(fcomp, nullptr, 0, &result);
    fcompc->GetFunction("GetFlightMode")->Execute(&stack);
    auto usesRightStickInput = flightModeCls->GetProperty("usesRightStickInput")->GetValue<bool>(mode);

    if (usesRightStickInput && !camera->data.isUsingMouse) {
      camera->data.pitchDelta = 0.0;
      camera->data.yawDelta = 0.0;
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