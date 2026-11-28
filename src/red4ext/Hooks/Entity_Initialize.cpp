
#include "Flight/Component.hpp"
#include "LoadResRef.hpp"
#include "Utils/FlightModule.hpp"
#include <RED4ext/Common.hpp>
#include <RED4ext/Scripting/Natives/Generated/WidgetHudComponent.hpp>
#include <RED4ext/Scripting/Natives/Generated/ink/HudEntriesResource.hpp>
#include <Extensions/MeshComponent.hpp>

static constexpr const char *FLIGHT_HUD_ENTRIES = "user\\jackhumbert\\widgets\\hud_flight.inkhud";

// Reads a Bool class default off hudFlightController (the "Flight UI Settings" page in Mod
// Settings). Mod Settings writes the user's choice into CClass::defaults (mod_settings
// ModClass::SetDefaultValue), so this is the current setting without needing a HUD
// controller instance. Unknown class/property/type falls back to aFallback.
static bool GetFlightHudSetting(RED4ext::CRTTISystem *rtti, RED4ext::CName aProperty, bool aFallback) {
  auto cls = rtti->GetClass("hudFlightController");
  if (!cls) {
    return aFallback;
  }
  auto variant = cls->defaults.Get(aProperty);
  if (!variant || !*variant) {
    return aFallback;
  }
  auto type = (*variant)->GetType();
  if (!type || type->GetName() != RED4ext::CName("Bool")) {
    return aFallback;
  }
  auto data = (*variant)->GetDataPtr();
  return data ? *reinterpret_cast<bool *>(data) : aFallback;
}

// "Flight UI Settings > Enabled".
static bool IsFlightHudEnabled(RED4ext::CRTTISystem *rtti) {
  return GetFlightHudSetting(rtti, "enabled", true);
}

// "Flight UI Settings > HUD attached to player (experimental)".
static bool IsFlightHudAttachedToPlayer(RED4ext::CRTTISystem *rtti) {
  return GetFlightHudSetting(rtti, "attachToPlayer", true);
}

/// @hash 3490519617
// void Entity_Initialize(RED4ext::ent::Entity *entity, void *a2, void *a3);

