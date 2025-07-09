#pragma once

#include "Engine/RTTIExpansion.hpp"
#include <RED4ext/Scripting/Natives/Generated/ent/IPlacedComponent.hpp>
#include <RED4ext/Scripting/Natives/Generated/ent/MeshComponent.hpp>
#include <RED4ext/Scripting/Natives/Generated/red/ResourceReferenceScriptToken.hpp>
#include <RED4ext/Scripting/Natives/vehicleChassisComponent.hpp>
#include <RED4ext/Scripting/Natives/entEntity.hpp>
//#include "FlightConfiguration.hpp"

class MeshComponentExt : public Engine::RTTIExpansion<MeshComponentExt, RED4ext::ent::MeshComponent> {
public:
  void SetMesh(RED4ext::ResRef mesh);

private:
	friend Descriptor;

  inline static void OnExpand(Descriptor *aType, RED4ext::CRTTISystem *) {
    aType->AddFunction<&MeshComponentExt::SetMesh>("SetMesh");
  }
};


class IPlacedComponentExt : public Engine::RTTIExpansion<IPlacedComponentExt, RED4ext::ent::IPlacedComponent> {
public:
  void SetParentTransform(RED4ext::CName bindName, RED4ext::CName slotName);

private:
  friend Descriptor;

  inline static void OnExpand(Descriptor *aType, RED4ext::CRTTISystem *) {
    aType->AddFunction<&IPlacedComponentExt::SetParentTransform>("SetParentTransform");
  }
};

class EntityExt : public Engine::RTTIExpansion<EntityExt, RED4ext::ent::Entity> {
public:
  void AddComponent(RED4ext::Handle<RED4ext::ent::IComponent> const & component);
  void AddSlot(RED4ext::CName boneName, RED4ext::CName slotName, RED4ext::Vector3 relativePosition, RED4ext::Quaternion relativeRotation);
private:
  friend Descriptor;

  static void OnExpand(Descriptor *aType, RED4ext::CRTTISystem *);
};
