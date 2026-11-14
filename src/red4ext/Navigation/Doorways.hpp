#pragma once

// The planner's doorway layer (Planner.cpp's Plan searches over it first for long routes).

#include "PlannerCommon.hpp"

#include <memory>
#include <mutex>
#include <queue>
#include <unordered_map>
#include <utility>
#include <vector>
#include <algorithm>
#include <cstring>

namespace Navigation::detail {

// --- doorways: the search in two layers ------------------------------------------------------
// The map in blocks of kBlock x kBlock columns (32 m). A block's doorways are where a car can cross
// its edge into the next block: for each run of side-by-side crossable columns in one height
// layer, the columns in its middle (and at both ends of a long run). The doorways of a block are
// joined by the cheapest way between them inside it, and to their partners across the edge. The
// first search over doorways finds which blocks the route goes through; the plan itself is then
// searched only in those blocks and their neighbours. Doorways come from the 2 m map, so every
// street and alley is still there (a search over coarse cells cannot thread the city's streets).
// A block is worked out the first time a search reaches it and kept for the session: the map does
// not change.
inline constexpr int32_t kBlock = 16;
inline constexpr char kDoorwayTag[8] = {'L', 'T', 'B', 'F', 'D', 'O', 'O', 'R'};

// The cost of climbing aRise metres in one step: aCost a metre, growing exponentially with the
// rise (aScale metres per e-fold; 0 = in proportion). A small rise costs about what it did, a wall
// far more, so routes go round a stadium rather than over it without going round the city.
float ClimbCost(float aRise, float aCost, float aScale);

// the fine search's cost of a step, without the turn and bearing parts, which depend on the way in
struct StepCosts {
  float cell = 2.0f, minOverlap = 0.5f, minPassage = 0.0f, preferredHeight = 25.0f, climbCost = 3.0f, underCost = 0.5f, climbScale = 0.0f;
  bool operator==(const StepCosts &aOther) const {
    return cell == aOther.cell && minOverlap == aOther.minOverlap && minPassage == aOther.minPassage &&
           preferredHeight == aOther.preferredHeight && climbCost == aOther.climbCost && underCost == aOther.underCost &&
           climbScale == aOther.climbScale;
  }
  bool Passable(GapM aFrom, GapM aTo) const {
    float overlap = std::min(aFrom.t, aTo.t) - std::max(aFrom.b, aTo.b);
    return overlap >= minOverlap && overlap >= minPassage;
  }
  float Cost(GapM aFrom, GapM aTo, bool aDiagonal) const {
    float step = aDiagonal ? cell * 1.41421356f : cell;
    return step * (1.0f + (std::isfinite(aTo.t) ? underCost : 0.0f)) +
           ClimbCost(Preferred(aTo, preferredHeight) - Preferred(aFrom, preferredHeight), climbCost, climbScale);
  }
};

inline int32_t BlockOf(int32_t aCell) { return aCell >= 0 ? aCell / kBlock : (aCell - kBlock + 1) / kBlock; }

// Cheapest costs from aSources over the columns x0 <= x < x1, y0 <= y < y1 (aBackward: the costs to
// reach the sources instead, each step counted the way it is flown).
std::unordered_map<uint64_t, float> RegionCosts(NavMap::View &aView, const StepCosts &aCosts, int32_t aWidth, int32_t aX0,
                                                int32_t aY0, int32_t aX1, int32_t aY1,
                                                const std::vector<std::pair<uint64_t, float>> &aSources, bool aBackward);

class Doorways {
public:
  struct Link {
    uint32_t to;
    float cost;
  };
  struct Door {
    int32_t cx, cy;
    uint32_t gap;
    std::vector<Link> links;
  };

  // aMask: the key of the column mask the doorways are worked out under (NavMap::View::MaskKey, 0 for none)
  Doorways(const StepCosts &aCosts, int32_t aWidth, uint64_t aMask = 0) : costs(aCosts), width(aWidth), mask(aMask) {}

  const StepCosts costs;
  const int32_t width;
  const uint64_t mask;
  std::vector<Door> doors;

  // the doorways of block (aBx, aBy), working it out the first time
  const std::vector<uint32_t> &Block(NavMap::View &aView, int32_t aBx, int32_t aBy) {
    int64_t id = (int64_t(aBy) << 32) ^ int64_t(uint32_t(aBx));
    auto it = blocks.find(id);
    if (it != blocks.end()) {
      return it->second;
    }
    std::vector<uint32_t> mine;
    for (int side = 0; side < 4; ++side) {
      Edge(aView, aBx, aBy, side, mine);
    }
    std::sort(mine.begin(), mine.end());
    mine.erase(std::unique(mine.begin(), mine.end()), mine.end());
    // inside the block: from each doorway to the others
    const int32_t x0 = aBx * kBlock, y0 = aBy * kBlock;
    for (uint32_t from : mine) {
      uint64_t fromKey = MakeKey(doors[from].cx, doors[from].cy, doors[from].gap, width);
      auto dist = RegionCosts(aView, costs, width, x0, y0, x0 + kBlock, y0 + kBlock, {{fromKey, 0.0f}}, false);
      for (uint32_t to : mine) {
        if (to == from) {
          continue;
        }
        auto d = dist.find(MakeKey(doors[to].cx, doors[to].cy, doors[to].gap, width));
        if (d != dist.end()) {
          Join(from, to, d->second);
        }
      }
    }
    return blocks[id] = std::move(mine);
  }

