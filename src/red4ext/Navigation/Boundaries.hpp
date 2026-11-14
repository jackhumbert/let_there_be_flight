#pragma once

// World boundaries: the areas the game keeps the player out of (gameWorldBoundaryNode, acted on by the
// native gameWorldBoundarySystem; vanilla autodrive stops at one, AutoDriveSystem.OnReachedWorldBoundary),
// so the Autopilot keeps out of them too (docs/design/world-boundaries.md). Loaded from boundaries.tsv in
// the plugin folder (tools/navmap_bake/boundaries.py writes it). The areas in force are laid over the nav
// map as a ColumnMask: their columns, grown by the car's radius as the bake grows solids, read as blocked
// at every height, so the planner, free space and the doorway layer all keep out.
//
// Some areas belong to a sector variant a quest switches on or off. The game offers no way to ask for a
// variant's state, so each such area carries the quest fact that decides it (its gate); the game side
// (FlightBoundaries, FlightBoundariesSystem) reads those facts and calls Apply when one changes. Game-free, so tests and the
// offline tools (navplan) use it as the plugin does.

#include "NavMap.hpp"

#include <atomic>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

namespace Navigation {

// when an area is there: always, or while a quest fact compares true against a value
struct BoundaryGate {
  enum class Op { Always, Greater, Less, GreaterEqual, LessEqual, Equal, NotEqual };
  Op op = Op::Always;
  std::string fact;
  int32_t value = 0;

  // "-" for always, else "<fact><op><value>" with op one of > < >= <= == !=; false when it is neither
  static bool Parse(const std::string &aText, BoundaryGate &aOut);
  bool Holds(int32_t aFactValue) const;
};

struct Boundary {
  std::string name; // the node's debug name
  BoundaryGate gate;
  float zMin = 0, zMax = 0;                      // metres; the mask ignores them (blocked at every height)
  std::vector<std::pair<float, float>> outline;  // x y, world metres, in order
  std::string line;                              // the line it was read from (the mask key hashes these)
};

// a fact's value by name (QuestsSystem.GetFact in the game)
using FactReader = std::function<int32_t(const std::string &)>;

class Boundaries {
public:
  static Boundaries &Get();

  bool Load(const std::filesystem::path &aPath);
  // the lines of a boundaries.tsv (tests); aName names it in the log
  bool Parse(const std::string &aText, const std::string &aName);
  bool IsLoaded() const { return loaded; }
  const std::vector<Boundary> &All() const { return areas; }

  // the distinct facts the gates read
  std::vector<std::string> Facts() const;
  // which areas are in force, by index into All, with the facts read through aFacts
  std::vector<bool> InForce(const FactReader &aFacts) const;

  // the mask of the areas aInForce marks, for aMap; built the first time and kept for the session
  // (one per map size and set of areas, a handful at most), so it outlives any plan using it.
  // nullptr when no area is in force.
  const ColumnMask *MaskFor(const NavMap &aMap, const std::vector<bool> &aInForce);
  // MaskFor the areas in force by aFacts, set on aMap; true when the map's mask changed
  bool Apply(NavMap &aMap, const FactReader &aFacts);
  // how many areas were in force at the last Apply
  int32_t InForceCount() const { return inForceCount; }

private:
  bool loaded = false;
  std::vector<Boundary> areas;
  std::atomic<int32_t> inForceCount{0};
  std::mutex lock;
  std::vector<std::unique_ptr<ColumnMask>> masks;
};

// the columns of aMap whose centre is within aMap.CarRadius() of one of the outlines (inside it, or
// that close to an edge), under the given key
std::unique_ptr<ColumnMask> BuildMask(const NavMap &aMap, const std::vector<const Boundary *> &aAreas, uint64_t aKey);

} // namespace Navigation