// void ent::Entity::Initialize(job::Builder &, ent::EntitySetupContext const &)
REGISTER_FLIGHT_HOOK_HASH(void, 3490519617, Entity_Initialize, EntityExt *entity, void *a2, void *a3) {
  auto rtti = RED4ext::CRTTISystem::Get();

  if (!entity) {
    Entity_Initialize_Original(entity, a2, a3);
    return;
  }

  // The flight HUD is an entry in the player's own HUD (ResourceSerializer_OnDependenciesReady.cpp adds it to every
  // HUD entries resource as it loads), so the player's HUD component is left as it is.

  auto isVehicle = entity->IsOfClass(rtti->GetClass("vehicleBaseObject"));

  auto vehicle = reinterpret_cast<RED4ext::vehicle::BaseObject *>(entity);

  auto canEnterFlight_func = rtti->GetClass("vehicleBaseObject")->GetFunction("CanEnterFlight");
  bool canEnterFlight = false;
  if (isVehicle && canEnterFlight_func) {
    RED4ext::ExecuteFunction(vehicle, canEnterFlight_func, &canEnterFlight);
  }

  if (isVehicle && canEnterFlight && !Utils::LooksLikeDynArray(vehicle->componentsStorage.components)) {
    LTBF_WARN_ONCE("[Entity_Initialize] componentsStorage.components header looks corrupt (size {} capacity {}); flight component not added",
                   vehicle->componentsStorage.components.size, vehicle->componentsStorage.components.capacity);
    canEnterFlight = false;
  }

  if (isVehicle && canEnterFlight) {
    auto fc = (FlightComponent *)FlightComponent::GetRTTIType()->CreateInstance(true);
    fc->name = "flightComponent";
    fc->id = RED4ext::CRUID::Next();
    // fc->entity = entity;

    auto fch = RED4ext::Handle<FlightComponent>(fc);
    vehicle->componentsStorage.components.EmplaceBack(fch);

    // auto fc = (FlightComponent*)FlightComponent::GetRTTIType()->CreateInstance(true);
    // fc->name = "flightComponent";
    // auto h = RED4ext::Handle<FlightComponent>(fc);
    // h.refCount->IncRef();
    // h.refCount->IncRef();
    // entity->componentsStorage.components.EmplaceBack(h);

    // auto const vcc = entity->GetComponent<RED4ext::ent::VisualControllerComponent>();
    // auto chassis = entity->GetComponent<RED4ext::vehicle::ChassisComponent>();
    auto const vs = entity->FindComponent<RED4ext::ent::SlotComponent>("vehicle_slots");
    // auto osc = entity->GetComponent<RED4ext::game::OccupantSlotComponent>();

    if (vs) {
      // {
        // auto slot = reinterpret_cast<RED4ext::ent::Slot *>(rtti->GetClass("entSlot")->CreateInstance(true));
        // slot->boneName = "roof_border_front";
        // slot->slotName = "roof_border_front";
        // vs->slots.EmplaceBack(*slot);
        // vs->slotIndexLookup.Emplace(slot->slotName, vs->slots.size - 1);
      // }
      // FlightWeapons::AddWeaponSlots(vs);

      auto configurationCls = IFlightConfiguration::GetConfigurationClass(entity);

      if (configurationCls) {
        // spdlog::info("Looked for class '{}' using '{}'", className, configurationCls->name.ToString());
        auto configuration = configurationCls->CreateInstance<IFlightConfiguration *>(true);
        if (!configuration) {
          LTBF_WARN_ONCE("[Entity_Initialize] could not instantiate flight configuration class '{}'", configurationCls->name.ToString());
          Entity_Initialize_Original(entity, a2, a3);
          return;
        }
        configurationCls->ConstructCls(configuration);

        // auto handle = RED4ext::Handle<IFlightConfiguration>(configuration);
        // configuration->ref = RED4ext::WeakHandle(*reinterpret_cast<RED4ext::Handle<RED4ext::ISerializable> *>(&handle));
        configuration->nativeType = configurationCls;
        configuration->component = RED4ext::WeakHandle<FlightComponent>(fch);
        configuration->component.refCount->IncRef();
        fc->configuration = RED4ext::Handle<IFlightConfiguration>(configuration);
        // fc->configuration.refCount->IncRef();

        configuration->Setup(vehicle);
        // configuration->AddMeshes(entity, vcc);
        if (Utils::LooksLikeDynArray(configuration->thrusters, 64)) {
          for (auto const & thruster : configuration->thrusters) {
            if (thruster && thruster->meshComponent) {
              thruster->meshComponent.refCount->IncRef();
              entity->AddComponent(thruster->meshComponent);
            }
          }
        } else {
          LTBF_WARN_ONCE("[Entity_Initialize] configuration->thrusters header looks corrupt (size {} capacity {}); thruster meshes not added",
                         configuration->thrusters.size, configuration->thrusters.capacity);
        }
        configuration->AddSlots(vs);
      } else {
        // spdlog::info("Looked for class '{}'", className);
      }
    }

    // UI
    //{
    //  // MeshComponent
    //  auto mc = (RED4ext::ent::MeshComponent *)rtti->GetClass("entMeshComponent")->CreateInstance(true);

    //  RED4ext::CName mesh = "user\\jackhumbert\\meshes\\flight_ui.mesh";
    //  mc->mesh.ref = mesh;
    //  mc->name = "flight_screen";
    //  mc->meshAppearance = "default";

    //  auto mcHandle = RED4ext::Handle<RED4ext::ent::MeshComponent>(mc);
    //  entity->componentsStorage.components.EmplaceBack(mcHandle);
    //}
    //{
    //  // WorldWidgetComponent
    //  auto wwc = (RED4ext::WorldWidgetComponent *)rtti->GetClass("WorldWidgetComponent")->CreateInstance(true);

    //  wwc->name = "flight_ui";

    //  RED4ext::CName fc = "user\\jackhumbert\\widgets\\flight_ui.inkwidget";
    //  wwc->widgetResource.ref = fc;

    //  auto mtb = (RED4ext::world::ui::MeshTargetBinding
    //  *)rtti->GetClass("worlduiMeshTargetBinding")->CreateInstance(true); mtb->bindName = "flight_screen";
    //  wwc->meshTargetBinding = RED4ext::Handle<RED4ext::world::ui::MeshTargetBinding>(mtb);

    //  auto wwcHandle = RED4ext::Handle<RED4ext::WorldWidgetComponent>(wwc);
    //  entity->componentsStorage.components.EmplaceBack(wwcHandle);
    //}

    // UI Info Panel
    //{
    //  // MeshComponent
    //  auto mc = (RED4ext::ent::MeshComponent *)rtti->GetClass("entMeshComponent")->CreateInstance(true);

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
    //  auto wwc = (RED4ext::WorldWidgetComponent *)rtti->GetClass("WorldWidgetComponent")->CreateInstance(true);

    //  wwc->name = "flight_ui_info";

    //  RED4ext::CName fc = "user\\jackhumbert\\widgets\\flight_ui.inkwidget";
    //  wwc->widgetResource.ref = fc;
    //  wwc->spawnDistanceOverride = 20.0;
    //  wwc->sceneWidgetProperties.renderingPlane = RED4ext::ERenderingPlane::RPl_Weapon;
    //  auto mtb = (RED4ext::world::ui::MeshTargetBinding
    //  *)rtti->GetClass("worlduiMeshTargetBinding")->CreateInstance(true); mtb->bindName = "flight_screen_info";
    //  wwc->meshTargetBinding = RED4ext::Handle<RED4ext::world::ui::MeshTargetBinding>(mtb);

    //  entity->componentsStorage.components.EmplaceBack(RED4ext::Handle<RED4ext::WorldWidgetComponent>(wwc));
    //}
    //{
    //  auto gpsp =
    //      (RED4ext::game::projectile::SpawnComponent
    //      *)rtti->GetClass("gameprojectileSpawnComponent")->CreateInstance(true);
    //  gpsp->name = "projectileSpawn8722";
    //  gpsp->projectileTemplates.EmplaceBack("exploding_bullet");
    //  auto htb = (RED4ext::ent::HardTransformBinding *)rtti->GetClass("entHardTransformBinding")->CreateInstance(true);
    //  htb->bindName = "vehicle_slots";
    //  htb->slotName = "ThrusterFL";
    //  gpsp->parentTransform = RED4ext::Handle<RED4ext::ent::ITransformBinding>(htb);
    //  entity->componentsStorage.components.EmplaceBack(RED4ext::Handle<RED4ext::game::projectile::SpawnComponent>(gpsp));
    //}

    // Fallback path (developer value "attachToPlayer" off): the FlightHUD WidgetHudComponent
    // rides on the vehicle, which makes the engine swap the whole HUD to hud_flight.inkhud on
    // every mount and back on every unmount. inkHUDLayer holds one inkHudEntriesResource at a
    // time and respawns every HUD entry when it changes: that was the enter/exit hitch and the
    // "HUD reset" reports. Skip it when the flight UI is disabled in Mod Settings; the
    // controller would only hide itself.
    if (!IsFlightHudEnabled(rtti)) {
      LTBF_WARN_ONCE("[Entity_Initialize] flight UI disabled in Mod Settings; FlightHUD component not added to vehicles");
    } else if (IsFlightHudAttachedToPlayer(rtti)) {
      LTBF_WARN_ONCE("[Entity_Initialize] FlightHUD is an entry of the player's HUD; not adding it to vehicles");
    } else {
      auto whc = rtti->GetClass("WidgetHudComponent")->CreateInstance<RED4ext::WidgetHudComponent *>(true);
      whc->name = "FlightHUD";
      whc->id = RED4ext::CRUID::Next();
      // auto resource =
      // RED4ext::Ref<RED4ext::ink::HudEntriesResource>("user\\jackhumbert\\widgets\\hud_flight.inkhud", true);
      whc->hudEntriesResource.path = FLIGHT_HUD_ENTRIES;
      // whc->hudEntriesResource.token = resource.token;
      LoadResRef<RED4ext::ink::HudEntriesResource>(&whc->hudEntriesResource.path, &whc->hudEntriesResource.token, false);
      entity->componentsStorage.components.EmplaceBack(RED4ext::Handle<RED4ext::WidgetHudComponent>(whc));
      LTBF_WARN_ONCE("[Entity_Initialize] FlightHUD component added per vehicle (HUD attached to player is off)");
    }

    //{
    //  auto gcc = (RED4ext::game::CameraComponent *)rtti->GetClass("gameCameraComponent")->CreateInstance(true);
    //  gcc->name = "frontCamera";
    //  auto htb = (RED4ext::ent::HardTransformBinding *)rtti->GetClass("entHardTransformBinding")->CreateInstance(true);
    //  htb->bindName = "vehicle_slots";
    //  htb->slotName = "roof_border_front";
    //  gcc->parentTransform = RED4ext::Handle<RED4ext::ent::ITransformBinding>(htb);
    //  entity->componentsStorage.components.EmplaceBack(RED4ext::Handle<RED4ext::game::CameraComponent>(gcc));
    //}
  }

  Entity_Initialize_Original(entity, a2, a3);
}