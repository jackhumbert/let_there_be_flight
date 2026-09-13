#include "Utils/FlightModule.hpp"
#include "LoadResRef.hpp"
#include <RED4ext/Common.hpp>
#include <RED4ext/Scripting/Natives/Generated/ent/Entity.hpp>
#include <RED4ext/Scripting/Natives/Generated/vehicle/BaseObject.hpp>
#include <RED4ext/Scripting/Natives/vehiclePhysics.hpp>
#include <RED4ext/Scripting/Natives/vehiclePhysicsData.hpp>
#include <RED4ext/Scripting/Natives/vehicleWeapon.hpp>
#include <RED4ext/Scripting/Natives/Generated/ent/AnimatedComponent.hpp>
#include <RED4ext/Scripting/Natives/Generated/ent/HardTransformBinding.hpp>
#include <RED4ext/Scripting/Natives/Generated/ent/PlaceholderComponent.hpp>
#include "Engine/RTTIExpansion.hpp"
#include "Utils/Utils.hpp"

class VehicleObject : public Engine::RTTIExpansion<VehicleObject, RED4ext::vehicle::BaseObject> {
public:
  inline bool UsesInertiaTensor() { return this->physicsData ? this->physicsData->usesInertiaTensor : false; }
  inline RED4ext::Vector3 GetMomentOfInertiaScale() { return this->physicsData ? this->physicsData->momentOfInertiaScale : RED4ext::Vector3(1.0, 1.0, 1.0); }
  inline RED4ext::Matrix GetInertiaTensor() { return this->physicsData ? this->physicsData->localInertiaTensor : RED4ext::Matrix(); }
  inline RED4ext::Matrix GetGlobalInertiaTensor() { return this->physicsData ? this->physicsData->worldInertiaTensor : RED4ext::Matrix(); }
  inline RED4ext::Vector3 GetCenterOfMass() { return this->physicsData ? this->physicsData->centerOfMass : RED4ext::Vector3(0.0, 0.0, 0.0); }
  inline RED4ext::Vector3 GetAngularVelocity() { return this->physicsData ? this->physicsData->angularVelocity : RED4ext::Vector3(0.0, 0.0, 0.0); }
  inline void EnableGravity(bool gravity) { if (this->physicsData) this->physicsData->unk1B0 = gravity; }
  inline bool HasGravity() { return this->physicsData ? this->physicsData->unk1B0 : true; }
  inline void EndActions() { this->actionInterface.EndActions(); }

  inline bool TurnOffAirControl() {
    auto ac = this->airControl;
    if (!ac) {
      LTBF_WARN_ONCE("[VehicleObject] TurnOffAirControl: vehicle has no airControl");
      return false;
    }

    ac->anglePID.X = 0.0;
    ac->velocityPID.X = 0.0;
    ac->yaw.multiplier = 0.0;
    ac->roll.multiplier = 0.0;
    ac->pitch.multiplier = 0.0;
    ac->massReference = 0.0;

    return true;
  }

  
  inline RED4ext::Quaternion GetWeaponPlaceholderOrientation(int index) {
    if (index >= 0 && Utils::LooksLikeDynArray(this->weapons, 64) && this->weapons.size > (uint32_t)index && this->weapons[index].weaponObject) {
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
      if (component && component->GetNativeType() == rtti->GetClass("entAnimatedComponent")) {
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
    auto ipct = rtti->GetClass("entIPlacedComponent");
    auto htbCls = rtti->GetClass("entHardTransformBinding");
    auto ra = RED4ext::DynArray<RED4ext::WeakHandle<RED4ext::ent::IComponent>>();
    auto doubleCheck = RED4ext::DynArray<RED4ext::Handle<RED4ext::ent::IComponent>>();

    if (!Utils::LooksLikeDynArray(this->componentsStorage.components)) {
      LTBF_WARN_ONCE("[VehicleObject] GetComponentsUsingSlot: componentsStorage.components header looks corrupt");
      return ra;
    }

    // returns the binding when the component is an entIPlacedComponent whose
    // parentTransform is an entHardTransformBinding, otherwise nullptr
    auto asHardBound = [&](RED4ext::ent::IComponent *c) -> RED4ext::ent::HardTransformBinding * {
      if (!c) {
        return nullptr;
      }
      auto ct = c->GetNativeType();
      if (!ct || !ipct || !ct->IsA(ipct)) {
        return nullptr;
      }
      auto ipc = reinterpret_cast<RED4ext::ent::IPlacedComponent *>(c);
      if (!ipc->parentTransform) {
        return nullptr;
      }
      auto ptType = ipc->parentTransform->GetNativeType();
      if (!ptType || !htbCls || !ptType->IsA(htbCls)) {
        return nullptr;
      }
      return reinterpret_cast<RED4ext::ent::HardTransformBinding *>(ipc->parentTransform.instance);
    };

    for (const auto &h : this->componentsStorage.components) {
      auto htb = asHardBound(h.GetPtr());
      if (htb) {
        if (htb->slotName == slotName) {
          auto wh = RED4ext::WeakHandle<RED4ext::ent::IComponent>(h);
          ra.EmplaceBack(wh);
        } else if (htb->slotName.hash == 0) {
          doubleCheck.EmplaceBack(h);
        }
      }
    }
    // probably only need to go one deep
    for (const auto &h : doubleCheck) {
      auto htb = asHardBound(h.GetPtr());
      if (!htb) {
        continue;
      }
      for (const auto &eh : ra) {
        auto ec = reinterpret_cast<RED4ext::ent::IComponent *>(eh.instance);
        if (ec && htb->bindName == ec->name) {
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
    if (!Utils::LooksLikeDynArray(this->weapons, 64)) {
      LTBF_WARN_ONCE("[VehicleObject] GetWeapons: weapons header looks corrupt (size {} capacity {})", this->weapons.size, this->weapons.capacity);
      return weapons;
    }
    for (const auto &weapon : this->weapons) {
      if (weapon.weaponObject) {
        weapon.weaponObject.refCount->IncRef();
        weapons.EmplaceBack(weapon.weaponObject);
      }
    }
    return weapons;
  }

  
  void __fastcall ResetQuestEnforceTransform() {
    // RED4ext::RelocFunc<decltype(&RED4ext::vehicle::PersistentDataPS::ResetQuestEnforceTransform)> call(
    //     vehiclePersistentDataPS_ResetQuestEnforceTransform_Addr);
    // call(this->PersistentDataPS.instance);
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
    aType->AddFunction<&VehicleObject::ForceEnablePhysics>("ForceEnablePhysics");
    aType->AddFunction<&VehicleObject::EndActions>("EndActions");

    aType->AddFunction<&VehicleObject::TurnOffAirControl>("TurnOffAirControl");
    aType->AddFunction<&VehicleObject::GetWeaponPlaceholderOrientation>("GetWeaponPlaceholderOrientation");
    aType->AddFunction<&VehicleObject::GetComponentsUsingSlot>("GetComponentsUsingSlot");
    aType->AddFunction<&VehicleObject::GetWeapons>("GetWeapons");
    aType->AddFunction<&VehicleObject::ResetQuestEnforceTransform>("ResetQuestEnforceTransform");
  }
};