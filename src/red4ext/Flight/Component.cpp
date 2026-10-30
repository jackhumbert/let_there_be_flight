#include "Flight/Component.hpp"
#include "FlightSystem.hpp"

#include <algorithm>
#include <cstring>
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
  // A teleport (TeleportationFacility, vehicleTeleportEvent: vehicle::BaseObject::OnTeleport) makes
  // the chassis kinematic with physics-disable bit 0x20 (the disable mask at BaseObject+0x260) until
  // the collision under the new place has streamed in (SuspensionBase::ProcessStreamingHack, then
  // EnablePhysicsAfterStreamingHack clears it), and bit 0x400 stays set while gameplay is hidden
  // (the loading screen; BaseObject::ProcessPhysicsUpdateRequest). With either set the game drives
  // the car kinematically and derives its velocity from the position change each frame
  // (BaseObject::PostMove_Common): the teleport's jump over one frame. ForceEnablePhysics below
  // cleared both at once, before the move had reached PhysX, and flight then flew against that
  // velocity. So while held, and for kTeleportClearFrames after, flight does nothing; then, after a
  // teleport (0x20 seen; 0x400 alone is a pause or a menu), the velocities start at zero and the
  // script is told (FlightComponent.OnTeleported).
  constexpr uint32_t kTeleportHoldBits = 0x20 | 0x400;
  constexpr float kTeleportHoldMax = 15.0f;
  constexpr int32_t kTeleportClearFrames = 5;
  const auto base = reinterpret_cast<uint8_t *>(vehicle);
  const auto disabled = *reinterpret_cast<uint32_t *>(base + 0x260);
  if (this->hasUpdate && (disabled & kTeleportHoldBits) && this->teleportHold < kTeleportHoldMax) {
    this->teleportHold = std::max(this->teleportHold, 0.0f) + deltaTime;
    this->teleportClear = 0;
    this->teleportJump = this->teleportJump || (disabled & 0x20);
  }
  if (this->teleportHold >= 0.0f) {
    this->force = Vector4();
    this->torque = Vector4();
    this->pendingImpulse = Vector3();
    this->pendingAngularImpulse = Vector3();
    if ((disabled & kTeleportHoldBits) && this->teleportHold < kTeleportHoldMax) {
      return;
    }
    if (++this->teleportClear < kTeleportClearFrames && this->teleportHold < kTeleportHoldMax) {
      this->teleportHold += deltaTime;
      return;
    }
    this->teleportHold = -1.0f;
    if (!this->teleportJump) {
      return; // a pause or a menu (0x400 alone): nothing moved
    }
    this->teleportJump = false;
    this->contactSpin = Vector3();
    this->groundContact = false;
    this->groundPush = 0.0f;
    this->framesSinceSupport = 1000;
    this->physxFrameDt = 0.0f;
    this->teleportSettle = 3;
    this->teleportQuiet = 10;
    // at rest: the rigid body, the previous velocity PostMove_Common differences against
    // (BaseObject+0x370) and the copy scripts read (vehicle::RuntimeData at BaseObject+0x5F0:
    // linearVelocity +0x30, linear acceleration +0x60, angular velocity +0xB0, speeds +0xC8)
    vehicle->physicsData->velocity = Vector3();
    vehicle->physicsData->angularVelocity = Vector3();
    std::memset(base + 0x370, 0, 12);
    if (auto runtime = *reinterpret_cast<uint8_t **>(base + 0x5F0)) {
      std::memset(runtime + 0x30, 0, 12);
      std::memset(runtime + 0x60, 0, 12);
      std::memset(runtime + 0xB0, 0, 12);
      std::memset(runtime + 0xC8, 0, 12);
    }
    ExecuteFunction(this, this->nativeType->GetFunction("OnTeleported"), nullptr);
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
    if (this->teleportQuiet > 0) {
      // the vehicle's velocity reads the teleport's jump over a frame or two after it: no
      // flight forces from it
      --this->teleportQuiet;
      this->force = Vector4();
      this->torque = Vector4();
    }
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