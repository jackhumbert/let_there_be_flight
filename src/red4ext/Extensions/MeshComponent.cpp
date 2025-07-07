#include "MeshComponent.hpp"
#include <PhysX3.hpp>
#include <RED4ext/Scripting/Natives/Generated/ent/HardTransformBinding.hpp>
#include <RED4ext/Scripting/Natives/Generated/ent/SlotComponent.hpp>
#include <RED4ext/Scripting/Natives/Generated/ent/VisualControllerComponent.hpp>
#include <RED4ext/Scripting/Natives/entEffectSpawnerComponent.hpp>
#include <RED4ext/Scripting/Natives/Generated/ent/EffectDesc.hpp>
#include <RED4ext/Scripting/Natives/Generated/physics/ColliderSphere.hpp>
#include <RED4ext/Scripting/Natives/Generated/physics/QueryFilter.hpp>
#include <RED4ext/Scripting/Natives/Generated/physics/SimulationFilter.hpp>
#include "FlightConfiguration.hpp"

void MeshComponent::SetMesh(RED4ext::ResRef mesh) {
	this->mesh.path = mesh.resource.path;
}

void IPlacedComponent::SetParentTransform(RED4ext::CName bindName, RED4ext::CName slotName) {
  auto rtti = RED4ext::CRTTISystem::Get();
  auto htb = (RED4ext::ent::HardTransformBinding *)rtti->GetClass("entHardTransformBinding")->CreateInstance(true);
  htb->bindName = bindName;
  htb->slotName = slotName;
  this->parentTransform = RED4ext::Handle<RED4ext::ent::ITransformBinding>(htb);
}

void Entity::AddComponent(RED4ext::Handle<RED4ext::ent::IComponent> const & componentToAdd) {
  RED4ext::ent::VisualControllerComponent *vcc = nullptr;
  RED4ext::ent::EffectSpawnerComponent *customization = nullptr;
  auto rtti = RED4ext::CRTTISystem::Get();
  auto vccClass = rtti->GetClass("entVisualControllerComponent");
  auto effectSpawnerClass = rtti->GetClass("entEffectSpawnerComponent");

  for (auto const &component : this->componentsStorage.components) {
    // auto component = handle.GetPtr();
    if (component->GetNativeType() == vccClass) {
      vcc = reinterpret_cast<RED4ext::ent::VisualControllerComponent *>(component.instance);
      break;
    }
  }

  for (auto const &component : this->componentsStorage.components) {
    // auto component = handle.GetPtr();
    if (component->GetNativeType() == effectSpawnerClass && component->name == "vehicleVisualCustomization") {
      customization = reinterpret_cast<RED4ext::ent::EffectSpawnerComponent *>(component.instance);
      break;
    }
  }

  if (componentToAdd->GetNativeType() == rtti->GetClass("entMeshComponent") || componentToAdd->GetNativeType() == rtti->GetClass("entPhysicalMeshComponent")) {
    if (vcc != NULL) {
        auto meshComponent = (RED4ext::ent::MeshComponent *)componentToAdd.instance;
        meshComponent->appearanceName = meshComponent->meshAppearance;
        auto vcd = reinterpret_cast<RED4ext::ent::VisualControllerDependency *>(
            rtti->GetClass("entVisualControllerDependency")->CreateInstance(true));
        vcd->appearanceName = meshComponent->meshAppearance;
        vcd->componentName = meshComponent->name;
        vcd->mesh.path = meshComponent->mesh.path;
        vcc->appearanceDependency.EmplaceBack(*vcd);

        if (vcc->resourcePaths.size) {
          for (int i = 0; i < vcc->resourcePaths.size; i++) {
            if (vcc->resourcePaths[i] == meshComponent->mesh.path) {
              break;
            } else if (vcc->resourcePaths[i] > meshComponent->mesh.path) {
              vcc->resourcePaths.Emplace(&vcc->resourcePaths[i], meshComponent->mesh.path);
              break;
            }
          }
        } else {
          vcc->resourcePaths.EmplaceBack(meshComponent->mesh.path);
        }
      }
    if (customization != NULL) {
      for (auto const & desc : customization->effectDescs) {
        if (desc->effectName == "vvc_color_instant") {
          desc->compiledEffectInfo.componentNames.PushBack(componentToAdd->name);
          for (auto & event : desc->compiledEffectInfo.eventsSortedByRUID) {
            if (event.componentIndexMask & 0x2)
              event.componentIndexMask |= (1ULL << (desc->compiledEffectInfo.componentNames.size - 1));
          }
        }
        if (desc->effectName == "vvc_color") {
          desc->compiledEffectInfo.componentNames.PushBack(componentToAdd->name);
          for (auto & event : desc->compiledEffectInfo.eventsSortedByRUID) {
            if (event.componentIndexMask & 0x2)
              event.componentIndexMask |= (1ULL << (desc->compiledEffectInfo.componentNames.size - 1));
          }
        }
      }
    }
  }

  if (componentToAdd->GetNativeType() == rtti->GetClass("entPhysicalMeshComponent")) {
    auto pmComponent = (RED4ext::ent::PhysicalMeshComponent *)componentToAdd.instance;
    
    auto filterData = (RED4ext::physics::FilterData*)malloc(sizeof(RED4ext::physics::FilterData));
    RED4ext::physics::FilterData::Init(filterData);

    pmComponent->filterData = RED4ext::Handle<RED4ext::physics::FilterData>(filterData);
    pmComponent->filterDataSource = FilterDataSource::Collider;
  }

  this->componentsStorage.components.PushBack(componentToAdd);
}

//RED4ext::Handle<RED4ext::physics::ColliderSphere> * createSphereColliderHandleWithRadius(RED4ext::Handle<RED4ext::physics::ICollider> *handle,
//                                                         float radius) {
//  RED4ext::RelocFunc<decltype(&RED4ext::physics::ColliderSphere::createHandleWithRadius)> call(
//      physicsColliderSphere_createHandleWithRadius_Addr);
//  return call(handle, radius);
//}

void Entity::AddSlot(RED4ext::CName boneName, RED4ext::CName slotName, RED4ext::Vector3 relativePosition, RED4ext::Quaternion relativeRotation) {
  RED4ext::ent::SlotComponent *slotComponent = nullptr;
  auto rtti = RED4ext::CRTTISystem::Get();

  for (auto const &handle : this->componentsStorage.components) {
    auto component = handle.GetPtr();
    if (component->GetNativeType() == rtti->GetClass("entSlotComponent")) {
      if (component->name == "vehicle_slots") {
        slotComponent = reinterpret_cast<RED4ext::ent::SlotComponent *>(component);
        break;
      }
    }
  }

  if (slotComponent != nullptr) {
    auto slot = reinterpret_cast<RED4ext::ent::Slot *>(rtti->GetClass("entSlot")->CreateInstance(true));
    slot->boneName = boneName;
    slot->slotName = slotName;
    slot->relativePosition = relativePosition;
    slot->relativeRotation = relativeRotation;
    slotComponent->slots.EmplaceBack(*slot);
    slotComponent->slotIndexLookup.Emplace(slot->slotName, slotComponent->slots.size - 1);
  }
}

void Entity::OnExpand(Descriptor *aType, RED4ext::CRTTISystem * _) {
  aType->AddFunction<&Entity::AddComponent>("AddComponent");
  aType->AddFunction<&Entity::AddSlot>("AddSlot");
}