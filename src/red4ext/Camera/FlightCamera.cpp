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




//RED4ext::EulerAngles * __fastcall Quaternion_ToEulerAngles(RED4ext::Quaternion* q, RED4ext::EulerAngles * e) {
//  RED4ext::RelocFunc<decltype(&Quaternion_ToEulerAngles)> call(Quaternion_ToEulerAnglesAddr);
//  return call(q, e);
//}


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