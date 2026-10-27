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

constexpr int kTurnBase = 4; // cells a bend is judged over (turnCost)

float Distance(Vec3 aA, Vec3 aB) {
  float dx = aB.x - aA.x, dy = aB.y - aA.y, dz = aB.z - aA.z;
  return std::sqrt(dx * dx + dy * dy + dz * dz);
}

// Longest stretch (m) of the polyline that is blocked, sampled every half metre along its whole
// length (so where the legs are cut does not change the answer).
constexpr float kGrazeStep = 0.5f;
constexpr float kGrazeAllowed = 1.0f; // m: the planner's own legs graze this much between their samples
// m, the finished line: moves are checked at SegmentClear's 1 m sampling, the line at half a metre,
// which finds a little more; still half the 3 m the bake grew solids by for the car
constexpr float kGrazeAllowedLine = 1.5f;

// The longest run of blocked space along the polyline, from aSkip metres on (the stretch from a
// car under cover out to the free space is blocked on the map and is flown anyway).
float LongestBlocked(const NavMap &aMap, const std::vector<Vec3> &aPoints, float aSkip = 0.0f) {
  float run = 0.0f, longest = 0.0f, carry = 0.0f, along = 0.0f;
  for (size_t i = 0; i + 1 < aPoints.size(); ++i) {
    Vec3 a = aPoints[i], b = aPoints[i + 1];
    float len = Distance(a, b);
    for (float s = carry; s <= len; s += kGrazeStep) {
      float t = len > 1e-4f ? s / len : 0.0f;
      bool free = along + s < aSkip || aMap.IsFree(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t);
      run = free ? 0.0f : run + kGrazeStep;
      longest = std::max(longest, run);
      carry = s + kGrazeStep - len;
    }
    along += len;
  }
  return longest;
}

// The leg is free but for a graze no longer than the planner's own legs have.
bool LegFree(const NavMap &aMap, Vec3 aA, Vec3 aB) {
  return Clear(aMap, aA, aB) >= 1.0f || LongestBlocked(aMap, {aA, aB}) <= kGrazeAllowed;
}

// How much of the leg is free, the leg and its two side-offset copies taken together (0..1).
float ClearSides(const NavMap &aMap, Vec3 aA, Vec3 aB, float aSide) {
  float clear = Clear(aMap, aA, aB);
  float dx = aB.x - aA.x, dy = aB.y - aA.y;
  float len = std::hypot(dx, dy);
  if (aSide <= 0.0f || len < 1e-3f || clear < 1.0f) {
    return clear;
  }
  float px = -dy / len * aSide, py = dx / len * aSide;
  for (float s : {1.0f, -1.0f}) {
    clear = std::min(clear, Clear(aMap, {aA.x + s * px, aA.y + s * py, aA.z}, {aB.x + s * px, aB.y + s * py, aB.z}));
  }
  return clear;
}

// Each leg of the polyline cut into equal steps of at most aSpacing, its corners kept: every new
// leg lies on an old one, so a free polyline stays free.
std::vector<Vec3> Resample(const std::vector<Vec3> &aPoints, float aSpacing) {
  std::vector<Vec3> out{aPoints.front()};
  for (size_t i = 0; i + 1 < aPoints.size(); ++i) {
    Vec3 a = aPoints[i], b = aPoints[i + 1];
    int steps = std::max(1, int(std::ceil(Distance(a, b) / aSpacing)));
    for (int k = 1; k <= steps; ++k) {
      float t = float(k) / float(steps);
      out.push_back({a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t});
    }
  }
  return out;
}

