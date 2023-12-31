#pragma once

#include "Engine/RTTIClass.hpp"
#include "Audio/FlightAudio.hpp"
#include <RED4ext/Common.hpp>
#include <RED4ext/Scripting/Natives/Generated/ent/IPlacedComponent.hpp>
#include <RED4ext/Scripting/Natives/gameIGameSystem.hpp>
//#include "Flight/Component.hpp"

struct FlightComponent;

class IFlightSystem : public Engine::RTTIClass<IFlightSystem, RED4ext::game::IGameSystem> {
public:
  virtual void RegisterComponent(RED4ext::WeakHandle<FlightComponent>) { };
  virtual void UnregisterComponent(RED4ext::WeakHandle<FlightComponent>) { };
private:
  friend Descriptor;
  static void OnRegister(Descriptor *aType) {
    aType->flags = {.isAbstract = true, .isNative = true, .isImportOnly = true};
  }

  static void OnDescribe(Descriptor *aType, RED4ext::CRTTISystem *) {}
};