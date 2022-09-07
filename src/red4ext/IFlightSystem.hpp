#pragma once

#include "Engine/RTTIClass.hpp"
#include "FlightAudio.hpp"
#include <RED4ext/RED4ext.hpp>
#include <RED4ext/Scripting/Natives/Generated/ent/IPlacedComponent.hpp>
#include <RED4ext/Scripting/Natives/gameIGameSystem.hpp>
//#include "FlightComponent.hpp"

struct FlightComponent;

class IFlightSystem : public Engine::RTTIClass<IFlightSystem, RED4ext::game::IGameSystem> {
public:
private:
  friend Descriptor;
  static void OnRegister(Descriptor *aType) {
    aType->flags = {.isAbstract = true, .isNative = true, .isImportOnly = true};
  }

  static void OnDescribe(Descriptor *aType, RED4ext::CRTTISystem *) {}
};