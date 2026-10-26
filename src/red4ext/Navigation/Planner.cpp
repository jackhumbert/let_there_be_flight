#include "Planner.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <limits>
#include <queue>
#include <spdlog/fmt/fmt.h>
#include <unordered_map>

namespace Navigation {

namespace {

constexpr float kInf = std::numeric_limits<float>::infinity();
constexpr float kTopCap = 1000.0f; // finite stand-in for open sky in the height profile

struct GapM {
  float b, t; // metres; t is +infinity for open sky
};

uint64_t MakeKey(int32_t aCx, int32_t aCy, uint32_t aGap, int32_t aWidth) {
  return ((uint64_t(aCy) * uint64_t(aWidth) + uint64_t(aCx)) << 8) | aGap;
}
int32_t KeyCx(uint64_t aKey, int32_t aWidth) { return int32_t((aKey >> 8) % uint64_t(aWidth)); }
int32_t KeyCy(uint64_t aKey, int32_t aWidth) { return int32_t((aKey >> 8) / uint64_t(aWidth)); }
uint32_t KeyGap(uint64_t aKey) { return uint32_t(aKey & 0xFF); }

GapM GapAt(NavMap::View &aView, int32_t aCx, int32_t aCy, uint32_t aGap) {
  const uint16_t *g;
  uint32_t n = aView.Column(aCx, aCy, g);
  if (aGap >= n) {
    return {kInf, -kInf};
  }
  return {aView.Map().VoxelZ(g[2 * aGap]), aView.Map().VoxelZ(g[2 * aGap + 1])};
}

// Height the route would like to fly at in a gap: preferredHeight above the floor, or halfway up
// a gap with something overhead if that is lower.
float Preferred(GapM aGap, float aHeight) {
  float z = aGap.b + aHeight;
  if (std::isfinite(aGap.t)) {
    z = std::min(z, 0.5f * (aGap.b + aGap.t));
  }
  return z;
}

// The gap nearest to p (containing it if possible). If p's column has nothing (the car is inside
// the inflated margin of a wall), the nearest column within a few cells is used.
bool Anchor(NavMap::View &aView, Vec3 aP, int32_t &aCx, int32_t &aCy, uint32_t &aGap) {
  const NavMap &map = aView.Map();
  int32_t c0x, c0y;
  if (!map.CellOf(aP.x, aP.y, c0x, c0y)) {
    return false;
  }
  float best = kInf;
  for (int32_t r = 0; r <= 6 && !std::isfinite(best); ++r) {
    for (int32_t dy = -r; dy <= r; ++dy) {
      for (int32_t dx = -r; dx <= r; ++dx) {
        if (std::max(std::abs(dx), std::abs(dy)) != r) {
          continue;
        }
        const uint16_t *g;
        uint32_t n = aView.Column(c0x + dx, c0y + dy, g);
        for (uint32_t i = 0; i < n; ++i) {
          float b = map.VoxelZ(g[2 * i]), t = map.VoxelZ(g[2 * i + 1]);
          float d = aP.z < b ? b - aP.z : (aP.z >= t ? aP.z - t : 0.0f);
          d += float(r) * map.CellSize();
          if (d < best) {
            best = d;
            aCx = c0x + dx;
            aCy = c0y + dy;
            aGap = i;
          }
        }
      }
    }
  }
  return std::isfinite(best);
}

float Clear(const NavMap &aMap, Vec3 aA, Vec3 aB) { return aMap.SegmentClear(aA.x, aA.y, aA.z, aB.x, aB.y, aB.z); }

// The leg and two parallel legs offset sideways by aSide must all be free.
bool ClearWide(const NavMap &aMap, Vec3 aA, Vec3 aB, float aSide) {
  if (Clear(aMap, aA, aB) < 1.0f) {
    return false;
  }
  float dx = aB.x - aA.x, dy = aB.y - aA.y;
  float len = std::hypot(dx, dy);
  if (aSide <= 0.0f || len < 1e-3f) {
    return true;
  }
  float px = -dy / len * aSide, py = dx / len * aSide;
  for (float s : {1.0f, -1.0f}) {
    Vec3 a{aA.x + s * px, aA.y + s * py, aA.z}, b{aB.x + s * px, aB.y + s * py, aB.z};
    if (Clear(aMap, a, b) < 1.0f) {
      return false;
    }
  }
  return true;
}

float Distance(Vec3 aA, Vec3 aB) {
  float dx = aB.x - aA.x, dy = aB.y - aA.y, dz = aB.z - aA.z;
  return std::sqrt(dx * dx + dy * dy + dz * dz);
}

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
  std::unordered_map<uint64_t, Rec> recs;
  recs.reserve(1 << 20);
  using Entry = std::pair<float, uint64_t>;
  std::priority_queue<Entry, std::vector<Entry>, std::greater<Entry>> open;

