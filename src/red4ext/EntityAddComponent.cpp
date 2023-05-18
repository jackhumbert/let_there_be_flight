#include "EntityAddComponent.hpp"
#include "Flight/Component.hpp"
#include "FlightSystem.hpp"
#include "FlightWeapons.hpp"
#include "LoadResRef.hpp"
#include "Physics/VehiclePhysicsUpdate.hpp"
#include "Utils/FlightModule.hpp"
#include <RED4ext/Common.hpp>
#include <RED4ext/Scripting/Natives/Generated/WidgetHudComponent.hpp>
#include <RED4ext/Scripting/Natives/Generated/WorldWidgetComponent.hpp>
#include <RED4ext/Scripting/Natives/Generated/ent/Entity.hpp>
#include <RED4ext/Scripting/Natives/Generated/ent/EntityTemplate.hpp>
#include <RED4ext/Scripting/Natives/Generated/ent/HardTransformBinding.hpp>
#include <RED4ext/Scripting/Natives/Generated/ent/IComponent.hpp>
#include <RED4ext/Scripting/Natives/Generated/ent/IPlacedComponent.hpp>
#include <RED4ext/Scripting/Natives/Generated/ent/ITransformBinding.hpp>
#include <RED4ext/Scripting/Natives/Generated/ent/MeshComponent.hpp>
#include <RED4ext/Scripting/Natives/Generated/ent/PhysicalMeshComponent.hpp>
#include <RED4ext/Scripting/Natives/Generated/ent/PlaceholderComponent.hpp>
#include <RED4ext/Scripting/Natives/Generated/ent/SlotComponent.hpp>
#include <RED4ext/Scripting/Natives/Generated/ent/VisualControllerComponent.hpp>
#include <RED4ext/Scripting/Natives/Generated/game/AnimParamSlotsOption.hpp>
#include <RED4ext/Scripting/Natives/Generated/game/AttachmentSlots.hpp>
#include <RED4ext/Scripting/Natives/Generated/game/CameraComponent.hpp>
#include <RED4ext/Scripting/Natives/Generated/game/OccupantSlotComponent.hpp>
#include <RED4ext/Scripting/Natives/Generated/game/projectile/SpawnComponent.hpp>
#include <RED4ext/Scripting/Natives/Generated/ink/HudEntriesResource.hpp>
#include <RED4ext/Scripting/Natives/Generated/physics/ColliderBox.hpp>
#include <RED4ext/Scripting/Natives/Generated/physics/ColliderSphere.hpp>
#include <RED4ext/Scripting/Natives/Generated/physics/FilterData.hpp>
#include <RED4ext/Scripting/Natives/Generated/physics/SystemBody.hpp>
#include <RED4ext/Scripting/Natives/Generated/physics/SystemBodyParams.hpp>
#include <RED4ext/Scripting/Natives/Generated/physics/SystemResource.hpp>
#include <RED4ext/Scripting/Natives/Generated/vehicle/BaseObject.hpp>
#include <RED4ext/Scripting/Natives/Generated/world/ui/MeshTargetBinding.hpp>
#include <RED4ext/Scripting/Natives/vehicleChassisComponent.hpp>
#include <spdlog/spdlog.h>
#include <thread>

// weird bug fix

// 48 89 5C 24 08 48 89 74 24 10 48 89 7C 24 18 4C 89 64 24 20 55 41 56 41 57 48 8D 6C 24 C9 48 81
// void __fastcall SpawnEffect(uintptr_t, uintptr_t, uintptr_t, uintptr_t, uintptr_t, uintptr_t);
// constexpr uintptr_t SpawnEffect_Addr = 0x00007FF73F0D86B0 - 0x7ff73dfe0000;
// decltype(&SpawnEffect) SpawnEffect_Original;
//
// void __fastcall SpawnEffect(uintptr_t a1, uintptr_t a2, uintptr_t a3, uintptr_t a4, uintptr_t a5, uintptr_t a6) {
//  if (a1) {
//    SpawnEffect_Original(a1, a2, a3, a4, a5, a6);
//  }
//}

