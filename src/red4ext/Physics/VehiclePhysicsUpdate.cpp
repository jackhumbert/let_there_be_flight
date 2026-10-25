#include "Utils/FlightModule.hpp"
#include <RED4ext/Scripting/Natives/vehiclePhysics.hpp>
#include <RED4ext/Scripting/Natives/vehicleCollisions.hpp>
#include <RED4ext/Scripting/Natives/Generated/vehicle/BaseObject.hpp>
#include <RED4ext/Scripting/Natives/Generated/vehicle/CarBaseObject.hpp>
#include <RED4ext/Common.hpp>
#include <spdlog/spdlog.h>
#include "VehiclePhysicsUpdate.hpp"
#include "Flight/Component.hpp"

#include "VehicleSpeedUnlimiter.hpp"
#include <queue>

#ifdef __APPLE__
#include <cmath>
#include <cstdlib>
#include "MacLayout.hpp"
// LTBF R41: the physics structs' parent pointer, drive helpers and bike fields are at other offsets on macOS (macos/MacLayout.hpp).
#define LTBF_PHYSICS_PARENT(physics) \
  (LtbfMac::Layout::At<RED4ext::vehicle::BaseObject *>((physics), LtbfMac::Layout::Physics_parent))
#define LTBF_DRIVE_HELPERS_SIZE(physics) \
  (LtbfMac::Layout::At<uint32_t>((physics), LtbfMac::Layout::WheeledPhysics_driveHelpers + 0xC))

// WheeledPhysics::ApplyAirResistance is inlined on macOS (inside ProcessAirResistance 0x1392998, block 0x13929F8-0x1392A94). This is
// that block: the same 0.01 threshold and global flag byte, drag = coefficient x -1.2 x |v|^2 along v, added to the rigid body's
// force times the step (AddForce 0x1359090 is `force += vector`), and |drag| stored to [[this+0xF0]+0x5D0]+0xEC.
static void MacApplyAirResistance(RED4ext::vehicle::WheeledPhysics *self, RED4ext::vehicle::PhysicsData *physicsData,
                                  const RED4ext::Vector3 &velocity, float deltaTime) {
  const float speedSquared = velocity.X * velocity.X + velocity.Y * velocity.Y + velocity.Z * velocity.Z;
  const auto flag = *reinterpret_cast<const uint8_t *>(LtbfMac::ImageBase() + LtbfMac::Layout::AirResistanceFlagRva);
  if (!(speedSquared > 0.01f) || flag == 0)
    return;
  RED4ext::Vector3 direction = velocity;
  const float speed = std::sqrt(speedSquared);
  if (speed != 0.0f) {
    direction.X /= speed;
    direction.Y /= speed;
    direction.Z /= speed;
  }
  const float magnitude =
      LtbfMac::Layout::At<float>(self, LtbfMac::Layout::WheeledPhysics_airDragCoefficient) * -1.2f * speedSquared;
  const RED4ext::Vector3 drag{direction.X * magnitude, direction.Y * magnitude, direction.Z * magnitude};
  physicsData->force.X += drag.X * deltaTime;
  physicsData->force.Y += drag.Y * deltaTime;
  physicsData->force.Z += drag.Z * deltaTime;
  auto vehicle = LtbfMac::Layout::At<uint8_t *>(self, LtbfMac::Layout::WheeledPhysics_vehicle);
  auto stats = vehicle ? *reinterpret_cast<uint8_t **>(vehicle + 0x5D0) : nullptr;
  if (stats) {
    *reinterpret_cast<float *>(stats + 0xEC) = std::sqrt(drag.X * drag.X + drag.Y * drag.Y + drag.Z * drag.Z);
  }
}
#else
#define LTBF_PHYSICS_PARENT(physics) ((physics)->parent)
#define LTBF_DRIVE_HELPERS_SIZE(physics) ((physics)->driveHelpers.size)
#endif