  // The whole set as bytes, for a bake: its costs, the doorways and their links, the blocks done.
  std::vector<uint8_t> Save() const {
    std::vector<uint8_t> out;
    auto put = [&](const void *aData, size_t aSize) {
      out.insert(out.end(), static_cast<const uint8_t *>(aData), static_cast<const uint8_t *>(aData) + aSize);
    };
    const uint32_t version = 2;
    put(&version, 4);
    put(&costs, sizeof(costs));
    put(&width, 4);
    put(&mask, 8);
    uint32_t n = uint32_t(doors.size());
    put(&n, 4);
    for (const auto &door : doors) {
      put(&door.cx, 4);
      put(&door.cy, 4);
      uint8_t gap = uint8_t(door.gap);
      put(&gap, 1);
      uint16_t links = uint16_t(door.links.size());
      put(&links, 2);
      for (const auto &link : door.links) {
        put(&link.to, 4);
        put(&link.cost, 4);
      }
    }
    n = uint32_t(blocks.size());
    put(&n, 4);
    for (const auto &[id, mine] : blocks) {
      put(&id, 8);
      uint32_t count = uint32_t(mine.size());
      put(&count, 4);
      put(mine.data(), mine.size() * 4);
    }
    return out;
  }

  // A set saved by Save, if it was made with these costs for a map this wide.
  bool Load(const std::vector<uint8_t> &aData) {
    size_t at = 0;
    bool ok = true;
    auto get = [&](void *aTo, size_t aSize) {
      if (!ok || at + aSize > aData.size()) {
        ok = false;
        return;
      }
      std::memcpy(aTo, aData.data() + at, aSize);
      at += aSize;
    };
    uint32_t version = 0;
    StepCosts saved;
    int32_t savedWidth = 0;
    get(&version, 4);
    get(&saved, sizeof(saved));
    get(&savedWidth, 4);
    // version 1 predates the masks: made with none
    uint64_t savedMask = 0;
    if (version == 2) {
      get(&savedMask, 8);
    }
    if (!ok || (version != 1 && version != 2) || !(saved == costs) || savedWidth != width || savedMask != mask) {
      return false;
    }
    uint32_t n = 0;
    get(&n, 4);
    std::vector<Door> loaded(ok ? n : 0);
    for (auto &door : loaded) {
      uint8_t gap = 0;
      uint16_t links = 0;
      get(&door.cx, 4);
      get(&door.cy, 4);
      get(&gap, 1);
      get(&links, 2);
      door.gap = gap;
      door.links.resize(ok ? links : 0);
      for (auto &link : door.links) {
        get(&link.to, 4);
        get(&link.cost, 4);
      }
    }
    get(&n, 4);
    std::unordered_map<int64_t, std::vector<uint32_t>> loadedBlocks;
    for (uint32_t i = 0; ok && i < n; ++i) {
      int64_t id = 0;
      uint32_t count = 0;
      get(&id, 8);
      get(&count, 4);
      std::vector<uint32_t> mine(ok ? count : 0);
      get(mine.data(), mine.size() * 4);
      loadedBlocks[id] = std::move(mine);
    }
    if (!ok) {
      return false;
    }
    doors = std::move(loaded);
    blocks = std::move(loadedBlocks);
    byKey.clear();
    for (uint32_t i = 0; i < doors.size(); ++i) {
      byKey[MakeKey(doors[i].cx, doors[i].cy, doors[i].gap, width)] = i;
    }
    return true;
  }

  // the doorway at (column, gap), if there is one
  bool Find(uint64_t aKey, uint32_t &aDoor) const {
    auto it = byKey.find(aKey);
    if (it == byKey.end()) {
      return false;
    }
    aDoor = it->second;
    return true;
  }

private:
  std::unordered_map<uint64_t, uint32_t> byKey;
  std::unordered_map<int64_t, std::vector<uint32_t>> blocks;

  uint32_t At(int32_t aCx, int32_t aCy, uint32_t aGap) {
    uint64_t key = MakeKey(aCx, aCy, aGap, width);
    auto it = byKey.find(key);
    if (it != byKey.end()) {
      return it->second;
    }
    doors.push_back({aCx, aCy, aGap, {}});
    return byKey[key] = uint32_t(doors.size() - 1);
  }

