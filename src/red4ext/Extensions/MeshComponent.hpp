#pragma once

#include "Engine/RTTIExpansion.hpp"
#include <RED4ext/Scripting/Natives/Generated/ent/IPlacedComponent.hpp>
#include <RED4ext/Scripting/Natives/Generated/ent/MeshComponent.hpp>
#include <RED4ext/Scripting/Natives/Generated/red/ResourceReferenceScriptToken.hpp>
#include <RED4ext/Scripting/Natives/vehicleChassisComponent.hpp>
//#include "FlightConfiguration.hpp"

class MeshComponent : public Engine::RTTIExpansion<MeshComponent, RED4ext::ent::MeshComponent> {
public:
  void SetMesh(RED4ext::ResRef mesh);

private:
	friend Descriptor;

  inline static void OnExpand(Descriptor *aType, RED4ext::CRTTISystem *) {
    aType->AddFunction<&MeshComponent::SetMesh>("SetMesh");
  }
};


class IPlacedComponent : public Engine::RTTIExpansion<IPlacedComponent, RED4ext::ent::IPlacedComponent> {
public:
  void SetParentTransform(RED4ext::CName bindName, RED4ext::CName slotName);

private:
  friend Descriptor;

  inline static void OnExpand(Descriptor *aType, RED4ext::CRTTISystem *) {
    aType->AddFunction<&IPlacedComponent::SetParentTransform>("SetParentTransform");
  }
};

/// Entity
class Entity : public Engine::RTTIExpansion<Entity, RED4ext::ent::Entity> {
public:
  /// AddComponent
  void AddComponent(RED4ext::Handle<RED4ext::ent::IComponent> component);
  /// AddSlot
  void AddSlot(RED4ext::CName boneName, RED4ext::CName slotName, RED4ext::Vector3 relativePosition,
                       RED4ext::Quaternion relativeRotation);
  private:
  friend Descriptor;

  inline static void OnExpand(Descriptor *aType, RED4ext::CRTTISystem *) {
    aType->AddFunction<&Entity::AddComponent>("AddComponent");
    aType->AddFunction<&Entity::AddSlot>("AddSlot");
  }
};