// void vehicle::WheelSuspensionBase::ApplyAllResistances(float)
REGISTER_FLIGHT_HOOK_HASH(void __fastcall, 2526549425, ProcessAirResistance, 
    RED4ext::vehicle::WheeledPhysics *self, float deltaTime) {
  auto physicsData = LTBF_PHYSICS_PARENT(self)->physicsData;
  auto velocity = physicsData->velocity;
  auto X = velocity.X;
  auto Y = velocity.Y;
  auto Z = velocity.Z;
  auto speedSquared = (float)((float)(X * X) + (float)(Y * Y)) + (float)(Z * Z);
  // only need to apply air resistance when speed is >= 100.0
#ifdef __APPLE__
  if (!std::isinf(speedSquared) && speedSquared >= 10000.0) {
    MacApplyAirResistance(self, physicsData, velocity, deltaTime);
    self->ApplyLowSpeedResistances(velocity, deltaTime);
  }
  // LTBF R43 (macOS, diagnostics): L6-L9 show a flying car that rises and turns but barely moves forward. With LTBF_MAC_DIAG=1, log
  // what the game's own resistance pass does to the velocity of a flying car, about once a second.
  static const bool s_macDiag = std::getenv("LTBF_MAC_DIAG") != nullptr;
  static uint32_t s_macCalls = 0;
  auto *macFc = s_macDiag ? FlightComponent::Get(LTBF_PHYSICS_PARENT(self)) : nullptr;
  if (macFc && macFc->active && (s_macCalls++ % 60) == 0) {
    auto before = physicsData->velocity;
    auto forceBefore = physicsData->force;
    ProcessAirResistance_Original(self, deltaTime);
    auto after = physicsData->velocity;
    spdlog::info("[ltbf][phys] resistances: velocity ({:.3f},{:.3f},{:.3f}) -> ({:.3f},{:.3f},{:.3f}), force ({:.1f},{:.1f},{:.1f}) -> "
                 "({:.1f},{:.1f},{:.1f}), dt={}",
                 before.X, before.Y, before.Z, after.X, after.Y, after.Z, forceBefore.X, forceBefore.Y, forceBefore.Z,
                 physicsData->force.X, physicsData->force.Y, physicsData->force.Z, deltaTime);
    return;
  }
#else
  if (_fdclass(speedSquared) != 1 && speedSquared >= 10000.0) {
    self->ApplyAirResistance(velocity, deltaTime);
    self->ApplyLowSpeedResistances(velocity, deltaTime);
  }
#endif
  ProcessAirResistance_Original(self, deltaTime);
}

  // void vehicle::RigidBody::ApplyAngularImpulse(Vector3 const &, Vector3 const &)
REGISTER_FLIGHT_HOOK_HASH(void __fastcall, 3303544265, vehiclePhysicsData_ApplyTorqueAtPosition,
                     RED4ext::vehicle::PhysicsData *physicsData, RED4ext::Vector3 *offset, RED4ext::Vector3 *torque) {
  auto fc = FlightComponent::Get(physicsData->vehicle);
  if (fc && fc->active) {
    return;
  } else {
    vehiclePhysicsData_ApplyTorqueAtPosition_Original(physicsData, offset, torque);
  }
}

// void vehicle::RigidBody::ApplyImpulse(Vector3 const &, Vector3 const &)
REGISTER_FLIGHT_HOOK_HASH(void __fastcall, 611586815, ApplyForceAtPosition,
                     RED4ext::vehicle::PhysicsData *physicsData, RED4ext::Vector3 *offset, RED4ext::Vector3 *force) {
  auto fc = FlightComponent::Get(physicsData->vehicle);
  if (fc && fc->active) {
    return;
  } else {
    ApplyForceAtPosition_Original(physicsData, offset, force);
  }
}

// where driverHelpers are processed
// void vehicle::WheelSuspensionBase::FixedUpdate_PostSolve(float)
// vehicleWheeledPhysics::sub_58
/// @hash 3281786499
// uintptr_t __fastcall VehicleHelperUpdate(RED4ext::vehicle::WheeledPhysics *, float);

