// The Autopilot's route (docs/design/autopilot.md, Route): the goal, the plans asked for and
// taken (with the plan-ahead of AutopilotPlanAhead.reds), the splices, the replans after a hit or off
// the line, the trajectory and the landing spot checked again on the approach. FlightModeAutopilot.reds
// line numbers name what each part ports.

#include "Autopilot.hpp"

#include "AutopilotDetail.hpp"
#include "Roads.hpp"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <cmath>

namespace Navigation {

using namespace AutopilotDetail;

namespace {
RouteShape RaceShape(const AutopilotParams &aParams, float aHeight) {
  return {aHeight, aParams.raceClimbCost, aParams.raceMinPassage, aParams.raceSearchWeight, aParams.raceTurnCost};
}

// a planner's line: more than two points, about lineSpacing (5 m) apart; a two-point hop has corners to round
bool IsLine(bool aRace, const std::vector<Vec3> &aPoints) {
  const size_t n = aPoints.size();
  return aRace && n > 2 && Distance(aPoints[0], aPoints[n - 1]) / float(n - 1) < 10.0f;
}
} // namespace

// ClearRoute (442)
void Autopilot::ClearRoute() { route = Route{}; }

// DropRequest (448): the plan asked for is no longer wanted
void Autopilot::DropRequest() {
  if (request > 0) {
    plans.Drop(request);
  }
  request = 0;
}

// A request about to be replaced by a new one: its plan, never taken, is freed. Not the trip planned ahead, whose
// rest is still wanted (AheadReset drops that one).
void Autopilot::ForgetRequest() {
  if (request > 0 && request != ahead.trip) {
    plans.Drop(request);
  }
}

// ChooseRouteGoal (710): where the route should end, above the landing spot or the point itself
void Autopilot::ChooseRouteGoal() {
  float top = 0.0f;
  landing = false;
  landingPad = -1;
  if (params.landingMode != kLandHover) {
    landingPad = FindLandingPad(world, params, destination);
    if (landingPad >= 0) {
      const Helipad &pad = world.Pads()[size_t(landingPad)];
      // the spot is where a resting car's centre sits; a pad summon's offset puts it across the deck from V
      const float x = pad.x + padLandingOffset.x, y = pad.y + padLandingOffset.y;
      landingSpot = {x, y, pad.z + world.CarHalfHeight()};
      // FlightNavMap.GetHelipadClearance gives open sky as 1e9
      top = pad.z + (std::isinf(pad.clearance) ? 1e9f : pad.clearance);
      landing = true;
    } else {
      GroundSpot found;
      landing = FindLandingSpot(world, params, destination, Keep(), found);
      if (landing) {
        landingSpot = found.spot;
        top = found.top;
      }
    }
  }
  if (landing) {
    routeGoal = {landingSpot.x, landingSpot.y, std::min(landingSpot.z + params.approachHeight, top - 2.0f)};
    spdlog::info("[Autopilot] landing {} at {:.1f}, {:.1f}, {:.1f} ({:.1f} m from the destination)",
                 landingPad >= 0 ? "pad " + world.Pads()[size_t(landingPad)].id : std::string("spot"), landingSpot.x,
                 landingSpot.y, landingSpot.z, Distance2D(landingSpot, destination));
  } else {
    routeGoal = destination;
  }
}

// AutopilotPlanAhead.Reset
void Autopilot::AheadReset() {
  ahead.firstLeg = false;
  ahead.waiting = false;
  if (ahead.trip > 0) {
    plans.Drop(ahead.trip);
  }
  ahead.trip = 0;
}

// AutopilotPlanAhead.Request: the trip planned ahead when the car has no route to keep; false: in one go
bool Autopilot::AheadRequest(const Vec3 &aFrom, const Vec3 &aVelocity) {
  AheadReset();
  if (!ahead.enabled || RouteSize() >= 2) {
    return false;
  }
  RefreshVehicle();
  AheadParams a;
  a.horizon = std::max(0.0f, ahead.horizon);
  a.margin = std::max(0.0f, ahead.margin);
  a.minTrip = ahead.minTrip;
  ahead.trip = plans.Trip(aFrom, routeGoal,
                          RaceParams(planBase, RaceShape(params, params.raceRouteHeight), &vehicle, true, hits, aVelocity),
                          a);
  request = ahead.trip;
  return true;
}

// AutopilotPlanAhead.Taken: a first leg waits for its rest, joining it joinS along
void Autopilot::AheadTaken() {
  if (ahead.trip == 0 || !lastPlanPartial) {
    AheadReset();
    return;
  }
  ahead.firstLeg = true;
  ahead.waiting = true;
  joinS = lastPlanJoin;
  spdlog::info("[Autopilot] plan ahead: flying a first leg, the rest joins {:.0f} m along", joinS);
}

// AutopilotPlanAhead.TakeRest: the rest once it has come, and whether it can be spliced on
bool Autopilot::AheadTakeRest(std::vector<Vec3> &aPoints) {
  if (!ahead.waiting || !plans.RestReady(ahead.trip)) {
    return false;
  }
  PlanResult rest;
  plans.TakeRest(ahead.trip, rest);
  lastPlanInfo = rest.info;
  lastPlanPartial = false;
  aPoints = rest.ok ? rest.points : std::vector<Vec3>{};
  ahead.waiting = false;
  ahead.trip = 0;
  const bool inTime = aPoints.size() >= 2 && joinS > tracker.closestS + 20.0f;
  if (inTime) {
    spdlog::info("[Autopilot] plan ahead: the rest came {:.0f} m before the join: {}", joinS - tracker.closestS,
                 lastPlanInfo);
    joinPoint = aPoints[0];
    ahead.firstLeg = false;
  } else {
    spdlog::info("[Autopilot] plan ahead: the rest came {}: {}", aPoints.size() >= 2 ? "too late" : "with no route",
                 lastPlanInfo);
  }
  return true;
}

// FlightNavMap.TakePlan: the points (none for a failed plan) and the last plan's info, partial and join
bool Autopilot::TakePlan(int32_t aId, std::vector<Vec3> &aPoints) {
  TakenPlan taken;
  if (!plans.Take(aId, taken)) {
    aPoints.clear();
    return false;
  }
  lastPlanInfo = taken.plan.info;
  lastPlanPartial = taken.plan.partial;
  lastPlanJoin = taken.joinS;
  aPoints = taken.plan.ok ? taken.plan.points : std::vector<Vec3>{};
  return true;
}

// RequestRoute (878)
void Autopilot::RequestRoute(const Vec3 &aFrom) {
  RefreshVehicle();
  if (!world.Loaded()) {
    status = "no nav map";
    return;
  }
  const Vec3 velocity = N(in->stats.d_velocity);
  // the Race: the planner spreads the route into a line of wide bends
  requestRace = true;
  requestJoin = false;
  // a replan towards a gate still ahead: through it again
  if (gateTail.empty() && !routeGateTail.empty() && gateEndS > tracker.closestS &&
      Distance(routeGoal, routeGateEntry) < 1.0f) {
    gateTail = routeGateTail;
  }
  ForgetRequest();
  // from rest: a first leg at once, the rest planned while the car flies it
  if (freeRoam) {
    const FreeRoamShape free{params.freeRoamHeightFree, params.freeRoamHeightWavelength, params.freeRoamOverCost};
    auto leg = FreeRoamParams(planBase, RaceShape(params, params.freeRoamRouteHeight), free, &vehicle, hits, velocity);
    if (params.freeRoamGuideCost > 0.0f && Roads::Get().IsLoaded()) {
      leg.guideCost = params.freeRoamGuideCost;
      leg.guideLine = params.freeRoamGuideLine;
      Roads::Get().Near(aFrom, routeGoal, 400.0f, leg.guide);
    }
    request = plans.Race(aFrom, routeGoal, leg);
  } else if (!AheadRequest(aFrom, velocity)) {
    request = plans.Race(aFrom, routeGoal,
                         RaceParams(planBase, RaceShape(params, params.raceRouteHeight), &vehicle, true, hits, velocity));
  }
  replanTimer = 3.0f;
  if (RouteSize() < 2) {
    // nothing to follow meanwhile: hold here (a replan keeps following the old route)
    status = "planning";
    holdPosition = aFrom;
  }
}

// UpdateRoute (910)
void Autopilot::UpdateRoute(float aDt) {
  replanTimer -= aDt;
  // the rest of a trip begun on a first leg: on from its join, or (late) the trip again from here
  std::vector<Vec3> rest;
  if (AheadTakeRest(rest)) {
    if (ahead.firstLeg) {
      AheadReset();
      RequestRoute(N(in->stats.d_position));
    } else {
      SpliceRoute(rest);
    }
  }
  Vec3 target;
  const bool has = CurrentDestination(target);
  if (!freeRoam && (status == "descending" || status == "landed" || status == "arrived")) {
    // the trip is over once it arrives, as vanilla autodrive's is: finish the landing (or the hover) whatever the game
    // tracks now. It untracks a map pin once the player reaches it, and then often tracks the quest's objective
    // again, which must not take the car off to it.
    return;
  }
  if (freeRoamChosen) {
    // free roam until the player switches, whatever gets tracked meanwhile
    if (!freeRoam) {
      BeginFreeRoam();
    }
    ChainFreeRoam();
  } else if (has) {
    if (!hasDestination || Distance(target, destination) > 20.0f) {
      destination = target;
      hasDestination = true;
      ClearRoute();
      ChooseRouteGoal();
      RequestRoute(N(in->stats.d_position));
    }
  } else {
    // the destination untracked on the way: as vanilla autodrive, the Autopilot hands back
    spdlog::info("[Autopilot] destination untracked on the way: handing back");
    pending.exitAutopilot = true;
    return;
  }
  if (request > 0 && plans.Ready(request)) {
    std::vector<Vec3> points;
    TakePlan(request, points);
    request = 0;
    if (requestRejoin) {
      requestRejoin = false;
      spdlog::info("[Autopilot] {}", lastPlanInfo);
      if (points.size() < 2) {
        RequestRoute(N(in->stats.d_position));
        return;
      }
    }
    // a free roam leg to a gate: planned to just before it, then straight through
    const bool gateLeg = points.size() >= 2 && !gateTail.empty();
    if (gateLeg) {
      points.insert(points.end(), gateTail.begin(), gateTail.end());
      routeGateTail = gateTail;
      routeGateEntry = gateEntry;
    }
    gateTail.clear();
    spliceGate = gateLeg;
    // free roam: no chained leg steeper than freeRoamMaxPitch anywhere; another leg instead
    if (requestJoin && points.size() >= 2 && SteepestPitch(points, 10.0f) > params.freeRoamMaxPitch) {
      spdlog::info("[Autopilot] free roam: leg steeper than {:.0f} degrees ({:.0f}), another one", params.freeRoamMaxPitch,
                   SteepestPitch(points, 10.0f));
      requestJoin = false;
      replanTimer = 0.2f;
      return;
    }
    if (requestJoin) {
      requestJoin = false;
      // the next free roam leg, if the car has not passed where it starts
      if (points.size() >= 2 && joinS > tracker.closestS + 20.0f) {
        SpliceRoute(points);
      } else {
        replanTimer = 0.5f; // ChainFreeRoam tries another
        spdlog::warn("[Autopilot] free roam: next leg {}", points.size() >= 2 ? "came too late" : "found no route");
      }
      spdlog::info("[Autopilot] {}", lastPlanInfo);
      return;
    }
    if (points.size() >= 2) {
      routeLine = IsLine(requestRace, points);
      // a line is already smooth: no corners to round
      route = Route::Make(routeLine ? points : RoundCorners(points));
      const float total = route.Length();
      tracker.segment = 0;
      tracker.closestS = 0.0f;
      landSlewOn = false;
      gateEndS = gateLeg ? total : 0.0f;
      status = "following";
      BuildTrajectory();
      AheadTaken();
    } else {
      status = "no route";
      holdPosition = N(in->stats.d_position);
    }
    spdlog::info("[Autopilot] {}", lastPlanInfo);
  }
}

// SpliceRoute (1205): the route kept from just behind the car up to joinPoint, then the new leg, the
// bend where they meet smoothed out over freeRoamSmoothSpan
void Autopilot::SpliceRoute(const std::vector<Vec3> &aPoints) {
  const bool line = IsLine(requestRace, aPoints);
  const std::vector<Vec3> tail = line ? aPoints : RoundCorners(aPoints);
  const int32_t first = std::max(tracker.segment - 2, 0);
  const float trimmed = route.Distance(first);
  std::vector<Vec3> joined;
  for (int32_t i = first; i < RouteSize() && route.Distance(i) < joinS - 0.5f; ++i) {
    joined.push_back(route.Point(i));
  }
  joined.push_back(joinPoint);
  const int32_t join = int32_t(joined.size()) - 1;
  for (size_t i = 1; i < tail.size(); ++i) {
    joined.push_back(tail[i]);
  }
  // the legs meet at an angle: the points round the join eased where the map allows, the stretch the car
  // is on left alone. The line's points are about 5 m apart.
  const int32_t span = int32_t(params.freeRoamSmoothSpan / 5.0f);
  int32_t lo = std::max(join - span, tracker.segment - first + 2);
  if (gateEndS >= joinS - 1.0f) {
    lo = std::max(lo, join); // joined at the end of a gate's run: the run stays straight
  }
  const int32_t hi = std::min(join + span, int32_t(joined.size()) - 2);
  route = Route::Make(SmoothJoin(std::move(joined), join, lo, hi, params.freeRoamSmoothSpan, 20, world));
  routeLine = routeLine && line;
  tracker.segment -= first;
  tracker.closestS -= trimmed;
  const float total = route.Length();
  gateEndS = spliceGate ? total : std::max(gateEndS - trimmed, 0.0f);
  BuildTrajectory();
  spdlog::info("[Autopilot] next leg joined, {:.0f} m ahead", total - tracker.closestS);
}

// RoundCorners (1248): full cut, then smaller; below raceRoundMin the corner stays sharp
std::vector<Vec3> Autopilot::RoundCorners(const std::vector<Vec3> &aPoints) const {
  return Navigation::RoundCorners(aPoints, params.raceCornerRound, params.raceRoundShare, params.raceRoundMin,
                                  params.raceRoundStep, world);
}

// WatchHits (2076): a hit remembered for the next plans for hitAvoidTime, and once the controls' damping
// is over a replan from the car's state
void Autopilot::WatchHits(float aDt, const Vec3 &aP, const Vec3 &aV) {
  clock += aDt;
  const float timer = in->collisionTimer;
  if (timer < lastCollisionTimer - 0.05f && status == "following") {
    hits.push_back({aP, params.hitAvoidRadius});
    hitTimes.push_back(clock);
    recovering = in->settings.collisionRecoveryDelay;
    spdlog::info("[Autopilot] hit at {:.1f}, {:.1f}, {:.1f} at {:.1f} m/s: holding, then replanning round it", aP.x, aP.y,
                 aP.z, Length(aV));
  }
  lastCollisionTimer = timer;
  while (!hitTimes.empty() && clock - hitTimes.front() > params.hitAvoidTime) {
    hits.erase(hits.begin());
    hitTimes.erase(hitTimes.begin());
  }
  if (recovering > 0.0f) {
    recovering -= aDt;
    if (recovering <= 0.0f && request == 0 && status == "following") {
      Replan(aP, aV);
    }
  }
}

// Replan (2102): back onto the line from where the car is, or a full replan with no line left to rejoin
void Autopilot::Replan(const Vec3 &aP, const Vec3 &aV) {
  RefreshVehicle();
  const int32_t n = RouteSize();
  // a first leg still waiting for its rest ends short of the goal, and a rejoin taken would drop the rest
  // (AheadTaken): the trip again from here (otherwise a teleport mid-leg "arrives" 100 m up at the leg's end)
  if (!routeLine || n - tracker.segment < 4 || ahead.firstLeg) {
    RequestRoute(aP);
    return;
  }
  std::vector<Vec3> line;
  for (int32_t i = tracker.segment; i < n; ++i) {
    line.push_back(route.Point(i));
  }
  requestRace = true;
  requestJoin = false;
  requestRejoin = true;
  ForgetRequest();
  request = plans.Rejoin(aP, std::move(line), std::max(params.rejoinMin, Length(aV) * params.rejoinAhead),
                         CarPlanParams(planBase, &vehicle, hits, aV));
  replanTimer = 3.0f;
}

// NearestFree (2123): up, sideways or down, 1 to 8 m out (p if none)
Vec3 Autopilot::NearestFree(const Vec3 &aP) const {
  if (world.IsFree(aP)) {
    return aP;
  }
  static const Vec3 kDirs[] = {{0, 0, 1}, {1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, -1}};
  for (float r = 1.0f; r <= 8.0f; r += 1.0f) {
    for (const Vec3 &d : kDirs) {
      const Vec3 q = Add(aP, Mul(d, r));
      if (world.IsFree(q)) {
        return q;
      }
    }
  }
  return aP;
}

// BuildTrajectory (2150): from the car's speed when the route starts at the car, free to start at any speed
// when it is rebuilt round a car already on it
void Autopilot::BuildTrajectory() {
  RefreshVehicle();
  if (RouteSize() < 2) {
    return;
  }
  const float start = tracker.closestS < 1.0f ? in->stats.d_speed : 1000.0f;
  TrajectoryParams tp;
  tp.startSpeed = std::max(start, 0.0f);
  tp.stopAtEnd = true;
  trajectory = Navigation::BuildTrajectory(vehicle, route.points, tp, world.Map());
  if (!trajectory.violations.empty()) {
    std::string bad;
    for (const auto &v : trajectory.violations) {
      bad += (bad.empty() ? "" : "; ") + v;
    }
    spdlog::warn("[Autopilot] trajectory outside the car's limits: {}", bad);
  }
  float top = 0.0f;
  const float length = FollowsTrajectory() ? trajectory.samples.back().s : 0.0f;
  for (float s = 0.0f; s <= length; s += 10.0f) {
    top = std::max(top, SampleAt(trajectory, s).speed);
  }
  const auto &st = in->statics;
  spdlog::info("[Autopilot] race (drone): {:.0f} m in {:.1f} s, top {:.1f} m/s (car {:.1f}), lateral {:.1f} m/s2, decel "
               "{:.1f} + {:.1f} at 30 m/s (mass {:.0f}, air {:.3f}, brake friction {:.2f})",
               length, trajectory.Duration(), top, TopSpeed(vehicle), Lateral(vehicle), Decel(vehicle, 0.0f),
               Decel(vehicle, 30.0f) - Decel(vehicle, 0.0f), st.mass, st.airResistanceFactor, st.brakingFrictionFactor);
}

// RecheckLandingSpot (2211): the spot again on the approach, once the cars round it have streamed in;
// taken (or not under the whole car), the route's end moved to the nearest free one
void Autopilot::RecheckLandingSpot(float aDt) {
  if (!landing || ahead.firstLeg || landingPad >= 0 || tracker.remaining > params.landingRecheck ||
      tracker.remaining < 15.0f) {
    return;
  }
  spotCheckTimer -= aDt;
  if (spotCheckTimer > 0.0f) {
    return;
  }
  spotCheckTimer = 0.5f;
  const bool taken = SpotTaken(world, landingSpot);
  if (!taken && SpotSupported(world, landingSpot, params.landingFootprint)) {
    return;
  }
  GroundSpot found;
  if (FindLandingSpot(world, params, destination, Keep(), found) && Distance2D(found.spot, landingSpot) > 2.0f) {
    const float over = Distance2D(found.spot, landingSpot);
    std::vector<Vec3> moved;
    if (ShiftRouteEnd(world, route, tracker.segment, found.spot, landingSpot.z, params.landingShiftSpan, moved)) {
      // ShiftRouteEnd (2178): the route rebuilt, its end the new goal, the trajectory again
      route = Route::Make(std::move(moved));
      routeGoal = route.Point(route.Size() - 1);
      BuildTrajectory();
      landingSpot = found.spot;
      spdlog::info("[Autopilot] landing spot {}, {:.0f} m out: route end moved {:.1f} m to {:.1f}, {:.1f}",
                   taken ? "taken by a vehicle" : "not under the whole car (an edge or a ledge)", tracker.remaining, over,
                   found.spot.x, found.spot.y);
    }
  }
}

} // namespace Navigation
