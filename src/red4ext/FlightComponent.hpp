#pragma once

#include "stdafx.hpp"
#include "Engine/RTTIClass.hpp"
#include <RED4ext/RED4ext.hpp>
#include <RED4ext/NativeTypes.hpp>
#include <RED4ext/Scripting/Natives/Generated/game/DeviceComponent.hpp>
#include <RED4ext/Scripting/Natives/Generated/Vector4.hpp>
#include <RED4ext/Scripting/Natives/gamePSInterface.hpp>
#include "FlightSystem.hpp"

//struct FlightSystem;

class FlightComponent : public Engine::RTTIClass<FlightComponent, RED4ext::game::Component> {
public:
  //virtual ~FlightComponent() override = default;
  //virtual ~PSInterface() override = default;
  virtual RED4ext::CClass* GetPSClass() override;

  // has scripted callbacks?
  inline virtual bool sub_240() override {
    return true;
  }

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

  RED4ext::Handle<FlightSystem> sys;
  bool active;
  bool hasUpdate;
  alignas(0x10) RED4ext::Vector4 force;
  RED4ext::Vector4 torque;

private:
  friend Descriptor;

  static void OnDescribe(Descriptor *aType, RED4ext::CRTTISystem *) {
    //aType->AddFunction<&FlightComponent::UpdateListenerMatrix>("UpdateListenerMatrix");
  }
};
RED4EXT_ASSERT_OFFSET(FlightComponent, sys, 0xA8);
RED4EXT_ASSERT_OFFSET(FlightComponent, active, 0xB8);
RED4EXT_ASSERT_OFFSET(FlightComponent, hasUpdate, 0xB9);
RED4EXT_ASSERT_OFFSET(FlightComponent, force, 0xC0);
RED4EXT_ASSERT_OFFSET(FlightComponent, torque, 0xD0);
//char (*__kaboom)[offsetof(FlightComponent, force] = 1;