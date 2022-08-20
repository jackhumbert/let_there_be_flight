#include "FlightModule.hpp"
#include <RED4ext/Scripting/Natives/vehiclePhysics.hpp>
#include <RED4ext/Scripting/Natives/Generated/vehicle/BaseObject.hpp>
#include <RED4ext/Scripting/Natives/Generated/vehicle/CarBaseObject.hpp>
#include <RED4ext/RED4ext.hpp>
#include <spdlog/spdlog.h>
#include "VehiclePhysicsUpdate.hpp"

// main vehicle physics update
uintptr_t __fastcall VehiclePhysicsUpdate(RED4ext::vehicle::Physics *, float);

// F3 0F 11 4C 24 10 55 53 57 41 54 41 55 41 56 48 8D AC 24 98 FD FF FF 48 81 EC 68 03 00 00 48 8B
constexpr uintptr_t VehiclePhysicsUpdateAddr = 0x141D0FE70 - RED4ext::Addresses::ImageBase;
decltype(&VehiclePhysicsUpdate) VehiclePhysicsUpdate_Original;

// where driverHelpers are processed
uintptr_t __fastcall VehicleHelperUpdate(RED4ext::vehicle::WheeledPhysics *, float);

// 48 8B C4 48 89 58 08 48 89 70 10 48 89 78 18 41 56 48 81 EC B0 00 00 00 0F 29 70 E8 4C 8B F1 0F
constexpr uintptr_t VehicleHelperUpdateAddr = 0x141D123B0 - RED4ext::Addresses::ImageBase;
decltype(&VehicleHelperUpdate) VehicleHelperUpdate_Original;

// where airControl is processed
void __fastcall AirControlProcess(RED4ext::vehicle::AirControl *ac, float deltaTime);

// 48 89 5C 24 20 56 48 83 EC 60 0F 29 74 24 50 48 8B F1 0F 57 F6 44 0F 29 44 24 30 0F 2E 71 08 44
constexpr uintptr_t AirControlProcessAddr = 0x141CE4080 - RED4ext::Addresses::ImageBase;
decltype(&AirControlProcess) AirControlProcess_Original;

// add vector to torque
void __fastcall TorqueUpdate(RED4ext::vehicle::PhysicsData *a1, uintptr_t);
// void __fastcall TorqueUpdate(RED4ext::physics::VehicleBaseObjectAirControl *ac, float deltaTime);

// F3 0F 10 41 0C F3 0F 58  02 F3 0F 11 41 0C F3 0F 10 4A 04 F3 0F 58 49 10 F3 0F 11 49 10 F3 0F 10
constexpr uintptr_t TorqueUpdateAddr = 0x1CE0D10;
decltype(&TorqueUpdate) TorqueUpdate_Original;

// update with pid
void __fastcall VehicleUpdateOrientationWithPID(RED4ext::vehicle::CarBaseObject *a1, RED4ext::Transform *, float,
                                                float);

// 48 8B C4 F3 0F 11 58 20 F3 0F 11 50 18 55 53 56 57 41 54 41 55 41 56 41 57 48 8D A8 58 FC FF FF
constexpr uintptr_t VehicleUpdateOrientationWithPIDAddr = 0x1C6E270;
decltype(&VehicleUpdateOrientationWithPID) VehicleUpdateOrientationWithPID_Original;

// update with pid
uintptr_t __fastcall AnimationUpdate(RED4ext::vehicle::CarPhysics *a1, float);

// 48 89 5C 24 18 56 48 81 EC D0 00 00 00 48 8B F1 0F 29 B4 24 C0 00 00 00 48 8B 89 20 0D 00 00 BA
constexpr uintptr_t AnimationUpdateAddr = 0x1D0C290;
decltype(&AnimationUpdate) AnimationUpdate_Original;


uintptr_t __fastcall BikeAnimationUpdate(RED4ext::vehicle::BikePhysics *a1);
decltype(&BikeAnimationUpdate) BikeAnimationUpdate_Original;

/// 48 8B 49 08 E9 47 1D 2D FF
constexpr uintptr_t UpdateAnimValueForCNameAddr = 0x1CFD9F0;
uint64_t __fastcall UpdateAnimValueForCName(RED4ext::vehicle::BaseObject *vehicle, RED4ext::CName name, float value) {
  RED4ext::RelocFunc<uint64_t (__fastcall *)(void *, uint64_t, float)> call(UpdateAnimValueForCNameAddr);
  return call(vehicle->unk570, name, value);
}