//int64_t RED4ext::ent::SlotComponent::GetSlotIndex(RED4ext::CName slotName) {
//  RelocFunc<decltype(&RED4ext::ent::SlotComponent::GetSlotIndex)> call(entSlotComponent_GetSlotIndex_Addr);
//  return call(this, slotName);
//}
//
//bool RED4ext::ent::SlotComponent::GetSlotLocalTransform(int slotIndex, RED4ext::WorldTransform *offset,
//                                                        RED4ext::WorldTransform *worldTransform) {
//  RelocFunc<decltype(&RED4ext::ent::SlotComponent::GetSlotLocalTransform)> call(
//      entSlotComponent_GetSlotLocalTransform_Addr);
//  return call(this, slotIndex, offset, worldTransform);
//}
//
//RED4ext::Handle<RED4ext::physics::ColliderSphere> *
//RED4ext::physics::ColliderSphere::createHandleWithRadius(RED4ext::Handle<RED4ext::physics::ICollider> *handle,
//                                                         float radius) {
//  RelocFunc<decltype(&RED4ext::physics::ColliderSphere::createHandleWithRadius)> call(
//      physicsColliderSphere_createHandleWithRadius_Addr);
//  return call(handle, radius);
//}
//
//
//bool __fastcall RED4ext::ent::SlotComponent::GetLocalSlotTransformFromIndex(int slotIndex, RED4ext::Transform *transform) {
//  RelocFunc<decltype(&RED4ext::ent::SlotComponent::GetLocalSlotTransformFromIndex)> call(
//      entSlotComponent_GetLocalSlotTransformFromIndex_Addr);
//  return call(this, slotIndex, transform);
//}

