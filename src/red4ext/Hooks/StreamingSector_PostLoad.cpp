#include <Utils/FlightModule.hpp>
#include "Navigation/Blockers.hpp"
#include <RED4ext/RED4ext.hpp>
#include <RED4ext/Scripting/Natives/Generated/world/CookedCollisionNode.hpp>
#include <RED4ext/Scripting/Natives/Generated/world/Node.hpp>
#include <RED4ext/Scripting/Natives/Generated/world/StreamingSector.hpp>
#include <spdlog/spdlog.h>

using namespace RED4ext;

namespace {
// worldStreamingSector + 0x40: the node buffer (layout from ArchiveXL's Red/StreamingSector.hpp)
struct SectorNodeBuffer {
  DataBuffer nodeSetups;               // 00
  uint32_t setupVersion;               // 28
  DynArray<Handle<world::Node>> nodes; // 30
};
constexpr uintptr_t NodeBufferOffset = 0x40;
// 2 km below the map, as ArchiveXL moves deleted collision actors (17 fractional bits)
constexpr int32_t BelowTheMap = static_cast<int32_t>(-2000 * (2 << 16));

// The cooked collision node (2.5 IDB: world::CollisionNodeInstance::OnInitialize -> sub_1420B8CD0
// -> sub_1420B8F80): +0x40 a byte array of group records, one per collision group (a placement's
// instanceIndex picks its group), +0x50 the record stride; +0x58 compiledData, the group data.
// A group record holds at +24 the offset of its data in compiledData and at +28 its actor count;
// the data starts with the actors, 48 bytes each, position first. The physics creation info is
// built from there when the node instance initializes, after PostLoad.
struct GroupTable {
  uint8_t *data; // 40 DynArray<uint8_t>
  uint32_t capacity;
  uint32_t size;
  uint32_t stride; // 50
};
constexpr uintptr_t GroupTableOffset = 0x40;
struct CollisionActor {
  int32_t x, y, z;           // 00 WorldPosition, 17 fractional bits
  uint8_t rest[0x30 - 0x0C]; // 0C orientation (+10), shape range, scale index
};
static_assert(sizeof(CollisionActor) == 0x30);

// Move the listed actors away: match each one's stored position among the node's actors and
// rewrite z. Returns how many were found.
size_t MoveActors(world::CookedCollisionNode *aNode, const std::vector<Navigation::Blockers::Actor> &aActors) {
  const auto &groups = *reinterpret_cast<GroupTable *>(reinterpret_cast<uintptr_t>(aNode) + GroupTableOffset);
  auto *blob = static_cast<uint8_t *>(aNode->compiledData.buffer.data);
  auto blobSize = aNode->compiledData.buffer.size;
  if (!groups.data || !blob || groups.stride < 30 || groups.size % groups.stride != 0) {
    return 0; // not the layout this was written for: leave the node alone
  }
  size_t found = 0;
  for (const auto &want : aActors) {
    bool done = false;
    for (uint32_t at = 0; at < groups.size && !done; at += groups.stride) {
      uint32_t offset = *reinterpret_cast<const uint32_t *>(groups.data + at + 24);
      uint16_t count = *reinterpret_cast<const uint16_t *>(groups.data + at + 28);
      if (offset + uint64_t(count) * sizeof(CollisionActor) > blobSize) {
        continue;
      }
      auto *actors = reinterpret_cast<CollisionActor *>(blob + offset);
      for (uint16_t i = 0; i < count; ++i) {
        if (actors[i].x == want.x && actors[i].y == want.y && actors[i].z == want.z) {
          actors[i].z = BelowTheMap;
          ++found;
          done = true;
          break;
        }
      }
    }
  }
  return found;
}
} // namespace

// void world::StreamingSector::PostLoad(...): the sector's nodes are read but not set up yet;
// ArchiveXL patches sectors at the same point
REGISTER_FLIGHT_HOOK_HASH(void, 3972601611, StreamingSector_PostLoad, world::StreamingSector *aSector, uint64_t a2) {
  StreamingSector_PostLoad_Original(aSector, a2);
  auto &blockers = Navigation::Blockers::Get();
  if (!aSector || !blockers.IsLoaded()) {
    return;
  }
  const auto *actors = blockers.Find(aSector->path.hash);
  if (!actors) {
    return;
  }
  auto *buffer = reinterpret_cast<SectorNodeBuffer *>(reinterpret_cast<uintptr_t>(aSector) + NodeBufferOffset);
  static const CName cookedCollision = "worldCookedCollisionNode";
  size_t found = 0;
  for (auto &handle : buffer->nodes) {
    auto *node = handle.GetPtr();
    if (node && node->GetNativeType()->GetName() == cookedCollision) {
      found += MoveActors(reinterpret_cast<world::CookedCollisionNode *>(node), *actors);
    }
  }
  if (found == actors->size()) {
    spdlog::info("[Blockers] {:016x}: moved {} invisible blocker(s) away", aSector->path.hash, found);
  } else {
    // a sector the list was not made for (another game version, or one a mod replaced)
    spdlog::warn("[Blockers] {:016x}: found {} of {} listed blockers", aSector->path.hash, found, actors->size());
  }
}
