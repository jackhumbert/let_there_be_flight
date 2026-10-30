#include "Planner.hpp"
#include "Doorways.hpp"
#include "PlannerCommon.hpp"

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

using namespace detail;

namespace {

bool TryDirect(NavMap::View &aView, Vec3 aStart, Vec3 aGoal, const PlanParams &aParams, PlanResult &aOut) {
  const NavMap &map = aView.Map();
  float dx = aGoal.x - aStart.x, dy = aGoal.y - aStart.y;
  int steps = std::max(1, int(std::ceil(std::sqrt(dx * dx + dy * dy) / (0.5f * map.CellSize()))));
  float highest = -kInf;
  for (int i = 0; i <= steps; ++i) {
    float t = float(i) / float(steps);
    int32_t cx, cy;
    if (!map.CellOf(aStart.x + dx * t, aStart.y + dy * t, cx, cy)) {
      return false;
    }
    const uint16_t *g;
    uint32_t n = aView.Column(cx, cy, g);
    if (!n) {
      return false; // no data or no reachable space: the straight line is not safe
    }
    highest = std::max(highest, map.VoxelZ(g[2 * (n - 1)])); // floor of the top gap
  }
  float cruise = std::max({highest + aParams.directMargin, aStart.z, aGoal.z});
  if (cruise > aParams.maxCruise) {
    return false;
  }
  std::vector<Vec3> points = {aStart, {aStart.x, aStart.y, cruise}, {aGoal.x, aGoal.y, cruise}, aGoal};
  for (size_t i = 0; i + 1 < points.size(); ++i) {
    if (Clear(map, points[i], points[i + 1]) < 1.0f) {
      return false;
    }
  }
  aOut.direct = true;
  aOut.points = std::move(points);
  return true;
}

} // namespace

PlanResult PlanWithRetries(const NavMap &aMap, Vec3 aStart, Vec3 aGoal, const PlanParams &aParams,
                           const std::function<void(const PlanResult &)> &aAttempt) {
  auto params = aParams;
  auto result = Plan(aMap, aStart, aGoal, params);
  // a long trip at the Race's weight 1.2 ran out of expansions: Northside to the Pacifica coast
  // (5.3 km) failed at 3 million, and the minPassage retry failed the same way; at 1.5 it took 1.1
  // million (2026-10-27)
  for (float weight : {1.5f, 2.0f}) {
    if (result.ok || result.expansions <= params.maxExpansions || params.heuristicWeight >= weight) {
      break;
    }
    if (aAttempt) {
      aAttempt(result);
    }
    params.heuristicWeight = weight;
    result = Plan(aMap, aStart, aGoal, params);
  }
  if (!result.ok && params.minPassage > 0.0f) {
    // no way through without a squeeze point (a start or destination tucked under something):
    // better a tight route than none
    if (aAttempt) {
      aAttempt(result);
    }
    params.minPassage = 0.0f;
    result = Plan(aMap, aStart, aGoal, params);
  }
  if (aAttempt) {
    aAttempt(result);
  }
  return result;
}

