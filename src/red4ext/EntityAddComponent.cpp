#include "Utils/FlightModule.hpp"
#include <RED4ext/RED4ext.hpp>
#include <RED4ext/Scripting/Natives/Generated/ent/Entity.hpp>
#include <RED4ext/Scripting/Natives/Generated/ent/EntityTemplate.hpp>
#include <RED4ext/Scripting/Natives/Generated/ent/IComponent.hpp>
#include <RED4ext/Scripting/Natives/Generated/ent/IPlacedComponent.hpp>
#include <RED4ext/Scripting/Natives/Generated/ent/MeshComponent.hpp>
#include <RED4ext/Scripting/Natives/Generated/ent/PhysicalMeshComponent.hpp>
#include <RED4ext/Scripting/Natives/Generated/ent/PlaceholderComponent.hpp>
#include <RED4ext/Scripting/Natives/Generated/ent/SlotComponent.hpp>
#include <RED4ext/Scripting/Natives/Generated/ent/VisualControllerComponent.hpp>
#include <RED4ext/Scripting/Natives/Generated/ent/ITransformBinding.hpp>
#include <RED4ext/Scripting/Natives/Generated/WorldWidgetComponent.hpp>
#include <RED4ext/Scripting/Natives/Generated/ent/HardTransformBinding.hpp>
#include <RED4ext/Scripting/Natives/Generated/world/ui/MeshTargetBinding.hpp>
#include <RED4ext/Scripting/Natives/Generated/physics/FilterData.hpp>
#include <RED4ext/Scripting/Natives/Generated/game/projectile/SpawnComponent.hpp>
#include <RED4ext/Scripting/Natives/Generated/game/AttachmentSlots.hpp>
#include <RED4ext/Scripting/Natives/Generated/game/AnimParamSlotsOption.hpp>
#include <RED4ext/Scripting/Natives/Generated/WidgetHudComponent.hpp>
#include <RED4ext/Scripting/Natives/Generated/vehicle/BaseObject.hpp>
#include <RED4ext/Scripting/Natives/vehicleWeapon.hpp>
#include <RED4ext/Scripting/Natives/Generated/ink/HudEntriesResource.hpp>
#include <RED4ext/Scripting/Natives/Generated/game/CameraComponent.hpp>
#include <RED4ext/Scripting/Natives/Generated/game/OccupantSlotComponent.hpp>
#include <RED4ext/Scripting/Natives/vehicleChassisComponent.hpp>
#include <RED4ext/Scripting/Natives/Generated/physics/SystemBody.hpp>
#include <RED4ext/Scripting/Natives/Generated/physics/SystemResource.hpp>
#include <RED4ext/Scripting/Natives/Generated/physics/ColliderBox.hpp>
#include <RED4ext/Scripting/Natives/Generated/physics/ColliderSphere.hpp>
#include <RED4ext/Scripting/Natives/Generated/physics/SystemBodyParams.hpp>
#include "LoadResRef.hpp"
#include <spdlog/spdlog.h>
#include "Physics/VehiclePhysicsUpdate.hpp"
#include "FlightWeapons.hpp"
#include "Signatures.hpp"
#include <thread>
#include "FlightSystem.hpp"
#include "FlightComponent.hpp"
#include "EntityAddComponent.hpp"
#include <RED4ext/VFTEnum.hpp>

// weird bug fix

// 48 89 5C 24 08 48 89 74 24 10 48 89 7C 24 18 4C 89 64 24 20 55 41 56 41 57 48 8D 6C 24 C9 48 81
//void __fastcall SpawnEffect(uintptr_t, uintptr_t, uintptr_t, uintptr_t, uintptr_t, uintptr_t);
//constexpr uintptr_t SpawnEffectAddr = 0x00007FF73F0D86B0 - 0x7ff73dfe0000;
//decltype(&SpawnEffect) SpawnEffect_Original;
//
//void __fastcall SpawnEffect(uintptr_t a1, uintptr_t a2, uintptr_t a3, uintptr_t a4, uintptr_t a5, uintptr_t a6) {
//  if (a1) {
//    SpawnEffect_Original(a1, a2, a3, a4, a5, a6);
//  }
//}


