#include "Flight/Component.hpp"
#include "FlightSystem.hpp"
#include <RED4ext/Scripting/Natives/vehiclePhysicsData.hpp>
#include <RED4ext/Scripting/Natives/vehicleDestruction.hpp>

#ifdef __APPLE__
#include <cstdlib>
// LTBF R42g (macOS, diagnostics): LTBF_MAC_DIAG=1 logs every FlightComponent attach, detach and destructor, so a crash log shows the
// last component touched before it.
static bool LtbfMacFcDiag() {
  static const bool s_on = std::getenv("LTBF_MAC_DIAG") != nullptr;
  return s_on;
}
#define LTBF_MAC_FC_DIAG(what, fc)                                                                                         \
  do {                                                                                                                      \
    if (LtbfMacFcDiag())                                                                                                    \
      spdlog::info("[ltbf][fc] {} this={} entity={} configuration={} refs={}", what, static_cast<const void *>(fc),       \
                   static_cast<const void *>((fc)->entity), static_cast<const void *>((fc)->configuration.GetPtr()),     \
                   (fc)->ref.refCount ? static_cast<int>((fc)->ref.refCount->strongRefs) : -1);                         \
  } while (0)
#else
#define LTBF_MAC_FC_DIAG(what, fc) ((void)0)
#endif

using namespace RED4ext;

FlightComponent::~FlightComponent() {
  LTBF_MAC_FC_DIAG("dtor", this);
  if (this->configuration) {
    this->configuration->~IFlightConfiguration();
  }
}

void FlightComponent::OnAttach(void* a1) {
  LTBF_MAC_FC_DIAG("attach", this);
  game::Component::OnAttach(a1);
  this->sys = FlightSystem::GetInstance();
  this->sys->RegisterComponent(Handle<FlightComponent>(this));
}

void FlightComponent::OnDetach(void* a1) {
  LTBF_MAC_FC_DIAG("detach", this);
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

void FlightComponent::OnUpdate(float deltaTime) { 
#ifdef __APPLE__
  static uint32_t s_macDiagUpdates = 0; // diagnostics only; shared by all components (the class layout is script-visible)
#endif
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
#ifdef __APPLE__
    // LTBF R43 (macOS, diagnostics): in-world flight test L3/L4 showed a flying car that does not move at all. Log what the
    // physics step sees, about once a second per active component.
    if (LtbfMacFcDiag() && this->active && (++s_macDiagUpdates % 60) == 1) {
      auto *pd = vehicle->physicsData;
      spdlog::info("[ltbf][fc] update this={} dt={} scriptForce=({:.2f},{:.2f},{:.2f}) scriptTorque=({:.2f},{:.2f},{:.2f}) "
                   "pd={} pdForce=({:.2f},{:.2f},{:.2f}) velocity=({:.3f},{:.3f},{:.3f}) invMass={}",
                   static_cast<const void *>(this), deltaTime, this->force.X, this->force.Y, this->force.Z, this->torque.X,
                   this->torque.Y, this->torque.Z, static_cast<const void *>(pd), pd ? pd->force.X : 0.f,
                   pd ? pd->force.Y : 0.f, pd ? pd->force.Z : 0.f, pd ? pd->velocity.X : 0.f, pd ? pd->velocity.Y : 0.f,
                   pd ? pd->velocity.Z : 0.f, pd ? pd->inverseMass : 0.f);
    }
#endif
    vehicle->physicsData->force += this->force.AsVector3();
    vehicle->physicsData->torque += this->torque.AsVector3();
    this->force = Vector4();
    this->torque = Vector4();
  }
#ifdef __APPLE__
  else if (LtbfMacFcDiag() && this->active && (++s_macDiagUpdates % 60) == 1) {
    spdlog::info("[ltbf][fc] update this={} active but hasUpdate=false: no flight forces this step", static_cast<const void *>(this));
  }
#endif
}