#include "FlightModule.hpp"
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
#include "LoadResRef.hpp"
#include <spdlog/spdlog.h>
#include "VehiclePhysicsUpdate.hpp"
#include "FlightWeapons.hpp"
#include "Signatures.hpp"
#include <thread>
#include "FlightSystem.hpp"
#include "FlightComponent.hpp"
#include "EntityAddComponent.hpp"

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

RED4ext::ent::PhysicalMeshComponent *CreateThrusterEngine(RED4ext::CName mesh,
                                                          RED4ext::CName name, RED4ext::CName slot,
                                                          RED4ext::CName bindName) {

  auto rtti = RED4ext::CRTTISystem::Get();
  auto mc = (RED4ext::ent::PhysicalMeshComponent *)rtti->GetClass("entPhysicalMeshComponent")->CreateInstance();
  mc->mesh.path = mesh;
  // not sure why this doesn't carry through
  //mc->isEnabled = false;
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
    auto ph = (RED4ext::ent::PlaceholderComponent*)vehicle->weapons[shootIndex].weaponObject.GetPtr()->placeholder;
    if (ph) {
      quat = ph->worldTransform.Orientation;
    }

    auto rtti = RED4ext::CRTTISystem::Get();
    auto fcc = rtti->GetClass("FlightComponent");
    auto fc = GetFlightComponent(vehicle);
    auto onFireWeapon = fcc->GetFunction("OnFireWeapon");
    RED4ext::CStackType args[3];
    args[0] = RED4ext::CStackType(rtti->GetType("Quaternion"), &quat);
    args[1] = RED4ext::CStackType(rtti->GetType("TweakDBID"), &vehicle->weapons[shootIndex].item);
    args[2] = RED4ext::CStackType(rtti->GetType("TweakDBID"), &vehicle->weapons[shootIndex].slot);
    auto stack = RED4ext::CStack(fc, args, 3, nullptr, 0);
    onFireWeapon->Execute(&stack);
  }
}

//#ifndef ZOLTAN
//#define ZOLTAN(name, returnType, signature) using name = returnType signature; \
//name name##_Hook;                                                                                                         \
//  decltype(&name##_Hook) name##_Original;                                                                                   \
//  returnType name##_Hook signature
//#endif
//ZOLTAN(Entity_InitializeComponents, void, (RED4ext::ent::Entity * entity, void *a2, void *a3))


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
    auto fch = RED4ext::Handle<FlightComponent>(fc);
    vehicle->componentsStorage.components.EmplaceBack(fch);

    //FlightWeapons::AddWeapons(vehicle);

    //auto fc = (FlightComponent*)FlightComponent::GetRTTIType()->CreateInstance();
    //fc->name = "flightComponent";
    //auto h = RED4ext::Handle<FlightComponent>(fc);
    //h.refCount->IncRef();
    //entity->componentsStorage.components.EmplaceBack(h);

    // entVisualControllerComponent
    RED4ext::ent::VisualControllerComponent *vcc = NULL;
    for (auto const &handle : entity->componentsStorage.components) {
      auto component = handle.GetPtr();
      if (component->GetNativeType() == rtti->GetClass("entVisualControllerComponent")) {
        vcc = reinterpret_cast<RED4ext::ent::VisualControllerComponent *>(component);
        break;
      }
    }

    RED4ext::game::OccupantSlotComponent *osc = NULL;
    for (auto const &handle : entity->componentsStorage.components) {
      auto component = handle.GetPtr();
      if (component->GetNativeType() == rtti->GetClass("gameOccupantSlotComponent")) {
        osc = reinterpret_cast<RED4ext::game::OccupantSlotComponent *>(component);

        {
          auto slot = reinterpret_cast<RED4ext::ent::Slot *>(rtti->GetClass("entSlot")->CreateInstance());
          slot->boneName = "roof_border_front";
          slot->slotName = "CustomFlightCamera";
          osc->slots.EmplaceBack();
          osc->slotIndexLookup.Emplace(slot->slotName, osc->slots.size - 1);
        }
        break;
      }
    }

    if (vcc != NULL) {
    
      RED4ext::ent::SlotComponent *vs = NULL;

      for (auto const &handle : entity->componentsStorage.components) {
        auto component = handle.GetPtr();
        if (component->GetNativeType() == rtti->GetClass("entSlotComponent")) {
          if (component->name == "vehicle_slots") {
            vs = reinterpret_cast<RED4ext::ent::SlotComponent *>(component);
            break;
          }
        }
      }

      if (vs != NULL) {
        {
          auto slot = reinterpret_cast<RED4ext::ent::Slot *>(rtti->GetClass("entSlot")->CreateInstance());
          slot->boneName = "roof_border_front";
          slot->slotName = "roof_border_front";
          vs->slots.EmplaceBack(*slot);
          vs->slotIndexLookup.Emplace(slot->slotName, vs->slots.size - 1);
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
        sprintf(className, "FlightConfiguration_%s", entity->currentAppearance.ToString());

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
          spdlog::info("Looked for class '{}' using '{}'", className, configurationCls->name.ToString());
          auto configuration = reinterpret_cast<IFlightConfiguration *>(configurationCls->CreateInstance(true));
          configurationCls->ConstructCls(configuration);

          auto handle = RED4ext::Handle<IFlightConfiguration>(configuration);
          configuration->ref = RED4ext::WeakHandle(*reinterpret_cast<RED4ext::Handle<RED4ext::ISerializable> *>(&handle));
          configuration->unk30 = configurationCls;

          fc->configuration = handle;

          configuration->Setup(fc);
          configuration->AddSlots(vs);
          configuration->AddMeshes(entity, vcc);
        } else {
          spdlog::info("Looked for class '{}'", className);
        }
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

    
    auto mc = rtti->GetClass("entMeshComponent");
    mc->props.EmplaceBack(RED4ext::CProperty::Create(rtti->GetType("Vector3"), "visualScale", nullptr, 0x178));
  }
};

REGISTER_FLIGHT_MODULE(EntityAddComponentModule);