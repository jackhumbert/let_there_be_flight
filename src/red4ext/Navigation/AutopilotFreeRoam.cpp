// The Autopilot's free roam (docs/design/native-ports.md, M6 step 4c): engaged with nothing tracked,
// legs to random points or gates ahead, chained into one route without stopping, around where it
// began. The picks draw from the Autopilot's Rng in the script's order. FlightModeAutopilot.reds line
// numbers name what each part ports.

#include "Autopilot.hpp"

#include "AutopilotDetail.hpp"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <cmath>

namespace Navigation {

using namespace AutopilotDetail;

// ToggleFreeRoam (1023): free roam to the tracked destination, or the destination to free roam; in free
// roam with nothing tracked it stays
void Autopilot::ToggleFreeRoam(const AutopilotInput &aIn) {
  in = &aIn;
  Vec3 target;
  const bool has = CurrentDestination(target);
  if (freeRoamChosen) {
    if (has) {
      freeRoamChosen = false;
      freeRoam = false;
      hasDestination = false;
      DropRequest();
      requestJoin = false;
      AheadReset();
      PublishFreeRoamUI();
      spdlog::info("[Autopilot] free roam off: to the tracked destination");
    }
  } else {
    freeRoamChosen = true;
    hasDestination = false;
    landing = false;
    DropRequest();
    AheadReset();
    spdlog::info("[Autopilot] free roam on");
  }
  in = nullptr;
}

// PublishFreeRoamUI (829): vanilla's panel in its free roam state, with the player in the car
void Autopilot::PublishFreeRoamUI() {
  if (!in->isPlayerMounted) {
    return;
  }
  pending.freeRoamUI = true;
  pending.freeRoamEnabled = freeRoam;
}

// BeginFreeRoam (1048)
void Autopilot::BeginFreeRoam() {
  const Vec3 position = N(in->stats.d_position);
  freeRoam = true;
  hasDestination = false;
  landing = false;
  landingPad = -1;
  DropRequest();
  requestJoin = false;
  freeRoamCentre = position;
  ClearRoute();
  holdPosition = position;
  replanTimer = 0.0f;
  status = "free roam";
  PublishFreeRoamUI();
  spdlog::info("[Autopilot] free roam around {:.1f}, {:.1f}", position.x, position.y);
}

// PickFreeRoamGoal (1069): freeRoamLegMin to freeRoamLegMax from aFrom, within freeRoamSpread of the heading
// (wider for each candidate that does not fit, up to aMaxTurn degrees), inside freeRoamRadius of the centre,
// freeRoamRouteHeight above the ground under open sky there
bool Autopilot::PickFreeRoamGoal(const Vec3 &aFrom, const Vec3 &aHeading, float aMaxTurn, Vec3 &aGoal) {
  const float base = std::atan2(aHeading.y, aHeading.x);
  for (int tries = 0; tries < 24; ++tries) {
    const float spread = std::min(Deg2Rad(params.freeRoamSpread) * (1.0f + float(tries) / 6.0f), Deg2Rad(aMaxTurn));
    // the script's order: the angle's draw, then the distance's
    const float angle = base + rng.RangeF(-1.0f, 1.0f) * spread;
    const float distance = rng.RangeF(params.freeRoamLegMin, params.freeRoamLegMax);
    const Vec3 c{aFrom.x + distance * std::cos(angle), aFrom.y + distance * std::sin(angle), aFrom.z};
    if (Distance2D(c, freeRoamCentre) <= params.freeRoamRadius) {
      const auto gaps = world.Gaps(c);
      if (!gaps.empty()) {
        const Gap &sky = gaps.back();
        const float z = std::min(sky.bottom + params.freeRoamRouteHeight, sky.top - 5.0f);
        // near the height of the leg before, until nothing like that fits; over land
        if (z > sky.bottom + 5.0f && sky.bottom >= -params.freeRoamMaxDepth &&
            (std::fabs(z - aFrom.z) <= params.freeRoamMaxRise || tries >= 16)) {
          aGoal = {c.x, c.y, z};
          return true;
        }
      }
    }
  }
  return false;
}

// PickFreeRoamGate (1101): a gate freeRoamGateMin to freeRoamLegMax away, within freeRoamSpread of the
// heading, inside freeRoamRadius, not one of the last few flown; aEntry is gateLead before it, gateTail the
// straight run from there through it to gateLead past it
bool Autopilot::PickFreeRoamGate(const Vec3 &aFrom, const Vec3 &aHeading, Vec3 &aEntry) {
  const auto &gates = world.GateList();
  std::vector<int32_t> candidates;
  for (int32_t i : GatesInRange(gates, aFrom.x, aFrom.y, params.freeRoamLegMax)) {
    const Gate &g = gates[size_t(i)];
    const Vec3 gate{g.x, g.y, g.z};
    const Vec3 toGate = Sub(gate, aFrom);
    if (std::find(recentGates.begin(), recentGates.end(), i) == recentGates.end() &&
        Length2D(toGate) >= params.freeRoamGateMin && Distance2D(gate, freeRoamCentre) <= params.freeRoamRadius &&
        Dot(Flat(toGate), Flat(aHeading)) >= std::cos(Deg2Rad(params.freeRoamSpread))) {
      candidates.push_back(i);
    }
  }
  while (!candidates.empty()) {
    const int32_t k = rng.Range(0, int32_t(candidates.size()));
    const int32_t i = candidates[size_t(k)];
    candidates.erase(candidates.begin() + k);
    const Gate &g = gates[size_t(i)];
    const Vec3 gate{g.x, g.y, g.z};
    const float yaw = Deg2Rad(g.yaw);
    Vec3 way{std::cos(yaw), std::sin(yaw), 0.0f};
    if (Dot(way, Sub(gate, aFrom)) < 0.0f) {
      way = Mul(way, -1.0f);
    }
    const Vec3 start = Sub(gate, Mul(way, params.gateLead));
    const Vec3 end = Add(gate, Mul(way, params.gateLead));
    if (world.SegmentClear(start, end) >= 1.0f) {
      aEntry = start;
      gateEntry = start;
      gateTail.clear();
      for (float d = params.gateStep; d <= 2.0f * params.gateLead + 0.01f; d += params.gateStep) {
        gateTail.push_back(Add(start, Mul(way, d)));
      }
      recentGates.push_back(i);
      if (int32_t(recentGates.size()) > params.gateMemory) {
        recentGates.erase(recentGates.begin());
      }
      spdlog::info("[Autopilot] free roam: through gate {} at {:.1f}, {:.1f}, {:.1f}", i, gate.x, gate.y, gate.z);
      return true;
    }
  }
  return false;
}

// NextFreeRoamLeg (1147): a leg from aFrom towards a new goal; aJoin: it continues the route from joinPoint
bool Autopilot::NextFreeRoamLeg(const Vec3 &aFrom, const Vec3 &aHeading, bool aJoin) {
  Vec3 goal{};
  gateTail.clear();
  // a gate ahead, else a point ahead; at the edge of the area, the area moved to here and the same again;
  // turning further only when nothing else fits
  bool found = PickFreeRoamGate(aFrom, aHeading, goal) || PickFreeRoamGoal(aFrom, aHeading, params.freeRoamMaxTurn, goal);
  if (!found) {
    freeRoamCentre = aFrom;
    spdlog::info("[Autopilot] free roam: nothing ahead, area moved to {:.1f}, {:.1f}", aFrom.x, aFrom.y);
    found = PickFreeRoamGate(aFrom, aHeading, goal) || PickFreeRoamGoal(aFrom, aHeading, params.freeRoamMaxTurn, goal) ||
            PickFreeRoamGoal(aFrom, aHeading, 180.0f, goal);
  }
  if (!found) {
    replanTimer = 1.0f;
    spdlog::warn("[Autopilot] free roam: no point found for the next leg");
    return false;
  }
  destination = gateTail.empty() ? goal : gateTail.back();
  routeGoal = goal;
  RequestRoute(aFrom);
  requestJoin = aJoin;
  spdlog::info("[Autopilot] free roam: next leg to {:.1f}, {:.1f}, {:.1f} ({:.0f} m)", goal.x, goal.y, goal.z,
               Distance2D(aFrom, goal));
  return true;
}

// ChainFreeRoam (1176): with no route a first leg from here; on a route running out, the next leg from a
// point ahead on it, so the car flies on into it without slowing
void Autopilot::ChainFreeRoam() {
  if (request > 0 || replanTimer > 0.0f) {
    return;
  }
  const auto &stats = in->stats;
  const Vec3 velocity = N(stats.d_velocity);
  const Vec3 forward = N(stats.d_forward);
  if (RouteSize() < 2) {
    const Vec3 heading = Length2D(velocity) > 3.0f ? Flat(velocity) : Flat(forward);
    NextFreeRoamLeg(N(stats.d_position), heading, false);
    return;
  }
  if (status != "following" || route.Length() - tracker.closestS > params.freeRoamChainAt) {
    return;
  }
  // the point at that distance, not before the end of a gate's straight run still ahead
  const float s = std::min(
      std::max(tracker.closestS + std::max(params.freeRoamJoinMin, stats.d_speed * params.freeRoamJoinAhead), gateEndS),
      route.Length());
  const Vec3 point = route.At(s);
  const Vec3 back = Sub(point, route.At(s - 10.0f));
  const Vec3 heading = Length2D(back) > 0.1f ? Flat(back) : Flat(forward);
  if (NextFreeRoamLeg(point, heading, true)) {
    joinS = s;
    joinPoint = point;
  }
}

// SteepestPitch (1525)
float SteepestPitch(const std::vector<Vec3> &aPoints, float aSpan) {
  float steepest = 0.0f;
  const size_t n = aPoints.size();
  size_t i = 0, j = 1;
  while (j < n) {
    const float flat = Distance2D(aPoints[i], aPoints[j]);
    const float rise = std::fabs(aPoints[j].z - aPoints[i].z);
    if (flat >= aSpan) {
      steepest = std::max(steepest, Rad2Deg(std::atan2(rise, flat)));
      ++i;
    } else {
      // a vertical stretch shorter than span across: steep as can be
      if (rise > aSpan) {
        steepest = std::max(steepest, Rad2Deg(std::atan2(rise, std::max(flat, 0.01f))));
      }
      ++j;
    }
  }
  return steepest;
}

} // namespace Navigation
