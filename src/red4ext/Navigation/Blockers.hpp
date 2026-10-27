#pragma once

#include <cstdint>
#include <filesystem>
#include <unordered_map>
#include <vector>

namespace Navigation {

// Invisible vehicle blockers to take out of the world as their sectors load: the collision
// actors of the presets "Vehicle Blocker" / "Block Player and Vehicles" that block flying cars
// where nothing is visible (tools/heightmap/blockers.py export writes blockers.txt).
//
// Why not ArchiveXL: on 2.5 a sector's collision is one shared worldCookedCollisionNode with the
// actors serialized in its compiledData. ArchiveXL's nodeDeletions swaps the placement's node for
// a meshless worldStaticMeshNode, which crashes streaming (null resource in
// StaticMeshNodeInstance::OnInitialize, reproduced twice in exterior_-5_-6_0_3), and its
// per-actor deletions only know the pre-2.5 worldCollisionNode. So the plugin does what
// ArchiveXL does for old collision nodes: it moves each listed actor 2 km below the map, by
// finding the actor's stored position (WorldPosition fixed point x, y, z) among the cooked
// node's actor records after the sector loads and before its nodes are set up
// (Hooks/StreamingSector_PostLoad.cpp has the layout).
//
// The same hook hides listed node placements (hidden_nodes.txt, by hand): e.g. the visual-only
// proxy of a landing pad that our own pad entity replaces (docs/helipads.md, step 8).
class Blockers {
public:
  struct Actor {
    int32_t x, y, z;
  };
  // a placement by its globalNodeId, with the x y it is stored at as a check; id 0 (foliage
  // clusters and other nodes without one) matches by the x y alone
  struct Node {
    uint64_t id;
    float x, y;
  };

  static Blockers &Get();
  bool Load(const std::filesystem::path &aPath);
  bool LoadHidden(const std::filesystem::path &aPath);
  // the actors listed for a sector (by resource path hash), or nullptr
  const std::vector<Actor> *Find(uint64_t aSectorPath) const;
  // the placements to hide in a sector, or nullptr
  const std::vector<Node> *FindHidden(uint64_t aSectorPath) const;
  bool IsLoaded() const { return loaded || !hidden.empty(); }

private:
  std::unordered_map<uint64_t, std::vector<Actor>> sectors;
  std::unordered_map<uint64_t, std::vector<Node>> hidden;
  bool loaded = false;
};

} // namespace Navigation
