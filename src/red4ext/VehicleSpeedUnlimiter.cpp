#include "VehicleSpeedUnlimiter.hpp"

// Vehicle Speed Unlimiter

namespace vehicle {

struct SpeedUnlimiter : FlightModule {
  void Load(const RED4ext::Sdk *aSdk, RED4ext::PluginHandle aHandle);
  void Unload(const RED4ext::Sdk *aSdk, RED4ext::PluginHandle aHandle);
  static short PhysicsStructUpdate(RED4ext::vehicle::PhysicsData *ps);
  static short PhysicsUnkStructVelocityUpdate(RED4ext::vehicle::PhysicsData *ps, RED4ext::Vector3 *);
};

// 1.6  RVA : 0x1D0E180 / 30466432
// 40 53 48 81 EC 80 00 00 00 F3 0F 10 41 40 48 8B D9 F3 0F 10 51 08 0F 28 C8 F3 0F 59 09 0F 29 74
constexpr uintptr_t PhysicsStructUpdateAddr = 0x1D0E180;
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

// 1.52 RVA: 0x1CE0FC0 / 30281664
// 1.6  RVA: 0x1D0D770 / 30463856
/// @pattern 48 89 5C 24 08 57 48 83 EC 30 0F 29 74 24 20 48 8B DA 0F 10 32 48 8B F9 66 0F 3A 40 F6 7F 0F 28
constexpr uintptr_t PhysicsUnkStructVelocityUpdateAddr = 0x1D0D770;
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