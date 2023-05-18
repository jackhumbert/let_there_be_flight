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

// 1.52 RVA: 0x1CE0FC0 / 30281664
// 1.6  RVA: 0x1D0D770 / 30463856
// 1.61 RVA: 0x1D0DB30
/// @pattern 48 89 5C 24 08 57 48 83 EC 30 0F 29 74 24 20 48 8B DA 0F 10 32 48 8B F9 66 0F 3A 40 F6 7F 0F 28
short PhysicsUnkStructVelocityUpdate(RED4ext::vehicle::PhysicsData *, RED4ext::Vector3 *);

REGISTER_FLIGHT_HOOK(short, PhysicsUnkStructVelocityUpdate, RED4ext::vehicle::PhysicsData *vps, RED4ext::Vector3 *velocity) {
  auto result = PhysicsUnkStructVelocityUpdate_Original(vps, velocity);

  if (result != FP_INFINITE) {
    vps->velocity = *velocity;
  }

  return result;
}