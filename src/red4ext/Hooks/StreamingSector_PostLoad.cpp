#include <Utils/FlightModule.hpp>
#include "Navigation/Blockers.hpp"
#include <RED4ext/RED4ext.hpp>
#include <RED4ext/Scripting/Natives/Generated/world/CookedCollisionNode.hpp>
#include <RED4ext/Scripting/Natives/Generated/world/Node.hpp>
#include <RED4ext/Scripting/Natives/Generated/world/StreamingSector.hpp>
#include <cmath>
#include <spdlog/fmt/fmt.h>
#include <spdlog/spdlog.h>
#include <string>

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

// A placement in nodeSetups (CompiledNodeInstanceSetupInfo, layout from ArchiveXL's
// Red/StreamingSector.hpp, 0x90 bytes before 2.5): +00 position (Vector4), +20 scale (Vector3),
// +58 globalNodeID; still so on 2.5 (2026-10-27, the tower pad proxy). The record is found by its
// globalNodeID and only touched when its stored x y are the listed ones, in case that changes. Hidden as ArchiveXL's
// nodeDeletions hides a node: 2 km below the map, scale 0.
constexpr uintptr_t SetupIdOffset = 0x58;
constexpr uintptr_t SetupScaleOffset = 0x20;
constexpr uint32_t SetupSize = 0x90;

void Hide(uint8_t *aRecord) {
  reinterpret_cast<float *>(aRecord)[2] = -2000.0f;
  auto *scale = reinterpret_cast<float *>(aRecord + SetupScaleOffset);
  scale[0] = scale[1] = scale[2] = 0.0f;
}

bool HideNode(DataBuffer &aSetups, const Navigation::Blockers::Node &aNode) {
  auto *data = static_cast<uint8_t *>(aSetups.buffer.data);
  uint32_t size = aSetups.buffer.size;
  if (!data) {
    return false;
  }
  // nodes without a globalNodeId (foliage clusters, cooked statics) are listed with id 0: match the
  // record by its stored x y alone, record by record
  if (aNode.id == 0) {
    for (uint32_t at = 0; at + SetupSize <= size; at += SetupSize) {
      auto *record = reinterpret_cast<const float *>(data + at);
      if (std::abs(record[0] - aNode.x) < 0.05f && std::abs(record[1] - aNode.y) < 0.05f) {
        Hide(data + at);
        return true;
      }
    }
    return false;
  }
  for (uint32_t at = SetupIdOffset; at + sizeof(uint64_t) <= size; at += 8) {
    if (*reinterpret_cast<const uint64_t *>(data + at) != aNode.id) {
      continue;
    }
    auto *record = reinterpret_cast<float *>(data + at - SetupIdOffset);
    if (std::abs(record[0] - aNode.x) < 0.05f && std::abs(record[1] - aNode.y) < 0.05f) {
      Hide(data + at - SetupIdOffset);
      return true;
    }
    // not the layout this was written for: log the words around the id to find it
    auto *words = reinterpret_cast<const float *>(data + at);
    std::string dump;
    for (int i = -36; i < 12 && at + i * 4 + 4 <= size && static_cast<int>(at) + i * 4 >= 0; ++i) {
      dump += fmt::format(" {:+d}:{:.3f}", i * 4, words[i]);
    }
    spdlog::warn("[Blockers] node {} found at +{:x}, position check failed:{}", aNode.id, at, dump);
    return false;
  }
  return false;
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
  auto *buffer = reinterpret_cast<SectorNodeBuffer *>(reinterpret_cast<uintptr_t>(aSector) + NodeBufferOffset);
  if (const auto *nodes = blockers.FindHidden(aSector->path.hash)) {
    size_t hiddenCount = 0;
    for (const auto &node : *nodes) {
      hiddenCount += HideNode(buffer->nodeSetups, node) ? 1 : 0;
    }
    spdlog::info("[Blockers] {:016x}: hid {} of {} listed node(s)", aSector->path.hash, hiddenCount, nodes->size());
  }
  const auto *actors = blockers.Find(aSector->path.hash);
  if (!actors) {
    return;
  }
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
