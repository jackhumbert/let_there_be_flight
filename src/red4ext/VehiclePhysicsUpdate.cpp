#include "FlightModule.hpp"
#include <RED4ext/Scripting/Natives/Generated/physics/VehiclePhysics.hpp>
#include <RED4ext/Scripting/Natives/Generated/vehicle/BaseObject.hpp>
#include <RED4ext/RED4ext.hpp>
#include <spdlog/spdlog.h>
#include "VehiclePhysicsUpdate.hpp"

// main vehicle physics update
uintptr_t __fastcall VehiclePhysicsUpdate(RED4ext::physics::VehiclePhysics *, float);

// F3 0F 11 4C 24 10 55 53 57 41 54 41 55 41 56 48 8D AC 24 98 FD FF FF 48 81 EC 68 03 00 00 48 8B
constexpr uintptr_t VehiclePhysicsUpdateAddr = 0x141D0FE70 - RED4ext::Addresses::ImageBase;
decltype(&VehiclePhysicsUpdate) VehiclePhysicsUpdate_Original;

// where driverHelpers are processed
uintptr_t __fastcall VehicleHelperUpdate(RED4ext::physics::VehiclePhysics *, float);

// 48 8B C4 48 89 58 08 48 89 70 10 48 89 78 18 41 56 48 81 EC B0 00 00 00 0F 29 70 E8 4C 8B F1 0F
constexpr uintptr_t VehicleHelperUpdateAddr = 0x141D123B0 - RED4ext::Addresses::ImageBase;
decltype(&VehicleHelperUpdate) VehicleHelperUpdate_Original;

// where airControl is processed
void __fastcall AirControlProcess(RED4ext::physics::VehicleBaseObjectAirControl *ac, float deltaTime);

// 48 89 5C 24 20 56 48 83 EC 60 0F 29 74 24 50 48 8B F1 0F 57 F6 44 0F 29 44 24 30 0F 2E 71 08 44
constexpr uintptr_t AirControlProcessAddr = 0x141CE4080 - RED4ext::Addresses::ImageBase;
decltype(&AirControlProcess) AirControlProcess_Original;

// add vector to torque
void __fastcall TorqueUpdate(RED4ext::physics::VehiclePhysicsStruct *a1, uintptr_t);
//void __fastcall TorqueUpdate(RED4ext::physics::VehicleBaseObjectAirControl *ac, float deltaTime);

// F3 0F 10 41 0C F3 0F 58  02 F3 0F 11 41 0C F3 0F 10 4A 04 F3 0F 58 49 10 F3 0F 11 49 10 F3 0F 10
constexpr uintptr_t TorqueUpdateAddr = 0x1CE0D10;
decltype(&TorqueUpdate) TorqueUpdate_Original;



RED4ext::ent::IComponent *GetFlightComponent(RED4ext::physics::VehiclePhysics *p) {
  auto rtti = RED4ext::CRTTISystem::Get();
  auto fcc = rtti->GetClass("FlightComponent");
  for (auto const &c : p->parent->components) {
    if (c.GetPtr()->GetType() == fcc) {
      return c.GetPtr();
    }
  }
}

RED4ext::ent::IComponent *GetFlightComponent(RED4ext::vehicle::BaseObject *v) {
  auto rtti = RED4ext::CRTTISystem::Get();
  auto fcc = rtti->GetClass("FlightComponent");
  for (auto const &c : v->components) {
    if (c.GetPtr()->GetType() == fcc) {
      return c.GetPtr();
    }
  }
}

#define NUM_FRAMES 0x0180

uint16_t frameIndex = 0;
clock_t fullFrame[NUM_FRAMES];
clock_t lastBegin = 0;