RED4ext::ent::IComponent *GetFlightComponent(RED4ext::vehicle::Physics *p) {
  auto rtti = RED4ext::CRTTISystem::Get();
  auto fcc = rtti->GetClass("FlightComponent");
  for (auto const &c : p->parent->componentsStorage.components) {
    if (c.GetPtr()->GetType() == fcc) {
      return c.GetPtr();
    }
  }
  return NULL;
}

RED4ext::ent::IComponent *GetFlightComponent(RED4ext::vehicle::BaseObject *v) {
  auto rtti = RED4ext::CRTTISystem::Get();
  auto fcc = rtti->GetClass("FlightComponent");
  for (auto const &c : v->componentsStorage.components) {
    if (c.GetPtr()->GetType() == fcc) {
      return c.GetPtr();
    }
  }
  return NULL;
}

//#define NUM_FRAMES 0x0180
//
//uint16_t frameIndex = 0;
//clock_t fullFrame[NUM_FRAMES];
//clock_t lastBegin = 0;

uintptr_t __fastcall VehiclePhysicsUpdate(RED4ext::vehicle::Physics *p, float deltaTime) {
  // spdlog::info(a2->c_str());
  //auto rtti = RED4ext::CRTTISystem::Get();
  //auto fcc = rtti->GetClass("FlightComponent");
  //auto fc = GetFlightComponent(p);
  //auto activeProp = fcc->GetProperty("hasUpdate");
  //if (activeProp->GetValue<bool>(fc) && p->parent->physicsData) {
  //  auto onUpdate = fcc->GetFunction("OnUpdate");
  //  auto args = RED4ext::CStackType(rtti->GetType("Float"), &deltaTime);
  //  auto stack = RED4ext::CStack(fc, &args, 1, nullptr, 0);
  //  onUpdate->Execute(&stack);
  //  auto forceProp = fcc->GetProperty("force");
  //  auto force = forceProp->GetValue<RED4ext::Vector4>(fc);
  //  auto torqueProp = fcc->GetProperty("torque");
  //  auto torque = torqueProp->GetValue<RED4ext::Vector4>(fc);

  //  p->parent->physicsData->force.X += force.X;
  //  p->parent->physicsData->force.Y += force.Y;
  //  p->parent->physicsData->force.Z += force.Z;

  //  p->parent->physicsData->torque.X += torque.X;
  //  p->parent->physicsData->torque.Y += torque.Y;
  //  p->parent->physicsData->torque.Z += torque.Z;

  //  force.X = 0.0;
  //  force.Y = 0.0;
  //  force.Z = 0.0;

  //  torque.X = 0.0;
  //  torque.Y = 0.0;
  //  torque.Z = 0.0;

  //  forceProp->SetValue(fc, force);
  //  torqueProp->SetValue(fc, torque);
  //}

  //clock_t begin = std::clock();
  //fullFrame[frameIndex] = begin - lastBegin;
  //lastBegin = begin;
  //if (frameIndex == 0) {
  //  float total = 0.0;
  //  for (auto i = 0; i < NUM_FRAMES; i++) {
  //    total += (fullFrame[i] / (double)CLOCKS_PER_SEC);
  //  }
  //  spdlog::info("Loop: {:.6f} ms average over {} s", total * 1000.0 / NUM_FRAMES, total);
  //}

  //frameIndex = (frameIndex + 1) % NUM_FRAMES;


  return VehiclePhysicsUpdate_Original(p, deltaTime);
}


// update with pid
void __fastcall ProcessAirResistance(RED4ext::vehicle::WheeledPhysics *a1, float deltaTime);

// 48 8B C4 53 48 81 EC A0 00 00 00 0F 29 70 E8 48 8B D9 0F 29 78 D8 44 0F 29 40 C8 44 0F 29 48 B8
constexpr uintptr_t ProcessAirResistanceAddr = 0x1D0E5F0;
decltype(&ProcessAirResistance) ProcessAirResistance_Original;

