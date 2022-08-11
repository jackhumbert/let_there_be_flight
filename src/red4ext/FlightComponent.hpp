#pragma once

#include "Engine/RTTIClass.hpp"
#include <RED4ext/RED4ext.hpp>
#include <RED4ext/Scripting/Natives/Generated/game/DeviceComponent.hpp>
#include <RED4ext/Scripting/Natives/Generated/Vector4.hpp>

struct FlightSystem;

class FlightComponent : public Engine::RTTIClass<FlightComponent, RED4ext::game::DeviceComponent> {
public:
  virtual uint64_t PS_Destruct(char a1) override;
  virtual uint64_t PS_sub_08() override;

  RED4ext::Handle<FlightSystem> sys;
  bool active;
  bool hasUpdate;
  RED4ext::Vector4 force;
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
RED4EXT_ASSERT_OFFSET(FlightComponent, force, 0xBC);
RED4EXT_ASSERT_OFFSET(FlightComponent, torque, 0xCC);
//char (*__kaboom)[offsetof(FlightComponent, force] = 1;