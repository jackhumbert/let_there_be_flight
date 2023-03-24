#include "FlightConfiguration.hpp"
#include <RED4ext/Memory/Allocators.hpp>
#include <PhysX3.hpp>
#include <RED4ext/Scripting/Natives/Generated/physics/ColliderSphere.hpp>
#include <RED4ext/Scripting/Natives/Generated/physics/SimulationFilter.hpp>
#include <RED4ext/Scripting/Natives/Generated/physics/QueryFilter.hpp>
#include <RED4ext/Scripting/Natives/Generated/ent/SlotComponent.hpp>
#include <RED4ext/Scripting/Natives/Generated/physics/FilterData.hpp>

void IFlightConfiguration::Setup(RED4ext::vehicle::BaseObject * vehicle) {

  this->thrusters = RED4ext::DynArray<RED4ext::Handle<IFlightThruster>>(new RED4ext::Memory::DefaultAllocator());
  auto onInit = GetType()->GetFunction("OnSetup");
  if (onInit) {
    auto rtti = RED4ext::CRTTISystem::Get();
    RED4ext::CStackType args[1];
    auto handle = RED4ext::Handle<RED4ext::vehicle::BaseObject>(vehicle);
    args[0] = RED4ext::CStackType(rtti->GetType("handle:vehicleBaseObject"), &handle);
    auto stack = RED4ext::CStack(this, args, 1, nullptr, 0);
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
    RED4ext::physics::GeoThing geoThing;
    cc->GetGeoThingAndLock(&geoThing);

    // could also update cc->unk174 shape mask?

    auto key = (RED4ext::vehicle::PhysicalSystemKey *) cc->geoCacheID.GetSystemKey();
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

    // add indices to bottom mask
    for (int i = this->originalShapeCount; i < newCount; i++) {
//      cc->unk174 |= (1 << i);
      geoThing.SetSimulationShape(true, 0, i);
      geoThing.SetIsQueryable(true, 0, i);
    }

//    float damping = 2.0;
//    geoThing.SetAngularDamping(&damping, 0);
    geoThing.CleanUp();
    geoThing.Unlock();
  }

//  // add a dummy shape & remove it to trigger an update
//  auto newCount = body->getNbShapes();
//
//  RED4ext::Handle<RED4ext::physics::ICollider> collider;
//  RED4ext::physics::ColliderSphere::createHandleWithRadius(&collider, 0.45);
//
//  auto shape = (physx::PxShape *) collider->CreatePxShape(&unk140, nullptr, 1, nullptr);
//  shape->setSimulationFilterData(&simulationFilter);
//  shape->setQueryFilterData(&queryFilter);
//
//  body->attachShape(*shape);
//  shape->release2();
//
//  physx::PxShape * shapes[1];
//  body->getShapes(shapes, 1, newCount);
//  body->detachShape(*shapes[0], true);
//
//  cc->sub_1E8(false);
//  cc->sub_1E8(true);
//cc->sub_148();
//  body->setAngularDamping(10.0);
//  body->putToSleep();
//  body->wakeUp();
//  auto vehicle = (RED4ext::vehicle::BaseObject*)this->component->entity;
//  vehicle->SetPhysicsState(RED4ext::vehicle::PhysicsState::Asleep, 1);
//  vehicle->SetPhysicsState(RED4ext::vehicle::PhysicsState::Asleep, 0);
//key->sub_58();
//  cc->UpdatePhysicsState(0x20, 0);
//  body->setActorFlag(1<<0, true);
//  this->component->entity->ExecuteFunction("ScheduleAppearanceChange", this->component->entity->currentAppearance);
  this->ExecuteFunction("OnActivation");
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
    RED4ext::physics::GeoThing geoThing;
    cc->GetGeoThingAndLock(&geoThing);

    auto key = (RED4ext::vehicle::PhysicalSystemKey *) cc->geoCacheID.GetSystemKey();
    auto body = (physx::PxRigidDynamic *) key->bodies.entries[0];

    auto nbShapes = body->getNbShapes();

    // remove indexes from bottom mask
//    for (int i = this->originalShapeCount; i < nbShapes; i++) {
//      cc->unk174 &= ~(1 << i);
//      geoThing.SetSimulationShape(false, 0, i);
//      geoThing.SetIsQueryable(false, 0, i);
//    }

    physx::PxShape *shapes[16];
    body->getShapes(shapes, 16, this->originalShapeCount);
    for (int i = 0; i < fmin(nbShapes - this->originalShapeCount, 16); i++) {
      body->detachShape(*shapes[i], true);
    }

//    float damping = 0.0;
//    geoThing.SetAngularDamping(&damping, 0);
    geoThing.CleanUp();
    geoThing.Unlock();
  }

  this->ExecuteFunction("OnDeactivation");
}