void __fastcall ProcessAirResistance(RED4ext::vehicle::WheeledPhysics *a1, float deltaTime) {
  auto physicsData = a1->parent->physicsData;
  auto velocity = physicsData->velocity;
  auto X = velocity.X;
  auto Y = velocity.Y;
  auto Z = velocity.Z;
  auto speedSquared = (float)((float)(X * X) + (float)(Y * Y)) + (float)(Z * Z);
  if (_fdclass(speedSquared) != 1 && speedSquared >= 10000.0) {
    auto unk568 = a1->parent2->unk568;
    if (speedSquared > 0.0099999998) {
      auto speed = sqrt(speedSquared);
      if (speed != 0.0) {
        X = X / speed;
        Y = Y / speed;
        Z = Z / speed;
      }
      RED4ext::Vector3 airResistanceForce;
      auto yankX = (float)((float)(X * -1.2) * a1->airResistanceFactor) * speedSquared;
      auto yankY = (float)((float)(Y * -1.2) * a1->airResistanceFactor) * speedSquared;
      auto yankZ = (float)((float)(Z * -1.2) * a1->airResistanceFactor) * speedSquared;
      airResistanceForce.X = yankX * deltaTime;
      airResistanceForce.Y = yankY * deltaTime;
      airResistanceForce.Z = yankZ * deltaTime;
      physicsData->force += airResistanceForce;
      unk568->unk108 = sqrt((float)((float)(yankX * yankX) + (float)(yankY * yankY)) + (float)(yankZ * yankZ));
    }
  }
  ProcessAirResistance_Original(a1, deltaTime);
}

void __fastcall AirControlProcess(RED4ext::vehicle::AirControl *ac, float deltaTime) {
  auto fc = GetFlightComponent(ac->vehicle);
  if (fc) {
    auto rtti = RED4ext::CRTTISystem::Get();
    auto fcc = rtti->GetClass("FlightComponent");
    auto activeProp = fcc->GetProperty("active");
    if (!activeProp->GetValue<bool>(fc)) {
      AirControlProcess_Original(ac, deltaTime);
    }
  }
}

uintptr_t __fastcall VehicleHelperUpdate(RED4ext::vehicle::WheeledPhysics *p, float deltaTime) {
  auto rtti = RED4ext::CRTTISystem::Get();
  auto fcc = rtti->GetClass("FlightComponent");
  auto fc = GetFlightComponent(p);
  if (fc) {
    auto activeProp = fcc->GetProperty("active");
    auto size = p->driveHelpers.size;
    if (activeProp->GetValue<bool>(fc)) {
      p->driveHelpers.size = 0;
    }
    auto result = VehicleHelperUpdate_Original(p, deltaTime);
    p->driveHelpers.size = size;
    return result;
  } else {
    return VehicleHelperUpdate_Original(p, deltaTime);
  }
}


void __fastcall TorqueUpdate(RED4ext::vehicle::PhysicsData* a1, uintptr_t a2) {
  auto rtti = RED4ext::CRTTISystem::Get();
  auto fcc = rtti->GetClass("FlightComponent");
  auto fc = GetFlightComponent(a1->vehicle);
  if (fc) {
    auto activeProp = fcc->GetProperty("active");
    if (!activeProp->GetValue<bool>(fc)) {
      TorqueUpdate_Original(a1, a2);
    }
  } else {
    TorqueUpdate_Original(a1, a2);
  }
}


void __fastcall VehicleUpdateOrientationWithPID(RED4ext::vehicle::CarBaseObject *a1, RED4ext::Transform * a2, float a3, float a4) {
  auto fc = GetFlightComponent(a1);
  if (fc) {
    auto rtti = RED4ext::CRTTISystem::Get();
    auto fcc = rtti->GetClass("FlightComponent");
    auto activeProp = fcc->GetProperty("active");
    if (!activeProp->GetValue<bool>(fc)) {
      VehicleUpdateOrientationWithPID_Original(a1, a2, a3, a4);
    }
  } else {
    VehicleUpdateOrientationWithPID_Original(a1, a2, a3, a4);
  }
}

uintptr_t __fastcall AnimationUpdate(RED4ext::vehicle::CarPhysics *a1, float timeDelta) {
  auto fc = GetFlightComponent(a1->parent3);
  if (fc) {
    auto rtti = RED4ext::CRTTISystem::Get();
    auto fcc = rtti->GetClass("FlightComponent");
    auto activeProp = fcc->GetProperty("active");
    auto rollProp = fcc->GetProperty("roll");
    if (activeProp->GetValue<bool>(fc)) {
      a1->parent3->turnInput = rollProp->GetValue<float>(fc);
    }
  }
  return AnimationUpdate_Original(a1, timeDelta);
}

