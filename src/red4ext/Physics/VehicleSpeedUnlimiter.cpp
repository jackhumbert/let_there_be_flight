#include "VehicleSpeedUnlimiter.hpp"

// Vehicle Speed Unlimiter

namespace vehicle {

REGISTER_FLIGHT_HOOK(short, PhysicsStructUpdate, RED4ext::vehicle::PhysicsData *ps) {
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

} // namespace vehicle

/// @hash 4067628720
short PhysicsUnkStructVelocityUpdate(RED4ext::vehicle::PhysicsData *, RED4ext::Vector3 *);

REGISTER_FLIGHT_HOOK(short, PhysicsUnkStructVelocityUpdate, RED4ext::vehicle::PhysicsData *vps, RED4ext::Vector3 *velocity) {
  auto result = PhysicsUnkStructVelocityUpdate_Original(vps, velocity);

  if (result != FP_INFINITE) {
    vps->velocity = *velocity;
  }

  return result;
}