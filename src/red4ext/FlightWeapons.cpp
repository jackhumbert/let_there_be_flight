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

namespace FlightWeapons {

void __fastcall AddWeapons(RED4ext::vehicle::BaseObject *vehicle) {
  auto rtti = RED4ext::CRTTISystem::Get();
  {
    auto gas = (RED4ext::game::AttachmentSlots *)rtti->GetClass("gameAttachmentSlots")->AllocInstance();
    gas->name = "AttachmentSlots";
    {
      auto gapso = (RED4ext::game::AnimParamSlotsOption *)rtti->GetClass("gameAnimParamSlotsOption")->AllocInstance();
      gapso->slotID = "AttachmentSlots.PanzerCannon";
      gapso->paramName = "renderPlane";
      gas->animParams.EmplaceBack(*gapso);
    }
    {
      auto gapso = (RED4ext::game::AnimParamSlotsOption *)rtti->GetClass("gameAnimParamSlotsOption")->AllocInstance();
      gapso->slotID = "AttachmentSlots.WeaponLeft";
      gapso->paramName = "renderPlane";
      gas->animParams.EmplaceBack(*gapso);
    }
    {
      auto gapso = (RED4ext::game::AnimParamSlotsOption *)rtti->GetClass("gameAnimParamSlotsOption")->AllocInstance();
      gapso->slotID = "AttachmentSlots.WeaponRight";
      gapso->paramName = "renderPlane";
      gas->animParams.EmplaceBack(*gapso);
    }
    {
      auto gapso = (RED4ext::game::AnimParamSlotsOption *)rtti->GetClass("gameAnimParamSlotsOption")->AllocInstance();
      gapso->slotID = "AttachmentSlots.PanamVehicleTurret";
      gapso->paramName = "renderPlane";
      gas->animParams.EmplaceBack(*gapso);
    }
    {
      auto gapso = (RED4ext::game::AnimParamSlotsOption *)rtti->GetClass("gameAnimParamSlotsOption")->AllocInstance();
      gapso->slotID = "AttachmentSlots.PanzerHomingMissiles";
      gapso->paramName = "renderPlane";
      gas->animParams.EmplaceBack(*gapso);
    }
    {
      auto gapso = (RED4ext::game::AnimParamSlotsOption *)rtti->GetClass("gameAnimParamSlotsOption")->AllocInstance();
      gapso->slotID = "AttachmentSlots.PanzerCounterMeasuresLeft";
      gapso->paramName = "renderPlane";
      gas->animParams.EmplaceBack(*gapso);
    }
    {
      auto gapso = (RED4ext::game::AnimParamSlotsOption *)rtti->GetClass("gameAnimParamSlotsOption")->AllocInstance();
      gapso->slotID = "AttachmentSlots.PanzerCounterMeasuresRight";
      gapso->paramName = "renderPlane";
      gas->animParams.EmplaceBack(*gapso);
    }
    vehicle->componentsStorage.components.EmplaceBack(RED4ext::Handle<RED4ext::game::AttachmentSlots>(gas));
  }

  //{
  //  auto weapon = new RED4ext::vehicle::Weapon();
  //  weapon->attackRange = 300.0;
  //  weapon->canFriendlyFire = true;
  //  weapon->cycleTime = 0.5;
  //  weapon->genericShoot = true;
  //  weapon->genericTick = true;
  //  weapon->item = RED4ext::TweakDBID("Items.Panzer_Cannon");
  //  weapon->maxPitch = 90.0;
  //  weapon->maxYaw = 120.0;
  //  weapon->minPitch = -90.0;
  //  weapon->minYaw = -120.0;
  //  weapon->singleProjectileCycleTime = 1.0;
  //  weapon->singleShotProjectiles = 1;
  //  weapon->slot = RED4ext::TweakDBID("AttachmentSlots.PanzerCannon");
  //  weapon->weaponShootAnimEvent = "shoot_rocket";
  //  weapon->wholeBurstProjectiles = 1;
  //  vehicle->weapons.EmplaceBack(*weapon);
  //}
  //{
  //  auto weapon = new RED4ext::vehicle::Weapon();
  //  weapon->attackRange = 100.0;
  //  weapon->canFriendlyFire = true;
  //  weapon->cycleTime = 0.5;
  //  weapon->genericShoot = false;
  //  weapon->genericTick = true;
  //  weapon->item = RED4ext::TweakDBID("Items.Panzer_Counter_Measures_Launcher");
  //  weapon->maxPitch = 90.0;
  //  weapon->maxYaw = 180.0;
  //  weapon->minPitch = -90.0;
  //  weapon->minYaw = -180.0;
  //  weapon->singleProjectileCycleTime = 1.0;
  //  weapon->singleShotProjectiles = 1;
  //  weapon->slot = RED4ext::TweakDBID("AttachmentSlots.PanzerCounterMeasuresRight");
  //  weapon->weaponShootAnimEvent = "shoot_rocket";
  //  weapon->wholeBurstProjectiles = 1;
  //  vehicle->weapons.EmplaceBack(*weapon);
  //}
  //{
  //  auto weapon = new RED4ext::vehicle::Weapon();
  //  weapon->attackRange = 100.0;
  //  weapon->canFriendlyFire = true;
  //  weapon->cycleTime = 0.5;
  //  weapon->genericShoot = true;
  //  weapon->genericTick = true;
  //  weapon->item = RED4ext::TweakDBID("Items.Panzer_Missile_Launcher");
  //  weapon->maxPitch = 90.0;
  //  weapon->maxYaw = 180.0;
  //  weapon->minPitch = -90.0;
  //  weapon->minYaw = -180.0;
  //  weapon->singleProjectileCycleTime = 0.017;
  //  weapon->singleShotProjectiles = 1;
  //  weapon->slot = RED4ext::TweakDBID("AttachmentSlots.PanzerHomingMissiles");
  //  weapon->weaponShootAnimEvent = "shoot_rocket";
  //  weapon->wholeBurstProjectiles = 15;
  //  vehicle->weapons.EmplaceBack(*weapon);
  //}
}

void __fastcall AddWeaponSlots(RED4ext::ent::SlotComponent *sc) {
  auto rtti = RED4ext::CRTTISystem::Get();
  {
    auto slot = reinterpret_cast<RED4ext::ent::Slot *>(rtti->GetClass("entSlot")->AllocInstance());
    slot->boneName = "swingarm_front_left";
    slot->relativePosition.X -= 0.25;
    slot->relativePosition.Y += 0.75;
    slot->slotName = "PanzerCannon";
    sc->slots.EmplaceBack(*slot);
    sc->slotIndexLookup.Emplace(slot->slotName, sc->slots.size - 1);
  }
  {
    auto slot = reinterpret_cast<RED4ext::ent::Slot *>(rtti->GetClass("entSlot")->AllocInstance());
    slot->boneName = "swingarm_front_right";
    slot->relativePosition.X += 0.25;
    slot->relativePosition.Y += 0.75;
    slot->slotName = "PanamVehicleTurret";
    sc->slots.EmplaceBack(*slot);
    sc->slotIndexLookup.Emplace(slot->slotName, sc->slots.size - 1);
  }
  {
    auto slot = reinterpret_cast<RED4ext::ent::Slot *>(rtti->GetClass("entSlot")->AllocInstance());
    slot->boneName = "swingarm_front_right";
    slot->relativePosition.X += 0.25;
    slot->relativePosition.Y += 0.75;
    slot->slotName = "PanzerHomingMissiles";
    sc->slots.EmplaceBack(*slot);
    sc->slotIndexLookup.Emplace(slot->slotName, sc->slots.size - 1);
  }
  {
    auto slot = reinterpret_cast<RED4ext::ent::Slot *>(rtti->GetClass("entSlot")->AllocInstance());
    slot->boneName = "swingarm_front_left";
    slot->relativePosition.X -= 0.25;
    // slot->relativePosition.Y += 0.75;
    slot->slotName = "WeaponLeft";
    sc->slots.EmplaceBack(*slot);
    sc->slotIndexLookup.Emplace(slot->slotName, sc->slots.size - 1);
  }
  {
    auto slot = reinterpret_cast<RED4ext::ent::Slot *>(rtti->GetClass("entSlot")->AllocInstance());
    slot->boneName = "swingarm_front_left";
    slot->relativePosition.X -= 0.25;
    // slot->relativePosition.Y += 0.75;
    slot->slotName = "PanzerCounterMeasuresLeft";
    sc->slots.EmplaceBack(*slot);
    sc->slotIndexLookup.Emplace(slot->slotName, sc->slots.size - 1);
  }
  {
    auto slot = reinterpret_cast<RED4ext::ent::Slot *>(rtti->GetClass("entSlot")->AllocInstance());
    slot->boneName = "swingarm_front_right";
    slot->relativePosition.X += 0.25;
    // slot->relativePosition.Y += 0.75;
    slot->slotName = "WeaponRight";
    sc->slots.EmplaceBack(*slot);
    sc->slotIndexLookup.Emplace(slot->slotName, sc->slots.size - 1);
  }
  {
    auto slot = reinterpret_cast<RED4ext::ent::Slot *>(rtti->GetClass("entSlot")->AllocInstance());
    slot->boneName = "swingarm_front_right";
    slot->relativePosition.X += 0.25;
    // slot->relativePosition.Y += 0.75;
    slot->slotName = "PanzerCounterMeasuresRight";
    sc->slots.EmplaceBack(*slot);
    sc->slotIndexLookup.Emplace(slot->slotName, sc->slots.size - 1);
  }
}

} // namespace FlightWeapons