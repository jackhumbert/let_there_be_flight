#include "FlightModule.hpp"
#include <RED4ext/RED4ext.hpp>
#include <RED4ext/Scripting/Natives/Generated/ent/Entity.hpp>
#include <RED4ext/Scripting/Natives/Generated/ent/EntityTemplate.hpp>
#include <RED4ext/Scripting/Natives/Generated/ent/IComponent.hpp>
#include <RED4ext/Scripting/Natives/Generated/ent/IPlacedComponent.hpp>
#include <RED4ext/Scripting/Natives/Generated/ent/MeshComponent.hpp>
#include <RED4ext/Scripting/Natives/Generated/ent/PhysicalMeshComponent.hpp>
#include <RED4ext/Scripting/Natives/Generated/ent/SlotComponent.hpp>
#include <RED4ext/Scripting/Natives/Generated/ent/VisualControllerComponent.hpp>
#include <RED4ext/Scripting/Natives/Generated/ent/ITransformBinding.hpp>
#include <RED4ext/Scripting/Natives/Generated/WorldWidgetComponent.hpp>
#include <RED4ext/Scripting/Natives/Generated/ent/HardTransformBinding.hpp>
#include <RED4ext/Scripting/Natives/Generated/world/ui/MeshTargetBinding.hpp>
#include <RED4ext/Scripting/Natives/Generated/physics/FilterData.hpp>
#include "LoadResRef.hpp"

RED4ext::ent::PhysicalMeshComponent *CreateThrusterEngine(RED4ext::CRTTISystem *rtti,
                                                          RED4ext::CName name, RED4ext::CName slot) {
  RED4ext::CName mesh = "user\\jackhumbert\\meshes\\engine_corpo.mesh";
  auto mc = (RED4ext::ent::PhysicalMeshComponent *)rtti->GetClass("entPhysicalMeshComponent")->AllocInstance();
  mc->mesh.ref = mesh;
  //mc->isEnabled = false;
  mc->visualScale.X = 0.0;
  mc->visualScale.Y = 0.0;
  mc->visualScale.Z = 0.0;
  mc->filterDataSource = RED4ext::physics::FilterDataSource::Collider;
  auto filterData = (RED4ext::physics::FilterData *)rtti->GetClass("physicsFilterData")->AllocInstance();
  mc->filterData = RED4ext::Handle<RED4ext::physics::FilterData>(filterData);
  mc->name = name;
  mc->motionBlurScale = 0.1;
  mc->meshAppearance = "default";
  mc->objectTypeID = RED4ext::ERenderObjectType::ROT_Vehicle;
  mc->LODMode = RED4ext::ent::MeshComponentLODMode::Appearance;
  auto htb = (RED4ext::ent::HardTransformBinding *)rtti->GetClass("entHardTransformBinding")->AllocInstance();
   htb->bindName = "thruster_slots";
  htb->slotName = slot;
  mc->parentTransform = RED4ext::Handle<RED4ext::ent::ITransformBinding>(htb);
  return mc;
}

void AddToController(RED4ext::CRTTISystem *rtti, RED4ext::ent::VisualControllerComponent *vcc,
                     RED4ext::ent::MeshComponent *mc) {
  auto vcd = reinterpret_cast<RED4ext::ent::VisualControllerDependency *>(
      rtti->GetClass("entVisualControllerDependency")->AllocInstance());
  vcd->appearanceName = mc->meshAppearance;
  vcd->componentName = mc->name;
  vcd->mesh.ref = mc->mesh.ref;
  vcc->appearanceDependency.EmplaceBack(*vcd);
}

// 48 89 54 24 10 55 53 56 57 41 54 41 55 41 56 41 57 48 8D AC 24 B8 FB FF FF 48 81 EC 48 05 00 00
void __fastcall Entity_InitializeComponents(RED4ext::ent::Entity *entity, uintptr_t, uintptr_t);
constexpr uintptr_t Entity_InitializeComponentsAddr = 0x140000C00 + 0x1035B00 - RED4ext::Addresses::ImageBase;
decltype(&Entity_InitializeComponents) Entity_InitializeComponents_Original;