// Racing line: gradient descent on the summed squared second differences of the points (their
// bend), each move kept only where both legs to the neighbours stay free, with lineSide to spare
// or at least no less than before. Points ahead of a corner move out and the apex moves
// in, so the bend spreads over all the room there is. Coarse spacings first (the bend of a long
// corner spreads over a few points per pass there), then finer ones down to lineSpacing.
std::vector<Vec3> RacingLine(const NavMap &aMap, const std::vector<Vec3> &aRoute, const PlanParams &aParams) {
  std::vector<Vec3> pts = aRoute;
  const float side = aParams.lineSide;
  for (auto [spacing, passes] : {std::pair{40.0f, 300}, {20.0f, 300}, {10.0f, 200}, {aParams.lineSpacing, 100}}) {
    pts = Resample(pts, spacing);
    const size_t n = pts.size();
    if (n < 7) {
      continue;
    }
    // route distance from the start, for the floor clearance's exemption at the ends
    std::vector<float> along(n, 0.0f);
    for (size_t i = 1; i < n; ++i) {
      along[i] = along[i - 1] + Distance(pts[i - 1], pts[i]);
    }
    for (int pass = 0; pass < passes; ++pass) {
      float moved = 0.0f;
      for (size_t i = 2; i + 2 < n; ++i) {
        const Vec3 &a = pts[i - 2], &b = pts[i - 1], &p = pts[i], &c = pts[i + 1], &d = pts[i + 2];
        // d/dp of |b - 2p + c|^2 + |a - 2b + p|^2 + |p - 2c + d|^2, halved
        Vec3 g{a.x - 4 * b.x + 6 * p.x - 4 * c.x + d.x, a.y - 4 * b.y + 6 * p.y - 4 * c.y + d.y,
               (a.z - 4 * b.z + 6 * p.z - 4 * c.z + d.z) * aParams.lineVerticalWeight};
        // away from blocked space close across the line: probe lineClearance to either side and
        // above / below; where one side is blocked and the other free, push towards the free one.
        // (Smoothing alone leaves the line skimming the map's margin wherever a wall is on the
        // inside of a bend, and the car's tracking error then reaches the wall, 2026-10-26)
        {
          float tx = c.x - b.x, ty = c.y - b.y;
          float tl = std::hypot(tx, ty);
          if (tl > 1e-3f) {
            const float r = aParams.lineClearance;
            const Vec3 dirs[2] = {{-ty / tl, tx / tl, 0.0f}, {0.0f, 0.0f, 1.0f}};
            for (const Vec3 &n : dirs) {
              bool plus = aMap.IsFree(p.x + n.x * r, p.y + n.y * r, p.z + n.z * r);
              bool minus = aMap.IsFree(p.x - n.x * r, p.y - n.y * r, p.z - n.z * r);
              if (plus != minus) {
                float k = (plus ? 1.0f : -1.0f) * aParams.linePush / aParams.lineStep; // undone by * step below
                g.x -= n.x * k;
                g.y -= n.y * k;
                g.z -= n.z * k;
              }
            }
          }
        }
        bool low = false; // under the floor clearance: lifted even where the line is straight
        if (along[i] > aParams.lineFloorEnds && along.back() - along[i] > aParams.lineFloorEnds) {
          Gap here;
          low = aMap.FindGap(p.x, p.y, p.z, here) && p.z < std::min(here.bottom + aParams.lineFloorClearance, here.top - 1.0f) - 0.01f;
        }
        if (!low && g.x * g.x + g.y * g.y + g.z * g.z < 1e-4f) {
          continue;
        }
        float before = -1.0f; // only needed when the move is not free with room to the sides
        for (float step = aParams.lineStep; step > aParams.lineStep * 0.2f; step *= 0.5f) {
          Vec3 q{p.x - g.x * step, p.y - g.y * step, p.z - g.z * step};
          // floor clearance: up to lineFloorClearance over the floor below, where the gap has room
          Gap gap;
          if (along[i] > aParams.lineFloorEnds && along.back() - along[i] > aParams.lineFloorEnds &&
              aMap.FindGap(q.x, q.y, q.z, gap)) {
            q.z = std::max(q.z, std::min(gap.bottom + aParams.lineFloorClearance, gap.top - 1.0f));
          }
          float after = std::min(ClearSides(aMap, b, q, side), ClearSides(aMap, q, c, side));
          bool ok = after >= 1.0f;
          // (grazes judged over the two legs either side together: one per leg can add up)
          if (!ok && LongestBlocked(aMap, {a, b, q, c, d}) <= kGrazeAllowed) {
            // free, but closer to the sides than lineSide: fine where the line already was
            if (before < 0.0f) {
              before = std::min(ClearSides(aMap, b, p, side), ClearSides(aMap, p, c, side));
            }
            ok = after >= before;
          }
          if (ok) {
            moved += Distance(p, q);
            pts[i] = q;
            break;
          }
        }
      }
      if (moved < 0.0005f * float(n)) {
        break; // settled: under half a millimetre per point
      }
    }
  }
  return pts;
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

  // One search, only over the columns aAllowed marks (none: anywhere). aPath gets the nodes from a
  // start seed to the goal; aLimit says the search ran out of expansions.
  auto search = [&](const Corridor *aAllowed, std::vector<uint64_t> &aPath, bool &aLimit) -> bool {
    std::unordered_map<uint64_t, Rec> recs;
    recs.reserve(size_t(1) << 20);
    std::priority_queue<Entry, std::vector<Entry>, std::greater<Entry>> open;
    uint32_t expansions = 0;
    aLimit = false;

    // The take-off is part of the search: it starts from every free gap within startRadius, each at
    // the cost of getting there from the start (the distance, rises counted three times over, and
    // blocked map along the way five times over), so it finds the way out from under a deck or
    // between buildings itself. One anchor gap could be a pocket the map closed off: from under a
    // structure in Kabuki the search had one expansion and no route (2026-10-27).
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
          float cost = flat + 3.0f * std::max(0.0f, z - aStart.z) + std::max(0.0f, aStart.z - z) +
                       5.0f * (1.0f - Clear(aMap, aStart, at)) * len;
          uint64_t key = MakeKey(cx, cy, i, width);
          auto it = recs.find(key);
          if (it == recs.end() || cost < it->second.g) {
            recs[key] = {cost, key};
            open.push({cost + heuristic(cx, cy), key});
          }
        }
      }
    }
    if (recs.empty()) {
      const uint64_t anchorKey = MakeKey(sx, sy, sg, width);
      recs[anchorKey] = {0.0f, anchorKey};
      open.push({heuristic(sx, sy), anchorKey});
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

  std::vector<uint64_t> path;
  bool limit = false;
  bool found = search(nullptr, path, limit);

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

} // namespace Navigation
