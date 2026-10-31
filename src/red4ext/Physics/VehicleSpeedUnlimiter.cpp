#include "VehicleSpeedUnlimiter.hpp"

// Vehicle Speed Unlimiter

namespace vehicle {

// void vehicle::RigidBody::Recalculate(void)
REGISTER_FLIGHT_HOOK_HASH(short, 593628708, RigidBody_Recalculate, RED4ext::vehicle::PhysicsData *ps) {
  // apply force to linear velocity
  RED4ext::Vector3 unlimitedVelocity;
  unlimitedVelocity.X = ps->velocity.X + ps->force.X * ps->inverseMass;
  unlimitedVelocity.Y = ps->velocity.Y + ps->force.Y * ps->inverseMass;
  unlimitedVelocity.Z = ps->velocity.Z + ps->force.Z * ps->inverseMass;

  auto result = RigidBody_Recalculate_Original(ps);

  // ignore speed limit in og function
  if (result != FP_INFINITE) {
    ps->velocity = unlimitedVelocity;
  }

  return result;
}

} // namespace vehicle

// vehicle::RigidBody::ForceSetLinearVelocity(Vector4 const &)
/// @hash 4067628720
// short RigidBody_ForceSetLinearVelocity(RED4ext::vehicle::PhysicsData *, RED4ext::Vector3 *);

REGISTER_FLIGHT_HOOK_HASH(short, 4067628720, RigidBody_ForceSetLinearVelocity, RED4ext::vehicle::PhysicsData *vps, RED4ext::Vector3 *velocity) {
  auto result = RigidBody_ForceSetLinearVelocity_Original(vps, velocity);

  if (result != FP_INFINITE) {
    vps->velocity = *velocity;
  }

  return result;
}