// void vehicle::WheelSuspensionBase::FixedUpdate_PostSolve(float)
REGISTER_FLIGHT_HOOK_HASH(uintptr_t __fastcall, 3281786499, VehicleHelperUpdate, RED4ext::vehicle::WheeledPhysics *p, float deltaTime) {
  auto fc = FlightComponent::Get(LTBF_PHYSICS_PARENT(p));
  if (fc) {
    auto size = LTBF_DRIVE_HELPERS_SIZE(p);
    if (fc->active) {
      LTBF_DRIVE_HELPERS_SIZE(p) = 0;
    }
    auto result = VehicleHelperUpdate_Original(p, deltaTime);
    LTBF_DRIVE_HELPERS_SIZE(p) = size;
    return result;
  } else {
    return VehicleHelperUpdate_Original(p, deltaTime);
  }
}

// disables angular damping
//REGISTER_FLIGHT_HOOK(void __fastcall, vehiclePhysicsData_AddTorque,
//    RED4ext::vehicle::PhysicsData *a1, RED4ext::Vector3 *torque) {
//  auto fc = FlightComponent::Get(a1->vehicle);
//  if (fc && fc->active) {
//    return;
//  } else {
//    vehiclePhysicsData_AddTorque_Original(a1, torque);
//  }
//}

// vehicle->sub_2A8
// void vehicle::CarBaseObject::AdjustSplineTransformToRoad(Transform &, float, float)
REGISTER_FLIGHT_HOOK_HASH(void __fastcall, 1414536155, VehicleUpdateOrientationWithPID, 
    RED4ext::vehicle::CarBaseObject *a1, RED4ext::Transform *a2, float a3, float a4) {
  auto fc = FlightComponent::Get(a1);
  if (fc && fc->active) {
    return;
  }
  VehicleUpdateOrientationWithPID_Original(a1, a2, a3, a4);
}

REGISTER_FLIGHT_HOOK_HASH(uintptr_t __fastcall, 2879787320, CarSuspension_AnimationUpdate, RED4ext::vehicle::CarPhysics *a1,
                     float timeDelta) {
  auto fc = FlightComponent::Get(LTBF_PHYSICS_PARENT(a1));
  if (fc) {
    auto rtti = RED4ext::CRTTISystem::Get();
    auto fcc = rtti->GetClass("FlightComponent");
    auto rollProp = fcc->GetProperty("roll");
    if (fc->active) {
      LTBF_PHYSICS_PARENT(a1)->input.turnInput = rollProp->GetValue<float>(fc);
    }
  }
  return CarSuspension_AnimationUpdate_Original(a1, timeDelta);
}

REGISTER_FLIGHT_HOOK_HASH(uintptr_t __fastcall, 3191280029, BikeSuspension_AnimationUpdate, RED4ext::vehicle::BikePhysics *a1, float timeDelta) {
  auto fc = FlightComponent::Get(LTBF_PHYSICS_PARENT(a1));
#ifdef __APPLE__
  // LTBF R41: turnRate 0xDC and tiltControlEnabled 0xE08 on macOS (R41b; Windows 0xF4 and 0xE20)
  if (fc && fc->active) {
    LTBF_PHYSICS_PARENT(a1)->input.turnInput = 0.0;
    LtbfMac::Layout::At<float>(a1, LtbfMac::Layout::WheeledPhysics_turnRate) = 0.0;
    LtbfMac::Layout::At<uint8_t>(a1, LtbfMac::Layout::BikePhysics_tiltControlEnabled) = 0;
  } else {
    LtbfMac::Layout::At<uint8_t>(a1, LtbfMac::Layout::BikePhysics_tiltControlEnabled) = 1;
  }
#else
  if (fc && fc->active) {
    a1->parent->input.turnInput = 0.0;
    a1->turnRate = 0.0;
    a1->tiltControlEnabled = 0;
  } else {
    a1->tiltControlEnabled = 1;
  }
#endif
  auto og = BikeSuspension_AnimationUpdate_Original(a1, timeDelta);
  // UpdateAnimValueForCName(a1->parent3, "throttle", 0.0);
  return og;
}