uintptr_t __fastcall BikeAnimationUpdate(RED4ext::vehicle::BikePhysics *a1) {
  auto fc = GetFlightComponent(a1->parent3);
  if (fc) {
    auto rtti = RED4ext::CRTTISystem::Get();
    auto fcc = rtti->GetClass("FlightComponent");
    auto activeProp = fcc->GetProperty("active");
    //auto rollProp = fcc->GetProperty("roll")
    ;
    if (activeProp->GetValue<bool>(fc)) {
      //a1->parent3->turnInput = rollProp->GetValue<float>(fc);
      a1->parent3->turnInput = 0.0;
      a1->turnRate = 0.0;
      a1->tiltControlEnabled = 0;
    } else {
      a1->tiltControlEnabled = 1;
    }
  }
  auto og = BikeAnimationUpdate_Original(a1);
  //UpdateAnimValueForCName(a1->parent3, "throttle", 0.0);
  return og;
}

struct VehiclePhysicsUpdateModule : FlightModule {
  void Load(const RED4ext::Sdk *aSdk, RED4ext::PluginHandle aHandle) {
    while (!aSdk->hooking->Attach(aHandle, RED4EXT_OFFSET_TO_ADDR(VehiclePhysicsUpdateAddr), &VehiclePhysicsUpdate,
                          reinterpret_cast<void **>(&VehiclePhysicsUpdate_Original)));
    while (!aSdk->hooking->Attach(aHandle, RED4EXT_OFFSET_TO_ADDR(ProcessAirResistanceAddr), &ProcessAirResistance,
                          reinterpret_cast<void **>(&ProcessAirResistance_Original)));
    while (!aSdk->hooking->Attach(aHandle, RED4EXT_OFFSET_TO_ADDR(VehicleHelperUpdateAddr), &VehicleHelperUpdate,
                          reinterpret_cast<void **>(&VehicleHelperUpdate_Original)));
    while (!aSdk->hooking->Attach(aHandle, RED4EXT_OFFSET_TO_ADDR(AirControlProcessAddr), &AirControlProcess,
                                  reinterpret_cast<void **>(&AirControlProcess_Original)));
    while (!aSdk->hooking->Attach(aHandle, RED4EXT_OFFSET_TO_ADDR(TorqueUpdateAddr), &TorqueUpdate,
                                  reinterpret_cast<void **>(&TorqueUpdate_Original)));
    while (!aSdk->hooking->Attach(aHandle, RED4EXT_OFFSET_TO_ADDR(VehicleUpdateOrientationWithPIDAddr),
                                  &VehicleUpdateOrientationWithPID,
                                  reinterpret_cast<void **>(&VehicleUpdateOrientationWithPID_Original)));
    while (!aSdk->hooking->Attach(aHandle, RED4EXT_OFFSET_TO_ADDR(AnimationUpdateAddr), &AnimationUpdate,
                                  reinterpret_cast<void **>(&AnimationUpdate_Original)));
    while (!aSdk->hooking->Attach(aHandle, RED4EXT_OFFSET_TO_ADDR(RED4ext::vehicle::BikePhysics::AnimationUpdateAddr), &BikeAnimationUpdate,
                                  reinterpret_cast<void **>(&BikeAnimationUpdate_Original)));
  }
  void Unload(const RED4ext::Sdk *aSdk, RED4ext::PluginHandle aHandle) {
    aSdk->hooking->Detach(aHandle, RED4EXT_OFFSET_TO_ADDR(ProcessAirResistanceAddr));
    aSdk->hooking->Detach(aHandle, RED4EXT_OFFSET_TO_ADDR(VehiclePhysicsUpdateAddr));
    aSdk->hooking->Detach(aHandle, RED4EXT_OFFSET_TO_ADDR(VehicleHelperUpdateAddr));
    aSdk->hooking->Detach(aHandle, RED4EXT_OFFSET_TO_ADDR(AirControlProcessAddr));
    aSdk->hooking->Detach(aHandle, RED4EXT_OFFSET_TO_ADDR(TorqueUpdateAddr));
    aSdk->hooking->Detach(aHandle, RED4EXT_OFFSET_TO_ADDR(VehicleUpdateOrientationWithPIDAddr));
    aSdk->hooking->Detach(aHandle, RED4EXT_OFFSET_TO_ADDR(AnimationUpdateAddr));
    aSdk->hooking->Detach(aHandle, RED4EXT_OFFSET_TO_ADDR(RED4ext::vehicle::BikePhysics::AnimationUpdateAddr));
  }
};

REGISTER_FLIGHT_MODULE(VehiclePhysicsUpdateModule);