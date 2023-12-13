#include "Addresses.hpp"
#include "Utils/FlightModule.hpp"
#include "FlightController.hpp"
#include "FlightSystem.hpp"
#include "FlightSettings.hpp"
#include <RED4ext/Common.hpp>
#include <RED4ext/Scripting/Natives/Generated/game/FPPCameraComponent.hpp>
#include <RED4ext/Scripting/Natives/Generated/anim/AnimFeature_FPPCamera.hpp>

// pre-2.0
// 1.6 RVA: 0x17190A0
/// @pattern 48 8B C4 F3 0F 11 48 10 53 48 81 EC 30 01 00 00 80 B9 A0 04 00 00 00 48 8B D9 0F 29 70 D8 0F 28

// 2.0+
/// @pattern 48 8B C4 48 89 58 08 48 89 78 10 55 48 8D 68 D8 48 81 EC 20 01 00 00 80 B9 D8 04 00 00 00 48 8B
char __fastcall FPPCameraUpdate(RED4ext::game::FPPCameraComponent *fpp, float deltaTime, float deltaYaw,
                                float deltaPitch, float deltaYawExternal, float deltaPitchExternal, char a7);

// adjusts FPP camera based on flight mode
REGISTER_FLIGHT_HOOK(char __fastcall, FPPCameraUpdate, RED4ext::game::FPPCameraComponent *fpp, float deltaTime,
                     float deltaYaw, float deltaPitch, float deltaYawExternal, float deltaPitchExternal, char a7) {
  // a1 is a shifted pointer - this gets the whole struct
  // auto fpp = reinterpret_cast<RED4ext::game::FPPCameraComponent *>(a1 - 0x120);

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
  // if (usesRightStickInput && !fpp->isUsingMouse) {
  if (lockCamera) {
    // fpp->pitchInput = 0.0;
    // fpp->yawInput = 0.0;
    fpp->yawOffset = 0.0;
    // fpp->headingLocked = true;
    // fpp->sensitivityMultX = 0.0;
    // fpp->sensitivityMultY = 0.0;
    deltaYaw = 0.0;
    deltaPitch = 0.0;
    // fpp->unk484 = 0.0;
  } else {
    // fpp->headingLocked = false;
    // fpp->sensitivityMultX = 1.0;
    // fpp->sensitivityMultY = 1.0;
  }

  auto og = FPPCameraUpdate_Original(fpp, deltaTime, deltaYaw, deltaPitch, deltaYawExternal, deltaPitchExternal, a7);

  if (lockCamera) {
    // fpp->animFeature->additiveCameraMovementsWeight = 1.0 - FlightSettings::GetFloat("lockFPPCameraForDrone");
    fpp->animFeature->vehicleProceduralCameraWeight = 1.0 - FlightSettings::GetFloat("lockFPPCameraForDrone");
    // fpp->animFeature->vehicleOffsetWeight = 1.0 - FlightSettings::GetFloat("lockFPPCameraForDrone");
    // fpp->animFeature->gameplayCameraPoseWeight = 1.0 - FlightSettings::GetFloat("lockFPPCameraForDrone");
    // fpp->pitchOffset = 0.0;
    // fpp->yawOffset = 0.0;
    auto vehicle = *(RED4ext::vehicle::BaseObject **)&fpp->entity;
    // if (vehicle->physics) {
    //   vehicle->physics->customTiltTarget = 0.0;
    // }

  } else {
    fpp->animFeature->vehicleProceduralCameraWeight = 1.0;
  }
  return og;
}