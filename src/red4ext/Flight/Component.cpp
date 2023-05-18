#include "Flight/Component.hpp"
#include <RED4ext/Scripting/Natives/vehiclePhysicsData.hpp>

void FlightComponent::ChaseTarget(RED4ext::WeakHandle<RED4ext::game::Object> target) {
  ////spdlog::info("[FlightComponent] ChaseTarget");
  //RED4ext::action::ActionBase * action;
  //auto vehicle = (RED4ext::vehicle::BaseObject *)this->entity;
  //vehicle->UnsetPhysicsStates();
  //vehicle->SetPhysicsState(RED4ext::vehicle::PhysicsState::Chase, false);
  //vehicle->CreateAction(&action, RED4ext::action::Type::DriveFollowTarget);
  //vehicle->unk615 = true;
  //if (action) {
  //  auto autonomousData =
  //      (RED4ext::vehicle::AutonomousData*)RED4ext::CRTTISystem::Get()->GetClass("vehicleAutonomousData")->AllocInstance();
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
  //  auto chaseTarget = (RED4ext::action::Drive*)action;
  //  chaseTarget->Lock();
  //  chaseTarget->sub_88(vehicle->unk368->moveComponent, autonomousData);
  //  chaseTarget->Unlock();
  //}
}

void FlightComponent::OnUpdate(float deltaTime) {
  auto vehicle = reinterpret_cast<RED4ext::vehicle::BaseObject *>(this->entity);
  if (this->hasUpdate) {
    /*if (this->chassis) {
      RED4ext::physics::ProxyHelper helper;
      this->chassis->GetProxyHelperAndLock(&helper);
      helper.SetLinearDamping(&this->linearDamp, 0);
      helper.SetAngularDamping(&this->angularDamp, 0);
      helper.UpdateProxyCache();
      helper.Unlock();
    }*/
//    vehicle->UnsetPhysicsStates();
    this->ExecuteFunction("OnUpdate", deltaTime);
    vehicle->physicsData->force += this->force;
    vehicle->physicsData->torque += this->torque;
    this->force = RED4ext::Vector4();
    this->torque = RED4ext::Vector4();
  }
}