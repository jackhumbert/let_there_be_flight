#include "FlightModule.hpp"
#include <RED4ext/Scripting/Natives/Generated/physics/VehiclePhysics.hpp>
#include <RED4ext/Scripting/Natives/Generated/vehicle/BaseObject.hpp>
#include <RED4ext/RED4ext.hpp>
#include <spdlog/spdlog.h>

uintptr_t __fastcall VehiclePhysicsUpdate(RED4ext::physics::VehiclePhysics *, float);

// F3 0F 11 4C 24 10 55 53  57 41 54 41 55 41 56 48 8D AC 24 98 FD FF FF 48 81 EC 68 03 00 00 48 8B
constexpr uintptr_t VehiclePhysicsUpdateAddr = 0x141D0FE70 - RED4ext::Addresses::ImageBase;
decltype(&VehiclePhysicsUpdate) VehiclePhysicsUpdate_Original;

uintptr_t __fastcall VehiclePhysicsUpdate(RED4ext::physics::VehiclePhysics *p, float deltaTime) {
  // spdlog::info(a2->c_str());
  auto rtti = RED4ext::CRTTISystem::Get();
  auto fcc = rtti->GetClass("FlightComponent");
  for (auto const &c : p->parent->components) {
    if (c.GetPtr()->GetType() == fcc) {
      auto activeProp = fcc->GetProperty("active");
      if (activeProp->GetValue<bool>(c) && p->parent->physicsStruct) {
        auto onUpdate = fcc->GetFunction("OnUpdate");
        auto args = RED4ext::CStackType(rtti->GetType("Float"), &deltaTime);
        auto stack = RED4ext::CStack(c, &args, 1, nullptr, 0);
        onUpdate->Execute(&stack);
        auto forceProp = fcc->GetProperty("force");
        auto force = forceProp->GetValue<RED4ext::Vector4>(c);
        auto torqueProp = fcc->GetProperty("torque");
        auto torque = torqueProp->GetValue<RED4ext::Vector4>(c);

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

        forceProp->SetValue(c, force);
        torqueProp->SetValue(c, torque);
      }
      break;
    }
  }
  return VehiclePhysicsUpdate_Original(p, deltaTime);
}

struct VehiclePhysicsUpdateModule : FlightModule {
  void Load(const RED4ext::Sdk *aSdk, RED4ext::PluginHandle aHandle) {
    aSdk->hooking->Attach(aHandle, RED4EXT_OFFSET_TO_ADDR(VehiclePhysicsUpdateAddr), &VehiclePhysicsUpdate,
                          reinterpret_cast<void **>(&VehiclePhysicsUpdate_Original));
  }
  void Unload(const RED4ext::Sdk *aSdk, RED4ext::PluginHandle aHandle) {
    aSdk->hooking->Detach(aHandle, RED4EXT_OFFSET_TO_ADDR(VehiclePhysicsUpdateAddr));
  }
};

REGISTER_FLIGHT_MODULE(VehiclePhysicsUpdateModule);