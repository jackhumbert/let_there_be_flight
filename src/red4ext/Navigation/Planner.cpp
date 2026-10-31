#include "Planner.hpp"
#include "Trajectory.hpp"
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
  // a long trip at the Race's weight 1.2 can run out of expansions where 1.5 finds it
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

namespace {
// The line's slots too tight for the car's speed: the worst sample of each stretch whose room
// costs more than the params allow, as places to route round.
std::vector<Avoid> Slots(const Trajectory &aTrajectory, const PlanParams &aParams, float &aWorst) {
  std::vector<Avoid> out;
  aWorst = 0.0f;
  const auto &q = aTrajectory.samples;
  for (size_t i = 0; i < q.size();) {
    auto over = [&](size_t k) {
      const float loss = q[k].roomLoss, free = q[k].speed + loss;
      return loss >= aParams.slotDetourLossMin && (loss > aParams.slotDetourLoss || loss > aParams.slotDetourShare * free);
    };
    if (!over(i)) {
      ++i;
      continue;
    }
    size_t worst = i;
    for (; i < q.size() && over(i); ++i) {
      worst = q[i].roomLoss > q[worst].roomLoss ? i : worst;
    }
    aWorst = std::max(aWorst, q[worst].roomLoss);
    out.push_back({q[worst].position, aParams.slotAvoidRadius, aParams.slotAvoidCost});
  }
  return out;
}
} // namespace

PlanResult PlanRace(const NavMap &aMap, Vec3 aStart, Vec3 aGoal, const PlanParams &aParams,
                    const std::function<void(const PlanResult &)> &aAttempt) {
  auto result = PlanWithRetries(aMap, aStart, aGoal, aParams, aAttempt);
  if (!result.ok || !aParams.line || result.points.size() < 3) {
    return result;
  }
  const auto first = BuildTrajectory(aParams.vehicle, result.points, {}, &aMap);
  float worst = 0.0f;
  auto slots = Slots(first, aParams, worst);
  if (slots.empty()) {
    return result;
  }
  // round the slots, and round any the new line finds next to them (a slot under an overhang moved
  // up is often another one over it), at most three times
  auto params = aParams;
  const size_t found = slots.size();
  char text[320];
  std::string why = "no way round";
  double extra = 0.0;
  for (int32_t round = 0; round < 3 && !slots.empty(); ++round) {
    params.avoid.insert(params.avoid.end(), slots.begin(), slots.end());
    auto detour = PlanWithRetries(aMap, aStart, aGoal, params);
    extra += detour.milliseconds;
    if (!detour.ok || detour.points.size() < 3) {
      why = "no way round";
      break;
    }
    const auto second = BuildTrajectory(aParams.vehicle, detour.points, {}, &aMap);
    float worst2 = 0.0f;
    slots = Slots(second, aParams, worst2);
    if (second.Duration() > 1.2f * first.Duration()) {
      std::snprintf(text, sizeof(text), "round them %.1f s", second.Duration());
      why = text;
      break;
    }
    // the detour's own turns can cost what the slot did: kept only if it loses less speed in dips
    const float lost1 = DipLoss(first), lost2 = DipLoss(second);
    if (slots.empty() && lost2 >= lost1) {
      std::snprintf(text, sizeof(text), "round them %.1f s but %.0f m/s lost in dips against %.0f", second.Duration(), lost2, lost1);
      why = text;
      break;
    }
    if (slots.empty()) {
      std::snprintf(text, sizeof(text),
                    " (round %zu slot(s) costing up to %.0f m/s in %d search(es): %.1f s against %.1f, %.0f m/s lost in dips against %.0f)",
                    found, worst, round + 1, second.Duration(), first.Duration(), lost2, lost1);
      detour.info += text;
      detour.milliseconds += result.milliseconds + extra - detour.milliseconds;
      return detour;
    }
    std::snprintf(text, sizeof(text), "%zu left at %.0f %.0f %.0f", slots.size(), slots[0].at.x, slots[0].at.y, slots[0].at.z);
    why = text;
  }
  std::snprintf(text, sizeof(text), " (%zu slot(s) costing up to %.0f m/s flown slower, first at %.0f %.0f %.0f: %s)", found,
                worst, first.samples.empty() ? 0.0f : params.avoid[aParams.avoid.size()].at.x,
                params.avoid[aParams.avoid.size()].at.y, params.avoid[aParams.avoid.size()].at.z, why.c_str());
  result.info += text;
  result.milliseconds += extra;
  return result;
}

