#include "VehicleSpeedUnlimiter.hpp"
#include <cmath>
#ifdef __APPLE__
#include <cstdlib>
#include <spdlog/spdlog.h>
#include "MacShims.hpp"
#endif

// Vehicle Speed Unlimiter

namespace vehicle {

// void vehicle::RigidBody::Recalculate(void)
REGISTER_FLIGHT_HOOK_HASH(short, 593628708, PhysicsStructUpdate, RED4ext::vehicle::PhysicsData *ps) {
  // apply force to linear velocity
  RED4ext::Vector3 unlimitedVelocity;
  unlimitedVelocity.X = ps->velocity.X + ps->force.X * ps->inverseMass;
  unlimitedVelocity.Y = ps->velocity.Y + ps->force.Y * ps->inverseMass;
  unlimitedVelocity.Z = ps->velocity.Z + ps->force.Z * ps->inverseMass;

#ifdef __APPLE__
  // LTBF R41: RigidBody::Recalculate returns nothing on macOS (Windows leaves the _fdclass of the speed in ax, which LTBF reads).
  // Recompute that class here: the macOS body zeroes the velocity when |v|^2 > 10000 or is not finite.
  PhysicsStructUpdate_Original(ps);
  const float speedSquared = unlimitedVelocity.X * unlimitedVelocity.X + unlimitedVelocity.Y * unlimitedVelocity.Y +
                             unlimitedVelocity.Z * unlimitedVelocity.Z;
  short result = std::isinf(speedSquared) ? FP_INFINITE : FP_NORMAL;
#else
  auto result = PhysicsStructUpdate_Original(ps);
#endif

  // ignore speed limit in og function
  if (result != FP_INFINITE) {
    ps->velocity = unlimitedVelocity;
  }

  return result;
}

} // namespace vehicle

// vehicle::RigidBody::ForceSetLinearVelocity(Vector4 const &)
/// @hash 4067628720
// short PhysicsUnkStructVelocityUpdate(RED4ext::vehicle::PhysicsData *, RED4ext::Vector3 *);

REGISTER_FLIGHT_HOOK_HASH(short, 4067628720, PhysicsUnkStructVelocityUpdate, RED4ext::vehicle::PhysicsData *vps, RED4ext::Vector3 *velocity) {
#ifdef __APPLE__
  // LTBF R43 (macOS, diagnostics): a flying car's speed stays pinned at about 0.5 m/s while LTBF pushes it. With LTBF_MAC_DIAG=1, log
  // who force-sets a velocity and to what (caller as an image offset), about once a second.
  static const bool s_macDiag = std::getenv("LTBF_MAC_DIAG") != nullptr;
  static uint32_t s_macCalls = 0;
  if (s_macDiag && (s_macCalls++ % 60) == 0) {
    const auto caller = reinterpret_cast<uintptr_t>(__builtin_return_address(0)) - LtbfMac::ImageBase();
    spdlog::info("[ltbf][phys] ForceSetLinearVelocity call {} from game+0x{:X}: ({:.3f},{:.3f},{:.3f}), was ({:.3f},{:.3f},{:.3f})", s_macCalls,
                 caller, velocity->X, velocity->Y, velocity->Z, vps->velocity.X, vps->velocity.Y, vps->velocity.Z);
  }
  // LTBF R41: ForceSetLinearVelocity returns nothing on macOS; recompute the class of |v|^2 the Windows result carried.
  PhysicsUnkStructVelocityUpdate_Original(vps, velocity);
  const float speedSquared = velocity->X * velocity->X + velocity->Y * velocity->Y + velocity->Z * velocity->Z;
  short result = std::isinf(speedSquared) ? FP_INFINITE : FP_NORMAL;
#else
  auto result = PhysicsUnkStructVelocityUpdate_Original(vps, velocity);
#endif

  if (result != FP_INFINITE) {
    vps->velocity = *velocity;
  }

  return result;
}