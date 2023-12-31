#include "FlightConfiguration.hpp"
#include <RED4ext/Memory/Allocators.hpp>
#include <PhysX3.hpp>
#include <RED4ext/Scripting/Natives/Generated/physics/ColliderSphere.hpp>
#include <RED4ext/Scripting/Natives/Generated/physics/SimulationFilter.hpp>
#include <RED4ext/Scripting/Natives/Generated/physics/QueryFilter.hpp>
#include <RED4ext/Scripting/Natives/Generated/ent/SlotComponent.hpp>
#include <RED4ext/Scripting/Natives/Generated/physics/FilterData.hpp>
#include <RED4ext/Scripting/Natives/physicsProxyHelper.hpp>
#include <RED4ext/Scripting/Natives/physicsPhysicalSystemProxy.hpp>
#include <RED4ext/Scripting/Natives/Generated/ent/HardTransformBinding.hpp>

RED4ext::CClass* IFlightConfiguration::GetConfigurationClass(RED4ext::ent::Entity* entity) {
  auto rtti = RED4ext::CRTTISystem::Get();

  auto type = entity->GetNativeType();
  bool isCar = false;
  bool isBike = false;
  auto carClass = rtti->GetClass("vehicleCarBaseObject");
  auto bikeClass = rtti->GetClass("vehicleBikeBaseObject");
  do {
    isCar |= type == carClass;
    isBike |= type == bikeClass;
  } while (type = type->parent);

  bool isSixWheeler = false;

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
      configurationCls = rtti->GetClassByScriptName("CustomSixWheelCarFlightConfiguration");
      if (!configurationCls) {
        configurationCls = rtti->GetClassByScriptName("SixWheelCarFlightConfiguration");
      }
    } else if (isCar) {
      configurationCls = rtti->GetClassByScriptName("CustomCarFlightConfiguration");
      if (!configurationCls) {
        configurationCls = rtti->GetClassByScriptName("CarFlightConfiguration");
      }
    } else if (isBike) {
      configurationCls = rtti->GetClassByScriptName("CustomBikeFlightConfiguration");
      if (!configurationCls) {
        configurationCls = rtti->GetClassByScriptName("BikeFlightConfiguration");
      }
    }
  } else {
    spdlog::info("Found custom flight configuration class: {}", className);
  }
  return configurationCls;
}

void IFlightConfiguration::Setup(RED4ext::vehicle::BaseObject * vehicle) {

  this->thrusters = RED4ext::DynArray<RED4ext::Handle<IFlightThruster>>(new RED4ext::Memory::DefaultAllocator());
  auto onInit = GetType()->GetFunction("OnSetup");
  if (onInit) {
    auto rtti = RED4ext::CRTTISystem::Get();
    RED4ext::CStackType args[1];
    auto handle = RED4ext::Handle<RED4ext::vehicle::BaseObject>(vehicle);
    args[0] = RED4ext::CStackType(rtti->GetType("handle:vehicleBaseObject"), &handle);
    auto stack = RED4ext::CStack(this, args, 1, nullptr);
    onInit->Execute(&stack);
  }
}

void IFlightConfiguration::AddSlots(RED4ext::ent::SlotComponent *slotComponent) {
  auto rtti = RED4ext::CRTTISystem::Get();

  auto slot = reinterpret_cast<RED4ext::ent::Slot *>(rtti->GetClass("entSlot")->CreateInstance());
  slot->boneName = this->flightCameraBone;
  slot->slotName = "CustomFlightCamera";
  slot->relativePosition = this->flightCameraOffset;
  slotComponent->slots.EmplaceBack(*slot);
  slotComponent->slotIndexLookup.Emplace(slot->slotName, slotComponent->slots.size - 1);

  //for (auto thruster : thrusters) {
  //  auto slot = reinterpret_cast<RED4ext::ent::Slot *>(rtti->GetClass("entSlot")->CreateInstance());
  //  slot->boneName = thruster->boneName;
  //  slot->slotName = thruster->slotName;
  //  slot->relativePosition = thruster->relativePosition;
  //  slot->relativeRotation = thruster->relativeRotation;
  //  //slot->relativeRotation = RED4ext::Quaternion(0.0, 0.0, 0.0, 1.0);
  //  slotComponent->slots.EmplaceBack(*slot);
  //  slotComponent->slotIndexLookup.Emplace(slot->slotName, slotComponent->slots.size - 1);
  //}
}

void IFlightConfiguration::AddMeshes(RED4ext::ent::Entity *entity, RED4ext::ent::VisualControllerComponent *vcc) {
  for (auto thruster : thrusters) {
    auto mesh = CreateThrusterEngine(thruster->meshPath, thruster->meshName, thruster->slotName);
    thruster->meshComponent = RED4ext::Handle<RED4ext::ent::MeshComponent>(mesh);
    entity->componentsStorage.components.EmplaceBack(thruster->meshComponent);
    AddToController(vcc, mesh);
  }
}


