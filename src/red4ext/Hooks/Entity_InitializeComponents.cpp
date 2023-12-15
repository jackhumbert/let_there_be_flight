#include "Addresses.hpp"
#include "Flight/Component.hpp"
#include "LoadResRef.hpp"
#include "Utils/FlightModule.hpp"
#include <RED4ext/Common.hpp>
#include <RED4ext/Scripting/Natives/Generated/WidgetHudComponent.hpp>
#include <RED4ext/Scripting/Natives/Generated/ink/HudEntriesResource.hpp>

// right before components are processed for entities, and an appropriate time to insert our own
// can also look for string "Entity/InitializeComponents"

// pre-2.0
/// @pattern 48 89 54 24 10 55 53 56 57 41 54 41 55 41 56 41 57 48 8D AC 24 ? FB FF FF 48 81 EC ? 05 00 00
/// @nth 0/2

// 2.0+
/// @pattern 48 89 5C 24 18 55 56 57 41 54 41 55 41 56 41 57 48 8D AC 24 E0 FD FF FF 48 81 EC 20 03 00 00 8A
void Entity_InitializeComponents(RED4ext::ent::Entity *entity, void *a2, void *a3);

REGISTER_FLIGHT_HOOK(void __fastcall, Entity_InitializeComponents, RED4ext::ent::Entity *entity, void *a2, void *a3) {
  auto rtti = RED4ext::CRTTISystem::Get();

  auto type = entity->GetNativeType();
  auto isVehicle = false;
  auto vehicleClass = rtti->GetClass("vehicleBaseObject");
  do {
    isVehicle |= type == vehicleClass;
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
    // entity->componentsStorage.components.EmplaceBack(h);

    RED4ext::ent::VisualControllerComponent *vcc = nullptr;
    RED4ext::vehicle::ChassisComponent *chassis = nullptr;
    RED4ext::game::OccupantSlotComponent *osc = nullptr;
    RED4ext::ent::SlotComponent *vs = nullptr;
    for (auto const &handle : entity->componentsStorage.components) {
      auto component = handle.GetPtr();
      if (vcc == nullptr && component->GetNativeType() == rtti->GetClass("entVisualControllerComponent")) {
        vcc = reinterpret_cast<RED4ext::ent::VisualControllerComponent *>(component);
      }
      if (chassis == nullptr && component->GetNativeType() == rtti->GetClass("vehicleChassisComponent")) {
        chassis = reinterpret_cast<RED4ext::vehicle::ChassisComponent *>(component);
      }
      if (vs == nullptr && component->GetNativeType() == rtti->GetClass("entSlotComponent")) {
        if (component->name == "vehicle_slots") {
          vs = reinterpret_cast<RED4ext::ent::SlotComponent *>(component);
        }
      }
      if (osc == nullptr && component->GetNativeType() == rtti->GetClass("gameOccupantSlotComponent")) {
        osc = reinterpret_cast<RED4ext::game::OccupantSlotComponent *>(component);
      }
    }

    // if (chassis != NULL) {
    //
    // }

    if (vcc != nullptr && vs != nullptr) {
      {
        // auto slot = reinterpret_cast<RED4ext::ent::Slot *>(rtti->GetClass("entSlot")->CreateInstance());
        // slot->boneName = "roof_border_front";
        // slot->slotName = "roof_border_front";
        // vs->slots.EmplaceBack(*slot);
        // vs->slotIndexLookup.Emplace(slot->slotName, vs->slots.size - 1);
      }
      // FlightWeapons::AddWeaponSlots(vs);

      auto configurationCls = IFlightConfiguration::GetConfigurationClass(entity);

      if (configurationCls) {
        // spdlog::info("Looked for class '{}' using '{}'", className, configurationCls->name.ToString());
        auto configuration = reinterpret_cast<IFlightConfiguration *>(configurationCls->CreateInstance(true));
        configurationCls->ConstructCls(configuration);

        auto handle = RED4ext::Handle<IFlightConfiguration>(configuration);
        configuration->ref = RED4ext::WeakHandle(*reinterpret_cast<RED4ext::Handle<RED4ext::ISerializable> *>(&handle));
        configuration->nativeType = configurationCls;
        configuration->component = RED4ext::Handle<FlightComponent>(fc);
        configuration->component.refCount->IncRef();
        fc->configuration = handle;
        handle.refCount->IncRef();

        configuration->Setup(vehicle);
        configuration->AddSlots(vs);
        // configuration->AddMeshes(entity, vcc);
      } else {
        // spdlog::info("Looked for class '{}'", className);
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
    //  auto mtb = (RED4ext::world::ui::MeshTargetBinding
    //  *)rtti->GetClass("worlduiMeshTargetBinding")->CreateInstance(); mtb->bindName = "flight_screen_info";
    //  wwc->meshTargetBinding = RED4ext::Handle<RED4ext::world::ui::MeshTargetBinding>(mtb);

    //  entity->componentsStorage.components.EmplaceBack(RED4ext::Handle<RED4ext::WorldWidgetComponent>(wwc));
    //}
    //{
    //  auto gpsp =
    //      (RED4ext::game::projectile::SpawnComponent
    //      *)rtti->GetClass("gameprojectileSpawnComponent")->CreateInstance();
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
      // auto resource =
      // RED4ext::Ref<RED4ext::ink::HudEntriesResource>("user\\jackhumbert\\widgets\\hud_flight.inkhud", true);
      whc->hudEntriesResource.path = "user\\jackhumbert\\widgets\\hud_flight.inkhud";
      // whc->hudEntriesResource.token = resource.token;
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