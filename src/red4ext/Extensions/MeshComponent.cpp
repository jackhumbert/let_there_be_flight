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
#include <RED4ext/Scripting/Natives/Generated/game/data/VehicleDestruction_Record.hpp>
#include <RED4ext/Scripting/Natives/Generated/game/data/VehicleDetachablePart_Record.hpp>
#include "FlightConfiguration.hpp"

using namespace RED4ext;

void MeshComponentExt::SetMesh(ResRef mesh) {
	this->mesh.path = mesh.resource.path;
}

void IPlacedComponentExt::SetParentTransform(CName bindName, CName slotName) {
  auto rtti = CRTTISystem::Get();
  auto htb = (ent::HardTransformBinding *)rtti->GetClass("entHardTransformBinding")->CreateInstance(true);
  htb->bindName = bindName;
  htb->slotName = slotName;
  this->parentTransform = Handle<ent::ITransformBinding>(htb);
}

void EntityExt::AddComponent(Handle<ent::IComponent> const & componentToAdd) {
  componentToAdd->id = CRUID::Next();

  auto rtti = CRTTISystem::Get();
  
  auto vcc = this->GetComponent<ent::VisualControllerComponent>();
  auto customization = this->GetComponent<ent::EffectSpawnerComponent>("vehicleVisualCustomization");

  if (componentToAdd->IsOfClass(rtti->GetClass("entMeshComponent"))) {

    if (vcc != NULL) {
        auto meshComponent = (ent::MeshComponent *)componentToAdd.instance;
        meshComponent->appearanceName = meshComponent->meshAppearance;

        auto vcd = reinterpret_cast<ent::VisualControllerDependency *>(
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

  if (componentToAdd->IsOfClass(rtti->GetClass("entPhysicalMeshComponent"))) {
    auto pmComponent = (ent::PhysicalMeshComponent *)componentToAdd.instance;
    
    auto filterData = (physics::FilterData*)rtti->GetClass("physicsFilterData")->CreateInstance(true);

    pmComponent->filterData = Handle<physics::FilterData>(filterData);
    pmComponent->filterDataSource = FilterDataSource::Collider;
  }

  // if (this->IsOfClass(rtti->GetClass("vehicleBaseObject"))) {
  //   auto vehicle = reinterpret_cast<vehicle::BaseObject*>(this);
  //   auto tweakDB = TweakDB::Get();
  //   auto vehicleRecord = reinterpret_cast<game::data::Vehicle_Record*>(vehicle->GetRecord());
  //   auto destructionRecord = reinterpret_cast<game::data::VehicleDestruction_Record*>(tweakDB->GetRecord(TweakDBID(vehicleRecord->recordID, ".destruction")).instance);
  //   auto detachableParts = tweakDB->GetValue<DynArray<WeakHandle<game::data::VehicleDetachablePart_Record>>>(TweakDBID(destructionRecord->recordID, ".detachableParts"));
  //   // TweakDBID()
  //   auto stack = CStackType(rtti->GetClass("components"), );
  //   tweakDB->AddFlat();
  // }
  

  this->componentsStorage.components.PushBack(componentToAdd);
}

//Handle<physics::ColliderSphere> * createSphereColliderHandleWithRadius(Handle<physics::ICollider> *handle,
//                                                         float radius) {
//  RelocFunc<decltype(&physics::ColliderSphere::createHandleWithRadius)> call(
//      physicsColliderSphere_createHandleWithRadius_Addr);
//  return call(handle, radius);
//}

void EntityExt::AddSlot(CName boneName, CName slotName, Vector3 relativePosition, Quaternion relativeRotation) {
  // ent::SlotComponent *slotComponent = nullptr;
  auto rtti = CRTTISystem::Get();

  // for (auto const &handle : this->componentsStorage.components) {
  //   auto component = handle.GetPtr();
  //   if (component->GetNativeType() == rtti->GetClass("entSlotComponent")) {
  //     if (component->name == "vehicle_slots") {
  //       slotComponent = reinterpret_cast<ent::SlotComponent *>(component);
  //       break;
  //     }
  //   }
  // }

  auto slotComponent = this->GetComponent<ent::SlotComponent>("vehicle_slots");

  if (slotComponent != nullptr) {
    auto slot = rtti->GetClass("entSlot")->CreateInstance<ent::Slot *>(true);
    slot->boneName = boneName;
    slot->slotName = slotName;
    slot->relativePosition = relativePosition;
    slot->relativeRotation = relativeRotation;
    slotComponent->slots.EmplaceBack(*slot);
    slotComponent->slotIndexLookup.Emplace(slot->slotName, slotComponent->slots.size - 1);
  }
}

void EntityExt::OnExpand(Descriptor *aType, CRTTISystem * _) {
  // aType->AddFunction<&EntityExt::AddComponent>("AddComponent");
  aType->AddFunction<&EntityExt::AddSlot>("AddSlot");
}