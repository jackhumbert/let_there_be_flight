
#include "Utils/FlightModule.hpp"
#include "FlightController.hpp"
#include "FlightSystem.hpp"
#include "FlightSettings.hpp"
#include <RED4ext/Common.hpp>
#include <RED4ext/Scripting/Natives/Generated/game/FPPCameraComponent.hpp>
#include <RED4ext/Scripting/Natives/Generated/anim/AnimFeature_FPPCamera.hpp>

#ifdef __APPLE__
#include "MacLayout.hpp"

// LTBF R41: FPPCameraComponent.animFeature (Handle instance at 0x360, same as Windows, R41b) is an AnimFeature_FPPCamera. The offset
// of vehicleProceduralCameraWeight inside it (Windows +0xEC) is not checked on macOS (LTBF_MAC_TODO_R41b_FPPAnimFeature_...), so
// the value is set through the class's reflected property; if the property is not found the write is skipped with a one-time log.
static void MacSetProceduralCameraWeight(RED4ext::game::FPPCameraComponent *aFpp, float aWeight) {
  auto feature = LtbfMac::Layout::At<RED4ext::IScriptable *>(aFpp, LtbfMac::Layout::FPPCamera_animFeature);
  if (!feature)
    return;
  if constexpr (LtbfMac::Layout::FPPAnimFeature_vehicleProceduralCameraWeight >= 0) {
    LtbfMac::Layout::At<float>(feature, LtbfMac::Layout::FPPAnimFeature_vehicleProceduralCameraWeight) = aWeight;
    return;
  }
  static RED4ext::CProperty *s_property = []() -> RED4ext::CProperty * {
    auto cls = RED4ext::CRTTISystem::Get()->GetClass("AnimFeature_FPPCamera");
    return cls ? cls->GetProperty("vehicleProceduralCameraWeight") : nullptr;
  }();
  if (!s_property) {
    LtbfMac::SkipOnce("LTBF_MAC_TODO_R41b_FPPAnimFeature_vehicleProceduralCameraWeight",
                      "FPP camera weight while flying (AnimFeature_FPPCamera.vehicleProceduralCameraWeight not found)");
    return;
  }
  s_property->SetValue<float>(feature, aWeight);
}
#endif

/// @hash 2531201123
// char __fastcall FPPCameraUpdate(RED4ext::game::FPPCameraComponent *fpp, float deltaTime, float deltaYaw,
                                // float deltaPitch, float deltaYawExternal, float deltaPitchExternal, char a7);

// adjusts FPP camera based on flight mode
// void game::FPPCameraComponent::Rotate(float, float, float, float, float, bool)
REGISTER_FLIGHT_HOOK_HASH(char __fastcall, 2531201123, FPPCameraUpdate, RED4ext::game::FPPCameraComponent *fpp, float deltaTime,
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
    auto stack = RED4ext::CStack(fcomp, nullptr, 0, &result);
    fcompc->GetFunction("GetFlightMode")->Execute(&stack);
    
    if (mode) {
      lockCamera |= flightModeCls->GetProperty("usesRightStickInput")->GetValue<bool>(mode);
    }
  }
  // once we find it
  // if (usesRightStickInput && !fpp->isUsingMouse) {
  if (lockCamera) {
    // fpp->pitchInput = 0.0;
    // fpp->yawInput = 0.0;
#ifdef __APPLE__
    LtbfMac::Layout::At<float>(fpp, LtbfMac::Layout::FPPCamera_yawOffset) = 0.0; // LTBF R41: 0x42C on both (R41b)
#else
    fpp->yawOffset = 0.0;
#endif
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
#ifdef __APPLE__
    MacSetProceduralCameraWeight(fpp, 1.0 - FlightSettings::GetFloat("lockFPPCameraForDrone"));
#else
    fpp->animFeature->vehicleProceduralCameraWeight = 1.0 - FlightSettings::GetFloat("lockFPPCameraForDrone");
#endif
    // fpp->animFeature->vehicleOffsetWeight = 1.0 - FlightSettings::GetFloat("lockFPPCameraForDrone");
    // fpp->animFeature->gameplayCameraPoseWeight = 1.0 - FlightSettings::GetFloat("lockFPPCameraForDrone");
    // fpp->pitchOffset = 0.0;
    // fpp->yawOffset = 0.0;
#ifndef __APPLE__
    auto vehicle = *(RED4ext::vehicle::BaseObject **)&fpp->entity;
#endif
    // if (vehicle->physics) {
    //   vehicle->physics->customTiltTarget = 0.0;
    // }

  } else {
#ifdef __APPLE__
    MacSetProceduralCameraWeight(fpp, 1.0);
#else
    fpp->animFeature->vehicleProceduralCameraWeight = 1.0;
#endif
  }
  return og;
}