#include "VehicleSpeedUnlimiter.hpp"

// Vehicle Speed Unlimiter

namespace vehicle {

REGISTER_FLIGHT_MODULE(SpeedUnlimiter);

// 40 53 48 81 EC 80 00 00  00 F3 0F 10 41 40 48 8B D9 F3 0F 10 51 08 0F 28 C8 F3 0F 59 09 0F 29 74
constexpr uintptr_t PhysicsStructUpdateAddr = 0x141CE1960 - RED4ext::Addresses::ImageBase;
decltype(&SpeedUnlimiter::PhysicsStructUpdate) PhysicsStructUpdate_Original;

short SpeedUnlimiter::PhysicsStructUpdate(RED4ext::physics::VehiclePhysicsStruct *ps) {

  // apply force to linear velocity
  RED4ext::Vector3 unlimitedVelocity;
  unlimitedVelocity.X = ps->velocity.X + ps->force.X * ps->inverseMass;
  unlimitedVelocity.Y = ps->velocity.Y + ps->force.Y * ps->inverseMass;
  unlimitedVelocity.Z = ps->velocity.Z + ps->force.Z * ps->inverseMass;

  auto result = PhysicsStructUpdate_Original(ps);

  // ignore speed limit in og function
  if (result != 1) {
    ps->velocity = unlimitedVelocity;
  }

  return result;
}

void SpeedUnlimiter::Load(const RED4ext::Sdk *aSdk, RED4ext::PluginHandle aHandle) {
  while (!aSdk->hooking->Attach(aHandle, RED4EXT_OFFSET_TO_ADDR(PhysicsStructUpdateAddr), &PhysicsStructUpdate,
                        reinterpret_cast<void **>(&PhysicsStructUpdate_Original)));
}

void SpeedUnlimiter::Unload(const RED4ext::Sdk *aSdk, RED4ext::PluginHandle aHandle) {
  aSdk->hooking->Detach(aHandle, RED4EXT_OFFSET_TO_ADDR(PhysicsStructUpdateAddr));
}
} // namespace vehicle