void IFlightConfiguration::OnActivationCore() {
  auto rtti = RED4ext::CRTTISystem::Get();

  RED4ext::ent::SlotComponent *sc = NULL;
  RED4ext::vehicle::ChassisComponent *cc = NULL;

  auto scCls = rtti->GetClass("entSlotComponent");
  auto ccCls = rtti->GetClass("vehicleChassisComponent");
  auto filterDataCls = rtti->GetClass("physicsFilterData");

  for (auto const &handle : this->component->entity->componentsStorage.components) {
    auto component = handle.GetPtr();
    if (sc == NULL && component->GetNativeType() == scCls) {
      if (component->name == "vehicle_slots") {
        sc = reinterpret_cast<RED4ext::ent::SlotComponent *>(component);
      }
    } else if (cc == NULL && component->GetNativeType() == ccCls) {
      cc = reinterpret_cast<RED4ext::vehicle::ChassisComponent *>(component);
    }
  }

  if (cc != NULL && sc != NULL) {
    FlightComponent::Get((RED4ext::vehicle::BaseObject*)this->component->entity)->chassis = cc;
    RED4ext::physics::ProxyHelper proxyHelper(cc->proxyID, &cc->sharedMutex);

    auto key = (RED4ext::physics::PhysicalSystemProxy *)cc->proxyID.GetProxy();
    auto body = (physx::PxRigidDynamic *) key->bodies.entries[0];

    this->originalShapeCount = body->getNbShapes();

    auto filterData = (RED4ext::physics::FilterData*)filterDataCls->CreateInstance();
    filterData->LoadPreset("Vehicle Chassis");
    RED4ext::Vector3 unk140(1.0, 1.0, 1.0);
    RED4ext::Transform transform;
    int index = 0;

    for (auto const &thruster: this->thrusters) {
      RED4ext::Handle<RED4ext::physics::ICollider> collider;
      RED4ext::physics::ColliderSphere::createHandleWithRadius(&collider, 0.4);
      collider.refCount->IncRef();

      collider->material = "vehicle_chassis.physmat";

      auto wtCls = rtti->GetClass("WorldTransform");

      index = sc->GetSlotIndex(thruster->slotName);
      if (index != -1) {
        sc->GetLocalSlotTransformFromIndex(index, &transform);
        collider->localToBody.position = transform.position - *cc->localTransform.Position.ToVector4();
        collider->localToBody.orientation = RED4ext::Quaternion(0.0, 0.0, 0.0, 1.0);
      }
      auto shape = (physx::PxShape *) collider->CreatePxShape(&unk140, nullptr, 1, nullptr);
      shape->setSimulationFilterData(&filterData->simulationFilter);
      shape->setQueryFilterData(&filterData->queryFilter);

      body->attachShape(*shape);

      shape->release2();
    }

    auto newCount = body->getNbShapes();
    
    proxyHelper.mutex->Lock();
    // add indices to bottom mask
    for (int i = this->originalShapeCount; i < newCount; i++) {
//      cc->unk174 |= (1 << i);
      proxyHelper.SetSimulationShape(true, 0, i);
      proxyHelper.SetIsQueryable(true, 0, i);
    }

    proxyHelper.UpdateProxyCache();
    proxyHelper.Unlock();
  }

  RED4ext::ExecuteFunction(this, this->nativeType->GetFunction("OnActivation"), nullptr);
}

void IFlightConfiguration::OnDeactivationCore() {
  auto rtti = RED4ext::CRTTISystem::Get();

  RED4ext::vehicle::ChassisComponent *cc = NULL;

  auto ccCls = rtti->GetClass("vehicleChassisComponent");

  for (auto const &handle : this->component->entity->componentsStorage.components) {
    auto component = handle.GetPtr();
    if (cc == NULL && component->GetNativeType() == ccCls) {
      cc = reinterpret_cast<RED4ext::vehicle::ChassisComponent *>(component);
    }
  }

  if (cc != NULL) {
    RED4ext::physics::ProxyHelper proxyHelper(cc->proxyID, &cc->sharedMutex);

    auto key = (RED4ext::physics::PhysicalSystemProxy *) cc->proxyID.GetProxy();
    auto body = (physx::PxRigidDynamic *) key->bodies.entries[0];

    auto nbShapes = body->getNbShapes();

    // remove indexes from bottom mask
//    for (int i = this->originalShapeCount; i < nbShapes; i++) {
//      cc->unk174 &= ~(1 << i);
//      proxyHelper.SetSimulationShape(false, 0, i);
//      proxyHelper.SetIsQueryable(false, 0, i);
//    }
    proxyHelper.mutex->Lock();

    physx::PxShape *shapes[16];
    body->getShapes(shapes, 16, this->originalShapeCount);
    for (int i = 0; i < fmin(nbShapes - this->originalShapeCount, 16); i++) {
      body->detachShape(*shapes[i], true);
    }

    proxyHelper.UpdateProxyCache();
    proxyHelper.Unlock();
  }

  RED4ext::ExecuteFunction(this, this->nativeType->GetFunction("OnDeactivation"), nullptr);
}