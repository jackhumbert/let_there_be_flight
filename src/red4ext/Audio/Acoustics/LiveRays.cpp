#include "LiveRays.hpp"
#include "Acoustics.hpp"
#include "Probe.hpp"

#include "Flight/Component.hpp"
#include <PhysX3.hpp>

#include <RED4ext/CNamePool.hpp>
#include <RED4ext/Scripting/Natives/Generated/vehicle/BaseObject.hpp>
#include <RED4ext/Scripting/Natives/Generated/vehicle/ChassisComponent.hpp>
#include <RED4ext/Scripting/Natives/physicsPhysicalSystemProxy.hpp>
#include <RED4ext/Scripting/Natives/physicsProxyHelper.hpp>
#include <spdlog/spdlog.h>

#include <chrono>
#include <cstring>
#include <mutex>
#include <string>
#include <unordered_map>
#include <windows.h>

namespace Acoustics {

namespace {

constexpr float kRange = 100.0f;

bool SafeRead(const void *aFrom, void *aTo, size_t aSize) {
  __try {
    std::memcpy(aTo, aFrom, aSize);
    return true;
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    return false;
  }
}

// The game's name for a PhysX material (.physmat), found by looking for a CName among the first
// words of the material and of what its userData points at; cached by material, "" if none.
std::string MaterialName(physx::PxShape *aShape) {
  void *material = nullptr;
  if (!aShape || aShape->getMaterials(&material, 1, 0) != 1 || !material) {
    return "";
  }
  static std::mutex lock;
  static std::unordered_map<void *, std::string> names;
  std::lock_guard guard(lock);
  if (auto it = names.find(material); it != names.end()) {
    return it->second;
  }
  std::string found;
  auto search = [&](const void *aObject, const char *aWhere) {
    uint64_t words[32];
    if (!aObject || !SafeRead(aObject, words, sizeof(words))) {
      return false;
    }
    for (int i = 0; i < 32; ++i) {
      const char *name = RED4ext::CNamePool::Get(RED4ext::CName(words[i]));
      if (name && std::strstr(name, ".physmat")) {
        found = name;
        spdlog::info("[Acoustics] PhysX material name {} at {} + {:#x}", found, aWhere, 8 * i);
        return true;
      }
    }
    return false;
  };
  void *userData = nullptr;
  SafeRead(static_cast<uint8_t *>(material) + 0x10, &userData, sizeof(userData)); // PxMaterial::userData
  if (!search(material, "PxMaterial") && !search(userData, "PxMaterial::userData")) {
    spdlog::info("[Acoustics] PhysX material {} has no name we can find", material);
  }
  names[material] = found;
  return found;
}

// Only shapes that take part in the simulation, what a car would hit: the scene also holds query-only
// shapes and triggers (zones, volumes round the car), which a ray from inside reports at 0 m.
struct SolidOnly : physx::PxQueryFilterCallback {
  physx::PxQueryHitType preFilter(const physx::PxFilterData &, const physx::PxShape *aShape, const physx::PxRigidActor *,
                                  uint16_t &) override {
    const uint8_t flags = aShape->getFlags().bits;
    return (flags & physx::PxShapeFlag_eSIMULATION_SHAPE) && !(flags & physx::PxShapeFlag_eTRIGGER_SHAPE)
               ? physx::PxQueryHitType_eBLOCK
               : physx::PxQueryHitType_eNONE;
  }
  physx::PxQueryHitType postFilter(const physx::PxFilterData &, const void *) override {
    return physx::PxQueryHitType_eBLOCK;
  }
};

} // namespace

void CastLiveRays(FlightComponent *aComponent) {
  if (!aComponent || !aComponent->active || !aComponent->chassis) {
    return;
  }
  RED4ext::physics::ProxyHelper proxyHelper(aComponent->chassis->proxyID, &aComponent->chassis->sharedMutex);
  auto proxy = (RED4ext::physics::PhysicalSystemProxy *)RED4ext::physics::ProxyID::GetProxy(aComponent->chassis->proxyID);
  if (!proxy || proxy->bodies.size == 0 || !proxy->bodies.entries[0]) {
    return;
  }
  auto body = (physx::PxRigidDynamic *)proxy->bodies.entries[0];
  // the game keeps the PhysX scene's origin shifted near the player: the car is known by its world
  // position, the rays leave from the body's place in the scene
  auto vehicle = reinterpret_cast<RED4ext::vehicle::BaseObject *>(aComponent->entity);
  const auto world = vehicle->worldTransform.Position.AsVector4();
  Vec3 origin, forward, up;
  if (!PhysXRaysDue({world.X, world.Y, world.Z}, origin, forward, up)) {
    return;
  }
  const auto bodyPose = body->getGlobalPose();
  auto scene = static_cast<const physx::PxScene *>(body->getScene());
  if (!scene) {
    return;
  }
  static bool once = false;
  if (!once) {
    once = true;
    spdlog::info("[Acoustics] PhysX scene {} vtable {}; car at ({:.1f}, {:.1f}, {:.1f}) in the world, ({:.1f}, {:.1f}, {:.1f}) in the scene",
                 (const void *)scene, *(void *const *)scene, world.X, world.Y, world.Z, bodyPose.p.X, bodyPose.p.Y,
                 bodyPose.p.Z);
  }
  const auto t0 = std::chrono::steady_clock::now();
  const auto rays = Probe::NearRays(forward, up);
  std::array<float, kNearRays> distance;
  std::array<std::string, kNearRays> material;
  // the static world only: buildings, terrain, blockers; not the car itself or traffic
  physx::PxQueryFilterData filter;
  filter.flags = physx::PxQueryFlag_eSTATIC | physx::PxQueryFlag_ePREFILTER;
  SolidOnly solid;
  for (int k = 0; k < kNearRays; ++k) {
    physx::PxRaycastCallback hit;
    const physx::PxVec3 from{bodyPose.p.X, bodyPose.p.Y, bodyPose.p.Z}, dir{rays[k].x, rays[k].y, rays[k].z};
    if (scene->raycast(from, dir, kRange, hit, physx::PxHitFlag_ePOSITION | physx::PxHitFlag_eDISTANCE, filter,
                       &solid) &&
        hit.hasBlock) {
      distance[k] = hit.block.distance;
      material[k] = MaterialName(hit.block.shape);
    } else {
      distance[k] = -1.0f;
      material[k].clear();
    }
  }
  SetPhysXCastTime(std::chrono::duration<float, std::milli>(std::chrono::steady_clock::now() - t0).count());
  SetLiveHits(origin, distance, material, RaySource::PhysX);
}

} // namespace Acoustics