//// hopefully stop wheel effects
//REGISTER_FLIGHT_HOOK(void __fastcall, vehicleUnk570_ProcessEffects, RED4ext::vehicle::Unk570 *unk570,
//                     unsigned int wheelIndex, RED4ext::vehicle::Unk570::Unk40 *unk40, float deltaTime) {
//  auto fc = FlightComponent::Get(unk570->vehicle);
//  if (fc && fc->active) {
//
//  } else {
//    vehicleUnk570_ProcessEffects_Original(unk570, wheelIndex, unk40, deltaTime);
//  }
//}
//
//// hopefully stop wheel effects somewhere else
//REGISTER_FLIGHT_HOOK(void __fastcall, vehicleUnk570_UpdateEffectsBlackboard, RED4ext::vehicle::Unk570 *unk570,
//                     unsigned int wheelCount) {
//  auto fc = FlightComponent::Get(unk570->vehicle);
//  if (fc && fc->active) {
//
//  } else {
//    vehicleUnk570_UpdateEffectsBlackboard_Original(unk570, wheelCount);
//  }
//}

//#include <RED4ext/Scripting/Natives/Generated/game/GameAudioSystem.hpp>
//
//void RED4ext::game::GameAudioSystem::UpdateParameter(int *audioReference, CName parameterName, float parameterValue) {
//  RelocFunc<decltype(&RED4ext::game::GameAudioSystem::UpdateParameter)> call(gameGameAudioSystem_UpdateParameter_Addr);
//  return call(audioReference, parameterName, parameterValue);
//}


//REGISTER_FLIGHT_HOOK(RED4ext::vehicle::Unk570::FxLookup *__fastcall, vehicleUnk570_GetFxForMaterial,
//                     RED4ext::vehicle::Unk570 *unk570, RED4ext::CName material, char isBackWheel) {
//  auto og = vehicleUnk570_GetFxForMaterial_Original(unk570, material, isBackWheel);
//  auto fc = FlightComponent::Get(unk570->vehicle);
//  if (fc && fc->active && fc->configuration) {
//    auto ng = new RED4ext::vehicle::Unk570::FxLookup();
//
//    ng->skid_marks_particle = fc->configuration.instance->particleEffect;
//    ng->skid_marks_decal = fc->configuration.instance->decalEffect;
//
//    ng->tire_tracks_particle = fc->configuration.instance->particleEffect;
//    ng->tire_tracks_decal = fc->configuration.instance->decalEffect;
//
//    ng->wet_skid_marks_particle = fc->configuration.instance->particleEffect;
//    ng->wet_skid_marks_decal = fc->configuration.instance->decalEffect;
//
//    ng->wet_tire_tracks_particle = fc->configuration.instance->particleEffect;
//    ng->wet_tire_tracks_decal = fc->configuration.instance->decalEffect;
//
//    ng->rain_skid_marks_particle = fc->configuration.instance->particleEffect;
//    ng->rain_skid_marks_decal = fc->configuration.instance->decalEffect;
//
//    ng->rain_tire_tracks_particle = fc->configuration.instance->particleEffect;
//    ng->rain_tire_tracks_decal = fc->configuration.instance->decalEffect;
//
//    return ng;
//  } else {
//    return og;
//  }
//}
//
//REGISTER_FLIGHT_HOOK(RED4ext::vehicle::Unk570::SmearFxLookup *__fastcall, vehicleUnk570_GetSmearFxForMaterial,
//                     RED4ext::vehicle::Unk570 *unk570, RED4ext::CName material, char isBackWheel) {
//  auto og = vehicleUnk570_GetSmearFxForMaterial_Original(unk570, material, isBackWheel);
//  auto fc = FlightComponent::Get(unk570->vehicle);
//  if (fc && fc->active && fc->configuration) {
//    auto ng = new RED4ext::vehicle::Unk570::SmearFxLookup();
//
//    ng->skid_marks_decal = fc->configuration.instance->decalEffect.resource.path;
//    ng->tire_tracks_decal = fc->configuration.instance->decalEffect.resource.path;
//
//    return ng;
//  } else {
//    return og;
//  }
//}