namespace {
// One rejoin: the connector to the line aJoinAhead metres on (PlanRejoin tries a few).
PlanResult RejoinAt(const NavMap &aMap, Vec3 aCar, const std::vector<Vec3> &aLine, float aJoinAhead,
                    const PlanParams &aParams) {
  auto t0 = std::chrono::steady_clock::now();
  PlanResult out;
  if (aLine.size() < 3) {
    out.info = "rejoin: no line ahead";
    return out;
  }
  // the join: aJoinAhead along the line from its point nearest the car; the lead-out: the line's
  // next lineFloorEnds after it, so the connector ends going the way the line goes
  size_t nearest = 0;
  for (size_t i = 1; i < aLine.size(); ++i) {
    nearest = Distance(aCar, aLine[i]) < Distance(aCar, aLine[nearest]) ? i : nearest;
  }
  size_t join = nearest;
  for (float d = 0.0f; join + 1 < aLine.size() && d < aJoinAhead; ++join) {
    d += Distance(aLine[join], aLine[join + 1]);
  }
  size_t leadOut = join;
  for (float d = 0.0f; leadOut + 1 < aLine.size() && d < aParams.lineFloorEnds; ++leadOut) {
    d += Distance(aLine[leadOut], aLine[leadOut + 1]);
  }
  if (leadOut <= join + 1) {
    out.info = "rejoin: the line ends too soon after the car";
    return out;
  }
  auto params = aParams;
  params.line = false; // the connector's route first, its line below with the lead-out on it
  params.doorways = false;
  // straight when the air allows: the lead-in along the car's velocity, then straight to the join
  // (knocked off a line, the car is mostly in open air); otherwise searched at the car's own height
  // over the floor, not the Race's (the search climbed 50 m and the car braked for it)
  PlanResult route;
  {
    const Vec3 v = aParams.startVelocity;
    const float speed = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    Vec3 ahead = aCar;
    if (speed > 5.0f) {
      const float lead = std::clamp(speed * aParams.startLead, 15.0f, 80.0f) / speed;
      ahead = {aCar.x + v.x * lead, aCar.y + v.y * lead, aCar.z + v.z * lead};
    }
    const Vec3 &to = aLine[join];
    const bool leadClear = ClearWide(aMap, aCar, ahead, aParams.lineSide), toClear = ClearWide(aMap, ahead, to, aParams.lineSide);
    out.info = fmt::format(" (lead-in {}, straight on {})", leadClear ? "clear" : "blocked", toClear ? "clear" : "blocked");
    if (leadClear && toClear) {
      route.ok = true;
      route.points = {aCar, ahead, to};
      route.length = Distance(aCar, ahead) + Distance(ahead, to);
    } else {
      Gap gap;
      if (aMap.FindGap(aCar.x, aCar.y, aCar.z, gap)) {
        params.preferredHeight = std::max(aCar.z - gap.bottom, 5.0f);
      }
      route = Plan(aMap, aCar, to, params);
    }
  }
  if (!route.ok) {
    out.info = "rejoin: " + route.info;
    return out;
  }
  std::vector<Vec3> pts = route.points;
  pts.insert(pts.end(), aLine.begin() + join + 1, aLine.begin() + leadOut + 1);
  params.line = true;
  params.lineTakeoff = false;
  params.lineLanding = false;
  out.points = RepairLine(aMap, Line(aMap, pts, params), pts, params, 0.0f, out.repaired);
  out.points.insert(out.points.end(), aLine.begin() + leadOut + 1, aLine.end());
  out.ok = true;
  out.expansions = route.expansions;
  out.milliseconds = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
  for (size_t i = 0; i + 1 < out.points.size(); ++i) {
    out.length += Distance(out.points[i], out.points[i + 1]);
  }
  out.info = fmt::format("rejoin: {:.0f} m {} to the line {:.0f} m on, {} points in all, {:.1f} ms{}", route.length,
                         route.points.size() == 3 && route.expansions == 0 ? "straight" : "searched", aJoinAhead, out.points.size(),
                         out.milliseconds, out.info);
  return out;
}
} // namespace