int64_t RED4ext::ent::SlotComponent::GetSlotIndex(RED4ext::CName slotName) {
  RelocFunc<decltype(&RED4ext::ent::SlotComponent::GetSlotIndex)> call(entSlotComponent_GetSlotIndexAddr);
  return call(this, slotName);
}

bool RED4ext::ent::SlotComponent::GetSlotLocalTransform(int slotIndex, RED4ext::WorldTransform *offset,
                                                        RED4ext::WorldTransform *worldTransform) {
  RelocFunc<decltype(&RED4ext::ent::SlotComponent::GetSlotLocalTransform)> call(
      entSlotComponent_GetSlotLocalTransformAddr);
  return call(this, slotIndex, offset, worldTransform);
}


RED4ext::Handle<RED4ext::physics::ColliderSphere> *
RED4ext::physics::ColliderSphere::createHandleWithRadius(RED4ext::Handle<RED4ext::physics::ICollider> * handle,
                                                         float radius) {
  RelocFunc<decltype(&RED4ext::physics::ColliderSphere::createHandleWithRadius)> call(
      physicsColliderSphere_createHandleWithRadiusAddr);
  return call(handle, radius);
}

REGISTER_FLIGHT_HOOK(uint32_t *, vehicle_ProcessPhysicalSystem, 
    uint32_t *geoCacheId, RED4ext::vehicle::PhysicalSystemDesc *desc) {
  auto collidersAdded = 0;
  if (desc->type == RED4ext::vehicle::SystemType::PhysicalSystemDesc && desc->numHandles && desc->entity) {
    auto rtti = RED4ext::CRTTISystem::Get();


    auto type = desc->entity->GetNativeType();
    bool isCar = false;
    bool isBike = false;
    auto isVehicle = false;
    do {
      isVehicle |= type == rtti->GetClass("vehicleBaseObject");
      isCar |= type == rtti->GetClass("vehicleCarBaseObject");
      isBike |= type == rtti->GetClass("vehicleBikeBaseObject");
    } while (type = type->parent);

    if (isCar) {
      if (desc->resource) {
        if (desc->resource->bodies.size && desc->resource->bodies[0]->params.simulationType == RED4ext::physics::SimulationType::Static) {
          RED4ext::ent::SlotComponent *sc = NULL;
          RED4ext::vehicle::ChassisComponent *cc = NULL;
          auto scCls = rtti->GetClass("entSlotComponent");
          auto ccCls = rtti->GetClass("vehicleChassisComponent");
          for (auto const &handle : desc->entity->componentsStorage.components) {
            auto component = handle.GetPtr();
            if (sc == NULL && component->GetNativeType() == scCls) {
              if (component->name == "vehicle_slots") {
                sc = reinterpret_cast<RED4ext::ent::SlotComponent *>(component);
              }
            } else if (cc == NULL && component->GetNativeType() == ccCls) {
              cc = reinterpret_cast<RED4ext::vehicle::ChassisComponent *>(component);
            }
          }

          if (sc == NULL || cc == NULL)
            goto Original;

          RED4ext::WorldTransform *wt;
          int index = 0;
          auto allocator = reinterpret_cast<RED4ext::Memory::IAllocator *>(desc->resource->bodies[0]->params.allocator);

          auto ra = RED4ext::DynArray<RED4ext::Handle<RED4ext::physics::ICollider>>(allocator);
          // doesn't work :/
          //desc->resource->bodies[0]->params.angularDamping = 10.0;
          const auto p_collisionShapes = &desc->resource->bodies[0]->collisionShapes;
          auto originalShapes = *p_collisionShapes;

          ra.Reserve(p_collisionShapes->size + 4);
          for (auto &shape : *p_collisionShapes) {
            shape.refCount->IncRef();
            ra.EmplaceBack(shape);
          }

          auto sphereCls = rtti->GetClass("physicsColliderSphere");
          auto wtCls = rtti->GetClass("WorldTransform");

          // auto shapeFL = reinterpret_cast<RED4ext::physics::ColliderSphere *>(
          // rtti->GetClass("physicsColliderSphere")->CreateInstance());

          RED4ext::Handle<RED4ext::physics::ICollider> shapeFL;
          RED4ext::physics::ColliderSphere::createHandleWithRadius(&shapeFL, 0.45);
          shapeFL.refCount->IncRef();

          // shapeFL->filterData = RED4ext::Handle(
          //     reinterpret_cast<RED4ext::physics::FilterData
          //     *>(rtti->GetClass("physicsFilterData")->CreateInstance()));
          // shapeFL->filterData->preset = "Vehicle Part";
          // shapeFL->filterData->simulationFilter.mask1 = 98304;
          // shapeFL->filterData->simulationFilter.mask2 = 9223372036854780414;
          shapeFL->material = "vehicle_chassis.physmat";

          wt = (RED4ext::WorldTransform *)wtCls->CreateInstance();
          index = sc->GetSlotIndex("thruster_front_left");
          if (index != -1) {
            sc->GetSlotLocalTransform(index, &sc->localTransform, wt);
            shapeFL->localToBody.position = *wt->Position.ToVector4() - *cc->localTransform.Position.ToVector4();
          }
          // handleFL.refCount->IncRef();
          // handleFL.refCount->IncWeakRef();
          ra.EmplaceBack(shapeFL);

          RED4ext::Handle<RED4ext::physics::ICollider> shapeFR;
          RED4ext::physics::ColliderSphere::createHandleWithRadius(&shapeFR, 0.45);
          shapeFR.refCount->IncRef();

          // shapeFR->filterData = RED4ext::Handle(
          //     reinterpret_cast<RED4ext::physics::FilterData
          //     *>(rtti->GetClass("physicsFilterData")->CreateInstance()));
          // shapeFR->filterData->preset = "Vehicle Part";
          // shapeFR->filterData->simulationFilter.mask1 = 98304;
          // shapeFR->filterData->simulationFilter.mask2 = 9223372036854780414;
          shapeFR->material = "vehicle_chassis.physmat";

          wt = (RED4ext::WorldTransform *)wtCls->CreateInstance();
          index = sc->GetSlotIndex("thruster_front_right");
          if (index != -1) {
            sc->GetSlotLocalTransform(index, &sc->localTransform, wt);
            shapeFR->localToBody.position = *wt->Position.ToVector4() - *cc->localTransform.Position.ToVector4();
          }
          // handleFR.refCount->IncRef();
          // handleFR.refCount->IncWeakRef();
          ra.EmplaceBack(shapeFR);

          RED4ext::Handle<RED4ext::physics::ICollider> shapeBL;
          RED4ext::physics::ColliderSphere::createHandleWithRadius(&shapeBL, 0.45);
          shapeBL.refCount->IncRef();

          // shapeBL->filterData = RED4ext::Handle(
          //     reinterpret_cast<RED4ext::physics::FilterData
          //     *>(rtti->GetClass("physicsFilterData")->CreateInstance()));
          // shapeBL->filterData->preset = "Vehicle Part";
          // shapeBL->filterData->simulationFilter.mask1 = 98304;
          // shapeBL->filterData->simulationFilter.mask2 = 9223372036854780414;
          shapeBL->material = "vehicle_chassis.physmat";

          wt = (RED4ext::WorldTransform *)wtCls->CreateInstance();
          index = sc->GetSlotIndex("thruster_back_left");
          if (index != -1) {
            sc->GetSlotLocalTransform(index, &sc->localTransform, wt);
            shapeBL->localToBody.position = *wt->Position.ToVector4() - *cc->localTransform.Position.ToVector4();
          }

          // handleBL.refCount->IncRef();
          // handleBL.refCount->IncWeakRef();
          ra.EmplaceBack(shapeBL);

          RED4ext::Handle<RED4ext::physics::ICollider> shapeBR;
          RED4ext::physics::ColliderSphere::createHandleWithRadius(&shapeBR, 0.45);
          shapeBR.refCount->IncRef();

          // shapeBR->filterData = RED4ext::Handle(
          //     reinterpret_cast<RED4ext::physics::FilterData
          //     *>(rtti->GetClass("physicsFilterData")->CreateInstance()));
          // shapeBR->filterData->preset = "Vehicle Part";
          // shapeBR->filterData->simulationFilter.mask1 = 98304;
          // shapeBR->filterData->simulationFilter.mask2 = 9223372036854780414;
          shapeBR->material = "vehicle_chassis.physmat";

          wt = (RED4ext::WorldTransform *)wtCls->CreateInstance();
          index = sc->GetSlotIndex("thruster_back_right");
          if (index != -1) {
            sc->GetSlotLocalTransform(index, &sc->localTransform, wt);
            shapeBR->localToBody.position = *wt->Position.ToVector4() - *cc->localTransform.Position.ToVector4();
          }

          //handleBR.refCount->IncRef();
          //handleBR.refCount->IncWeakRef();
          ra.EmplaceBack(shapeBR);

          *p_collisionShapes = ra;

          auto og = vehicle_ProcessPhysicalSystem_Original(geoCacheId, desc);
          
          *p_collisionShapes = originalShapes;

          //for (auto &shape : ra) {
            //ra.Remove(shape);
            //shape.refCount->DecRef();
          //}
          //ra.size = 0;
          ra.Clear();
  /*        shapeFL.~Handle<RED4ext::physics::ICollider>();
          shapeFR.~Handle<RED4ext::physics::ICollider>();
          shapeBL.~Handle<RED4ext::physics::ICollider>();
          shapeBR.~Handle<RED4ext::physics::ICollider>();*/

          return og;
        }

        // auto body =
        // reinterpret_cast<RED4ext::physics::SystemBody*>(rtti->GetClass("physicsSystemBody")->CreateInstance());
        // body->name = "Thruster Test";
        // body->mappedBoneName = "swingarm_front_left";

        // body->params.simulationType = RED4ext::physics::SimulationType::Static;
        // body->params.inertia = RED4ext::Vector3(500.0, 500.0, 500.0);
        // body->params.mass = 500.0;
        // body->params.comOffset.orientation = RED4ext::Quaternion();

        //
        // auto shape = reinterpret_cast<RED4ext::physics::ColliderSphere *>(
        //    rtti->GetClass("physicsColliderSphere")->CreateInstance());
        // shape->radius = 1.0;
        // shape->localToBody.position = RED4ext::Vector4(0.0, 0.0, 0.0, 0.0);
        // body->collisionShapes.EmplaceBack(RED4ext::Handle<RED4ext::physics::ColliderSphere>(shape));
        // desc->resource->bodies.EmplaceBack(RED4ext::Handle<RED4ext::physics::SystemBody>(body));
      }
    }
  }
Original:
  return vehicle_ProcessPhysicalSystem_Original(geoCacheId, desc);
}

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