// could be used to adjust the sound events? maybe
//REGISTER_FLIGHT_HOOK(void __fastcall, vehicleUnk580_Update, RED4ext::vehicle::Unk580* unk580, __int64 a2, float a3, uint64_t *a4) {
//  auto fc = FlightComponent::Get(unk580->vehicle);
//  if (fc && fc->active) {
//    //unk580->player_audio_resource_metadata->generalData.skid = "v_car_dst_default_scrape_start";
//
//    //unk580->player_audio_resource_metadata->generalData.skid = "ph_metal_barrel_solid_roll";
//    unk580->player_audio_resource_metadata->collisionCooldown = 0.1;
//    //std::queue<RED4ext::CName> starts, stops;
//    //for (auto &event : unk580->player_audio_resource_metadata->wheelData.wheelStartEvents) {
//    //  starts.push(event);
//    //  event = "ph_metal_car";
//    //  //event = "ph_metal_barrel_solid_roll";
//    //}
//    //for (auto &event : unk580->player_audio_resource_metadata->wheelData.wheelStopEvents) {
//    //  stops.push(event);
//    //  event = "ph_metal_car";
//    //  //event = "ph_metal_barrel_solid_roll";
//    //}
//
//    vehicleUnk580_Update_Original(unk580, a2, a3, a4);
//
//    //for (auto &event : unk580->player_audio_resource_metadata->wheelData.wheelStartEvents) {
//    //  event = starts.front();
//    //  starts.pop();
//    //}
//    //for (auto &event : unk580->player_audio_resource_metadata->wheelData.wheelStopEvents) {
//    //  event = stops.front();
//    //  stops.pop();
//    //}
//
//    //unk580->player_audio_resource_metadata->generalData.skid = "veh_wheel_skid";
//  } else {
//    unk580->player_audio_resource_metadata->collisionCooldown = 0.5;
//    vehicleUnk580_Update_Original(unk580, a2, a3, a4);
//  }
//}


//REGISTER_FLIGHT_HOOK(void __fastcall, vehicleUnk580_UpdateEvents, 
//  RED4ext::vehicle::Unk580 *unk580, RED4ext::vehicle::WheelUpdate *update, __int64 a2) {
//  auto fc = FlightComponent::Get(unk580->vehicle);
//  if (fc && fc->active) {
//  } else {
//    vehicleUnk580_UpdateEvents(unk580, update, a2);
//  }
//}