void __fastcall Entity_InitializeComponents(RED4ext::ent::Entity* entity, uintptr_t a2, uintptr_t a3) {

  auto rtti = RED4ext::CRTTISystem::Get();
  
  auto type = entity->GetNativeType();
  auto isVehicle = false;
  do {
    isVehicle |= type == rtti->GetClass("vehicleCarBaseObject");
  } while (type = type->parent);

  if (isVehicle) {
    // entVisualControllerComponent
    RED4ext::ent::VisualControllerComponent *vcc = NULL;
    for (auto const &handle : entity->components) {
      auto component = handle.GetPtr();
      if (component->GetNativeType() == rtti->GetClass("entVisualControllerComponent")) {
        vcc = reinterpret_cast<RED4ext::ent::VisualControllerComponent *>(component);
        break;
      }
    }

    if (vcc != NULL) {
      {
        // SlotComponent
        auto sc = (RED4ext::ent::SlotComponent *)rtti->GetClass("entSlotComponent")->AllocInstance();
        sc->name = "thruster_slots";
        auto htb = (RED4ext::ent::HardTransformBinding *)rtti->GetClass("entHardTransformBinding")->AllocInstance();
        htb->bindName = "deformation_rig";
        sc->parentTransform = RED4ext::Handle<RED4ext::ent::ITransformBinding>(htb);

        {
          auto slot = reinterpret_cast<RED4ext::ent::Slot *>(rtti->GetClass("entSlot")->AllocInstance());
          slot->boneName = "swingarm_front_left";
          slot->slotName = "thruster_front_left";
          sc->slots.EmplaceBack(*slot);
          sc->slotIndexLookup.Emplace(slot->slotName, 0);
        }
        {
          auto slot = reinterpret_cast<RED4ext::ent::Slot *>(rtti->GetClass("entSlot")->AllocInstance());
          slot->boneName = "swingarm_front_right";
          slot->slotName = "thruster_front_right";
          sc->slots.EmplaceBack(*slot);
          sc->slotIndexLookup.Emplace(slot->slotName, 1);
        }
        {
          auto slot = reinterpret_cast<RED4ext::ent::Slot *>(rtti->GetClass("entSlot")->AllocInstance());
          slot->boneName = "swingarm_back_left";
          slot->slotName = "thruster_back_left";
          sc->slots.EmplaceBack(*slot);
          sc->slotIndexLookup.Emplace(slot->slotName, 2);
        }
        {
          auto slot = reinterpret_cast<RED4ext::ent::Slot *>(rtti->GetClass("entSlot")->AllocInstance());
          slot->boneName = "swingarm_back_right";
          slot->slotName = "thruster_back_right";
          sc->slots.EmplaceBack(*slot);
          sc->slotIndexLookup.Emplace(slot->slotName, 3);
        }

        auto scHandle = RED4ext::Handle<RED4ext::ent::SlotComponent>(sc);
        entity->components.EmplaceBack(scHandle);
      }

      {
        auto fl = CreateThrusterEngine(rtti, "ThrusterFL", "thruster_front_left");
        entity->components.EmplaceBack(RED4ext::Handle<RED4ext::ent::PhysicalMeshComponent>(fl));
        AddToController(rtti, vcc, fl);

        auto fr = CreateThrusterEngine(rtti, "ThrusterFR", "thruster_front_right");
        entity->components.EmplaceBack(RED4ext::Handle<RED4ext::ent::PhysicalMeshComponent>(fr));
        AddToController(rtti, vcc, fr);

        auto bl = CreateThrusterEngine(rtti, "ThrusterBL", "thruster_back_left");
        entity->components.EmplaceBack(RED4ext::Handle<RED4ext::ent::PhysicalMeshComponent>(bl));
        AddToController(rtti, vcc, bl);

        auto br = CreateThrusterEngine(rtti, "ThrusterBR", "thruster_back_right");
        entity->components.EmplaceBack(RED4ext::Handle<RED4ext::ent::PhysicalMeshComponent>(br));
        AddToController(rtti, vcc, br);
      }

      // UI
      {
        // MeshComponent
        auto mc = (RED4ext::ent::MeshComponent *)rtti->GetClass("entMeshComponent")->AllocInstance();

        RED4ext::CName mesh = "user\\jackhumbert\\meshes\\flight_ui.mesh";
        mc->mesh.ref = mesh;
        mc->name = "flight_screen";
        mc->meshAppearance = "screen_ui";

        auto mcHandle = RED4ext::Handle<RED4ext::ent::MeshComponent>(mc);
        entity->components.EmplaceBack(mcHandle);
      }
      //{
      //  // WorldWidgetComponent
      //  auto wwc = (RED4ext::WorldWidgetComponent *)rtti->GetClass("WorldWidgetComponent")->AllocInstance();

      //  wwc->name = "flight_ui";

      //  RED4ext::CName fc = "user\\jackhumbert\\widgets\\flight_ui.inkwidget";
      //  wwc->widgetResource.ref = fc;

      //  auto mtb = (RED4ext::world::ui::MeshTargetBinding
      //  *)rtti->GetClass("worlduiMeshTargetBinding")->AllocInstance(); mtb->bindName = "flight_screen";
      //  wwc->meshTargetBinding = RED4ext::Handle<RED4ext::world::ui::MeshTargetBinding>(mtb);

      //  auto wwcHandle = RED4ext::Handle<RED4ext::WorldWidgetComponent>(wwc);
      //  entity->components.EmplaceBack(wwcHandle);
      //}

      // UI Info Panel
      {
        // MeshComponent
        auto mc = (RED4ext::ent::MeshComponent *)rtti->GetClass("entMeshComponent")->AllocInstance();

        RED4ext::CName mesh = "user\\jackhumbert\\meshes\\flight_ui_info.mesh";
        mc->mesh.ref = mesh;
        mc->name = "flight_screen_info";
        mc->meshAppearance = "screen_ui";
        mc->forcedLodDistance = RED4ext::ent::ForcedLodDistance::Vehicle;

        auto mcHandle = RED4ext::Handle<RED4ext::ent::MeshComponent>(mc);
        entity->components.EmplaceBack(mcHandle);
      }
      {
        // WorldWidgetComponent
        auto wwc = (RED4ext::WorldWidgetComponent *)rtti->GetClass("WorldWidgetComponent")->AllocInstance();

        wwc->name = "flight_ui_info";

        RED4ext::CName fc = "user\\jackhumbert\\widgets\\flight_ui.inkwidget";
        wwc->widgetResource.ref = fc;
        wwc->spawnDistanceOverride = 20.0;
        auto mtb = (RED4ext::world::ui::MeshTargetBinding *)rtti->GetClass("worlduiMeshTargetBinding")->AllocInstance();
        mtb->bindName = "flight_screen_info";
        wwc->meshTargetBinding = RED4ext::Handle<RED4ext::world::ui::MeshTargetBinding>(mtb);

        auto wwcHandle = RED4ext::Handle<RED4ext::WorldWidgetComponent>(wwc);
        entity->components.EmplaceBack(wwcHandle);
      }
    }

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
  // ent->components.EmplaceBack(handle);
  ent->components.EmplaceBack(value);

  if (aOut) {
    *aOut = true;
  }
}

void EntityAddWorldWidgetComponent(RED4ext::IScriptable *aContext, RED4ext::CStackFrame *aFrame, bool *aOut, int64_t a4) {
  auto ent = reinterpret_cast<RED4ext::ent::Entity *>(aContext);

  aFrame->code++; // skip ParamEnd

  auto rtti = RED4ext::CRTTISystem::Get();


  // MeshComponent
  auto mc = (RED4ext::ent::MeshComponent*)rtti->GetClass("entMeshComponent")->AllocInstance();
  
  RED4ext::CName mesh =
      "base\\environment\\decoration\\advertising\\holograms\\common\\common_holograms_transparent_a_w500_h150.mesh";
  mc->mesh.ref = mesh;
  mc->name = "radio_screen";
  mc->meshAppearance = "screen_ui";
  mc->localTransform.Position.x.Bits = 216270;
  mc->visualScale.X = 10.0;
  mc->visualScale.X = 10.0;
  mc->visualScale.X = 10.0;

  auto mcHandle = RED4ext::Handle<RED4ext::ent::MeshComponent>(mc);
  ent->components.EmplaceBack(mcHandle);

  // WorldWidgetComponent
  auto wwc = (RED4ext::WorldWidgetComponent *)rtti->GetClass("WorldWidgetComponent")->AllocInstance();

  wwc->name = "radio_ui";

  RED4ext::CName fc = "base\\gameplay\\gui\\world\\radio\\radio_ui.inkwidget";
  wwc->widgetResource.ref = fc;

  auto mtb = (RED4ext::world::ui::MeshTargetBinding *)rtti->GetClass("worlduiMeshTargetBinding")->AllocInstance();
  mtb->bindName = "radio_screen";
  wwc->meshTargetBinding = RED4ext::Handle<RED4ext::world::ui::MeshTargetBinding>(mtb);

  auto wwcHandle = RED4ext::Handle<RED4ext::WorldWidgetComponent>(wwc);
  ent->components.EmplaceBack(wwcHandle);

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
//    ent->components.EmplaceBack(handle);
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
    aSdk->hooking->Attach(aHandle, RED4EXT_OFFSET_TO_ADDR(Entity_InitializeComponentsAddr), &Entity_InitializeComponents,
                          reinterpret_cast<void **>(&Entity_InitializeComponents_Original));
  }
  void Unload(const RED4ext::Sdk *aSdk, RED4ext::PluginHandle aHandle) {
    aSdk->hooking->Detach(aHandle, RED4EXT_OFFSET_TO_ADDR(Entity_InitializeComponentsAddr));
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