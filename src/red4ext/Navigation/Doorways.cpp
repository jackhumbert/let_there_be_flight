#include "Doorways.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <limits>
#include <memory>
#include <mutex>
#include <queue>
#include <spdlog/fmt/fmt.h>
#include <spdlog/spdlog.h>
#include <unordered_map>

namespace Navigation {
namespace detail {

float ClimbCost(float aRise, float aCost, float aScale) {
  if (aRise <= 0.0f) {
    return 0.0f;
  }
  return aScale > 0.0f ? aCost * aScale * (std::exp(std::min(aRise / aScale, 10.0f)) - 1.0f) : aCost * aRise;
}

// Cheapest costs from aSources over the columns x0 <= x < x1, y0 <= y < y1 (aBackward: the costs to
// reach the sources instead, each step counted the way it is flown).
std::unordered_map<uint64_t, float> RegionCosts(NavMap::View &aView, const StepCosts &aCosts, int32_t aWidth, int32_t aX0,
                                                int32_t aY0, int32_t aX1, int32_t aY1,
                                                const std::vector<std::pair<uint64_t, float>> &aSources, bool aBackward) {
  static const int32_t kDx[8] = {1, -1, 0, 0, 1, 1, -1, -1};
  static const int32_t kDy[8] = {0, 0, 1, -1, 1, -1, 1, -1};
  std::unordered_map<uint64_t, float> dist;
  using Entry = std::pair<float, uint64_t>;
  std::priority_queue<Entry, std::vector<Entry>, std::greater<Entry>> open;
  for (auto [key, cost] : aSources) {
    auto it = dist.find(key);
    if (it == dist.end() || cost < it->second) {
      dist[key] = cost;
      open.push({cost, key});
    }
  }
  while (!open.empty()) {
    auto [d, key] = open.top();
    open.pop();
    if (d > dist[key]) {
      continue;
    }
    int32_t cx = KeyCx(key, aWidth), cy = KeyCy(key, aWidth);
    GapM cur = GapAt(aView, cx, cy, KeyGap(key));
    for (int k = 0; k < 8; ++k) {
      int32_t nx = cx + kDx[k], ny = cy + kDy[k];
      if (nx < aX0 || ny < aY0 || nx >= aX1 || ny >= aY1) {
        continue;
      }
      const uint16_t *gaps;
      uint32_t n = aView.Column(nx, ny, gaps);
      for (uint32_t j = 0; j < n; ++j) {
        GapM next{aView.Map().VoxelZ(gaps[2 * j]), aView.Map().VoxelZ(gaps[2 * j + 1])};
        if (!aCosts.Passable(cur, next)) {
          continue;
        }
        float nd = d + (aBackward ? aCosts.Cost(next, cur, k >= 4) : aCosts.Cost(cur, next, k >= 4));
        uint64_t nextKey = MakeKey(nx, ny, j, aWidth);
        auto it = dist.find(nextKey);
        if (it == dist.end() || nd < it->second) {
          dist[nextKey] = nd;
          open.push({nd, nextKey});
        }
      }
    }
  }
  return dist;
}

// One set of doorways per map and set of step costs (the Race and its no-minPassage retry), kept
// for the session; plans running at once take turns.
std::mutex doorwaysLock;
std::vector<std::unique_ptr<Doorways>> doorwaySets;

// The set for these costs; a new one starts from the map's baked doorways when they were made
// with the same costs (the Race's), else empty, filled in as searches reach blocks.
Doorways &DoorwaysFor(const NavMap &aMap, const StepCosts &aCosts) {
  for (auto &set : doorwaySets) {
    if (set->costs == aCosts && set->width == aMap.Width()) {
      return *set;
    }
  }
  doorwaySets.push_back(std::make_unique<Doorways>(aCosts, aMap.Width()));
  std::vector<uint8_t> baked;
  if (aMap.Section(kDoorwayTag, baked) && doorwaySets.back()->Load(baked)) {
    spdlog::info("[NavMap] {} baked doorways", doorwaySets.back()->doors.size());
  }
  return *doorwaySets.back();
}

} // namespace detail

using namespace detail;

DoorwayCount WorkOutDoorways(const NavMap &aMap, const PlanParams &aParams, float aX0, float aY0, float aX1, float aY1) {
  auto t0 = std::chrono::steady_clock::now();
  DoorwayCount count;
  NavMap::View view(aMap);
  StepCosts costs;
  costs.cell = aMap.CellSize();
  costs.minOverlap = aMap.VoxelHeight();
  costs.minPassage = aParams.minPassage;
  costs.preferredHeight = aParams.preferredHeight;
  costs.climbCost = aParams.climbCost;
  costs.climbScale = aParams.climbScale;
  costs.underCost = aParams.underCost;
  std::lock_guard guard(doorwaysLock);
  Doorways &doorways = DoorwaysFor(aMap, costs);
  int32_t cx0, cy0, cx1, cy1;
  aMap.CellOf(aX0, aY0, cx0, cy0);
  aMap.CellOf(aX1, aY1, cx1, cy1);
  for (int32_t by = BlockOf(cy0); by <= BlockOf(cy1); ++by) {
    for (int32_t bx = BlockOf(cx0); bx <= BlockOf(cx1); ++bx) {
      doorways.Block(view, bx, by);
      ++count.blocks;
    }
  }
  count.doors = uint32_t(doorways.doors.size());
  for (const auto &door : doorways.doors) {
    count.links += uint32_t(door.links.size());
  }
  count.milliseconds = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
  count.saved = doorways.Save();
  return count;
}

} // namespace Navigation
