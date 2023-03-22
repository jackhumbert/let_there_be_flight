#include "MeshComponent.hpp"
#include <PhysX3.hpp>
#include <RED4ext/Scripting/Natives/Generated/ent/HardTransformBinding.hpp>
#include <RED4ext/Scripting/Natives/Generated/ent/SlotComponent.hpp>
#include <RED4ext/Scripting/Natives/Generated/ent/VisualControllerComponent.hpp>
#include <RED4ext/Scripting/Natives/Generated/physics/ColliderSphere.hpp>
#include <RED4ext/Scripting/Natives/Generated/physics/QueryFilter.hpp>
#include <RED4ext/Scripting/Natives/Generated/physics/SimulationFilter.hpp>
#include "FlightConfiguration.hpp"

void MeshComponent::SetMesh(RED4ext::ResRef mesh) {
	this->mesh.path = mesh.resource.path;
}

void IPlacedComponent::SetParentTransform(RED4ext::CName bindName, RED4ext::CName slotName) {
  auto rtti = RED4ext::CRTTISystem::Get();
  auto htb = (RED4ext::ent::HardTransformBinding *)rtti->GetClass("entHardTransformBinding")->CreateInstance();
  htb->bindName = bindName;
  htb->slotName = slotName;
  this->parentTransform = RED4ext::Handle<RED4ext::ent::ITransformBinding>(htb);
}

void Entity::AddComponent(RED4ext::Handle<RED4ext::ent::IComponent> componentToAdd) {
  RED4ext::ent::VisualControllerComponent *vcc = NULL;
  auto rtti = RED4ext::CRTTISystem::Get();
  auto vccClass =  rtti->GetClass("entVisualControllerComponent");

  for (auto const &handle : this->componentsStorage.components) {
    auto component = handle.GetPtr();
    if (component->GetNativeType() == vccClass) {
      vcc = reinterpret_cast<RED4ext::ent::VisualControllerComponent *>(component);
      break;
    }
  }

  if (vcc != NULL) {
    if (componentToAdd->GetNativeType() == rtti->GetClass("entMeshComponent")) {
      auto meshComponent = (RED4ext::ent::MeshComponent *)componentToAdd.instance;
      this->componentsStorage.components.EmplaceBack(componentToAdd);
      auto vcd = reinterpret_cast<RED4ext::ent::VisualControllerDependency *>(
          rtti->GetClass("entVisualControllerDependency")->CreateInstance());
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
  }
}

//RED4ext::Handle<RED4ext::physics::ColliderSphere> * createSphereColliderHandleWithRadius(RED4ext::Handle<RED4ext::physics::ICollider> *handle,
//                                                         float radius) {
//  RED4ext::RelocFunc<decltype(&RED4ext::physics::ColliderSphere::createHandleWithRadius)> call(
//      physicsColliderSphere_createHandleWithRadius_Addr);
//  return call(handle, radius);
//}

void Entity::AddSlot(RED4ext::CName boneName, RED4ext::CName slotName, RED4ext::Vector3 relativePosition, RED4ext::Quaternion relativeRotation) {
  RED4ext::ent::SlotComponent *slotComponent = NULL;
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

  if (slotComponent != NULL) {
    auto slot = reinterpret_cast<RED4ext::ent::Slot *>(rtti->GetClass("entSlot")->CreateInstance());
    slot->boneName = boneName;
    slot->slotName = slotName;
    slot->relativePosition = relativePosition;
    slot->relativeRotation = relativeRotation;
    slotComponent->slots.EmplaceBack(*slot);
    slotComponent->slotIndexLookup.Emplace(slot->slotName, slotComponent->slots.size - 1);
  }
}