VehicleProcessWeapons VehicleProcessWeapons_Hook;
decltype(&VehicleProcessWeapons_Hook) VehicleProcessWeapons_Original;

void __fastcall VehicleProcessWeapons_Hook(RED4ext::vehicle::BaseObject *vehicle, float timeDelta,
                                           unsigned int shootIndex) {
  VehicleProcessWeapons_Original(vehicle, timeDelta, shootIndex);
  if (vehicle->weapons[shootIndex].cycleTimer == 0.0) {
    RED4ext::Quaternion quat = {0.0, 0.0, 0.0, 1.0};
    auto ph = (RED4ext::ent::PlaceholderComponent *)vehicle->weapons[shootIndex].weaponObject.GetPtr()->placeholder;
    if (ph) {
      quat = ph->worldTransform.Orientation;
    }

    auto rtti = RED4ext::CRTTISystem::Get();
    auto fcc = rtti->GetClass("FlightComponent");
    auto fc = FlightComponent::Get(vehicle);
    auto onFireWeapon = fcc->GetFunction("OnFireWeapon");
    RED4ext::CStackType args[3];
    args[0] = RED4ext::CStackType(rtti->GetType("Quaternion"), &quat);
    args[1] = RED4ext::CStackType(rtti->GetType("TweakDBID"), &vehicle->weapons[shootIndex].item);
    args[2] = RED4ext::CStackType(rtti->GetType("TweakDBID"), &vehicle->weapons[shootIndex].slot);
    auto stack = RED4ext::CStack(fc, args, 3, nullptr, 0);
    onFireWeapon->Execute(&stack);
  }
}

