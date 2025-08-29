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
  auto htb = rtti->GetClass("entHardTransformBinding")->CreateInstance<ent::HardTransformBinding *>(true);
  htb->bindName = bindName;
  htb->slotName = slotName;
  this->parentTransform = Handle<ent::ITransformBinding>(htb);
}

void EntityExt::AddComponent(Handle<ent::IComponent> const & componentToAdd) {
  componentToAdd->id = CRUID::Next();

  this->componentsStorage.components.PushBack(componentToAdd);

  auto rtti = CRTTISystem::Get();
  
  auto const vcc = this->GetComponent<ent::VisualControllerComponent>();
  auto const customization = this->GetComponent<ent::EffectSpawnerComponent>("vehicleVisualCustomization");

  if (componentToAdd->IsOfClass(rtti->GetClass("entMeshComponent"))) {

    if (vcc) {
      auto meshComponent = (ent::MeshComponent *)componentToAdd.instance;
      meshComponent->appearanceName = meshComponent->meshAppearance;

      auto vcd = rtti->GetClass("entVisualControllerDependency")->CreateInstance<ent::VisualControllerDependency *>(true);
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

    // if (componentToAdd->IsOfClass(rtti->GetClass("entIVisualComponent"))) {
    // disable customization for now
      if (customization && false) {
        // customization->StopAllEffects(1);
        for (auto & desc : customization->effectDescs) {
          // desc->compiledEffectInfo.componentNames.Reserve(desc->compiledEffectInfo.componentNames.size + 1);
          // if (desc->compiledEffectInfo.componentNames.size == 16)
            // continue;
          if (desc->effectName == "vvc_color" || desc->effectName == "vvc_color_instant" || desc->effectName == "vvc_damage_glitch") {
          // if (desc->effectName == "vcc_color_rims" || desc->effectName == "vvc_color_rims_instant") {
            uint32_t body_index = 0;
            for (auto name : desc->compiledEffectInfo.componentNames) {
              auto str = std::string(name.ToString());
              if (str.find("body") == std::string::npos) {
                body_index++;
              } else {
                break;
              }
            }
            if (body_index != desc->compiledEffectInfo.componentNames.size) {
              uint64_t bitToCopy = (1ULL << body_index);
              uint64_t bitToAdd = (1ULL << desc->compiledEffectInfo.componentNames.size);
              desc->compiledEffectInfo.componentNames.PushBack(componentToAdd->name);
              for (auto & event : desc->compiledEffectInfo.eventsSortedByRUID) {
                if ((event.flags & 1) != 1)
                  continue;
                if ((event.componentIndexMask & bitToCopy) == bitToCopy)
                  event.componentIndexMask |= bitToAdd;
              }
            }
          }
        }
        // componentToAdd.refCount->IncWeakRef();
        // WeakHandle<ent::IVisualComponent> weakHandle;
        // weakHandle.instance = reinterpret_cast<ent::IVisualComponent*>(componentToAdd.instance);
        // weakHandle.refCount = componentToAdd.refCount;
        // weakHandle.refCount->IncWeakRef();
        // customization->components.Emplace(componentToAdd->name, weakHandle);
      }
    }
  // }

  if (componentToAdd->IsOfClass(rtti->GetClass("entPhysicalMeshComponent"))) {
    auto pmComponent = (ent::PhysicalMeshComponent *)componentToAdd.instance;
    
    auto filterData = rtti->GetClass("physicsFilterData")->CreateInstance<physics::FilterData *>(true);

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

  if (slotComponent) {
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