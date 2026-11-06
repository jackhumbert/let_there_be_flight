#include "LiveRays.hpp"
#include "Acoustics.hpp"
#include "Occlusion.hpp"
#include "Probe.hpp"

#include "Flight/Component.hpp"
#include "Flight/DeveloperSettings.hpp"
#include <PhysX3.hpp>

#include <RED4ext/CNamePool.hpp>
#include <RED4ext/Scripting/Natives/Generated/vehicle/BaseObject.hpp>
#include <RED4ext/Scripting/Natives/Generated/vehicle/ChassisComponent.hpp>
#include <RED4ext/Scripting/Natives/physicsPhysicalSystemProxy.hpp>
#include <RED4ext/Scripting/Natives/physicsProxyHelper.hpp>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>
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

// The NPC cars that hold a channel, a few a cast: one ray from the listener to each car's centre through
// the static solid shapes (Occlusion.hpp). Blocked is a hit nearer than the car less its own size.
// The shift moves the world to the scene, as for the near rays.
void CastOcclusion(const physx::PxScene *aScene, float aShiftX, float aShiftY, float aShiftZ,
                   const physx::PxQueryFilterData &aFilter, SolidOnly &aSolid) {
  if (!Occlusion::enabled.load(std::memory_order_relaxed)) {
    return;
  }
  static Occlusion::RoundRobin robin;
  const auto listener = Occlusion::Listener();
  for (int k : robin.Next(Occlusion::Count())) {
    Occlusion::Target car;
    if (!Occlusion::Slot(k, car)) {
      continue;
    }
    const float dx = car.position.x - listener.x, dy = car.position.y - listener.y, dz = car.position.z - listener.z;
    const float distance = std::sqrt(dx * dx + dy * dy + dz * dz);
    if (distance < 1e-3f) {
      continue;
    }
    const float range = std::min(distance, Occlusion::kRangeM);
    const physx::PxVec3 from{listener.x + aShiftX, listener.y + aShiftY, listener.z + aShiftZ},
        dir{dx / distance, dy / distance, dz / distance};
    physx::PxRaycastCallback hit;
    const bool blocked = aScene->raycast(from, dir, range, hit, physx::PxHitFlag_eDISTANCE, aFilter, &aSolid) &&
                         hit.hasBlock && hit.block.distance < distance - Occlusion::kCarSizeM;
    Occlusion::StoreBlocked(k, car.car, blocked);
    Occlusion::CountCast();
  }
}

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
  // the fan turns a little each cast (-4.5, -1.5, 1.5, 4.5 degrees) so the hits cover the surfaces
  static unsigned casts = 0;
  const auto rays = Probe::NearRays(forward, up, (float(casts++ % 4) - 1.5f) * 3.0f);
  std::array<float, kNearRays> distance;
  std::array<Vec3, kNearRays> position{}, normal{};
  std::array<std::string, kNearRays> material;
  // the static world only: buildings, terrain, blockers; not the car itself or traffic
  physx::PxQueryFilterData filter;
  filter.flags = physx::PxQueryFlag_eSTATIC | physx::PxQueryFlag_ePREFILTER;
  SolidOnly solid;
  for (int k = 0; k < kNearRays; ++k) {
    physx::PxRaycastCallback hit;
    const physx::PxVec3 from{bodyPose.p.X, bodyPose.p.Y, bodyPose.p.Z}, dir{rays[k].x, rays[k].y, rays[k].z};
    if (scene->raycast(from, dir, kRange, hit, physx::PxHitFlag_ePOSITION | physx::PxHitFlag_eNORMAL | physx::PxHitFlag_eDISTANCE, filter,
                       &solid) &&
        hit.hasBlock) {
      distance[k] = hit.block.distance;
      // scene to world: the shift of the car's own position between the two
      const auto &h = hit.block;
      position[k] = {h.position.x - bodyPose.p.X + world.X, h.position.y - bodyPose.p.Y + world.Y,
                     h.position.z - bodyPose.p.Z + world.Z};
      normal[k] = {h.normal.x, h.normal.y, h.normal.z};
      material[k] = MaterialName(hit.block.shape);
    } else {
      distance[k] = -1.0f;
      material[k].clear();
    }
  }
  // the live sphere: the developer setting acousticsRays past the near rays, bunched where the car is going (its
  // velocity from cast to cast), for the enclosure and the mean free path (Probe::SphereRays)
  static Vec3 lastWorld{};
  static auto lastTime = std::chrono::steady_clock::time_point{};
  const Vec3 here{world.X, world.Y, world.Z};
  const float dt = lastTime == std::chrono::steady_clock::time_point{}
                       ? 0.0f
                       : std::chrono::duration<float>(t0 - lastTime).count();
  Vec3 velocity{};
  if (dt > 0.0f && dt < 0.5f) {
    velocity = {(here.x - lastWorld.x) / dt, (here.y - lastWorld.y) / dt, (here.z - lastWorld.z) / dt};
  }
  lastWorld = here;
  lastTime = t0;
  float speed = std::sqrt(velocity.x * velocity.x + velocity.y * velocity.y + velocity.z * velocity.z);
  if (speed > 300.0f) {
    speed = 0.0f; // a teleport is not a speed
  }
  const Vec3 ahead = speed > 1.0f ? Vec3{velocity.x / speed, velocity.y / speed, velocity.z / speed} : forward;
  static std::vector<Vec3> sphereDir;
  static std::vector<float> sphereWeight, sphereDistance;
  Probe::SphereRays(std::max(int(Flight::Developer().acousticsRays) - kNearRays, 0), ahead, speed, 0.37f * float(casts % 17),
                    sphereDir, sphereWeight);
  sphereDistance.resize(sphereDir.size());
  for (size_t i = 0; i < sphereDir.size(); ++i) {
    physx::PxRaycastCallback hit;
    const physx::PxVec3 from{bodyPose.p.X, bodyPose.p.Y, bodyPose.p.Z}, dir{sphereDir[i].x, sphereDir[i].y, sphereDir[i].z};
    sphereDistance[i] = scene->raycast(from, dir, Probe::kMaxDistance, hit, physx::PxHitFlag_eDISTANCE, filter, &solid) &&
                                hit.hasBlock
                            ? hit.block.distance
                            : -1.0f;
  }
  SetPhysXCastTime(std::chrono::duration<float, std::milli>(std::chrono::steady_clock::now() - t0).count());
  SetLiveHits(origin, distance, material, position, normal, sphereDir, sphereDistance, sphereWeight);
  CastOcclusion(scene, bodyPose.p.X - world.X, bodyPose.p.Y - world.Y, bodyPose.p.Z - world.Z, filter, solid);
}

} // namespace Acoustics
