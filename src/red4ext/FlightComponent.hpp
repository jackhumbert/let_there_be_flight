#pragma once

#include "stdafx.hpp"
#include "Engine/RTTIClass.hpp"
#include <RED4ext/RED4ext.hpp>
#include <RED4ext/NativeTypes.hpp>
#include <RED4ext/Scripting/Natives/Generated/game/Component.hpp>
#include <RED4ext/Scripting/Natives/Generated/game/ComponentPS.hpp>
#include <RED4ext/Scripting/Natives/Generated/Vector4.hpp>
#include <RED4ext/Scripting/Natives/gamePSInterface.hpp>
#include "IFlightSystem.hpp"
#include <RED4ext/Scripting/Natives/Generated/vehicle/BaseObject.hpp>
#include <RED4ext/Scripting/Natives/actionActionBase.hpp>
#include <RED4ext/Scripting/Natives/actionDriveChaseTarget.hpp>
#include <RED4ext/Scripting/Natives/Generated/vehicle/AutonomousData.hpp>

class FlightComponentPS : public Engine::RTTIClass<FlightComponentPS, RED4ext::game::ComponentPS> {
public:
private:
  friend Descriptor;
};

class FlightComponent : public Engine::RTTIClass<FlightComponent, RED4ext::game::Component> {
public:
  // 1.52 RVA: 0x1CA0980 / 30017920
  /// @pattern E9 0B 42 6E FF
  static constexpr const uintptr_t VehicleControllerAllocator = 0x1CA0980;
  inline virtual RED4ext::Memory::IAllocator* GetAllocator() {
    RED4ext::RelocFunc<decltype(&FlightComponent::GetAllocator)> call(VehicleControllerAllocator);
    return call(this);
  }

  //virtual ~FlightComponent() override = default;
  //virtual ~PSInterface() override = default;
  inline virtual RED4ext::CClass* GetPSClass() override {
    //return FlightComponentPS::GetRTTIType();
    return nullptr;
  }

  // has scripted callbacks?
  inline virtual bool sub_240() override {
    return true;
  }

  void ChaseTarget(RED4ext::WeakHandle<RED4ext::game::Object> target);

  //inline virtual void sub_188(void *a1) override {
  //  //spdlog::info("[FlightComponent] sub_188");
  //  //if (this) {
  //    //RED4ext::game::Component::sub_188(a1);
  //  //}
  //  this->unk88 |= 2u;
  //  this->ExecuteFunction("OnGameAttach");
  //}

  //inline virtual bool sub_198(void *a1) override {
  //  //spdlog::info("[FlightComponent] sub_198");
  //  //if (this) {
  //    //return RED4ext::game::Component::sub_198(a1);
  //  //}
  //  this->unk88 &= 0xFD;
  //  this->unk88 |= 8u;
  //  this->ExecuteFunction("OnGameDetach");
  //  return 0;
  //}

  RED4ext::Handle<IFlightSystem> sys;
  bool active;
  bool hasUpdate;
  alignas(0x10) RED4ext::Vector4 force;
  RED4ext::Vector4 torque;

private:
  friend Descriptor;

  static void OnDescribe(Descriptor *aType, RED4ext::CRTTISystem *) {
    aType->AddFunction<&FlightComponent::ChaseTarget>("ChaseTarget");
  }
};
RED4EXT_ASSERT_OFFSET(FlightComponent, sys, 0xA8);
RED4EXT_ASSERT_OFFSET(FlightComponent, active, 0xB8);
RED4EXT_ASSERT_OFFSET(FlightComponent, hasUpdate, 0xB9);
RED4EXT_ASSERT_OFFSET(FlightComponent, force, 0xC0);
RED4EXT_ASSERT_OFFSET(FlightComponent, torque, 0xD0);
//char (*__kaboom)[offsetof(FlightComponent, sys] = 1;