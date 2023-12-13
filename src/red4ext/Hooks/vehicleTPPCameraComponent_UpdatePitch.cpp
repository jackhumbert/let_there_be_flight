#include "Addresses.hpp"
#include "Utils/FlightModule.hpp"
#include "Flight/Component.hpp"
#include "FlightSettings.hpp"
#include <RED4ext/Common.hpp>
#include <RED4ext/Scripting/Natives/Generated/vehicle/TPPCameraComponent.hpp>

// ignores pitch slope adjustment
// updates cameraDirection
// updates cameraPitch from slopeAdjustment

// pre 2.0
// 1.6  RVA: 0x1CF5DB0 / 30367152
// 1.61hf1 RVA: 0x1CF6870 / 30369904
/// @pattern 48 8B C4 48 89 58 08 57 48 81 EC 90 00 00 00 F3 0F 10 05 ?  ?  ?  01 0F 57 ED 0F 29 70 E8 49 8B

// post 2.0
/// @pattern 48 8B C4 55 48 8D 68 A1 48 81 EC A0 00 00 00 F3 0F 10 05 C9 46 B9 02 4C 8B D1 0F 29 70 E8 4D 8B
void __fastcall UpdatePitch(RED4ext::vehicle::TPPCameraComponent *camera, RED4ext::Vector4 *localPosition, RED4ext::Vector3 *cameraPosition, RED4ext::vehicle::TPPCameraUpdate *update);

REGISTER_FLIGHT_HOOK(void __fastcall, UpdatePitch,
                     RED4ext::vehicle::TPPCameraComponent *camera, RED4ext::Vector4 *localPosition,
                     RED4ext::Vector3 *cameraPosition, RED4ext::vehicle::TPPCameraUpdate *update) {
  auto vehicle = camera->vehicle;
  auto fc = FlightComponent::Get(vehicle);
  if (fc && fc->active) {
    camera->cameraDirection = update->locationFromOffset;
    camera->cameraPitch = FlightSettings::GetProperty<float>("tppCameraPitchOffset");
  } else {
    UpdatePitch_Original(camera, localPosition, cameraPosition, update);
  }
}