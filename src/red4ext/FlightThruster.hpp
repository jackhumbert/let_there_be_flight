#pragma once

#include <RED4ext/RED4ext.hpp>
#include "Engine/RTTIClass.hpp"
#include "FlightComponent.hpp"
#include <RED4ext/Scripting/Natives/Generated/ent/MeshComponent.hpp>

struct FlightComponent;

class IFlightThruster : public Engine::RTTIClass<IFlightThruster, RED4ext::IScriptable> {
public:
  RED4ext::Handle<FlightComponent> flightComponent;
  RED4ext::CName boneName;
  RED4ext::CName slotName;
  RED4ext::Vector3 relativePosition = RED4ext::Vector3(0.0, 0.0, 0.0);
  alignas(0x10) RED4ext::Quaternion relativeRotation = RED4ext::Quaternion(0.0, 0.0, 0.0, 1.0);
  RED4ext::CName meshPath = "user\\jackhumbert\\meshes\\engine_corpo.mesh";
  RED4ext::CName meshName = "Thruster";
  RED4ext::Handle<RED4ext::ent::MeshComponent> meshComponent;

private:
  friend Descriptor;
  static void OnRegister(Descriptor *aType) {
    aType->flags.isAbstract = true;
    // aType->flags.b20000 = true;
    // aType->flags.isAlwaysTransient = true;
  }

  static void OnDescribe(Descriptor *aType, RED4ext::CRTTISystem *) {}
};
RED4EXT_ASSERT_OFFSET(IFlightThruster, flightComponent, 0x40);
RED4EXT_ASSERT_OFFSET(IFlightThruster, boneName, 0x50);
RED4EXT_ASSERT_OFFSET(IFlightThruster, slotName, 0x58);
RED4EXT_ASSERT_OFFSET(IFlightThruster, relativePosition, 0x60);
RED4EXT_ASSERT_OFFSET(IFlightThruster, relativeRotation, 0x70);
RED4EXT_ASSERT_OFFSET(IFlightThruster, meshPath, 0x80);
RED4EXT_ASSERT_OFFSET(IFlightThruster, meshName, 0x88);
RED4EXT_ASSERT_OFFSET(IFlightThruster, meshComponent, 0x90);