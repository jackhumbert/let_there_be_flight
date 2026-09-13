#pragma once

#include "Engine/RTTIExpansion.hpp"
#include <RED4ext/Scripting/Natives/Generated/ent/IPlacedComponent.hpp>
#include <RED4ext/Scripting/Natives/Generated/ent/MeshComponent.hpp>
#include <RED4ext/Scripting/Natives/Generated/red/ResourceReferenceScriptToken.hpp>
#include <RED4ext/Scripting/Natives/vehicleChassisComponent.hpp>
#include <RED4ext/Scripting/Natives/entEntity.hpp>
#include "Utils/Utils.hpp"
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

  // Null-safe variant of ent::Entity::GetComponent (the SDK version dereferences
  // every handle in componentsStorage without checking it, and trusts the
  // componentsStorage DynArray header).
  template <class T>
  RED4ext::Handle<T> FindComponent(RED4ext::CName name = RED4ext::CName()) {
    RED4ext::Handle<T> result;
    if (!Utils::LooksLikeDynArray(this->componentsStorage.components)) {
      LTBF_WARN_ONCE("[EntityExt] componentsStorage.components header looks corrupt (size {} capacity {}); skipping component lookup",
                     this->componentsStorage.components.size, this->componentsStorage.components.capacity);
      return result;
    }
    auto cls = RED4ext::CRTTISystem::Get()->GetClass(T::NAME);
    if (!cls) {
      return result;
    }
    for (auto const &handle : this->componentsStorage.components) {
      auto component = handle.GetPtr();
      if (!component || !handle.refCount) {
        continue;
      }
      if (component->GetNativeType() != cls) {
        continue;
      }
      if (name.hash != 0 && component->name != name) {
        continue;
      }
      result.instance = reinterpret_cast<T *>(component);
      result.refCount = handle.refCount;
      result.refCount->IncRef();
      break;
    }
    return result;
  }

  static EntityExt *From(RED4ext::ent::Entity *entity) { return reinterpret_cast<EntityExt *>(entity); }
private:
  friend Descriptor;

  static void OnExpand(Descriptor *aType, RED4ext::CRTTISystem *);
};