//REGISTER_FLIGHT_HOOK(uint32_t *, vehicle_ProcessPhysicalSystem, uint32_t *geoCacheId,
//                     RED4ext::vehicle::PhysicalSystemDesc *desc) {
//  auto collidersAdded = 0;
//  if (desc->type == RED4ext::vehicle::SystemType::PhysicalSystemDesc && desc->numHandles && desc->entity) {
//    auto rtti = RED4ext::CRTTISystem::Get();
//    auto type = desc->entity->GetNativeType();
//    bool isCar = false;
//    bool isBike = false;
//    auto isVehicle = false;
//    auto vehicleClass = rtti->GetClass("vehicleBaseObject");
//    auto carClass = rtti->GetClass("vehicleCarBaseObject");
//    auto bikeClass = rtti->GetClass("vehicleBikeBaseObject");
//    do {
//      isVehicle |= type == vehicleClass;
//      isCar |= type == carClass;
//      isBike |= type == bikeClass;
//    } while (type = type->parent);
//
//    if (isCar && false) {
//      auto fc = FlightComponent::Get((RED4ext::vehicle::BaseObject*)desc->entity.instance);
//      if (desc->resource) {
//        if (desc->resource->bodies.size &&
//            desc->resource->bodies[0]->params.simulationType == RED4ext::physics::SimulationType::Static) {
//          auto radius = 0.45;
//
//          RED4ext::ent::SlotComponent *sc = NULL;
//          RED4ext::vehicle::ChassisComponent *cc = NULL;
//
//          auto scCls = rtti->GetClass("entSlotComponent");
//          auto ccCls = rtti->GetClass("vehicleChassisComponent");
//
//          for (auto const &handle : desc->entity->componentsStorage.components) {
//            auto component = handle.GetPtr();
//            if (sc == NULL && component->GetNativeType() == scCls) {
//              if (component->name == "vehicle_slots") {
//                sc = reinterpret_cast<RED4ext::ent::SlotComponent *>(component);
//              }
//            } else if (cc == NULL && component->GetNativeType() == ccCls) {
//              cc = reinterpret_cast<RED4ext::vehicle::ChassisComponent *>(component);
//            }
//          }
//
//          if (sc == NULL || cc == NULL)
//            goto Original;
//
//          RED4ext::WorldTransform *wt;
//          int index = 0;
//          auto allocator = reinterpret_cast<RED4ext::Memory::IAllocator *>(desc->resource->bodies[0]->params.allocator);
//
//          auto ra = RED4ext::DynArray<RED4ext::Handle<RED4ext::physics::ICollider>>(allocator);
//          // doesn't work :/
//          // desc->resource->bodies[0]->params.angularDamping = 10.0;
//          const auto p_collisionShapes = &desc->resource->bodies[0]->collisionShapes;
//          auto originalShapes = *p_collisionShapes;
//
//          fc->configuration->originalShapeCount = p_collisionShapes->size;
//
//          ra.Reserve(p_collisionShapes->size + 4);
//          for (auto &shape : *p_collisionShapes) {
//            shape.refCount->IncRef();
//            ra.EmplaceBack(shape);
//          }
//
//          auto sphereCls = rtti->GetClass("physicsColliderSphere");
//          auto wtCls = rtti->GetClass("WorldTransform");
//
//          // auto shapeFL = reinterpret_cast<RED4ext::physics::ColliderSphere *>(
//          // rtti->GetClass("physicsColliderSphere")->CreateInstance());
//
//          RED4ext::Handle<RED4ext::physics::ICollider> shapeFL;
//          RED4ext::physics::ColliderSphere::createHandleWithRadius(&shapeFL, radius);
//          shapeFL.refCount->IncRef();
//
//           shapeFL->filterData = RED4ext::Handle(
//               reinterpret_cast<RED4ext::physics::FilterData
//               *>(rtti->GetClass("physicsFilterData")->CreateInstance()));
//           shapeFL->filterData->preset = "Vehicle Part";
//          shapeFL->filterData->simulationFilter.mask1 = 0x0080010000000000;
//          shapeFL->filterData->simulationFilter.mask2 = 0xFE11000000000080;
//          shapeFL->filterData->queryFilter.mask2 = 0x1000480400000000;
//          shapeFL->material = "vehicle_chassis.physmat";
//          shapeFL->isQueryShapeOnly = true;
//
//          wt = (RED4ext::WorldTransform *)wtCls->CreateInstance();
//          index = sc->GetSlotIndex("thruster_front_left");
//          if (index != -1) {
//            sc->GetSlotLocalTransform(index, &sc->localTransform, wt);
//            shapeFL->localToBody.position = *wt->Position.ToVector4() - *cc->localTransform.Position.ToVector4();
//          }
//          // handleFL.refCount->IncRef();
//          // handleFL.refCount->IncWeakRef();
//          ra.EmplaceBack(shapeFL);
//
//          RED4ext::Handle<RED4ext::physics::ICollider> shapeFR;
//          RED4ext::physics::ColliderSphere::createHandleWithRadius(&shapeFR, radius);
//          shapeFR.refCount->IncRef();
//
//          shapeFR->filterData = RED4ext::Handle(
//              reinterpret_cast<RED4ext::physics::FilterData
//                                   *>(rtti->GetClass("physicsFilterData")->CreateInstance()));
//          shapeFR->filterData->preset = "Vehicle Part";
//          shapeFR->filterData->simulationFilter.mask1 = 0x0080010000000000;
//          shapeFR->filterData->simulationFilter.mask2 = 0xFE11000000000080;
//          shapeFR->filterData->queryFilter.mask2 = 0x1000480400000000;
//          shapeFR->material = "vehicle_chassis.physmat";
//          shapeFR->isQueryShapeOnly = true;
//
//          wt = (RED4ext::WorldTransform *)wtCls->CreateInstance();
//          index = sc->GetSlotIndex("thruster_front_right");
//          if (index != -1) {
//            sc->GetSlotLocalTransform(index, &sc->localTransform, wt);
//            shapeFR->localToBody.position = *wt->Position.ToVector4() - *cc->localTransform.Position.ToVector4();
//          }
//          // handleFR.refCount->IncRef();
//          // handleFR.refCount->IncWeakRef();
//          ra.EmplaceBack(shapeFR);
//
//          RED4ext::Handle<RED4ext::physics::ICollider> shapeBL;
//          RED4ext::physics::ColliderSphere::createHandleWithRadius(&shapeBL, radius);
//          shapeBL.refCount->IncRef();
//
//          shapeBL->filterData = RED4ext::Handle(
//              reinterpret_cast<RED4ext::physics::FilterData
//                                   *>(rtti->GetClass("physicsFilterData")->CreateInstance()));
//          shapeBL->filterData->preset = "Vehicle Part";
//          shapeBL->filterData->simulationFilter.mask1 = 0x0080010000000000;
//          shapeBL->filterData->simulationFilter.mask2 = 0xFE11000000000080;
//          shapeBL->filterData->queryFilter.mask2 = 0x1000480400000000;
//          shapeBL->material = "vehicle_chassis.physmat";
//          shapeBL->isQueryShapeOnly = true;
//
//          wt = (RED4ext::WorldTransform *)wtCls->CreateInstance();
//          index = sc->GetSlotIndex("thruster_back_left");
//          if (index != -1) {
//            sc->GetSlotLocalTransform(index, &sc->localTransform, wt);
//            shapeBL->localToBody.position = *wt->Position.ToVector4() - *cc->localTransform.Position.ToVector4();
//          }
//
//          // handleBL.refCount->IncRef();
//          // handleBL.refCount->IncWeakRef();
//          ra.EmplaceBack(shapeBL);
//
//          RED4ext::Handle<RED4ext::physics::ICollider> shapeBR;
//          RED4ext::physics::ColliderSphere::createHandleWithRadius(&shapeBR, radius);
//          shapeBR.refCount->IncRef();
//
//          shapeBR->filterData = RED4ext::Handle(
//              reinterpret_cast<RED4ext::physics::FilterData
//                                   *>(rtti->GetClass("physicsFilterData")->CreateInstance()));
//          shapeBR->filterData->preset = "Vehicle Part";
//          shapeBR->filterData->simulationFilter.mask1 = 0x0080010000000000;
//          shapeBR->filterData->simulationFilter.mask2 = 0xFE11000000000080;
//          shapeBR->filterData->queryFilter.mask2 = 0x1000480400000000;
//          shapeBR->material = "vehicle_chassis.physmat";
//          shapeBR->isQueryShapeOnly = true;
//
//          wt = (RED4ext::WorldTransform *)wtCls->CreateInstance();
//          index = sc->GetSlotIndex("thruster_back_right");
//          if (index != -1) {
//            sc->GetSlotLocalTransform(index, &sc->localTransform, wt);
//            shapeBR->localToBody.position = *wt->Position.ToVector4() - *cc->localTransform.Position.ToVector4();
//          }
//
//          // handleBR.refCount->IncRef();
//          // handleBR.refCount->IncWeakRef();
//          ra.EmplaceBack(shapeBR);
//
//          *p_collisionShapes = ra;
//
//          auto og = vehicle_ProcessPhysicalSystem_Original(geoCacheId, desc);
//
//          *p_collisionShapes = originalShapes;
//
//          // for (auto &shape : ra) {
//          // ra.Remove(shape);
//          // shape.refCount->DecRef();
//          //}
//          // ra.size = 0;
//          ra.Clear();
//          /*        shapeFL.~Handle<RED4ext::physics::ICollider>();
//                  shapeFR.~Handle<RED4ext::physics::ICollider>();
//                  shapeBL.~Handle<RED4ext::physics::ICollider>();
//                  shapeBR.~Handle<RED4ext::physics::ICollider>();*/
//
//          return og;
//        }
//
//        // auto body =
//        // reinterpret_cast<RED4ext::physics::SystemBody*>(rtti->GetClass("physicsSystemBody")->CreateInstance());
//        // body->name = "Thruster Test";
//        // body->mappedBoneName = "swingarm_front_left";
//
//        // body->params.simulationType = RED4ext::physics::SimulationType::Static;
//        // body->params.inertia = RED4ext::Vector3(500.0, 500.0, 500.0);
//        // body->params.mass = 500.0;
//        // body->params.comOffset.orientation = RED4ext::Quaternion();
//
//        //
//        // auto shape = reinterpret_cast<RED4ext::physics::ColliderSphere *>(
//        //    rtti->GetClass("physicsColliderSphere")->CreateInstance());
//        // shape->radius = 1.0;
//        // shape->localToBody.position = RED4ext::Vector4(0.0, 0.0, 0.0, 0.0);
//        // body->collisionShapes.EmplaceBack(RED4ext::Handle<RED4ext::physics::ColliderSphere>(shape));
//        // desc->resource->bodies.EmplaceBack(RED4ext::Handle<RED4ext::physics::SystemBody>(body));
//      }
//    }
//  }
//Original:
//  return vehicle_ProcessPhysicalSystem_Original(geoCacheId, desc);
//}

