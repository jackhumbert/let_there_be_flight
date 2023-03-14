#include "FlightCamera.hpp"
#include "FlightController.hpp"
#include "FlightSystem.hpp"
#include "FlightSettings.hpp"
#include <RED4ext/Scripting/Natives/vehiclePhysics.hpp>
#include <RED4ext/Scripting/Natives/Generated/EulerAngles.hpp>
#include <spdlog/spdlog.h>
#include "Addresses.hpp"
#include "Engine/RTTIExpansion.hpp"

//float defaultSlopeCorrectionOnGroundStrength = 0.0;

// adjusts TPP camera based on flight mode
REGISTER_FLIGHT_HOOK(uintptr_t, TPPCameraStatsUpdate, RED4ext::vehicle::TPPCameraComponent *camera, uintptr_t data) {
  uintptr_t result = TPPCameraStatsUpdate_Original(camera, data);
  
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
    auto stack = RED4ext::CStack(fcomp, nullptr, 0, &result, 0);
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

// adjusts FPP camera based on flight mode
REGISTER_FLIGHT_HOOK(char __fastcall, FPPCameraUpdate, 
    RED4ext::game::FPPCameraComponent *fpp, float deltaTime, float deltaYaw, float deltaPitch, 
    float deltaYawExternal, float deltaPitchExternal, char a7) {
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

 // gets the location for the camera to look at
REGISTER_FLIGHT_HOOK(RED4ext::Vector4 *__fastcall, vehicleTPPCameraComponent_GetLocationFromOffset,
                     RED4ext::vehicle::TPPCameraComponent *camera, RED4ext::Vector4 *location,
                     RED4ext::Vector3 *offset) {
  auto v = new RED4ext::Vector4();
  auto vehicle = camera->vehicle;
  auto fc = FlightComponent::Get(vehicle);
  if (fc && fc->active) {
    // ignore pitch adjustments
    // camera->pitch = camera->cameraPitch;
    camera->data.isInAir = false;
    if (FlightSettings::GetProperty<bool>("tppCameraCenterOnMass")) {
      *v = *vehicle->worldTransform.Position.ToVector4() +
             (vehicle->worldTransform.Orientation * vehicle->physicsData->centerOfMass);
    } else {
      *v = *vehicleTPPCameraComponent_GetLocationFromOffset_Original(camera, location, offset);
    }
    return v;
  } else {
    return vehicleTPPCameraComponent_GetLocationFromOffset_Original(camera, location, offset);
  }
}

//RED4ext::EulerAngles * __fastcall Quaternion_ToEulerAngles(RED4ext::Quaternion* q, RED4ext::EulerAngles * e) {
//  RED4ext::RelocFunc<decltype(&Quaternion_ToEulerAngles)> call(Quaternion_ToEulerAnglesAddr);
//  return call(q, e);
//}

// ignores driving direction correction
REGISTER_FLIGHT_HOOK(void __fastcall, vehicleTPPCameraComponent_GetYaw, RED4ext::vehicle::TPPCameraComponent *camera,
                     float *yaw, RED4ext::Vector4 *position, float isInAir) {
  auto vehicle = camera->vehicle;
  auto fc = FlightComponent::Get(vehicle);
  if (fc && fc->active) {
    *yaw = 0.0;
  } else {
    vehicleTPPCameraComponent_GetYaw_Original(camera, yaw, position, isInAir);
  }
}

// ignores pitch slope adjustment
REGISTER_FLIGHT_HOOK(void __fastcall, vehicleTPPCameraComponent_UpdatePitch,
                     RED4ext::vehicle::TPPCameraComponent *camera, RED4ext::Vector4 *localPosition,
                     RED4ext::Vector3 *cameraPosition, RED4ext::vehicle::TPPCameraUpdate *update) {
  auto vehicle = camera->vehicle;
  auto fc = FlightComponent::Get(vehicle);
  if (fc && fc->active) {
    camera->cameraDirection = update->locationFromOffset;
    camera->cameraPitch = FlightSettings::GetProperty<float>("tppCameraPitchOffset");
  } else {
    vehicleTPPCameraComponent_UpdatePitch_Original(camera, localPosition, cameraPosition, update);
  }
}

//REGISTER_FLIGHT_HOOK(void __fastcall, vehicleTPPCameraComponent_UpdatePosition, 
//    RED4ext::vehicle::TPPCameraComponent *camera, RED4ext::vehicle::TPPCameraPreset *preset) {
//  auto vehicle = camera->vehicle;
//  auto fc = FlightComponent::Get(vehicle);
//  if (fc && fc->active) {
//    vehicleTPPCameraComponent_UpdatePosition_Original(camera, preset);
//    camera->initialTransform.Position =
//        *vehicle->worldTransform.Position.ToVector4() +
//        (vehicle->worldTransform.Orientation * (vehicle->physicsData->centerOfMass + RED4ext::Vector3(0.0, -5.0, 5.0)));
//    camera->initialTransform.Orientation = vehicle->worldTransform.Orientation;
//  } else {
//    vehicleTPPCameraComponent_UpdatePosition_Original(camera, preset);
//  }
//}

//REGISTER_FLIGHT_OVERRIDE(RED4ext::vehicle::TPPCameraComponent::Update, void __fastcall, vehicleTPPCameraComponent_Update, RED4ext::vehicle::TPPCameraComponent *camera) {
//  RED4ext::vehicle::TPPCameraComponent::Update(camera);
//  auto vehicle = camera->vehicle;
//  auto fc = FlightComponent::Get(vehicle);
//  if (fc && fc->active) {
//    // camera->initialTransform.Position += vehicle->physics->velocity;
//  }
//}

//class TPPCameraComponent : Engine::RTTIExpansion<TPPCameraComponent, RED4ext::vehicle::TPPCameraComponent> {
//public:
//  void Update() override {
//    RED4ext::vehicle::TPPCameraComponent::Update();
//  }
//
//private:
//  friend Descriptor;
//  
//  static void OnLoad(const RED4ext::Sdk *aSdk, RED4ext::PluginHandle aHandle) {
//    while (!aSdk->hooking->Attach(aHandle, RED4EXT_OFFSET_TO_ADDR(vehicleTPPCameraComponent_UpdateAddr),
//                                  reinterpret_cast<void *>(&TPPCameraComponent::Update),
//                                  reinterpret_cast<void **>(&RED4ext::vehicle::TPPCameraComponent::Update)))
//      ;
//  }
//};