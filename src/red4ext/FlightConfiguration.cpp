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

using namespace RED4ext;

IFlightConfiguration::~IFlightConfiguration() {
  component.~WeakHandle();
  for (auto& thruster : this->thrusters) {
    thruster.~Handle();
  }
}

CClass* IFlightConfiguration::GetConfigurationClass(ent::Entity* entity) {
  auto rtti = CRTTISystem::Get();

  if (!entity) {
    return nullptr;
  }

  bool isCar = entity->IsOfClass(rtti->GetClass("vehicleCarBaseObject"));
  bool isBike = entity->IsOfClass(rtti->GetClass("vehicleBikeBaseObject"));

  uint8_t isSixWheeler = 0;

  auto placedCls = rtti->GetClass("entIPlacedComponent");
  auto hardBindingCls = rtti->GetClass("entHardTransformBinding");

  if (!Utils::LooksLikeDynArray(entity->componentsStorage.components)) {
    LTBF_WARN_ONCE("[FlightConfiguration] componentsStorage.components header looks corrupt (size {} capacity {}); no flight configuration",
                   entity->componentsStorage.components.size, entity->componentsStorage.components.capacity);
    return nullptr;
  }

  for (auto const &handle : entity->componentsStorage.components) {
    auto component = handle.GetPtr();
    if (!component) {
      LTBF_WARN_ONCE("[FlightConfiguration] null component handle in componentsStorage while looking for a configuration class");
      continue;
    }
    auto type = component->GetNativeType();
    bool isPlacedComponent = type && placedCls && type->IsA(placedCls);

    if (isPlacedComponent) {
      auto pth = ((ent::IPlacedComponent *)component)->parentTransform;
      if (pth) {
        // only entHardTransformBinding has slotName; other ITransformBinding subclasses do not
        auto ptType = pth->GetNativeType();
        if (ptType && hardBindingCls && ptType->IsA(hardBindingCls)) {
          auto pt = reinterpret_cast<ent::HardTransformBinding *>(pth.GetPtr());
          if (pt->slotName == "wheel_front_left_b") {
            isSixWheeler |= 1;
          }
          if (pt->slotName == "wheel_back_left_b") {
            isSixWheeler |= 2;
          }
        }
      }
    }
  }

  char className[256];
  auto appearance = entity->currentAppearance.ToString();
  sprintf_s(className, "FlightConfiguration_%s", appearance ? appearance : "None");

  auto configurationCls = rtti->GetClassByScriptName(className);
  if (!configurationCls) {
    if (isSixWheeler & 1) {
      configurationCls = rtti->GetClassByScriptName("CustomSixWheelCarFlightConfiguration");
      if (!configurationCls) {
        configurationCls = rtti->GetClassByScriptName("SixWheelCarFlightConfiguration");
      }
    } else if (isSixWheeler & 2) {
      configurationCls = rtti->GetClassByScriptName("CustomSixWheelRearCarFlightConfiguration");
      if (!configurationCls) {
        configurationCls = rtti->GetClassByScriptName("SixWheelRearCarFlightConfiguration");
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

void IFlightConfiguration::Setup(vehicle::BaseObject * vehicle) {

  this->thrusters = DynArray<Handle<IFlightThruster>>(new Memory::DefaultAllocator());
  auto onInit = GetType()->GetFunction("OnSetup");
  if (onInit) {
    auto rtti = CRTTISystem::Get();
    CStackType args[1];
    auto handle = Handle<vehicle::BaseObject>(vehicle);
    args[0] = CStackType(rtti->GetType("handle:vehicleBaseObject"), &handle);
    auto stack = CStack(this, args, 1, nullptr);
    onInit->Execute(&stack);
  }
}

void IFlightConfiguration::AddSlots(ent::SlotComponent *slotComponent) {
  auto rtti = CRTTISystem::Get();

  if (!slotComponent || !Utils::LooksLikeDynArray(slotComponent->slots)) {
    LTBF_WARN_ONCE("[FlightConfiguration] vehicle_slots component missing or its slots header looks corrupt; flight camera slot not added");
    return;
  }

  auto slot = reinterpret_cast<ent::Slot *>(rtti->GetClass("entSlot")->CreateInstance(true));
  slot->boneName = this->flightCameraBone;
  slot->slotName = "CustomFlightCamera";
  slot->relativePosition = this->flightCameraOffset;
  slotComponent->slots.EmplaceBack(*slot);
  slotComponent->slotIndexLookup.Emplace(slot->slotName, slotComponent->slots.size - 1);

  //for (auto thruster : thrusters) {
  //  auto slot = reinterpret_cast<ent::Slot *>(rtti->GetClass("entSlot")->CreateInstance(true));
  //  slot->boneName = thruster->boneName;
  //  slot->slotName = thruster->slotName;
  //  slot->relativePosition = thruster->relativePosition;
  //  slot->relativeRotation = thruster->relativeRotation;
  //  //slot->relativeRotation = Quaternion(0.0, 0.0, 0.0, 1.0);
  //  slotComponent->slots.EmplaceBack(*slot);
  //  slotComponent->slotIndexLookup.Emplace(slot->slotName, slotComponent->slots.size - 1);
  //}
}

// void IFlightConfiguration::AddMeshes(ent::Entity *entity, ent::VisualControllerComponent *vcc) {
//   for (auto thruster : thrusters) {
//     auto mesh = CreateThrusterEngine(thruster->meshPath, thruster->meshName, thruster->slotName);
//     thruster->meshComponent = Handle<ent::MeshComponent>(mesh);
//     entity->componentsStorage.components.EmplaceBack(thruster->meshComponent);
//     AddToController(vcc, mesh);
//   }
// }

void IFlightConfiguration::AddColliders() {
  auto rtti = CRTTISystem::Get();

  auto filterDataCls = rtti->GetClass("physicsFilterData");

  auto flightComponent = this->component.Lock();

  if (!flightComponent || !flightComponent->entity) {
    LTBF_WARN_ONCE("[FlightConfiguration] AddColliders: flight component or its entity is gone");
    return;
  }

  if (!flightComponent->entity->IsOfClass(rtti->GetClass("vehicleBaseObject")))
    return;

  auto chassis = EntityExt::From(flightComponent->entity)->FindComponent<vehicle::ChassisComponent>();
  auto slot = EntityExt::From(flightComponent->entity)->FindComponent<ent::SlotComponent>("vehicle_slots");

  if (chassis != NULL && slot != NULL) {
    // FlightComponent::Get((vehicle::BaseObject*)flightComponent->entity)->chassis = chassis;
    flightComponent->chassis = chassis;
    physics::ProxyHelper proxyHelper(chassis->proxyID, &chassis->sharedMutex);

    auto key = (physics::PhysicalSystemProxy *) physics::ProxyID::GetProxy(chassis->proxyID);
    if (!key || !Utils::LooksLikeDynArray(key->bodies) || key->bodies.size == 0 || !key->bodies.entries[0]) {
      LTBF_WARN_ONCE("[FlightConfiguration] AddColliders: chassis physics proxy has no body; thruster colliders not added");
      return;
    }
    auto body = (physx::PxRigidDynamic *) key->bodies.entries[0];

    if (this->originalShapeCount == -1)
      this->originalShapeCount = body->getNbShapes();

    auto filterData = (physics::FilterData*)malloc(sizeof(physics::FilterData));
    physics::FilterData::Init(filterData);
    
    filterData->LoadPreset("Vehicle Chassis");
    Vector3 unk140(1.0, 1.0, 1.0);
    Transform transform;
    int index = 0;

    for (auto const &thruster: this->thrusters) {
      if (thruster && thruster->attached) {
        Handle<physics::ICollider> collider;
        float radius = 0.4;
        physics::ColliderSphere::createHandleWithRadius(&collider, &radius);
        // collider.refCount->IncRef();

        collider->material = "vehicle_chassis.physmat";

        index = slot->GetSlotIndex(thruster->slotName);
        if (index != -1) {
          slot->GetLocalSlotTransformFromIndex(index, &transform);
          collider->localToBody.position = transform.position - chassis->localTransform.Position.AsVector4();
          collider->localToBody.orientation = Quaternion(0.0, 0.0, 0.0, 1.0);
            
          auto shape = (physx::PxShape *) collider->CreatePxShape(&unk140, nullptr, 1, nullptr);
          shape->setSimulationFilterData(&filterData->simulationFilter);
          shape->setQueryFilterData(&filterData->queryFilter);

          body->attachShape(*shape);

          shape->release2();
        }
      }
    }

    free(filterData);

    auto newCount = body->getNbShapes();
    
    proxyHelper.mutex->Lock();
    // add indices to bottom mask
    for (int i = this->originalShapeCount; i < newCount; i++) {
//      chassis->unk174 |= (1 << i);
      proxyHelper.SetSimulationShape(true, 0, i);
      proxyHelper.SetIsQueryable(true, 0, i);
    }

    proxyHelper.UpdateProxyCache();
    proxyHelper.Unlock();
  }
}

void IFlightConfiguration::OnActivationCore() {
  this->AddColliders();
  ExecuteFunction(this, this->nativeType->GetFunction("OnActivation"), nullptr);
}

void IFlightConfiguration::RemoveColliders() {
  if (this->originalShapeCount == -1)
    return;

  auto rtti = CRTTISystem::Get();

  auto flightComponent = this->component.Lock();

  if (!flightComponent || !flightComponent->entity) {
    LTBF_WARN_ONCE("[FlightConfiguration] RemoveColliders: flight component or its entity is gone");
    return;
  }

  if (!flightComponent->entity->IsOfClass(rtti->GetClass("vehicleBaseObject")))
    return;

  auto chassis = EntityExt::From(flightComponent->entity)->FindComponent<vehicle::ChassisComponent>();

  if (chassis != NULL) {
    physics::ProxyHelper proxyHelper(chassis->proxyID, &chassis->sharedMutex);

    auto key = (physics::PhysicalSystemProxy *) physics::ProxyID::GetProxy(chassis->proxyID);
    if (!key || !Utils::LooksLikeDynArray(key->bodies) || key->bodies.size == 0 || !key->bodies.entries[0]) {
      LTBF_WARN_ONCE("[FlightConfiguration] RemoveColliders: chassis physics proxy has no body");
      return;
    }
    auto body = (physx::PxRigidDynamic *) key->bodies.entries[0];

    auto nbShapes = body->getNbShapes();

    // remove indexes from bottom mask
//    for (int i = this->originalShapeCount; i < nbShapes; i++) {
//      chassis->unk174 &= ~(1 << i);
//      proxyHelper.SetSimulationShape(false, 0, i);
//      proxyHelper.SetIsQueryable(false, 0, i);
//    }
    proxyHelper.mutex->Lock();

    physx::PxShape *shapes[16];
    body->getShapes(shapes, 16, this->originalShapeCount);
    for (int i = 0; i < fmin(nbShapes - this->originalShapeCount, 16); i++) {
      // this might cause a crash when the game exits with:
      // * vehicle flight active?
      // * more than one vehicle flight active?
      body->detachShape(*shapes[i], true);
    }

    proxyHelper.UpdateProxyCache();
    proxyHelper.Unlock();
  }
}

void IFlightConfiguration::OnDeactivationCore() {
  this->RemoveColliders();
  ExecuteFunction(this, this->nativeType->GetFunction("OnDeactivation"), nullptr);
}