#include "Flight/Component.hpp"
#include "FlightSystem.hpp"
#include "Audio/Inputs/Gather.hpp"
#include "Flight/FlightStats.hpp"

#include "Flight/DeveloperSettings.hpp"
#include "FlightConfiguration.hpp"

#include <PhysX3.hpp>
#include <RED4ext/Scripting/Natives/physicsPhysicalSystemProxy.hpp>
#include <RED4ext/Scripting/Natives/physicsProxyHelper.hpp>
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
  Gather::Forget(this->entity->entityID.hash);
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
// flight HUD settings in Entity_Initialize.cpp).
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

float GetFlightSettingsFloat(RED4ext::CRTTISystem *rtti, RED4ext::CName aProperty, float aFallback) {
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
  // the chassis kinematic with PhysicsState::StreamingHold (BaseObject::physicsState, a mask) until
  // the collision under the new place has streamed in (SuspensionBase::ProcessStreamingHack, then
  // EnablePhysicsAfterStreamingHack clears it), and PhysicsState::GameplayHidden stays set while gameplay is hidden
  // (the loading screen; BaseObject::ProcessPhysicsUpdateRequest). With either set the game drives
  // the car kinematically and derives its velocity from the position change each frame
  // (BaseObject::PostMove_Common): the teleport's jump over one frame. ForceEnablePhysics below
  // cleared both at once, before the move had reached PhysX, and flight then flew against that
  // velocity. So while held, and for kTeleportClearFrames after, flight does nothing; then, after a
  // teleport (StreamingHold seen; GameplayHidden alone is a pause or a menu), the velocities start at zero and the
  // script is told (FlightComponent.OnTeleported).
  constexpr uint32_t kTeleportHoldBits = vehicle::PhysicsState::StreamingHold | vehicle::PhysicsState::GameplayHidden;
  constexpr float kTeleportHoldMax = 15.0f;
  constexpr int32_t kTeleportClearFrames = 5;
  const auto base = reinterpret_cast<uint8_t *>(vehicle);
  const auto disabled = static_cast<uint32_t>(vehicle->physicsState);
  if (this->hasUpdate && (disabled & kTeleportHoldBits) && this->teleportHold < kTeleportHoldMax) {
    this->teleportHold = std::max(this->teleportHold, 0.0f) + deltaTime;
    this->teleportClear = 0;
    this->teleportJump = this->teleportJump || (disabled & vehicle::PhysicsState::StreamingHold);
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
      return; // a pause or a menu (GameplayHidden alone): nothing moved
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
    // (BaseObject::unk360) and the copy scripts read (vehicle::RuntimeData at BaseObject+0x5F0:
    // linearVelocity +0x30, linear acceleration +0x60, angular velocity +0xB0, speeds +0xC8)
    vehicle->physicsData->velocity = Vector3();
    vehicle->physicsData->angularVelocity = Vector3();
    vehicle->unk360 = Vector3();
    if (auto runtime = *reinterpret_cast<uint8_t **>(base + 0x5F0)) {
      std::memset(runtime + 0x30, 0, 12);
      std::memset(runtime + 0x60, 0, 12);
      std::memset(runtime + 0xB0, 0, 12);
      std::memset(runtime + 0xC8, 0, 12);
    }
    // the modes start over from the stats (the Autopilot plans from d_position), which this frame's update below
    // has not refreshed yet: still the old place's (the Autopilot planned from there and held the jump off,
    // 2026-11-05)
    if (auto stats = FlightStats::Of(this); stats && stats->nativeUpdate) {
      stats->UpdateNative(vehicle);
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
    // the stats' dynamic half, when the plugin owns it (FlightComponent.nativeStats); the script
    // returns before its own update in the photo mode, so this does too
    if (auto stats = FlightStats::Of(this); stats && stats->nativeUpdate) {
      bool photoMode = false;
      Gather::Call("gamePhotoModeSystem", "IsPhotoModeActive", &photoMode);
      if (!photoMode) {
        stats->UpdateNative(vehicle);
      }
    }
    // StackArgs_t args;
    // float delta = deltaTime;
    // args.emplace_back(CRTTISystem::Get()->GetType("Float"), &delta);
    ExecuteFunction(this, this->nativeType->GetFunction("OnUpdate"), nullptr, deltaTime);
    // the audio inputs, from what the script just worked out
    Gather::Component(this, deltaTime);
    if (this->teleportQuiet > 0) {
      // the vehicle's velocity reads the teleport's jump over a frame or two after it: no
      // flight forces from it
      --this->teleportQuiet;
      this->force = Vector4();
      this->torque = Vector4();
    }
    this->physxOwned = this->active && GetFlightSettingsBool(rtti, "physxFlightPhysics", true);
    this->UpdateDepenetration();
    this->UpdateThrusterContacts();
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

void FlightComponent::UpdateDepenetration() {
  // the chassis's PhysX body, as the acoustics' live rays find it (Audio/Acoustics/LiveRays.cpp)
  physx::PxRigidDynamic *body = nullptr;
  if (this->chassis) {
    RED4ext::physics::ProxyHelper lock(this->chassis->proxyID, &this->chassis->sharedMutex);
    auto proxy = (RED4ext::physics::PhysicalSystemProxy *)RED4ext::physics::ProxyID::GetProxy(this->chassis->proxyID);
    if (proxy && proxy->bodies.size > 0)
      body = (physx::PxRigidDynamic *)proxy->bodies.entries[0];
  }
  const float cap = Flight::Developer().maxDepenetration;
  const bool want = this->physxOwned && body && cap > 0.0f;
  auto capped = static_cast<physx::PxRigidDynamic *>(this->depenetrationBody);
  if (capped && (!want || capped != body)) {
    // a body that is gone is not touched again
    if (capped == body)
      capped->setMaxDepenetrationVelocity(this->depenetrationOwn);
    this->depenetrationBody = nullptr;
  }
  if (want && this->depenetrationBody != body) {
    this->depenetrationOwn = body->getMaxDepenetrationVelocity();
    this->depenetrationBody = body;
  }
  if (want)
    body->setMaxDepenetrationVelocity(cap); // each frame: the developer setting may have changed
}

void FlightComponent::UpdateThrusterContacts() {
  if (!this->chassis || !this->configuration)
    return;
  const bool off = this->physxOwned && !Flight::Developer().thrusterContacts;
  if (off == this->thrusterContactsOff && this->thrusterOffsetsLogged)
    return;
  RED4ext::physics::ProxyHelper helper(this->chassis->proxyID, &this->chassis->sharedMutex);
  auto proxy = (RED4ext::physics::PhysicalSystemProxy *)RED4ext::physics::ProxyID::GetProxy(this->chassis->proxyID);
  if (!proxy || proxy->bodies.size == 0 || !proxy->bodies.entries[0])
    return;
  auto body = (physx::PxRigidDynamic *)proxy->bodies.entries[0];
  physx::PxShape *shapes[64];
  const auto count = body->getShapes(shapes, 64, 0);
  if (!this->thrusterOffsetsLogged) {
    for (uint32_t i = 0; i < count; ++i)
      spdlog::info("[FlightComponent] chassis shape {}{}: contact offset {:.3f} m, rest offset {:.3f} m", i,
                   this->configuration->IsTrackedShape(shapes[i]) ? " (thruster sphere)" : "",
                   shapes[i]->getContactOffset(), shapes[i]->getRestOffset());
    this->thrusterOffsetsLogged = true;
  }
  if (off == this->thrusterContactsOff)
    return;
  helper.mutex->Lock();
  for (uint32_t i = 0; i < count; ++i)
    if (this->configuration->IsTrackedShape(shapes[i]))
      helper.SetSimulationShape(!off, 0, i);
  helper.UpdateProxyCache();
  helper.Unlock();
  this->thrusterContactsOff = off;
}
