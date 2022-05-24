#include "FlightModule.hpp"
#include "LoadResRef.hpp"
#include "FlightHelperWrapper.hpp"
#include <RED4ext/RED4ext.hpp>
#include <RED4ext/Scripting/Natives/Generated/ent/Entity.hpp>
#include <RED4ext/Scripting/Natives/Generated/vehicle/BaseObject.hpp>
#include <RED4ext/Scripting/Natives/Generated/ent/HardTransformBinding.hpp>



void VehicleUsesInertiaTensor(RED4ext::IScriptable *aContext, RED4ext::CStackFrame *aFrame, bool *aOut, int64_t a4) {
  aFrame->code++; // skip ParamEnd

  auto v = reinterpret_cast<RED4ext::vehicle::BaseObject *>(aContext);
  auto ps = v->physicsStruct;

  if (aOut) {
    *aOut = ps->usesInertiaTensor;
  }
}

void VehicleTurnOffAirControl(RED4ext::IScriptable *aContext, RED4ext::CStackFrame *aFrame, bool *aOut, int64_t a4) {
  aFrame->code++; // skip ParamEnd

  auto v = reinterpret_cast<RED4ext::vehicle::BaseObject *>(aContext);
  auto ac = v->airControl;

  ac->anglePID.X = 0.0;
  ac->velocityPID.X = 0.0;
  ac->yaw.multiplier = 0.0;
  ac->roll.multiplier = 0.0;
  ac->pitch.multiplier = 0.0;
  ac->massReference = 0.0;

  if (aOut) {
    *aOut = true;
  }
}

void VehicleGetMomentOfInertiaScale(RED4ext::IScriptable *aContext, RED4ext::CStackFrame *aFrame,
                                    RED4ext::Vector3 *aOut, int64_t a4) {
  aFrame->code++; // skip ParamEnd

  auto v = reinterpret_cast<RED4ext::vehicle::BaseObject *>(aContext);
  auto ps = v->physicsStruct;

  if (aOut) {
    *aOut = ps->momentOfInertiaScale;
  }
}

void VehicleGetInertiaTensor(RED4ext::IScriptable *aContext, RED4ext::CStackFrame *aFrame, RED4ext::Matrix *aOut,
                             int64_t a4) {
  aFrame->code++; // skip ParamEnd

  auto v = reinterpret_cast<RED4ext::vehicle::BaseObject *>(aContext);
  auto ps = v->physicsStruct;

  if (aOut) {
    *aOut = ps->localInertiaTensor;
  }
}

void VehicleGetWorldInertiaTensor(RED4ext::IScriptable *aContext, RED4ext::CStackFrame *aFrame, RED4ext::Matrix *aOut,
                             int64_t a4) {
  aFrame->code++; // skip ParamEnd

  auto v = reinterpret_cast<RED4ext::vehicle::BaseObject *>(aContext);
  auto ps = v->physicsStruct;

  if (aOut) {
    *aOut = ps->worldInertiaTensor;
  }
}

void VehicleGetCenterOfMass(RED4ext::IScriptable *aContext, RED4ext::CStackFrame *aFrame, RED4ext::Vector3 *aOut,
                            int64_t a4) {
  aFrame->code++; // skip ParamEnd

  auto v = reinterpret_cast<RED4ext::vehicle::BaseObject *>(aContext);
  auto ps = v->physicsStruct;

  if (aOut) {
    *aOut = ps->centerOfMass;
  }
}

void VehicleGetAngularVelocity(RED4ext::IScriptable *aContext, RED4ext::CStackFrame *aFrame, RED4ext::Vector3 *aOut,
                               int64_t a4) {
  aFrame->code++; // skip ParamEnd

  auto v = reinterpret_cast<RED4ext::vehicle::BaseObject *>(aContext);
  auto ps = v->physicsStruct;

  if (aOut) {
    *aOut = ps->angularVelocity;
  }
}

void VehicleAddFlightHelper(RED4ext::IScriptable *aContext, RED4ext::CStackFrame *aFrame,
                            RED4ext::Handle<vehicle::flight::HelperWrapper> *aOut, int64_t a4) {
  // RED4ext::ScriptInstance fc;
  // RED4ext::GetParameter(aFrame, &fc);
  aFrame->code++; // skip ParamEnd

  auto v = reinterpret_cast<RED4ext::vehicle::BaseObject *>(aContext);
  auto p = v->physics;
  if (p) {
    auto helper = vehicle::flight::Helper::AddToDriverHelpers(&p->driveHelpers);
    if (aOut) {
      *aOut = helper;
    }
  } else {
    if (aOut) {
      *aOut = nullptr;
    }
  }
}

