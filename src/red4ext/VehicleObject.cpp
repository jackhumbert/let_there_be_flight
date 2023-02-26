#include "FlightModule.hpp"
#include "LoadResRef.hpp"
#include "FlightHelperWrapper.hpp"
#include <RED4ext/RED4ext.hpp>
#include <RED4ext/Scripting/Natives/Generated/ent/Entity.hpp>
#include <RED4ext/Scripting/Natives/Generated/vehicle/BaseObject.hpp>
#include <RED4ext/Scripting/Natives/vehicleWeapon.hpp>
#include <RED4ext/Scripting/Natives/Generated/ent/AnimatedComponent.hpp>
#include <RED4ext/Scripting/Natives/Generated/ent/HardTransformBinding.hpp>
#include <RED4ext/Scripting/Natives/Generated/ent/PlaceholderComponent.hpp>
#include "Engine/RTTIExpansion.hpp"

class VehicleObject : public Engine::RTTIExpansion<VehicleObject, RED4ext::vehicle::BaseObject> {
public:
  inline bool UsesInertiaTensor() { return this->physicsData->usesInertiaTensor; }
  inline RED4ext::Vector3 GetMomentOfInertiaScale() { return this->physicsData->momentOfInertiaScale; }
  inline RED4ext::Matrix GetInertiaTensor() { return this->physicsData->localInertiaTensor; }
  inline RED4ext::Matrix GetGlobalInertiaTensor() { return this->physicsData->worldInertiaTensor; }
  inline RED4ext::Vector3 GetCenterOfMass() { return this->physicsData->centerOfMass; }
  inline RED4ext::Vector3 GetAngularVelocity() { return this->physicsData->angularVelocity; }
  inline void EnableGravity(bool gravity) { this->physicsData->unk1B0 = gravity; }
  inline bool HasGravity() { return this->physicsData->unk1B0; }
  inline void EndActions() { this->actionInterface.EndActions(); }

  inline bool TurnOffAirControl() {
    auto ac = this->airControl;

    ac->anglePID.X = 0.0;
    ac->velocityPID.X = 0.0;
    ac->yaw.multiplier = 0.0;
    ac->roll.multiplier = 0.0;
    ac->pitch.multiplier = 0.0;
    ac->massReference = 0.0;

    return true;
  }

  
  inline RED4ext::Quaternion GetWeaponPlaceholderOrientation(int index) {
    if (this->weapons.size > index && this->weapons[index].weaponObject) {
      auto ph = (RED4ext::ent::PlaceholderComponent *)this->weapons[index].weaponObject.GetPtr()->placeholder;
      if (ph) {
        return ph->worldTransform.Orientation;
      }
    }
    return {0.0, 0.0, 0.0, 1.0};
  }

  // not used yet
  inline void VehicleGetRig() {
    RED4ext::ent::AnimatedComponent *vehicleRig = NULL;
    auto rtti = RED4ext::CRTTISystem::Get();

    for (auto const &handle : this->componentsStorage.components) {
      auto component = handle.GetPtr();
      if (component->GetNativeType() == rtti->GetClass("entAnimatedComponent")) {
        vehicleRig = reinterpret_cast<RED4ext::ent::AnimatedComponent *>(component);
        break;
      }
    }

    if (vehicleRig != NULL) {
      // vehicleRig->rig;
    }
  }

  inline RED4ext::DynArray<RED4ext::WeakHandle<RED4ext::ent::IComponent>> GetComponentsUsingSlot(RED4ext::CName slotName) {
    auto rtti = RED4ext::CRTTISystem::Get();
    auto ipct = rtti->GetType("entIPlacedComponent");
    auto htb = rtti->GetType("endHardTransformBinding");
    auto ra = RED4ext::DynArray<RED4ext::WeakHandle<RED4ext::ent::IComponent>>();
    auto doubleCheck = RED4ext::DynArray<RED4ext::Handle<RED4ext::ent::IComponent>>();

    for (const auto &h : this->componentsStorage.components) {
      auto c = h.GetPtr();
      bool isIPC = false;
      auto ct = c->GetType();
      while (ct->parent) {
        if (ct == ipct) {
          isIPC = true;
        }
        ct = ct->parent;
      }
      if (isIPC) {
        auto ipc = reinterpret_cast<RED4ext::ent::IPlacedComponent *>(c);
        if (ipc->parentTransform) {
          auto htb = reinterpret_cast<RED4ext::ent::HardTransformBinding *>(ipc->parentTransform.instance);
          if (htb->slotName == slotName) {
            auto wh = RED4ext::WeakHandle<RED4ext::ent::IComponent>(h);
            ra.EmplaceBack(wh);
          } else if (htb->slotName.hash == 0) {
            doubleCheck.EmplaceBack(h);
          }
        }
      }
    }
    // probably only need to go one deep
    for (const auto &h : doubleCheck) {
      auto c = h.GetPtr();
      auto ipc = reinterpret_cast<RED4ext::ent::IPlacedComponent *>(c);
      auto htb = reinterpret_cast<RED4ext::ent::HardTransformBinding *>(ipc->parentTransform.instance);
      for (const auto &eh : ra) {
        auto ec = reinterpret_cast<RED4ext::ent::IComponent *>(eh.instance);
        if (htb->bindName == ec->name) {
          auto wh = RED4ext::WeakHandle<RED4ext::ent::IComponent>(h);
          ra.EmplaceBack(wh);
          break;
        }
      }
    }
    return ra;
  }

  inline RED4ext::DynArray < RED4ext::Handle<RED4ext::game::weapon::Object>> GetWeapons() {
    auto allocator = new RED4ext::Memory::DefaultAllocator();
    auto weapons = RED4ext::DynArray<RED4ext::Handle<RED4ext::game::weapon::Object>>(allocator);
    for (const auto &weapon : this->weapons) {
      if (weapon.weaponObject) {
        weapon.weaponObject.refCount->IncRef();
        weapons.EmplaceBack(weapon.weaponObject);
      }
    }
    return weapons;
  }

private:
  friend Descriptor;

  inline static void OnExpand(Descriptor *aType, RED4ext::CRTTISystem *) {
    aType->AddFunction<&VehicleObject::UsesInertiaTensor>("UsesInertiaTensor");
    aType->AddFunction<&VehicleObject::GetMomentOfInertiaScale>("GetMomentOfInertiaScale");
    aType->AddFunction<&VehicleObject::GetInertiaTensor>("GetInertiaTensor");
    aType->AddFunction<&VehicleObject::GetGlobalInertiaTensor>("GetGlobalInertiaTensor");
    aType->AddFunction<&VehicleObject::GetCenterOfMass>("GetCenterOfMass");
    aType->AddFunction<&VehicleObject::GetAngularVelocity>("GetAngularVelocity");
    aType->AddFunction<&VehicleObject::EnableGravity>("EnableGravity");
    aType->AddFunction<&VehicleObject::HasGravity>("HasGravity");
    aType->AddFunction<&VehicleObject::UnsetPhysicsStates>("UnsetPhysicsStates");
    aType->AddFunction<&VehicleObject::EndActions>("EndActions");

    aType->AddFunction<&VehicleObject::TurnOffAirControl>("TurnOffAirControl");
    aType->AddFunction<&VehicleObject::GetWeaponPlaceholderOrientation>("GetWeaponPlaceholderOrientation");
    aType->AddFunction<&VehicleObject::GetComponentsUsingSlot>("GetComponentsUsingSlot");
    aType->AddFunction<&VehicleObject::GetWeapons>("GetWeapons");
  }
};