Entity_InitializeComponents Entity_InitializeComponents_Hook;
decltype(&Entity_InitializeComponents_Hook) Entity_InitializeComponents_Original;

void __fastcall Entity_InitializeComponents_Hook(RED4ext::ent::Entity *entity, void *a2, void *a3) {
  auto rtti = RED4ext::CRTTISystem::Get();

  auto type = entity->GetNativeType();
  bool isCar = false;
  bool isBike = false;
  auto isVehicle = false;
  do {
    isVehicle |= type == rtti->GetClass("vehicleBaseObject");
    isCar |= type == rtti->GetClass("vehicleCarBaseObject");
    isBike |= type == rtti->GetClass("vehicleBikeBaseObject");
  } while (type = type->parent);

  if (isVehicle) {
    auto vehicle = reinterpret_cast<RED4ext::vehicle::BaseObject *>(entity);

    auto fc = (FlightComponent *)FlightComponent::GetRTTIType()->CreateInstance(true);
    fc->name = "flightComponent";
    fc->entity = entity;
    auto fch = RED4ext::Handle<FlightComponent>(fc);
    vehicle->componentsStorage.components.EmplaceBack(fch);

    // vehicle->entityTags.tags;

    // FlightWeapons::AddWeapons(vehicle);

    // auto fc = (FlightComponent*)FlightComponent::GetRTTIType()->CreateInstance();
    // fc->name = "flightComponent";
    // auto h = RED4ext::Handle<FlightComponent>(fc);
    // h.refCount->IncRef();
    // h.refCount->IncRef();
    //entity->componentsStorage.components.EmplaceBack(h);

    RED4ext::ent::VisualControllerComponent *vcc = NULL;
    RED4ext::vehicle::ChassisComponent *chassis = NULL;
    RED4ext::game::OccupantSlotComponent *osc = NULL;
    RED4ext::ent::SlotComponent *vs = NULL;
    for (auto const &handle : entity->componentsStorage.components) {
      auto component = handle.GetPtr();
      if (vcc == NULL && component->GetNativeType() == rtti->GetClass("entVisualControllerComponent")) {
        vcc = reinterpret_cast<RED4ext::ent::VisualControllerComponent *>(component);
      }
      if (chassis == NULL && component->GetNativeType() == rtti->GetClass("vehicleChassisComponent")) {
        chassis = reinterpret_cast<RED4ext::vehicle::ChassisComponent *>(component);
      }
      if (vs == NULL && component->GetNativeType() == rtti->GetClass("entSlotComponent")) {
        if (component->name == "vehicle_slots") {
          vs = reinterpret_cast<RED4ext::ent::SlotComponent *>(component);
        }
      }
      if (osc == NULL && component->GetNativeType() == rtti->GetClass("gameOccupantSlotComponent")) {
        osc = reinterpret_cast<RED4ext::game::OccupantSlotComponent *>(component);
      }
    }

    //if (chassis != NULL) {
    //
    //}

    if (vcc != NULL && vs != NULL) {
      {
        //auto slot = reinterpret_cast<RED4ext::ent::Slot *>(rtti->GetClass("entSlot")->CreateInstance());
        //slot->boneName = "roof_border_front";
        //slot->slotName = "roof_border_front";
        //vs->slots.EmplaceBack(*slot);
        //vs->slotIndexLookup.Emplace(slot->slotName, vs->slots.size - 1);
      }
      //FlightWeapons::AddWeaponSlots(vs);

      bool isSixWheeler = false;
      //for (auto const &slot : vs->slots) {
      //  if (slot.slotName == "wheel_front_left_b")
      //    isSixWheeler = true;
      //}

      for (auto const &handle : entity->componentsStorage.components) {
        auto component = handle.GetPtr();
        type = component->GetNativeType();
        bool isPlacedComponent = false;
        do {
          isPlacedComponent |= type == rtti->GetClass("entIPlacedComponent");
        } while (type = type->parent);

        if (isPlacedComponent) {
          auto pth = ((RED4ext::ent::IPlacedComponent *)component)->parentTransform;
          if (pth) {
            auto pt = reinterpret_cast<RED4ext::ent::HardTransformBinding *>(pth.GetPtr());
            if (pt && pt->slotName == "wheel_front_left_b") {
              isSixWheeler |= true;
            }
          }
        }
      }

      char className[256];
      sprintf_s(className, "FlightConfiguration_%s", entity->currentAppearance.ToString());

      auto configurationCls = rtti->GetClassByScriptName(className);
      if (!configurationCls) {
        if (isSixWheeler) {
          configurationCls = rtti->GetClassByScriptName("SixWheelCarFlightConfiguration");
        } else if (isCar) {
          configurationCls = rtti->GetClassByScriptName("CarFlightConfiguration");
        } else if (isBike) {
          configurationCls = rtti->GetClassByScriptName("BikeFlightConfiguration");
        }
      }

      if (configurationCls) {
        //spdlog::info("Looked for class '{}' using '{}'", className, configurationCls->name.ToString());
        auto configuration = reinterpret_cast<IFlightConfiguration *>(configurationCls->CreateInstance(true));
        configurationCls->ConstructCls(configuration);

        auto handle = RED4ext::Handle<IFlightConfiguration>(configuration);
        configuration->ref = RED4ext::WeakHandle(*reinterpret_cast<RED4ext::Handle<RED4ext::ISerializable> *>(&handle));
        configuration->unk30 = configurationCls;
        configuration->component = RED4ext::Handle<FlightComponent>(fc);
        configuration->component.refCount->IncRef();
        fc->configuration = handle;
        handle.refCount->IncRef();

        configuration->Setup(vehicle);
        configuration->AddSlots(vs);
        //configuration->AddMeshes(entity, vcc);
      } else {
        //spdlog::info("Looked for class '{}'", className);
      }
    }

    // UI
    //{
    //  // MeshComponent
    //  auto mc = (RED4ext::ent::MeshComponent *)rtti->GetClass("entMeshComponent")->CreateInstance();

    //  RED4ext::CName mesh = "user\\jackhumbert\\meshes\\flight_ui.mesh";
    //  mc->mesh.ref = mesh;
    //  mc->name = "flight_screen";
    //  mc->meshAppearance = "default";

    //  auto mcHandle = RED4ext::Handle<RED4ext::ent::MeshComponent>(mc);
    //  entity->componentsStorage.components.EmplaceBack(mcHandle);
    //}
    //{
    //  // WorldWidgetComponent
    //  auto wwc = (RED4ext::WorldWidgetComponent *)rtti->GetClass("WorldWidgetComponent")->CreateInstance();

    //  wwc->name = "flight_ui";

    //  RED4ext::CName fc = "user\\jackhumbert\\widgets\\flight_ui.inkwidget";
    //  wwc->widgetResource.ref = fc;

    //  auto mtb = (RED4ext::world::ui::MeshTargetBinding
    //  *)rtti->GetClass("worlduiMeshTargetBinding")->CreateInstance(); mtb->bindName = "flight_screen";
    //  wwc->meshTargetBinding = RED4ext::Handle<RED4ext::world::ui::MeshTargetBinding>(mtb);

    //  auto wwcHandle = RED4ext::Handle<RED4ext::WorldWidgetComponent>(wwc);
    //  entity->componentsStorage.components.EmplaceBack(wwcHandle);
    //}

    // UI Info Panel
    //{
    //  // MeshComponent
    //  auto mc = (RED4ext::ent::MeshComponent *)rtti->GetClass("entMeshComponent")->CreateInstance();

    //  RED4ext::CName mesh = "user\\jackhumbert\\meshes\\flight_ui_info.mesh";
    //  mc->mesh.ref = mesh;
    //  mc->name = "flight_screen_info";
    //  mc->meshAppearance = "screen_ui";
    //  mc->renderingPlane = RED4ext::ERenderingPlane::RPl_Weapon;
    //  mc->forcedLodDistance = RED4ext::ent::ForcedLodDistance::VehicleInterior;

    //  entity->componentsStorage.components.EmplaceBack(RED4ext::Handle<RED4ext::ent::MeshComponent>(mc));
    //}
    //{
    //  // WorldWidgetComponent
    //  auto wwc = (RED4ext::WorldWidgetComponent *)rtti->GetClass("WorldWidgetComponent")->CreateInstance();

    //  wwc->name = "flight_ui_info";

    //  RED4ext::CName fc = "user\\jackhumbert\\widgets\\flight_ui.inkwidget";
    //  wwc->widgetResource.ref = fc;
    //  wwc->spawnDistanceOverride = 20.0;
    //  wwc->sceneWidgetProperties.renderingPlane = RED4ext::ERenderingPlane::RPl_Weapon;
    //  auto mtb = (RED4ext::world::ui::MeshTargetBinding *)rtti->GetClass("worlduiMeshTargetBinding")->CreateInstance();
    //  mtb->bindName = "flight_screen_info";
    //  wwc->meshTargetBinding = RED4ext::Handle<RED4ext::world::ui::MeshTargetBinding>(mtb);

    //  entity->componentsStorage.components.EmplaceBack(RED4ext::Handle<RED4ext::WorldWidgetComponent>(wwc));
    //}
    //{
    //  auto gpsp =
    //      (RED4ext::game::projectile::SpawnComponent *)rtti->GetClass("gameprojectileSpawnComponent")->CreateInstance();
    //  gpsp->name = "projectileSpawn8722";
    //  gpsp->projectileTemplates.EmplaceBack("exploding_bullet");
    //  auto htb = (RED4ext::ent::HardTransformBinding *)rtti->GetClass("entHardTransformBinding")->CreateInstance();
    //  htb->bindName = "vehicle_slots";
    //  htb->slotName = "ThrusterFL";
    //  gpsp->parentTransform = RED4ext::Handle<RED4ext::ent::ITransformBinding>(htb);
    //  entity->componentsStorage.components.EmplaceBack(RED4ext::Handle<RED4ext::game::projectile::SpawnComponent>(gpsp));
    //}

    {
      auto whc = (RED4ext::WidgetHudComponent *)rtti->GetClass("WidgetHudComponent")->CreateInstance();
      whc->name = "FlightHUD";
      //auto resource =
          //RED4ext::Ref<RED4ext::ink::HudEntriesResource>("user\\jackhumbert\\widgets\\hud_flight.inkhud", true);
      whc->hudEntriesResource.path = "user\\jackhumbert\\widgets\\hud_flight.inkhud";
      //whc->hudEntriesResource.token = resource.token;
      LoadResRef<RED4ext::ink::HudEntriesResource>(&whc->hudEntriesResource.path, &whc->hudEntriesResource.token,
                                                   false);
      entity->componentsStorage.components.EmplaceBack(RED4ext::Handle<RED4ext::WidgetHudComponent>(whc));
    }

    //{
    //  auto gcc = (RED4ext::game::CameraComponent *)rtti->GetClass("gameCameraComponent")->CreateInstance();
    //  gcc->name = "frontCamera";
    //  auto htb = (RED4ext::ent::HardTransformBinding *)rtti->GetClass("entHardTransformBinding")->CreateInstance();
    //  htb->bindName = "vehicle_slots";
    //  htb->slotName = "roof_border_front";
    //  gcc->parentTransform = RED4ext::Handle<RED4ext::ent::ITransformBinding>(htb);
    //  entity->componentsStorage.components.EmplaceBack(RED4ext::Handle<RED4ext::game::CameraComponent>(gcc));
    //}
  }

  Entity_InitializeComponents_Original(entity, a2, a3);
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

//base\vehicles\common\gameplay\flight_components.ent

//void EntityAddFlightComponent(RED4ext::IScriptable *aContext, RED4ext::CStackFrame *aFrame, bool *aOut, int64_t a4) {
//  auto ent = reinterpret_cast<RED4ext::ent::Entity *>(aContext);
//
//
//  RED4ext::CName *name;
//  RED4ext::GetParameter(aFrame, &name);
//  aFrame->code++; // skip ParamEnd
//
//  if (name != 0) {
//    auto resHandle = new RED4ext::ResourceHandle<RED4ext::ent::EntityTemplate>();
//    RED4ext::CName fc = "base\\vehicles\\common\\gameplay\\flight_components.ent";
//    LoadResRef<RED4ext::ent::EntityTemplate>((uint64_t *)&fc, resHandle, true);
//
//    auto et = resHandle->wrapper->resource.GetPtr();
//    et->sub_28(true);
//
//    auto handle = RED4ext::Handle<RED4ext::ent::IComponent>(value);
//    ent->componentsStorage.components.EmplaceBack(handle);
//
//    if (aOut) {
//      *aOut = true;
//    }
//  }
//}

void IPlacedComponentUpdateHardTransformBinding(RED4ext::IScriptable *aContext, RED4ext::CStackFrame *aFrame, bool *aOut, int64_t a4) {
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
  void Load(const RED4ext::Sdk *aSdk, RED4ext::PluginHandle aHandle) {
    while (!aSdk->hooking->Attach(aHandle, RED4EXT_OFFSET_TO_ADDR(Entity_InitializeComponentsAddr),
                                  &Entity_InitializeComponents_Hook,
                                  reinterpret_cast<void **>(&Entity_InitializeComponents_Original)))
      ;
    while (!aSdk->hooking->Attach(aHandle, RED4EXT_OFFSET_TO_ADDR(VehicleProcessWeaponsAddr),
                                  &VehicleProcessWeapons_Hook,
                                  reinterpret_cast<void **>(&VehicleProcessWeapons_Original)))
      ;
    //while (!aSdk->hooking->Attach(aHandle, RED4EXT_OFFSET_TO_ADDR(SpawnEffectAddr), &SpawnEffect,
    //                              reinterpret_cast<void **>(&SpawnEffect_Original)))
    //  ;
  }
  void Unload(const RED4ext::Sdk *aSdk, RED4ext::PluginHandle aHandle) {
    aSdk->hooking->Detach(aHandle, RED4EXT_OFFSET_TO_ADDR(Entity_InitializeComponentsAddr));
    aSdk->hooking->Detach(aHandle, RED4EXT_OFFSET_TO_ADDR(VehicleProcessWeaponsAddr));
    //aSdk->hooking->Detach(aHandle, RED4EXT_OFFSET_TO_ADDR(SpawnEffectAddr));
  }
  void PostRegisterTypes() {
    auto rtti = RED4ext::CRTTISystem::Get();
    auto ent = rtti->GetClass("entEntity");
    ent->RegisterFunction(
        RED4ext::CClassFunction::Create(ent, "AddComponent", "AddComponent", &EntityAddComponent, {.isNative = true}));
    ent->RegisterFunction(
        RED4ext::CClassFunction::Create(ent, "AddWorldWidgetComponent", "AddWorldWidgetComponent", &EntityAddWorldWidgetComponent, {.isNative = true}));

    auto ipc = rtti->GetClass("entIPlacedComponent");
    ipc->RegisterFunction(RED4ext::CClassFunction::Create(ipc, "UpdateHardTransformBinding", "UpdateHardTransformBinding",
                                                          &IPlacedComponentUpdateHardTransformBinding, {.isNative = true}));

    
    //auto mc = rtti->GetClass("entMeshComponent");
    //mc->props.EmplaceBack(RED4ext::CProperty::Create(rtti->GetType("Vector3"), "visualScale", nullptr, 0x178));
  }
};

REGISTER_FLIGHT_MODULE(EntityAddComponentModule);