  void Join(uint32_t aFrom, uint32_t aTo, float aCost) {
    for (auto &link : doors[aFrom].links) {
      if (link.to == aTo) {
        link.cost = std::min(link.cost, aCost);
        return;
      }
    }
    doors[aFrom].links.push_back({aTo, aCost});
  }

  // The crossings over one edge of the block: side 0 east, 1 north, 2 west, 3 south. Each run of
  // crossings along the edge that stays in one height layer gets doorways at its middle (and at
  // its ends if it is long), on both sides of the edge, joined across.
  void Edge(NavMap::View &aView, int32_t aBx, int32_t aBy, int aSide, std::vector<uint32_t> &aMine) {
    struct Crossing {
      int32_t k;
      uint32_t in, out;
      float lo, hi;
    };
    std::vector<std::vector<Crossing>> runs;
    std::vector<size_t> open; // runs still being extended, by index
    for (int32_t k = 0; k < kBlock; ++k) {
      int32_t ix, iy, ox, oy;
      switch (aSide) {
      case 0:
        ix = aBx * kBlock + kBlock - 1, iy = aBy * kBlock + k, ox = ix + 1, oy = iy;
        break;
      case 1:
        ix = aBx * kBlock + k, iy = aBy * kBlock + kBlock - 1, ox = ix, oy = iy + 1;
        break;
      case 2:
        ix = aBx * kBlock, iy = aBy * kBlock + k, ox = ix - 1, oy = iy;
        break;
      default:
        ix = aBx * kBlock + k, iy = aBy * kBlock, ox = ix, oy = iy - 1;
        break;
      }
      const uint16_t *gi, *go;
      uint32_t ni = aView.Column(ix, iy, gi), no = aView.Column(ox, oy, go);
      std::vector<size_t> next;
      for (uint32_t i = 0; i < ni; ++i) {
        GapM a{aView.Map().VoxelZ(gi[2 * i]), aView.Map().VoxelZ(gi[2 * i + 1])};
        for (uint32_t j = 0; j < no; ++j) {
          GapM b{aView.Map().VoxelZ(go[2 * j]), aView.Map().VoxelZ(go[2 * j + 1])};
          if (!costs.Passable(a, b) || !costs.Passable(b, a)) {
            continue;
          }
          Crossing c{k, i, j, std::max(a.b, b.b), std::min(a.t, b.t)};
          // carried on from a run at the previous column whose height range it overlaps
          size_t run = runs.size();
          for (size_t r : open) {
            const Crossing &last = runs[r].back();
            if (std::min(last.hi, c.hi) - std::max(last.lo, c.lo) >= costs.minOverlap) {
              run = r;
              break;
            }
          }
          if (run == runs.size()) {
            runs.push_back({});
          }
          if (std::find(next.begin(), next.end(), run) == next.end()) {
            runs[run].push_back(c);
            next.push_back(run);
          }
        }
      }
      open = std::move(next);
    }
    auto cell = [&](int32_t aK, bool aOut, int32_t &aX, int32_t &aY) {
      switch (aSide) {
      case 0:
        aX = aBx * kBlock + kBlock - 1 + (aOut ? 1 : 0), aY = aBy * kBlock + aK;
        break;
      case 1:
        aX = aBx * kBlock + aK, aY = aBy * kBlock + kBlock - 1 + (aOut ? 1 : 0);
        break;
      case 2:
        aX = aBx * kBlock - (aOut ? 1 : 0), aY = aBy * kBlock + aK;
        break;
      default:
        aX = aBx * kBlock + aK, aY = aBy * kBlock - (aOut ? 1 : 0);
        break;
      }
    };
    for (const auto &run : runs) {
      // one doorway an opening (with doorways at the ends of long ones as well, a 1 km square of
      // the city had 206 000 links: about 50 MB for the city in a bake)
      std::vector<size_t> picks{run.size() / 2};
      for (size_t p : picks) {
        const Crossing &c = run[p];
        int32_t ix, iy, ox, oy;
        cell(c.k, false, ix, iy);
        cell(c.k, true, ox, oy);
        uint32_t in = At(ix, iy, c.in), out = At(ox, oy, c.out);
        GapM a = GapAt(aView, ix, iy, c.in), b = GapAt(aView, ox, oy, c.out);
        Join(in, out, costs.Cost(a, b, false));
        Join(out, in, costs.Cost(b, a, false));
        aMine.push_back(in);
      }
    }
  }
};

// One set of doorways per map, set of step costs and column mask, kept for the session; plans take turns.
extern std::mutex doorwaysLock;
// The set for these costs under the view's mask; a new one starts from the map's baked doorways when
// they were made with the same costs (the Race's) and mask, else empty, filled in as searches reach blocks.
Doorways &DoorwaysFor(const NavMap::View &aView, const StepCosts &aCosts);

} // namespace Navigation::detail