// prevents wheels from adding torque/etc
//REGISTER_FLIGHT_HOOK(void __fastcall, vehicleWheeledPhysics_SomethingWheelRayTrace,
//    RED4ext::vehicle::WheeledPhysics *physics, unsigned __int8 index) {
//  auto fc = FlightComponent::Get(physics->parent);
//  if (fc && fc->active) {
//    for (int i = 0; i < physics->unkD10->numWheels; i++) {
//      auto wheel = physics->unkD10->wheel[i];
//      wheel.unk8C = RED4ext::Vector3(INFINITY, INFINITY, INFINITY);
//      wheel.unkA0.position.X = INFINITY;
//      wheel.unkA0.position.Y = INFINITY;
//      wheel.unkA0.position.Z = INFINITY;
//      wheel.unkA0.position.W = 0.0;
//      wheel.unkA0.orientation = RED4ext::Quaternion(0.0, 0.0, 0.0, 1.0);
//      wheel.physMaterial.hash = 0;
//      wheel.unkC0[0] = 0x00000000;
//      wheel.unkC0[1] = 0x00000000;
//      wheel.unkC0[2] = 0xffffffff;
//      wheel.unkC0[3] = 0x00000000;
//      wheel.unkC0[4] = 0xffffffff;
//      wheel.unkD4 = 0;
//      wheel.unkD8 = 0x00030000;
//      wheel.unkCF0 = false;
//      wheel.unkCF8 = INFINITY;
//      wheel.relatedToAir = INFINITY;
//    }
//    for (int i = 0; i < physics->insert2Count; i++) {
//      physics->insert2[i].worldPosition.X = 0.0;
//      physics->insert2[i].worldPosition.Y = 0.0;
//      physics->insert2[i].worldPosition.Z = 0.0;
//      physics->insert2[i].unk0C.X = 0.0;
//      physics->insert2[i].unk0C.Y = 0.0;
//      physics->insert2[i].unk18 = 0.0;
//      physics->insert2[i].unk110 = 0.0;
//      physics->insert2[i].unk118 = 0.0;
//      physics->insert2[i].unk11C = 0.0;
//      physics->insert2[i].unk120 = 0.0;
//      physics->insert2[i].unk150 = INFINITY;
//      physics->insert2[i].unk154 = INFINITY;
//      physics->insert2[i].unk158 = INFINITY;
//      physics->insert2[i].unk15C = 0.0;
//      physics->insert2[i].unk160 = 0.0;
//    }
//
//    physics->unkAC = 0xFFFFFFFF;
//    physics->wheelsNeedUpdate = 0;
//    physics->unkC40 = 0.0;
//  } else {
//    vehicleWheeledPhysics_SomethingWheelRayTrace_Original(physics, index);
//  }
//}


//REGISTER_FLIGHT_HOOK(void __fastcall, vehicleWheeledPhysics_Update, RED4ext::vehicle::WheeledPhysics *physics) {
//  auto fc = FlightComponent::Get(physics->parent);
//  if (fc && fc->active) {
//    //physics->isMoving = true;
//  }
//  vehicleWheeledPhysics_Update_Original(physics);
//  //if (fc && fc->active) {
//  //  physics->isMoving = 1;
//  //  physics->unkAC = -1;
//  //  physics->unkB0 = 0.0;
//  //  physics->unkA4 = -INFINITY;
//  //}
//}

// disables tire torque & friction
// void vehicle::DrivingForcesHelper::Apply(float)
REGISTER_FLIGHT_HOOK_HASH(void __fastcall, 1989218322, RollFactorTorqueThing,
    uint64_t *a1, float a2) {
  auto physics = (RED4ext::vehicle::WheeledPhysics *)a1[2];
  if (LTBF_PHYSICS_PARENT(physics)) {
    auto fc = FlightComponent::Get(LTBF_PHYSICS_PARENT(physics));
    if (fc && fc->active) {
      return;
    }
  }
  RollFactorTorqueThing_Original(a1, a2);
}

// disables tire friction
// void vehicle::WheelSuspensionBase::ApplyAntiSwaybarForce(vehicle::CarWheel, vehicle::CarWheel, float, math::Transform const &)
REGISTER_FLIGHT_HOOK_HASH(void __fastcall, 2268865059, FourWheelTorque,
    RED4ext::vehicle::WheeledPhysics *physics, unsigned __int8 rearWheelIndex, unsigned __int8 frontWheelIndex, float a4, RED4ext::Transform *transform) {
  if (LTBF_PHYSICS_PARENT(physics)) {
    auto fc = FlightComponent::Get(LTBF_PHYSICS_PARENT(physics));
    if (fc && fc->active) {
      vehicle::PhysicsStructUpdate(LTBF_PHYSICS_PARENT(physics)->physicsData);
      return;
    }
  }
  FourWheelTorque_Original(physics, rearWheelIndex, frontWheelIndex, a4, transform);
}

//REGISTER_FLIGHT_HOOK(void __fastcall, vehicleUnk588_UpdateTransform, RED4ext::vehicle::Unk588 * unk588, RED4ext::Transform * transform) {
//  ;
//  vehicleUnk588_UpdateTransform_Original(unk588, transform);
//  ;
//}