PlanResult Plan(const NavMap &aMap, Vec3 aStart, Vec3 aGoal, const PlanParams &aParams) {
  auto t0 = std::chrono::steady_clock::now();
  PlanResult out;
  auto finish = [&](const char *aWhat) {
    out.milliseconds = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
    out.length = 0;
    for (size_t i = 0; i + 1 < out.points.size(); ++i) {
      out.length += Distance(out.points[i], out.points[i + 1]);
    }
    out.info = fmt::format("{}: {} points, {:.0f} m, {} expansions, {} grid nodes, {:.1f} ms", aWhat,
                           out.points.size(), out.length, out.expansions, out.gridNodes, out.milliseconds);
    if (out.doorwayExpansions > 0) {
      out.info += fmt::format(" ({} doorway expansions, {} blocks)", out.doorwayExpansions, out.doorwayBlocks);
    }
    if (out.searchMilliseconds > 0.0) {
      out.info += fmt::format(" (search {:.1f} ms, line {:.1f} ms)", out.searchMilliseconds, out.milliseconds - out.searchMilliseconds);
    }
    return out;
  };

  if (!aMap.IsLoaded()) {
    return finish("no nav map");
  }
  NavMap::View view(aMap);
  const int32_t width = aMap.Width();
  const float cell = aMap.CellSize();

  int32_t sx, sy, gx, gy;
  uint32_t sg, gg;
  if (!Anchor(view, aStart, sx, sy, sg)) {
    return finish("start is not near free space");
  }
  if (!Anchor(view, aGoal, gx, gy, gg)) {
    return finish("goal is not near free space");
  }
  // goal height: inside its gap, as close to the requested height as allowed
  GapM goalGap = GapAt(view, gx, gy, gg);
  Vec3 goal = aGoal;
  goal.z = std::clamp(goal.z, goalGap.b, std::min(goalGap.t, kTopCap) - aMap.VoxelHeight());

  if (aParams.tryDirect && TryDirect(view, aStart, goal, aParams, out)) {
    out.ok = true;
    return finish("direct");
  }

  // A* over (column, gap)
  struct Rec {
    float g;
    uint64_t parent;
  };
  using Entry = std::pair<float, uint64_t>;

  float goalX, goalY;
  aMap.CellCentre(gx, gy, goalX, goalY);
  auto heuristic = [&](int32_t aCx, int32_t aCy) {
    float x, y;
    aMap.CellCentre(aCx, aCy, x, y);
    return aParams.heuristicWeight * std::hypot(x - goalX, y - goalY);
  };
  const uint64_t goalKey = MakeKey(gx, gy, gg, width);
  static const int32_t kDx[8] = {1, -1, 0, 0, 1, 1, -1, -1};
  static const int32_t kDy[8] = {0, 0, 1, -1, 1, -1, 1, -1};
  const float minOverlap = aMap.VoxelHeight();

  // columns a search may use (none: all of them)
  struct Corridor {
    int32_t x0 = 0, y0 = 0, w = 0, h = 0;
    std::vector<uint8_t> cells;
    bool Has(int32_t aX, int32_t aY) const {
      int32_t x = aX - x0, y = aY - y0;
      return x >= 0 && y >= 0 && x < w && y < h && cells[size_t(y) * w + x];
    }
  };

  // The take-off is part of the search: it starts from every free gap within startRadius, each at
  // the cost of getting there from the start (the distance, rises counted three times over, and
  // blocked map along the way five times over), so it finds the way out from under a deck or
  // between buildings itself. One anchor gap could be a pocket the map closed off: from under a
  // structure in Kabuki the search had one expansion and no route (2026-10-27).
  std::vector<std::pair<uint64_t, float>> seeds;
  {
    const int32_t r = int32_t(std::ceil(aParams.startRadius / cell));
    for (int32_t dy = -r; dy <= r; ++dy) {
      for (int32_t dx = -r; dx <= r; ++dx) {
        int32_t cx = sx + dx, cy = sy + dy;
        float x, y;
        aMap.CellCentre(cx, cy, x, y);
        float flat = std::hypot(x - aStart.x, y - aStart.y);
        if (flat > aParams.startRadius) {
          continue;
        }
        const uint16_t *g;
        uint32_t count = view.Column(cx, cy, g);
        for (uint32_t i = 0; i < count; ++i) {
          GapM gap = GapAt(view, cx, cy, i);
          float z = std::clamp(std::max(aStart.z, gap.b + aParams.startLift), gap.b, std::min(gap.t, kTopCap) - aMap.VoxelHeight());
          if (z < gap.b) {
            continue; // no room
          }
          Vec3 at{x, y, z};
          float len = Distance(aStart, at);
          seeds.push_back({MakeKey(cx, cy, i, width), flat + 3.0f * std::max(0.0f, z - aStart.z) + std::max(0.0f, aStart.z - z) +
                                                          5.0f * (1.0f - Clear(aMap, aStart, at)) * len});
        }
      }
    }
    if (seeds.empty()) {
      seeds.push_back({MakeKey(sx, sy, sg, width), 0.0f});
    }
  }

  // One search, only over the columns aAllowed marks (none: anywhere). aPath gets the nodes from a
  // start seed to the goal; aLimit says the search ran out of expansions.
  auto search = [&](const Corridor *aAllowed, std::vector<uint64_t> &aPath, bool &aLimit) -> bool {
    std::unordered_map<uint64_t, Rec> recs;
    recs.reserve(size_t(1) << 20);
    std::priority_queue<Entry, std::vector<Entry>, std::greater<Entry>> open;
    uint32_t expansions = 0;
    aLimit = false;

    for (auto [key, cost] : seeds) {
      auto it = recs.find(key);
      if (it == recs.end() || cost < it->second.g) {
        recs[key] = {cost, key};
        open.push({cost + heuristic(KeyCx(key, width), KeyCy(key, width)), key});
      }
    }

    uint64_t endKey = 0;
    bool found = false;
    while (!open.empty()) {
      auto [f, key] = open.top();
      open.pop();
      int32_t cx = KeyCx(key, width), cy = KeyCy(key, width);
      float g = recs[key].g;
      if (f > g + heuristic(cx, cy) + 1e-3f) {
        continue; // stale entry
      }
      GapM cur = GapAt(view, cx, cy, KeyGap(key));
      if (key == goalKey) {
        endKey = key;
        found = true;
        break;
      }
      if (++expansions > aParams.maxExpansions) {
        aLimit = true;
        break;
      }
      float curPreferred = Preferred(cur, aParams.preferredHeight);
      // for turnCost: the nodes kTurnBase and kTurnBase - 1 steps back along the search's path, so
      // a bend is judged over a few cells (a straight line at 25 degrees is a staircase of 0 and 45
      // degree steps; judged step by step it would turn at every stair)
      int32_t backX = cx, backY = cy, nearX = cx, nearY = cy;
      bool haveBack = false;
      if (aParams.turnCost > 0.0f) {
        uint64_t k = key;
        int steps = 0;
        while (steps < kTurnBase) {
          uint64_t parent = recs[k].parent;
          if (parent == k) {
            break;
          }
          k = parent;
          ++steps;
          if (steps == kTurnBase - 1) {
            nearX = KeyCx(k, width);
            nearY = KeyCy(k, width);
          }
        }
        if (steps == kTurnBase) {
          backX = KeyCx(k, width);
          backY = KeyCy(k, width);
          haveBack = true;
        }
      }
      for (int d = 0; d < 8; ++d) {
        int32_t nx = cx + kDx[d], ny = cy + kDy[d];
        if (aAllowed && !aAllowed->Has(nx, ny)) {
          continue;
        }
        const uint16_t *gaps;
        uint32_t n = view.Column(nx, ny, gaps);
        float step = d < 4 ? cell : cell * 1.41421356f;
        for (uint32_t j = 0; j < n; ++j) {
          GapM next{aMap.VoxelZ(gaps[2 * j]), aMap.VoxelZ(gaps[2 * j + 1])};
          float overlap = std::min(cur.t, next.t) - std::max(cur.b, next.b);
          if (overlap < minOverlap) {
            continue;
          }
          // squeeze points: a passage lower than minPassage is left out (but for reaching the goal)
          if (overlap < aParams.minPassage && MakeKey(nx, ny, j, width) != goalKey) {
            continue;
          }
          float turn = 0.0f;
          if (haveBack) {
            // angle between the last kTurnBase steps and the kTurnBase steps ending at the next cell
            float ax = float(cx - backX), ay = float(cy - backY);
            float bx = float(nx - nearX), by = float(ny - nearY);
            float angle = std::fabs(std::atan2(ax * by - ay * bx, ax * bx + ay * by));
            turn = aParams.turnCost * angle / 0.78539816f;
          }
          // bearing: a step heading away from the goal costs more, (1 - cos) / 2 of bearingCost per
          // metre, so the route keeps going the way a watcher expects it to go
          float bearing = 0.0f;
          if (aParams.bearingCost > 0.0f) {
            float tx = float(gx - cx), ty = float(gy - cy);
            float tl = std::hypot(tx, ty);
            if (tl > 0.5f) {
              float cosine = (float(kDx[d]) * tx + float(kDy[d]) * ty) / (tl * std::hypot(float(kDx[d]), float(kDy[d])));
              bearing = aParams.bearingCost * step * 0.5f * (1.0f - cosine);
            }
          }
          float cost = turn + bearing + step * (1.0f + (std::isfinite(next.t) ? aParams.underCost : 0.0f)) +
                       ClimbCost(Preferred(next, aParams.preferredHeight) - curPreferred, aParams.climbCost, aParams.climbScale);
          uint64_t nextKey = MakeKey(nx, ny, j, width);
          float ng = g + cost;
          auto it = recs.find(nextKey);
          if (it == recs.end() || ng < it->second.g) {
            recs[nextKey] = {ng, key};
            open.push({ng + heuristic(nx, ny), nextKey});
          }
        }
      }
    }
    out.expansions += expansions;
    if (!found) {
      return false;
    }
    aPath.clear();
    for (uint64_t k = endKey;; k = recs[k].parent) {
      aPath.push_back(k);
      if (recs[k].parent == k) {
        break; // a start seed
      }
    }
    std::reverse(aPath.begin(), aPath.end());
    return true;
  };

  // The doorways first (a long plan): which blocks the route goes through, then the plan itself
  // only in those blocks and the ones round them (the whole map if that finds no way).
  std::vector<uint64_t> path;
  bool limit = false;
  bool found = false;
  if (aParams.doorways && Distance(aStart, goal) > aParams.doorwaysFrom) {
    std::lock_guard guard(doorwaysLock);
    StepCosts costs;
    costs.cell = cell;
    costs.minOverlap = minOverlap;
    costs.minPassage = aParams.minPassage;
    costs.preferredHeight = aParams.preferredHeight;
    costs.climbCost = aParams.climbCost;
  costs.climbScale = aParams.climbScale;
    costs.underCost = aParams.underCost;
    Doorways &doorways = DoorwaysFor(aMap, costs);
    // from the start seeds to the doorways of the blocks they are in, and from the doorways of the
    // goal's block and the ones round it to the goal
    int32_t sx0 = width, sy0 = aMap.Height(), sx1 = -1, sy1 = -1;
    for (auto [key, cost] : seeds) {
      sx0 = std::min(sx0, KeyCx(key, width));
      sy0 = std::min(sy0, KeyCy(key, width));
      sx1 = std::max(sx1, KeyCx(key, width));
      sy1 = std::max(sy1, KeyCy(key, width));
    }
    const int32_t bsx0 = BlockOf(sx0), bsy0 = BlockOf(sy0), bsx1 = BlockOf(sx1), bsy1 = BlockOf(sy1);
    auto fromStart = RegionCosts(view, costs, width, bsx0 * kBlock, bsy0 * kBlock, (bsx1 + 1) * kBlock, (bsy1 + 1) * kBlock, seeds, false);
    const int32_t gbx = BlockOf(gx), gby = BlockOf(gy);
    auto toGoal = RegionCosts(view, costs, width, (gbx - 1) * kBlock, (gby - 1) * kBlock, (gbx + 2) * kBlock, (gby + 2) * kBlock,
                              {{goalKey, 0.0f}}, true);

    // A* over doorways; a start doorway is its own parent
    constexpr uint32_t kNone = std::numeric_limits<uint32_t>::max();
    std::unordered_map<uint32_t, Rec> doorRecs;
    using DoorEntry = std::pair<float, uint32_t>;
    std::priority_queue<DoorEntry, std::vector<DoorEntry>, std::greater<DoorEntry>> doorOpen;
    auto doorHeuristic = [&](uint32_t aDoor) { return heuristic(doorways.doors[aDoor].cx, doorways.doors[aDoor].cy); };
    for (int32_t by = bsy0; by <= bsy1; ++by) {
      for (int32_t bx = bsx0; bx <= bsx1; ++bx) {
        const std::vector<uint32_t> blockDoors = doorways.Block(view, bx, by);
        for (uint32_t door : blockDoors) {
          const auto &d = doorways.doors[door];
          auto it = fromStart.find(MakeKey(d.cx, d.cy, d.gap, width));
          if (it != fromStart.end()) {
            doorRecs[door] = {it->second, uint64_t(door)};
            doorOpen.push({it->second + doorHeuristic(door), door});
          }
        }
      }
    }
    // the goal's doorways: their cost onwards to the goal
    std::unordered_map<uint32_t, float> goalDoors;
    for (int32_t by = gby - 1; by <= gby + 1; ++by) {
      for (int32_t bx = gbx - 1; bx <= gbx + 1; ++bx) {
        const std::vector<uint32_t> blockDoors = doorways.Block(view, bx, by);
        for (uint32_t door : blockDoors) {
          const auto &d = doorways.doors[door];
          auto it = toGoal.find(MakeKey(d.cx, d.cy, d.gap, width));
          if (it != toGoal.end()) {
            goalDoors[door] = it->second;
          }
        }
      }
    }
    float best = kInf;
    uint32_t bestDoor = kNone;
    while (!doorOpen.empty()) {
      auto [f, door] = doorOpen.top();
      doorOpen.pop();
      float g = doorRecs[door].g;
      if (f > g + doorHeuristic(door) + 1e-3f) {
        continue; // stale entry
      }
      if (f >= best) {
        break; // nothing left beats the way to the goal already found
      }
      if (++out.doorwayExpansions > aParams.maxExpansions) {
        break;
      }
      auto onward = goalDoors.find(door);
      if (onward != goalDoors.end() && g + onward->second < best) {
        best = g + onward->second;
        bestDoor = door;
      }
      // its links inside its own block (Block can add doorways: no references kept across it)
      doorways.Block(view, BlockOf(doorways.doors[door].cx), BlockOf(doorways.doors[door].cy));
      const auto links = doorways.doors[door].links;
      for (auto [to, cost] : links) {
        float ng = g + cost;
        auto it = doorRecs.find(to);
        if (it == doorRecs.end() || ng < it->second.g) {
          doorRecs[to] = {ng, uint64_t(door)};
          doorOpen.push({ng + doorHeuristic(to), to});
        }
      }
    }
    if (bestDoor != kNone) {
      // the blocks of the doorways on the way, with the start's and the goal's, and doorwayMargin
      // blocks round each
      std::vector<std::pair<int32_t, int32_t>> through;
      for (uint32_t door = bestDoor;; door = uint32_t(doorRecs[door].parent)) {
        through.push_back({BlockOf(doorways.doors[door].cx), BlockOf(doorways.doors[door].cy)});
        if (doorRecs[door].parent == door) {
          break;
        }
      }
      for (int32_t by = bsy0; by <= bsy1; ++by) {
        for (int32_t bx = bsx0; bx <= bsx1; ++bx) {
          through.push_back({bx, by});
        }
      }
      through.push_back({gbx, gby});
      int32_t bx0 = through[0].first, bx1 = bx0, by0 = through[0].second, by1 = by0;
      for (auto [bx, by] : through) {
        bx0 = std::min(bx0, bx);
        bx1 = std::max(bx1, bx);
        by0 = std::min(by0, by);
        by1 = std::max(by1, by);
      }
      const int32_t margin = aParams.doorwayMargin;
      Corridor corridor;
      corridor.x0 = (bx0 - margin) * kBlock;
      corridor.y0 = (by0 - margin) * kBlock;
      corridor.w = (bx1 - bx0 + 1 + 2 * margin) * kBlock;
      corridor.h = (by1 - by0 + 1 + 2 * margin) * kBlock;
      corridor.cells.assign(size_t(corridor.w) * corridor.h, 0);
      for (auto [bx, by] : through) {
        for (int32_t y = (by - margin) * kBlock; y < (by + margin + 1) * kBlock; ++y) {
          uint8_t *row = &corridor.cells[size_t(y - corridor.y0) * corridor.w];
          std::fill(row + ((bx - margin) * kBlock - corridor.x0), row + ((bx + margin + 1) * kBlock - corridor.x0), uint8_t(1));
        }
      }
      out.doorwayBlocks = uint32_t(through.size());
      found = search(&corridor, path, limit);
    }
  }
  if (!found) {
    found = search(nullptr, path, limit);
  }

  if (!found) {
    return finish(limit ? "search limit reached" : "no route");
  }
  const size_t n = path.size();
  out.gridNodes = uint32_t(n);

  // height profile
  std::vector<GapM> gaps(n);
  std::vector<float> xs(n), ys(n), lo(n), hi(n), z(n), seg(n, 0.0f);
  for (size_t i = 0; i < n; ++i) {
    int32_t cx = KeyCx(path[i], width), cy = KeyCy(path[i], width);
    gaps[i] = GapAt(view, cx, cy, KeyGap(path[i]));
    aMap.CellCentre(cx, cy, xs[i], ys[i]);
  }
  for (size_t i = 0; i < n; ++i) {
    // stay inside this gap and the gaps it is joined to, so each step between columns is free
    float l = gaps[i].b, h = std::min(gaps[i].t, kTopCap);
    for (size_t j : {i - 1, i + 1}) {
      if (j < n) {
        l = std::max(l, gaps[j].b);
        h = std::min(h, std::min(gaps[j].t, kTopCap));
      }
    }
    if (h - l < aMap.VoxelHeight()) {
      l = gaps[i].b;
      h = std::min(gaps[i].t, kTopCap);
    }
    lo[i] = l;
    hi[i] = h - aMap.VoxelHeight();
    z[i] = std::clamp(Preferred(gaps[i], aParams.preferredHeight), lo[i], hi[i]);
    if (i + 1 < n) {
      seg[i] = std::hypot(xs[i + 1] - xs[i], ys[i + 1] - ys[i]);
    }
  }
  z[0] = std::clamp(std::max(aStart.z, lo[0] + aParams.startLift), lo[0], hi[0]);
  z[n - 1] = goal.z;
  for (size_t i = n - 1; i-- > 0;) { // climb early before obstacles
    z[i] = std::min(std::max(z[i], z[i + 1] - aParams.maxGradient * seg[i]), hi[i]);
  }
  for (size_t i = 1; i < n; ++i) { // descend gradually after them
    z[i] = std::min(std::max(z[i], z[i - 1] - aParams.maxGradient * seg[i - 1]), hi[i]);
  }

  std::vector<Vec3> dense(n);
  for (size_t i = 0; i < n; ++i) {
    dense[i] = {xs[i], ys[i], z[i]};
  }
  dense.back() = goal;
  // the route begins at the start itself, then the gap the search left from (the take-off)
  float startLeg = 0.0f;
  if (Distance(aStart, dense.front()) > 1.0f) {
    startLeg = Distance(aStart, dense.front());
    dense.insert(dense.begin(), aStart);
  } else {
    dense.front() = aStart;
  }
  const size_t nd = dense.size();

  // string pulling: from each anchor, the furthest node with a free straight leg. Legs keep
  // legClearance to the sides where that still allows a leg of a useful length; in tight spots
  // the plain test is used, so the route does not turn into a staircase of single cells.
  auto furthest = [&](size_t aFrom, float aSide) {
    size_t good = aFrom + 1, bad = nd;
    for (size_t stride = 2;; stride *= 2) {
      size_t j = std::min(nd - 1, aFrom + stride);
      if (ClearWide(aMap, dense[aFrom], dense[j], aSide)) {
        good = j;
        if (j == nd - 1) {
          break;
        }
      } else {
        bad = j;
        break;
      }
    }
    while (bad < nd && bad - good > 1) {
      size_t mid = (good + bad) / 2;
      if (ClearWide(aMap, dense[aFrom], dense[mid], aSide)) {
        good = mid;
      } else {
        bad = mid;
      }
    }
    return good;
  };
  const size_t minWideNodes = size_t(std::ceil(20.0f / cell));
  out.points.push_back(dense.front());
  size_t a = 0;
  while (a + 1 < nd) {
    size_t good = furthest(a, aParams.legClearance);
    if (good < nd - 1 && good - a < minWideNodes) {
      good = std::max(good, furthest(a, 0.0f));
    }
    out.points.push_back(dense[good]);
    a = good;
  }
  if (out.points.size() == 1) {
    out.points.push_back(goal); // start and goal share a column
  }
  out.ok = true;
  out.searchMilliseconds = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
  if (aParams.racingLine && out.points.size() > 2) {
    // kept only if the finished line is free all the way; otherwise the pulled route stays
    auto line = RacingLine(aMap, out.points, aParams);
    // (the stretch out from under cover to the free space is blocked on the map: not counted)
    if (LongestBlocked(aMap, line, startLeg + 5.0f) <= kGrazeAllowedLine) {
      out.points = std::move(line);
      return finish("racing line");
    }
    size_t blocked = 0;
    while (blocked + 1 < line.size() && LegFree(aMap, line[blocked], line[blocked + 1])) {
      ++blocked;
    }
    out.rejectedLine = line;
    return finish(fmt::format("route (racing line blocked at leg {} of {}, {:.1f} {:.1f} {:.1f})", blocked, line.size() - 1,
                              line[blocked].x, line[blocked].y, line[blocked].z)
                      .c_str());
  }
  return finish("route");
}

void RacePreset(PlanParams &aParams) {
  aParams.racingLine = true;
  aParams.preferredHeight = 40.0f;
  aParams.climbCost = 0.5f;
  aParams.minPassage = 12.0f;
  aParams.heuristicWeight = 1.2f;
  aParams.turnCost = 5.0f;
  // a climb's cost grows exponentially with its rise (ClimbCost): round the Dogtown stadium, not over it
  aParams.climbScale = 30.0f;
}

} // namespace Navigation