void VehicleGetComponentsUsingSlot(RED4ext::IScriptable *aContext, RED4ext::CStackFrame *aFrame,
                                   RED4ext::DynArray<RED4ext::WeakHandle<RED4ext::ent::IComponent>> *aOut, int64_t a4) {
  // RED4ext::ScriptInstance fc;
  // RED4ext::GetParameter(aFrame, &fc);
  RED4ext::CName slotName;
  RED4ext::GetParameter(aFrame, &slotName);
  aFrame->code++; // skip ParamEnd

  if (aOut) {
    auto rtti = RED4ext::CRTTISystem::Get();
    auto ipct = rtti->GetType("entIPlacedComponent");
    auto htb = rtti->GetType("endHardTransformBinding");
    *aOut = RED4ext::DynArray<RED4ext::WeakHandle<RED4ext::ent::IComponent>>();
    auto doubleCheck = RED4ext::DynArray<RED4ext::Handle<RED4ext::ent::IComponent>>();

    auto v = reinterpret_cast<RED4ext::vehicle::BaseObject *>(aContext);
    for (const auto &h : v->components) {
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
            aOut->EmplaceBack(wh);
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
      for (const auto &eh : *aOut) {
        auto ec = reinterpret_cast<RED4ext::ent::IComponent *>(eh.instance);
        if (htb->bindName == ec->name) {
          auto wh = RED4ext::WeakHandle<RED4ext::ent::IComponent>(h);
          aOut->EmplaceBack(wh);
          break;
        }
      }
    }
  }
}

struct VehicleObjectModule : FlightModule {
  void PostRegisterTypes() {
    auto rtti = RED4ext::CRTTISystem::Get();
    auto vbc = rtti->GetClass("vehicleBaseObject");
    vbc->props.PushBack(RED4ext::CProperty::Create(rtti->GetType("Bool"), "isOnGround", nullptr, 0x24C));
    // vbc->props.PushBack(RED4ext::CProperty::Create(
    //  rtti->GetType("WorldTransform"), "unkWorldTransform", nullptr, 0x330));
    // vbc->props.PushBack(RED4ext::CProperty::Create(
    //  rtti->GetType("handle:entIPlacedComponent"), "chassis", nullptr, 0x2D0));
    auto getInertiaTensor = RED4ext::CClassFunction::Create(vbc, "GetInertiaTensor", "GetInertiaTensor", &VehicleGetInertiaTensor, {.isNative = true});
    vbc->RegisterFunction(getInertiaTensor);

    auto getWorldInertiaTensor = RED4ext::CClassFunction::Create(vbc, "GetWorldInertiaTensor", "GetWorldInertiaTensor", &VehicleGetWorldInertiaTensor, {.isNative = true});
    vbc->RegisterFunction(getWorldInertiaTensor);

    auto getMomentOfInertiaScale = RED4ext::CClassFunction::Create(vbc, "GetMomentOfInertiaScale", "GetMomentOfInertiaScale", &VehicleGetMomentOfInertiaScale, {.isNative = true});
    vbc->RegisterFunction(getMomentOfInertiaScale);

    auto usesInertiaTensor = RED4ext::CClassFunction::Create(vbc, "UsesInertiaTensor", "UsesInertiaTensor", &VehicleUsesInertiaTensor, {.isNative = true});
    vbc->RegisterFunction(usesInertiaTensor);

    auto getCenterOfMass = RED4ext::CClassFunction::Create(vbc, "GetCenterOfMass", "GetCenterOfMass",  &VehicleGetCenterOfMass, {.isNative = true});
    vbc->RegisterFunction(getCenterOfMass);

    auto getAngularVelocity = RED4ext::CClassFunction::Create(vbc, "GetAngularVelocity", "GetAngularVelocity", &VehicleGetAngularVelocity, {.isNative = true});
    vbc->RegisterFunction(getAngularVelocity);

    auto turnOffAirControl = RED4ext::CClassFunction::Create(vbc, "TurnOffAirControl", "TurnOffAirControl",  &VehicleTurnOffAirControl, {.isNative = true});
    vbc->RegisterFunction(turnOffAirControl);

    auto addFlightHelper = RED4ext::CClassFunction::Create(vbc, "AddFlightHelper", "AddFlightHelper", &VehicleAddFlightHelper, {.isNative = true});
    vbc->RegisterFunction(addFlightHelper);

    auto getComponentsUsingSlot = RED4ext::CClassFunction::Create(vbc, "GetComponentsUsingSlot", "GetComponentsUsingSlot", &VehicleGetComponentsUsingSlot, {.isNative = true});
    vbc->RegisterFunction(getComponentsUsingSlot);
  }
};

REGISTER_FLIGHT_MODULE(VehicleObjectModule);