RED4ext::ent::PhysicalMeshComponent *CreateThrusterEngine(RED4ext::CName mesh, RED4ext::CName name, RED4ext::CName slot,
                                                          RED4ext::CName bindName) {

  auto rtti = RED4ext::CRTTISystem::Get();
  auto mc = (RED4ext::ent::PhysicalMeshComponent *)rtti->GetClass("entPhysicalMeshComponent")->CreateInstance();
  mc->mesh.path = mesh;
  // not sure why this doesn't carry through
  // mc->isEnabled = false;
  mc->visualScale.X = 0.0;
  mc->visualScale.Y = 0.0;
  mc->visualScale.Z = 0.0;
  mc->filterDataSource = RED4ext::physics::FilterDataSource::Collider;
  auto filterData = (RED4ext::physics::FilterData *)rtti->GetClass("physicsFilterData")->CreateInstance();
  mc->filterData = RED4ext::Handle<RED4ext::physics::FilterData>(filterData);
  mc->name = name;
  mc->motionBlurScale = 0.1;
  mc->meshAppearance = "default";
  mc->objectTypeID = RED4ext::ERenderObjectType::ROT_Vehicle;
  mc->LODMode = RED4ext::ent::MeshComponentLODMode::Appearance;
  auto htb = (RED4ext::ent::HardTransformBinding *)rtti->GetClass("entHardTransformBinding")->CreateInstance();
  htb->bindName = bindName;
  htb->slotName = slot;
  mc->parentTransform = RED4ext::Handle<RED4ext::ent::ITransformBinding>(htb);
  return mc;
}

