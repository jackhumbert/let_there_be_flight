#include "VehicleSpeedUnlimiter.hpp"
#include "Addresses.hpp"

// Vehicle Speed Unlimiter

namespace vehicle {

decltype(&SpeedUnlimiter::PhysicsStructUpdate) PhysicsStructUpdate_Original;

short SpeedUnlimiter::PhysicsStructUpdate(RED4ext::vehicle::PhysicsData *ps) {

  // apply force to linear velocity
  RED4ext::Vector3 unlimitedVelocity;
  unlimitedVelocity.X = ps->velocity.X + ps->force.X * ps->inverseMass;
  unlimitedVelocity.Y = ps->velocity.Y + ps->force.Y * ps->inverseMass;
  unlimitedVelocity.Z = ps->velocity.Z + ps->force.Z * ps->inverseMass;

  auto result = PhysicsStructUpdate_Original(ps);

  // ignore speed limit in og function
  if (result != FP_INFINITE) {
    ps->velocity = unlimitedVelocity;
  }

  return result;
}

decltype(&SpeedUnlimiter::PhysicsUnkStructVelocityUpdate) PhysicsUnkStructVelocityUpdate_Original;

short SpeedUnlimiter::PhysicsUnkStructVelocityUpdate(RED4ext::vehicle::PhysicsData *vps,
                                                     RED4ext::Vector3 *velocity) {
  auto result = PhysicsUnkStructVelocityUpdate_Original(vps, velocity);

  if (result != FP_INFINITE) {
    vps->velocity = *velocity;
  }

  return result;
}

void SpeedUnlimiter::Load(const RED4ext::Sdk *aSdk, RED4ext::PluginHandle aHandle) {
  while (!aSdk->hooking->Attach(aHandle, RED4EXT_OFFSET_TO_ADDR(PhysicsStructUpdateAddr), &PhysicsStructUpdate,
                                reinterpret_cast<void **>(&PhysicsStructUpdate_Original)))
    ;
  while (!aSdk->hooking->Attach(aHandle, RED4EXT_OFFSET_TO_ADDR(PhysicsUnkStructVelocityUpdateAddr),
                                &PhysicsUnkStructVelocityUpdate,
                                reinterpret_cast<void **>(&PhysicsUnkStructVelocityUpdate_Original)))
    ;
}

void SpeedUnlimiter::Unload(const RED4ext::Sdk *aSdk, RED4ext::PluginHandle aHandle) {
  aSdk->hooking->Detach(aHandle, RED4EXT_OFFSET_TO_ADDR(PhysicsStructUpdateAddr));
  aSdk->hooking->Detach(aHandle, RED4EXT_OFFSET_TO_ADDR(PhysicsUnkStructVelocityUpdateAddr));
}

REGISTER_FLIGHT_MODULE(SpeedUnlimiter);

} // namespace vehicle