  float goalX, goalY;
  aMap.CellCentre(gx, gy, goalX, goalY);
  auto heuristic = [&](int32_t aCx, int32_t aCy) {
    float x, y;
    aMap.CellCentre(aCx, aCy, x, y);
    return aParams.heuristicWeight * std::hypot(x - goalX, y - goalY);
  };

  const uint64_t startKey = MakeKey(sx, sy, sg, width);
  const uint64_t goalKey = MakeKey(gx, gy, gg, width);
  recs[startKey] = {0.0f, startKey};
  open.push({heuristic(sx, sy), startKey});
  static const int32_t kDx[8] = {1, -1, 0, 0, 1, 1, -1, -1};
  static const int32_t kDy[8] = {0, 0, 1, -1, 1, -1, 1, -1};
  const float minOverlap = aMap.VoxelHeight();
  bool found = false;

  while (!open.empty()) {
    auto [f, key] = open.top();
    open.pop();
    int32_t cx = KeyCx(key, width), cy = KeyCy(key, width);
    float g = recs[key].g;
    if (f > g + heuristic(cx, cy) + 1e-3f) {
      continue; // stale entry
    }
    if (key == goalKey) {
      found = true;
      break;
    }
    if (++out.expansions > aParams.maxExpansions) {
      break;
    }
    GapM cur = GapAt(view, cx, cy, KeyGap(key));
    float curPreferred = Preferred(cur, aParams.preferredHeight);
    for (int d = 0; d < 8; ++d) {
      int32_t nx = cx + kDx[d], ny = cy + kDy[d];
      const uint16_t *gaps;
      uint32_t n = view.Column(nx, ny, gaps);
      float step = d < 4 ? cell : cell * 1.41421356f;
      for (uint32_t j = 0; j < n; ++j) {
        GapM next{aMap.VoxelZ(gaps[2 * j]), aMap.VoxelZ(gaps[2 * j + 1])};
        if (std::min(cur.t, next.t) - std::max(cur.b, next.b) < minOverlap) {
          continue;
        }
        float cost = step * (1.0f + (std::isfinite(next.t) ? aParams.underCost : 0.0f)) +
                     aParams.climbCost * std::max(0.0f, Preferred(next, aParams.preferredHeight) - curPreferred);
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
  if (!found) {
    return finish(out.expansions > aParams.maxExpansions ? "search limit reached" : "no route");
  }

  std::vector<uint64_t> path;
  for (uint64_t k = goalKey;; k = recs[k].parent) {
    path.push_back(k);
    if (k == startKey) {
      break;
    }
  }
  std::reverse(path.begin(), path.end());
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
  z[0] = std::clamp(aStart.z, lo[0], hi[0]);
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
  dense.front() = aStart;
  dense.back() = goal;

  // string pulling: from each anchor, the furthest node with a free straight leg. Legs keep
  // legClearance to the sides where that still allows a leg of a useful length; in tight spots
  // the plain test is used, so the route does not turn into a staircase of single cells.
  auto furthest = [&](size_t aFrom, float aSide) {
    size_t good = aFrom + 1, bad = n;
    for (size_t stride = 2;; stride *= 2) {
      size_t j = std::min(n - 1, aFrom + stride);
      if (ClearWide(aMap, dense[aFrom], dense[j], aSide)) {
        good = j;
        if (j == n - 1) {
          break;
        }
      } else {
        bad = j;
        break;
      }
    }
    while (bad < n && bad - good > 1) {
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
  while (a + 1 < n) {
    size_t good = furthest(a, aParams.legClearance);
    if (good < n - 1 && good - a < minWideNodes) {
      good = std::max(good, furthest(a, 0.0f));
    }
    out.points.push_back(dense[good]);
    a = good;
  }
  if (out.points.size() == 1) {
    out.points.push_back(goal); // start and goal share a column
  }
  out.ok = true;
  return finish("route");
}

} // namespace Navigation
