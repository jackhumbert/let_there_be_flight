#include "Flight/Component.hpp"
#include "FlightSystem.hpp"
#include <RED4ext/Scripting/Natives/vehiclePhysicsData.hpp>
#include <RED4ext/Scripting/Natives/vehicleDestruction.hpp>

using namespace RED4ext;

FlightComponent::~FlightComponent() {
  if (this->configuration) {
    this->configuration->~IFlightConfiguration();
  }
}

void FlightComponent::OnAttach(void* a1) {
  game::Component::OnAttach(a1);
  this->sys = FlightSystem::GetInstance();
  this->sys->RegisterComponent(Handle<FlightComponent>(this));
}

void FlightComponent::OnDetach(void* a1) {
  this->sys->UnregisterComponent(Handle<FlightComponent>(this));
  game::Component::OnDetach(a1);
}

void FlightComponent::ChaseTarget(WeakHandle<game::Object> target) {
  ////spdlog::info("[FlightComponent] ChaseTarget");
  //action::ActionBase * action;
  //auto vehicle = (vehicle::BaseObject *)this->entity;
  //vehicle->ForceEnablePhysics();
  //vehicle->SetPhysicsState(vehicle::PhysicsState::Chase, false);
  //vehicle->CreateAction(&action, action::Type::DriveFollowTarget);
  //vehicle->unk615 = true;
  //if (action) {
  //  auto autonomousData =
  //      (vehicle::AutonomousData*)CRTTISystem::Get()->GetClass("vehicleAutonomousData")->AllocInstance();
  //  autonomousData->targetObjToFollow.instance = target.instance;
  //  autonomousData->targetObjToFollow.refCount = target.refCount;
  //  autonomousData->needDriver = false;
  //  autonomousData->distanceMin = 0.0;
  //  autonomousData->distanceMax = 10000.0;
  //  autonomousData->canClearActions = true;
  //  autonomousData->useKinematic = true;
  //  autonomousData->allowStubMovement = true;
  //  //autonomousData->drivingID = vehicle->autonomousData.drivingID;
  //  target.refCount->IncWeakRef();
  //  auto chaseTarget = (action::Drive*)action;
  //  chaseTarget->Lock();
  //  chaseTarget->sub_88(vehicle->unk368->moveComponent, autonomousData);
  //  chaseTarget->Unlock();
  //}
}

void FlightComponent::OnEnabled(bool enabled) { 
  spdlog::info("FlightComponent {}", enabled);
};

// FlightSettings Bool read from the class defaults, which Mod Settings writes (same as the
// flight HUD settings in Entity_InitializeComponents.cpp).
static bool GetFlightSettingsBool(RED4ext::CRTTISystem *rtti, RED4ext::CName aProperty, bool aFallback) {
  auto cls = rtti->GetClass("FlightSettings");
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

static float GetFlightSettingsFloat(RED4ext::CRTTISystem *rtti, RED4ext::CName aProperty, float aFallback) {
  auto cls = rtti->GetClass("FlightSettings");
  if (!cls) {
    return aFallback;
  }
  auto variant = cls->defaults.Get(aProperty);
  if (!variant || !*variant) {
    return aFallback;
  }
  auto type = (*variant)->GetType();
  if (!type || type->GetName() != RED4ext::CName("Float")) {
    return aFallback;
  }
  auto data = (*variant)->GetDataPtr();
  return data ? *reinterpret_cast<float *>(data) : aFallback;
}

void FlightComponent::OnUpdate(float deltaTime) { 
  auto rtti = RED4ext::CRTTISystem::Get();
  vehicle::BaseObject * vehicle;
  if (!this->entity->IsOfClass(rtti->GetClass("vehicleBaseObject"))) {
    return;
  } else {
    vehicle = reinterpret_cast<vehicle::BaseObject *>(this->entity);
  }
  if (this->hasUpdate) {
    /*if (this->chassis) {
      physics::ProxyHelper helper;
      this->chassis->GetProxyHelperAndLock(&helper);
      helper.SetLinearDamping(&this->linearDamp, 0);
      helper.SetAngularDamping(&this->angularDamp, 0);
      helper.UpdateProxyCache();
      helper.Unlock();
    }*/
    vehicle->ForceEnablePhysics();
    // StackArgs_t args;
    // float delta = deltaTime;
    // args.emplace_back(CRTTISystem::Get()->GetType("Float"), &delta);
    ExecuteFunction(this, this->nativeType->GetFunction("OnUpdate"), nullptr, deltaTime);
    this->physxOwned = this->active && GetFlightSettingsBool(rtti, "physxFlightPhysics", true);
    this->groundFriction = GetFlightSettingsFloat(rtti, "groundFriction", 0.1f);
    if (this->physxOwned) {
      // applied straight to the PhysX body in the PushToPhysX hook
      this->pendingImpulse += this->force.AsVector3();
      this->pendingAngularImpulse += this->torque.AsVector3();
    } else {
      vehicle->physicsData->force += this->force.AsVector3();
      vehicle->physicsData->torque += this->torque.AsVector3();
    }
    this->force = Vector4();
    this->torque = Vector4();
  }
}