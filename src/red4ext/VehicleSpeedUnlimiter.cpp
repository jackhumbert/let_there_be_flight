#include "VehicleSpeedUnlimiter.hpp"

// Vehicle Speed Unlimiter

namespace vehicle {

struct SpeedUnlimiter : FlightModule {
  void Load(const RED4ext::Sdk *aSdk, RED4ext::PluginHandle aHandle);
  void Unload(const RED4ext::Sdk *aSdk, RED4ext::PluginHandle aHandle);
  static short PhysicsStructUpdate(RED4ext::vehicle::PhysicsData *ps);
  static short PhysicsUnkStructVelocityUpdate(RED4ext::vehicle::PhysicsData *ps, RED4ext::Vector3 *);
};

// 40 53 48 81 EC 80 00 00 00 F3 0F 10 41 40 48 8B D9 F3 0F 10 51 08 0F 28 C8 F3 0F 59 09 0F 29 74
constexpr uintptr_t PhysicsStructUpdateAddr = 0x1CE1960;
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

constexpr uintptr_t PhysicsUnkStructVelocityUpdateAddr = 0x1CE03C0 + 0xC00;
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