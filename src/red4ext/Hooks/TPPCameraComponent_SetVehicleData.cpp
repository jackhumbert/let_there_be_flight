
#include "Utils/FlightModule.hpp"
#include "FlightController.hpp"
#include "FlightSystem.hpp"
#include "FlightSettings.hpp"
#include <RED4ext/Common.hpp>
#include <RED4ext/Scripting/Natives/Generated/vehicle/TPPCameraComponent.hpp>

using namespace RED4ext;

// Camera tweak params in game 2.5, as SetParamsFromTweak lays them out. The SDK struct still has the 1.6
// layout past `data`: its drivingDirectionCompensation* fields land on slopeCorrectionInAir* here.
constexpr size_t kDrivingDirectionCompensationSpeedCoef = 0x578;
constexpr size_t kDrivingDirectionCompensationAngleSmooth = 0x580;
// Adds (change in forward speed per frame) * 10, clamped to +-1, * 0.5 m to the boom length. In flight
// the speed is never that smooth (the speed surges in turns, the Autopilot's speed hold flickers), so
// the camera pumps in and out.
constexpr size_t kElasticBoomAcceleration = 0x527;

template <typename T> static T &CameraField(vehicle::TPPCameraComponent *camera, size_t offset) {
  return *reinterpret_cast<T *>(reinterpret_cast<uintptr_t>(camera) + offset);
}

// vanilla values of the params changed for flight, put back when flight ends
static struct {
  vehicle::TPPCameraComponent *camera = nullptr;
  float drivingDirectionCompensationSpeedCoef;
  float drivingDirectionCompensationAngleSmooth;
  bool elasticBoomAcceleration;
} saved;

static void SaveFlightParams(vehicle::TPPCameraComponent *camera) {
  if (saved.camera == camera)
    return;
  saved.camera = camera;
  saved.drivingDirectionCompensationSpeedCoef = CameraField<float>(camera, kDrivingDirectionCompensationSpeedCoef);
  saved.drivingDirectionCompensationAngleSmooth = CameraField<float>(camera, kDrivingDirectionCompensationAngleSmooth);
  saved.elasticBoomAcceleration = CameraField<bool>(camera, kElasticBoomAcceleration);
}

static void RestoreFlightParams(vehicle::TPPCameraComponent *camera) {
  // a different camera means the saved one belonged to another (possibly gone) vehicle: drop it
  if (saved.camera == camera) {
    CameraField<float>(camera, kDrivingDirectionCompensationSpeedCoef) = saved.drivingDirectionCompensationSpeedCoef;
    CameraField<float>(camera, kDrivingDirectionCompensationAngleSmooth) = saved.drivingDirectionCompensationAngleSmooth;
    CameraField<bool>(camera, kElasticBoomAcceleration) = saved.elasticBoomAcceleration;
  }
  saved.camera = nullptr;
}

// adjusts TPP camera based on flight mode
// void vehicle::TPPCameraComponent::SetVehicleData(vehicle::CameraParentData const &)
REGISTER_FLIGHT_HOOK_HASH(uintptr_t, 4125170300, TPPCameraComponent_SetVehicleData, vehicle::TPPCameraComponent *camera, uintptr_t data) {
  uintptr_t result = TPPCameraComponent_SetVehicleData_Original(camera, data);

  auto fc = FlightController::FlightController::GetInstance();
  //bool resetSlope = false;
  if (fc->active) {
    camera->data.isInAir = false;
    SaveFlightParams(camera);
    CameraField<float>(camera, kDrivingDirectionCompensationAngleSmooth) =
        FlightSettings::GetProperty<float>("drivingDirectionCompensationAngleSmooth");
    CameraField<float>(camera, kDrivingDirectionCompensationSpeedCoef) =
        FlightSettings::GetProperty<float>("drivingDirectionCompensationSpeedCoef");
    CameraField<bool>(camera, kElasticBoomAcceleration) = false;

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
    }
    //camera->slopeCorrectionOnGroundPitchMax = 90.0;
    //camera->slopeCorrectionOnGroundPitchMin = -90.0;
    //camera->slopeCorrectionInAirPitchMax = 90.0;
    //camera->slopeCorrectionInAirPitchMin = -90.0;
  } else {
    //resetSlope = true;
    if (saved.camera)
      RestoreFlightParams(camera);
  }

  //if (resetSlope && camera->slopeCorrectionOnGroundStrength == 0.0 && defaultSlopeCorrectionOnGroundStrength != 0.0) {
  //  camera->slopeCorrectionOnGroundStrength = defaultSlopeCorrectionOnGroundStrength;
  //  defaultSlopeCorrectionOnGroundStrength = 0.0;
  //}

  return result;
}