void AddResourceToController(RED4ext::ent::VisualControllerComponent *vcc, RED4ext::ResourcePath resourcePath) {
  if (vcc->resourcePaths.size) {
    for (int i = 0; i < vcc->resourcePaths.size; i++) {
      if (vcc->resourcePaths[i] == resourcePath) {
        break;
      } else if (vcc->resourcePaths[i] > resourcePath) {
        vcc->resourcePaths.Emplace(&vcc->resourcePaths[i], resourcePath);
        break;
      }
    }
  } else {
    vcc->resourcePaths.EmplaceBack(resourcePath);
  }
}

void AddToController(RED4ext::ent::VisualControllerComponent *vcc, RED4ext::ent::MeshComponent *mc) {
  auto rtti = RED4ext::CRTTISystem::Get();
  auto vcd = reinterpret_cast<RED4ext::ent::VisualControllerDependency *>(
      rtti->GetClass("entVisualControllerDependency")->CreateInstance());
  vcd->appearanceName = mc->meshAppearance;
  vcd->componentName = mc->name;
  vcd->mesh.path = mc->mesh.path;
  vcc->appearanceDependency.EmplaceBack(*vcd);
  AddResourceToController(vcc, mc->mesh.path);
}


void EntityAddComponent(RED4ext::IScriptable *aContext, RED4ext::CStackFrame *aFrame, bool *aOut, int64_t a4) {
  auto ent = reinterpret_cast<RED4ext::ent::Entity *>(aContext);

  RED4ext::Handle<RED4ext::ent::IComponent> value;
  RED4ext::GetParameter(aFrame, &value);
  aFrame->code++; // skip ParamEnd
  // auto handle = RED4ext::Handle<RED4ext::ent::IComponent>(value);
  // auto handle = *(RED4ext::Handle<RED4ext::ent::IComponent> *)&value;
  // ent->componentsStorage.components.EmplaceBack(handle);
  ent->componentsStorage.components.EmplaceBack(value);

  if (aOut) {
    *aOut = true;
  }
}

void EntityAddWorldWidgetComponent(RED4ext::IScriptable *aContext, RED4ext::CStackFrame *aFrame, bool *aOut,
                                   int64_t a4) {
  auto ent = reinterpret_cast<RED4ext::ent::Entity *>(aContext);

  aFrame->code++; // skip ParamEnd

  auto rtti = RED4ext::CRTTISystem::Get();

  // MeshComponent
  auto mc = (RED4ext::ent::MeshComponent *)rtti->GetClass("entMeshComponent")->CreateInstance();

  mc->mesh.path =
      "base\\environment\\decoration\\advertising\\holograms\\common\\common_holograms_transparent_a_w500_h150.mesh";
  mc->name = "radio_screen";
  mc->meshAppearance = "screen_ui";
  mc->localTransform.Position.x.Bits = 216270;
  mc->visualScale.X = 10.0;
  mc->visualScale.X = 10.0;
  mc->visualScale.X = 10.0;

  auto mcHandle = RED4ext::Handle<RED4ext::ent::MeshComponent>(mc);
  ent->componentsStorage.components.EmplaceBack(mcHandle);

  // WorldWidgetComponent
  auto wwc = (RED4ext::WorldWidgetComponent *)rtti->GetClass("WorldWidgetComponent")->CreateInstance();

  wwc->name = "radio_ui";
  wwc->widgetResource.path = "base\\gameplay\\gui\\world\\radio\\radio_ui.inkwidget";

  auto mtb = (RED4ext::world::ui::MeshTargetBinding *)rtti->GetClass("worlduiMeshTargetBinding")->CreateInstance();
  mtb->bindName = "radio_screen";
  wwc->meshTargetBinding = RED4ext::Handle<RED4ext::world::ui::MeshTargetBinding>(mtb);

  auto wwcHandle = RED4ext::Handle<RED4ext::WorldWidgetComponent>(wwc);
  ent->componentsStorage.components.EmplaceBack(wwcHandle);

  if (aOut) {
    *aOut = true;
  }
}