PlanResult PlanRejoin(const NavMap &aMap, Vec3 aCar, const std::vector<Vec3> &aLine, float aJoinAhead,
                      const PlanParams &aParams) {
  // a few joins, nearer and further, each scored by the time its trajectory takes to the end (from
  // the car's speed) plus a second per 10 m/s it loses in dips; the best kept
  auto t0 = std::chrono::steady_clock::now();
  const Vec3 v = aParams.startVelocity;
  TrajectoryParams tp;
  tp.startSpeed = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
  PlanResult best;
  float bestScore = 1e30f, bestAhead = 0.0f;
  std::string tried;
  for (float share : {0.7f, 1.0f, 1.5f}) {
    auto r = RejoinAt(aMap, aCar, aLine, aJoinAhead * share, aParams);
    if (!r.ok) {
      continue;
    }
    const auto trajectory = BuildTrajectory(aParams.vehicle, r.points, tp, &aMap);
    const float score = trajectory.Duration() + 0.1f * DipLoss(trajectory);
    tried += fmt::format("{}{:.0f} m {:.1f}", tried.empty() ? "" : ", ", aJoinAhead * share, score);
    if (score < bestScore) {
      bestScore = score;
      bestAhead = aJoinAhead * share;
      best = std::move(r);
    }
  }
  if (!best.ok) {
    best.info = "rejoin: no way back onto the line";
    return best;
  }
  best.milliseconds = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
  best.info += fmt::format(" (joins tried, m and score: {}; {:.0f} m kept, {:.1f} ms in all)", tried, bestAhead, best.milliseconds);
  return best;
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
  // already flying: searched from the end of a straight lead-in along the velocity, the car's own
  // position put back in front, so the line leaves the start the way the car is going
  const Vec3 car = aStart;
  bool leadIn = false;
  {
    const Vec3 v = aParams.startVelocity;
    const float speed = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    if (speed > 5.0f) {
      const float lead = std::clamp(speed * aParams.startLead, 15.0f, 80.0f) / speed;
      const Vec3 ahead{car.x + v.x * lead, car.y + v.y * lead, car.z + v.z * lead};
      if (aMap.SegmentClear(car.x, car.y, car.z, ahead.x, ahead.y, ahead.z) >= 1.0f) {
        aStart = ahead;
        leadIn = true;
      }
    }
  }
  NavMap::View view(aMap);
  const int32_t width = aMap.Width();
  const float cell = aMap.CellSize();
  // the car's heading for the first stretch of the search (startHeadingCost)
  float headingReach = 0.0f;
  Vec3 heading{0.0f, 0.0f, 0.0f};
  {
    const Vec3 v = aParams.startVelocity;
    const float level = std::hypot(v.x, v.y);
    if (level > 5.0f && aParams.startHeadingCost > 0.0f) {
      heading = {v.x / level, v.y / level, 0.0f};
      headingReach = std::max(40.0f, level * aParams.startHeadingTime);
    }
  }

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
  uint64_t goalKey = MakeKey(gx, gy, gg, width); // moved to a first leg's end (horizon)
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
  // between buildings itself (one anchor gap can be a pocket the map closed off).
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

  // street level per 32 m block: the lowest floor in it (overCost); a block worked out once a search
  std::unordered_map<uint64_t, float> streetLevel;
  auto street = [&](int32_t aCx, int32_t aCy) -> float {
    const int32_t bx = BlockOf(aCx), by = BlockOf(aCy);
    const uint64_t k = (uint64_t(uint32_t(bx)) << 32) | uint32_t(by);
    auto it = streetLevel.find(k);
    if (it != streetLevel.end()) {
      return it->second;
    }
    float lo = kInf;
    for (int32_t y = by * kBlock; y < (by + 1) * kBlock; ++y) {
      for (int32_t x = bx * kBlock; x < (bx + 1) * kBlock; ++x) {
        const uint16_t *g;
        if (view.Column(x, y, g) > 0) {
          lo = std::min(lo, aMap.VoxelZ(g[0]));
        }
      }
    }
    streetLevel[k] = lo;
    return lo;
  };

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
          if (aParams.overCost > 0.0f && next.b > street(nx, ny) + aParams.overHeight) {
            cost += aParams.overCost * step; // over a building
          }
          if (headingReach > 0.0f) {
            float x, y;
            aMap.CellCentre(nx, ny, x, y);
            const float from = std::hypot(x - aStart.x, y - aStart.y);
            if (from < headingReach) {
              const float cosine = (float(kDx[d]) * heading.x + float(kDy[d]) * heading.y) / std::hypot(float(kDx[d]), float(kDy[d]));
              cost += aParams.startHeadingCost * step * 0.5f * (1.0f - cosine) * (1.0f - from / headingReach);
            }
          }
          if (!aParams.avoid.empty()) {
            float x, y;
            aMap.CellCentre(nx, ny, x, y);
            const float z = Preferred(next, aParams.preferredHeight);
            for (const auto &a : aParams.avoid) {
              if (std::hypot(x - a.at.x, y - a.at.y) < a.radius && next.b < a.at.z + a.radius && next.t > a.at.z - a.radius &&
                  std::abs(std::clamp(a.at.z, next.b, next.t) - a.at.z) < a.radius && std::isfinite(z)) {
                cost += a.cost * step;
              }
            }
          }
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
    int32_t gbx = BlockOf(gx), gby = BlockOf(gy);
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
      if (++out.doorwayExpansions > aParams.maxDoorwayExpansions) {
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
    if (bestDoor != kNone && aParams.horizon > 0.0f) {
      // a first leg: the goal moves to the doorway about horizon metres along the doorway route,
      // when the real goal is well beyond it (the rest is planned while the car flies this)
      std::vector<uint32_t> chain;
      for (uint32_t door = bestDoor;; door = uint32_t(doorRecs[door].parent)) {
        chain.push_back(door);
        if (doorRecs[door].parent == door) {
          break;
        }
      }
      std::reverse(chain.begin(), chain.end());
      float along = 0.0f;
      Vec3 prev = aStart;
      for (uint32_t door : chain) {
        const auto &d = doorways.doors[door];
        float x, y;
        aMap.CellCentre(d.cx, d.cy, x, y);
        const GapM gap = GapAt(view, d.cx, d.cy, d.gap);
        const Vec3 at{x, y, std::clamp(Preferred(gap, aParams.preferredHeight), gap.b, std::min(gap.t, kTopCap) - aMap.VoxelHeight())};
        along += Distance(prev, at);
        prev = at;
        if (along < aParams.horizon) {
          continue;
        }
        if (Distance(at, goal) > 0.5f * aParams.horizon) {
          gx = d.cx;
          gy = d.cy;
          gg = d.gap;
          goal = at;
          goalKey = MakeKey(gx, gy, gg, width);
          aMap.CellCentre(gx, gy, goalX, goalY);
          gbx = BlockOf(gx);
          gby = BlockOf(gy);
          bestDoor = door;
          out.partial = true;
        }
        break;
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
    } else if (aParams.minPassage > 0.0f) {
      // no way through the doorways at this passage (searched out, or past maxDoorwayExpansions):
      // the whole map is taken to have none either. Searching both to their limits took 45 s for a
      // rooftop north of the city centre (doorways 671k steps, the map 3M); PlanWithRetries drops
      // the passage next
      return finish("no route through passages this tall");
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
  if (leadIn) {
    out.points.insert(out.points.begin(), car);
  }
  out.ok = true;
  out.searchMilliseconds = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
  if (aParams.line && out.points.size() > 2) {
    auto lineParams = aParams;
    lineParams.lineTakeoff = lineParams.lineTakeoff && !leadIn;
    lineParams.lineLanding = lineParams.lineLanding && !out.partial; // a first leg flies on
    auto line = Line(aMap, out.points, lineParams);
    // where the smoothing left it blocked, those stretches only are taken along the route (the
    // stretch out from under cover to the free space is blocked on the map: not counted)
    out.points = RepairLine(aMap, std::move(line), out.points, lineParams, startLeg + 5.0f, out.repaired);
    const char *what = out.partial ? "first leg" : "line";
    return finish(out.repaired > 0 ? fmt::format("{} ({} blocked stretch(es) taken along the route)", what, out.repaired).c_str()
                                   : what);
  }
  return finish("route");
}

void RacePreset(PlanParams &aParams) {
  aParams.preferredHeight = 40.0f;
  aParams.climbCost = 0.5f;
  aParams.minPassage = 12.0f;
  aParams.heuristicWeight = 1.2f;
  aParams.turnCost = 6.559f;
  // a climb's cost grows exponentially with its rise (ClimbCost): round the Dogtown stadium, not over it
  aParams.climbScale = 30.0f;
}

} // namespace Navigation