uintptr_t __fastcall VehiclePhysicsUpdate(RED4ext::physics::VehiclePhysics *p, float deltaTime) {
  // spdlog::info(a2->c_str());
  auto rtti = RED4ext::CRTTISystem::Get();
  auto fcc = rtti->GetClass("FlightComponent");
  auto fc = GetFlightComponent(p);
  auto activeProp = fcc->GetProperty("hasUpdate");
  if (activeProp->GetValue<bool>(fc) && p->parent->physicsStruct) {
    auto onUpdate = fcc->GetFunction("OnUpdate");
    auto args = RED4ext::CStackType(rtti->GetType("Float"), &deltaTime);
    auto stack = RED4ext::CStack(fc, &args, 1, nullptr, 0);
    onUpdate->Execute(&stack);
    auto forceProp = fcc->GetProperty("force");
    auto force = forceProp->GetValue<RED4ext::Vector4>(fc);
    auto torqueProp = fcc->GetProperty("torque");
    auto torque = torqueProp->GetValue<RED4ext::Vector4>(fc);

    p->parent->physicsStruct->force.X += force.X;
    p->parent->physicsStruct->force.Y += force.Y;
    p->parent->physicsStruct->force.Z += force.Z;

    p->parent->physicsStruct->torque.X += torque.X;
    p->parent->physicsStruct->torque.Y += torque.Y;
    p->parent->physicsStruct->torque.Z += torque.Z;

    force.X = 0.0;
    force.Y = 0.0;
    force.Z = 0.0;

    torque.X = 0.0;
    torque.Y = 0.0;
    torque.Z = 0.0;

    forceProp->SetValue(fc, force);
    torqueProp->SetValue(fc, torque);
  }

  clock_t begin = std::clock();
  fullFrame[frameIndex] = begin - lastBegin;
  lastBegin = begin;
  if (frameIndex == 0) {
    float total = 0.0;
    for (auto i = 0; i < NUM_FRAMES; i++) {
      total += (fullFrame[i] / (double)CLOCKS_PER_SEC);
    }
    spdlog::info("Loop: {:.6f} ms average over {} s", total * 1000.0 / NUM_FRAMES, total);
  }

  frameIndex = (frameIndex + 1) % NUM_FRAMES;


  return VehiclePhysicsUpdate_Original(p, deltaTime);
}

void __fastcall AirControlProcess(RED4ext::physics::VehicleBaseObjectAirControl *ac, float deltaTime) {
  auto fc = GetFlightComponent(ac->vehicle);
  auto rtti = RED4ext::CRTTISystem::Get();
  auto fcc = rtti->GetClass("FlightComponent");
  auto activeProp = fcc->GetProperty("active");
  if (!activeProp->GetValue<bool>(fc)) {
    AirControlProcess_Original(ac, deltaTime);
  }
}

uintptr_t __fastcall VehicleHelperUpdate(RED4ext::physics::VehiclePhysics *p, float deltaTime) {
  auto rtti = RED4ext::CRTTISystem::Get();
  auto fcc = rtti->GetClass("FlightComponent");
  auto fc = GetFlightComponent(p);
  auto activeProp = fcc->GetProperty("active");
  auto size = p->driveHelpers.size;
  if (activeProp->GetValue<bool>(fc)) {
    p->driveHelpers.size = 0;
  }
  auto result = VehicleHelperUpdate_Original(p, deltaTime);
  p->driveHelpers.size = size;
  return result;
}


void __fastcall TorqueUpdate(RED4ext::physics::VehiclePhysicsStruct* a1, uintptr_t a2) {
  auto rtti = RED4ext::CRTTISystem::Get();
  auto fcc = rtti->GetClass("FlightComponent");
  auto fc = GetFlightComponent(a1->vehicle);
  auto activeProp = fcc->GetProperty("active");
  if (!activeProp->GetValue<bool>(fc)) {
    TorqueUpdate_Original(a1, a2);
  }
}

struct VehiclePhysicsUpdateModule : FlightModule {
  void Load(const RED4ext::Sdk *aSdk, RED4ext::PluginHandle aHandle) {
    while (!aSdk->hooking->Attach(aHandle, RED4EXT_OFFSET_TO_ADDR(VehiclePhysicsUpdateAddr), &VehiclePhysicsUpdate,
                          reinterpret_cast<void **>(&VehiclePhysicsUpdate_Original)));
    while (!aSdk->hooking->Attach(aHandle, RED4EXT_OFFSET_TO_ADDR(VehicleHelperUpdateAddr), &VehicleHelperUpdate,
                          reinterpret_cast<void **>(&VehicleHelperUpdate_Original)));
    while (!aSdk->hooking->Attach(aHandle, RED4EXT_OFFSET_TO_ADDR(AirControlProcessAddr), &AirControlProcess,
                          reinterpret_cast<void **>(&AirControlProcess_Original)));
    while (!aSdk->hooking->Attach(aHandle, RED4EXT_OFFSET_TO_ADDR(TorqueUpdateAddr), &TorqueUpdate,
                          reinterpret_cast<void **>(&TorqueUpdate_Original)));
  }
  void Unload(const RED4ext::Sdk *aSdk, RED4ext::PluginHandle aHandle) {
    aSdk->hooking->Detach(aHandle, RED4EXT_OFFSET_TO_ADDR(VehiclePhysicsUpdateAddr));
    aSdk->hooking->Detach(aHandle, RED4EXT_OFFSET_TO_ADDR(VehicleHelperUpdateAddr));
    aSdk->hooking->Detach(aHandle, RED4EXT_OFFSET_TO_ADDR(AirControlProcessAddr));
    aSdk->hooking->Detach(aHandle, RED4EXT_OFFSET_TO_ADDR(TorqueUpdateAddr));
  }
};

REGISTER_FLIGHT_MODULE(VehiclePhysicsUpdateModule);