// base\vehicles\common\gameplay\flight_components.ent

// void EntityAddFlightComponent(RED4ext::IScriptable *aContext, RED4ext::CStackFrame *aFrame, bool *aOut, int64_t a4) {
//   auto ent = reinterpret_cast<RED4ext::ent::Entity *>(aContext);
//
//
//   RED4ext::CName *name;
//   RED4ext::GetParameter(aFrame, &name);
//   aFrame->code++; // skip ParamEnd
//
//   if (name != 0) {
//     auto resHandle = new RED4ext::ResourceHandle<RED4ext::ent::EntityTemplate>();
//     RED4ext::CName fc = "base\\vehicles\\common\\gameplay\\flight_components.ent";
//     LoadResRef<RED4ext::ent::EntityTemplate>((uint64_t *)&fc, resHandle, true);
//
//     auto et = resHandle->wrapper->resource.GetPtr();
//     et->sub_28(true);
//
//     auto handle = RED4ext::Handle<RED4ext::ent::IComponent>(value);
//     ent->componentsStorage.components.EmplaceBack(handle);
//
//     if (aOut) {
//       *aOut = true;
//     }
//   }
// }

void IPlacedComponentUpdateHardTransformBinding(RED4ext::IScriptable *aContext, RED4ext::CStackFrame *aFrame,
                                                bool *aOut, int64_t a4) {
  auto ipc = reinterpret_cast<RED4ext::ent::IPlacedComponent *>(aContext);

  RED4ext::CName bindName;
  RED4ext::CName slotName;
  RED4ext::GetParameter(aFrame, &bindName);
  RED4ext::GetParameter(aFrame, &slotName);
  aFrame->code++; // skip ParamEnd

  auto rtti = RED4ext::CRTTISystem::Get();
  auto transform = ipc->parentTransform.GetPtr();
  if (transform->GetNativeType() == rtti->GetClass("entHardTransformBinding")) {
    auto ht = (RED4ext::ent::HardTransformBinding *)(transform);
    ht->bindName = bindName;
    ht->slotName = slotName;
    if (aOut) {
      *aOut = true;
    }
  } else {
    if (aOut) {
      *aOut = false;
    }
  }
}

struct EntityAddComponentModule : FlightModule {
  void PostRegisterTypes() {
    auto rtti = RED4ext::CRTTISystem::Get();
    auto ent = rtti->GetClass("entEntity");
    ent->RegisterFunction(
        RED4ext::CClassFunction::Create(ent, "AddComponent", "AddComponent", &EntityAddComponent, {.isNative = true}));
    ent->RegisterFunction(RED4ext::CClassFunction::Create(ent, "AddWorldWidgetComponent", "AddWorldWidgetComponent",
                                                          &EntityAddWorldWidgetComponent, {.isNative = true}));

    auto ipc = rtti->GetClass("entIPlacedComponent");
    ipc->RegisterFunction(
        RED4ext::CClassFunction::Create(ipc, "UpdateHardTransformBinding", "UpdateHardTransformBinding",
                                        &IPlacedComponentUpdateHardTransformBinding, {.isNative = true}));

    // auto mc = rtti->GetClass("entMeshComponent");
    // mc->props.EmplaceBack(RED4ext::CProperty::Create(rtti->GetType("Vector3"), "visualScale", nullptr, 0x178));
  }
};

REGISTER_FLIGHT_MODULE(EntityAddComponentModule);