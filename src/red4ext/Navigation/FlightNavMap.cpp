#include "FlightNavMap.hpp"
#include "PlanAhead.hpp"
#include "Gates.hpp"
#include "Helipads.hpp"
#include "NavDraw.hpp"
#include "NavMap.hpp"
#include "Planner.hpp"

#include <algorithm>
#include <cmath>
#include <memory>
#include <mutex>
#include <spdlog/spdlog.h>
#include <thread>
#include <unordered_map>

namespace {
constexpr float kSky = 1e9f;
constexpr float kBlocked = -1e9f;

float Finite(float aZ) { return std::isinf(aZ) ? kSky : aZ; }

Navigation::PlanParams planParams;
std::string lastPlanInfo;
bool lastPlanPartial = false;
float lastPlanJoin = 0.0f;
// s the rest of a trip planned ahead took last (eased): when its join is due
float restSeconds = 2.0f;

struct Request {
  bool ready = false;
  Navigation::PlanResult result;
  float joinS = 0.0f;       // a first leg's join (PlanTripAsync)
  bool restReady = false;   // its rest has been planned
  Navigation::PlanResult rest;
};
bool hasTargetOverride = false;
RED4ext::Vector4 targetOverride{};

std::mutex requestsLock;
std::unordered_map<int32_t, std::shared_ptr<Request>> requests;
int32_t nextRequest = 1;

RED4ext::DynArray<RED4ext::Vector4> ToScript(const Navigation::PlanResult &aResult) {
  RED4ext::DynArray<RED4ext::Vector4> out;
  if (aResult.ok) {
    for (const auto &p : aResult.points) {
      out.EmplaceBack(p.x, p.y, p.z, 1.0f);
    }
  }
  return out;
}

const Navigation::Helipad *PadAt(int32_t aIndex) {
  const auto &pads = Navigation::Helipads::Get().All();
  return aIndex >= 0 && size_t(aIndex) < pads.size() ? &pads[aIndex] : nullptr;
}

const Navigation::Gate *GateAt(int32_t aIndex) {
  const auto &gates = Navigation::Gates::Get().All();
  return aIndex >= 0 && size_t(aIndex) < gates.size() ? &gates[aIndex] : nullptr;
}

void LogPlan(Navigation::Vec3 aStart, Navigation::Vec3 aGoal, const Navigation::PlanResult &aResult) {
  spdlog::info("[NavMap] plan ({:.0f}, {:.0f}, {:.0f}) -> ({:.0f}, {:.0f}, {:.0f}): {}", aStart.x, aStart.y,
               aStart.z, aGoal.x, aGoal.y, aGoal.z, aResult.info);
}
} // namespace

bool FlightNavMap::IsLoaded() { return Navigation::NavMap::Get().IsLoaded(); }

float FlightNavMap::GetCarHalfHeight() { return Navigation::NavMap::Get().CarHalfHeight(); }

RED4ext::DynArray<RED4ext::Vector2> FlightNavMap::GetGaps(RED4ext::Vector4 aPosition) {
  RED4ext::DynArray<RED4ext::Vector2> out;
  for (const auto &gap : Navigation::NavMap::Get().GetGaps(aPosition.X, aPosition.Y)) {
    out.EmplaceBack(gap.bottom, Finite(gap.top));
  }
  return out;
}

// Drawing geometry for the planner's view in game (NavDraw.hpp); done in C++ because in script,
// 10,000 GetGaps a refresh held frames up to 66 ms.
namespace {
RED4ext::DynArray<RED4ext::Vector4> ToScript(const std::vector<Navigation::DrawPoint> &aPoints) {
  RED4ext::DynArray<RED4ext::Vector4> out;
  out.Reserve(uint32_t(aPoints.size()));
  for (const auto &p : aPoints) {
    out.EmplaceBack(p.x, p.y, p.z, 1.0f);
  }
  return out;
}

// GetNavEdges is called every frame; the wireframe is kept until the centre moves 4 m
struct NavEdgesCache {
  float x = 0.0f, y = 0.0f, radius = -1.0f, zMin = 0.0f, zMax = 0.0f;
  RED4ext::DynArray<RED4ext::Vector4> edges;
};
std::mutex navEdgesLock;
NavEdgesCache navEdges, navBlocks;
} // namespace

RED4ext::DynArray<RED4ext::Vector4> FlightNavMap::GetOutlines(RED4ext::Vector4 aCentre, float aRadius,
                                                               RED4ext::DynArray<float> aHeights) {
  std::vector<float> heights(aHeights.begin(), aHeights.end());
  std::vector<Navigation::DrawPoint> points;
  Navigation::Outlines(Navigation::NavMap::Get(), aCentre.X, aCentre.Y, aRadius, heights, points);
  return ToScript(points);
}

RED4ext::DynArray<RED4ext::Vector4> FlightNavMap::GetNavEdges(RED4ext::Vector4 aCentre, float aRadius, float aZMin,
                                                               float aZMax) {
  std::lock_guard guard(navEdgesLock);
  if (navEdges.radius == aRadius && navEdges.zMin == aZMin && navEdges.zMax == aZMax &&
      std::hypot(aCentre.X - navEdges.x, aCentre.Y - navEdges.y) < 4.0f) {
    return navEdges.edges;
  }
  std::vector<Navigation::DrawPoint> points;
  Navigation::NavEdges(Navigation::NavMap::Get(), aCentre.X, aCentre.Y, aRadius, aZMin, aZMax, points);
  navEdges.x = aCentre.X;
  navEdges.y = aCentre.Y;
  navEdges.radius = aRadius;
  navEdges.zMin = aZMin;
  navEdges.zMax = aZMax;
  navEdges.edges = ToScript(points);
  return navEdges.edges;
}

RED4ext::DynArray<RED4ext::Vector4> FlightNavMap::GetNavBlocks(RED4ext::Vector4 aCentre, float aRadius, float aZMin,
                                                                float aZMax) {
  std::lock_guard guard(navEdgesLock);
  if (navBlocks.radius == aRadius && std::abs(navBlocks.zMin - aZMin) < 4.0f && std::abs(navBlocks.zMax - aZMax) < 4.0f &&
      std::hypot(aCentre.X - navBlocks.x, aCentre.Y - navBlocks.y) < 4.0f) {
    return navBlocks.edges;
  }
  std::vector<Navigation::DrawPoint> points;
  Navigation::NavBlocks(Navigation::NavMap::Get(), aCentre.X, aCentre.Y, aRadius, aZMin, aZMax, points);
  navBlocks.x = aCentre.X;
  navBlocks.y = aCentre.Y;
  navBlocks.radius = aRadius;
  navBlocks.zMin = aZMin;
  navBlocks.zMax = aZMax;
  navBlocks.edges = ToScript(points);
  return navBlocks.edges;
}

bool FlightNavMap::IsFree(RED4ext::Vector4 aPosition) {
  return Navigation::NavMap::Get().IsFree(aPosition.X, aPosition.Y, aPosition.Z);
}

float FlightNavMap::GetFloor(RED4ext::Vector4 aPosition) {
  Navigation::Gap gap;
  return Navigation::NavMap::Get().FindGap(aPosition.X, aPosition.Y, aPosition.Z, gap) ? gap.bottom : kBlocked;
}

float FlightNavMap::GetCeiling(RED4ext::Vector4 aPosition) {
  Navigation::Gap gap;
  return Navigation::NavMap::Get().FindGap(aPosition.X, aPosition.Y, aPosition.Z, gap) ? Finite(gap.top) : kBlocked;
}

float FlightNavMap::SegmentClear(RED4ext::Vector4 aFrom, RED4ext::Vector4 aTo) {
  return Navigation::NavMap::Get().SegmentClear(aFrom.X, aFrom.Y, aFrom.Z, aTo.X, aTo.Y, aTo.Z);
}

RED4ext::DynArray<RED4ext::Vector4> FlightNavMap::Plan(RED4ext::Vector4 aStart, RED4ext::Vector4 aGoal, bool aDirect) {
  auto params = planParams;
  params.tryDirect = aDirect;
  Navigation::Vec3 start{aStart.X, aStart.Y, aStart.Z}, goal{aGoal.X, aGoal.Y, aGoal.Z};
  auto result = Navigation::Plan(Navigation::NavMap::Get(), start, goal, params);
  lastPlanInfo = result.info;
  LogPlan(start, goal, result);
  return ToScript(result);
}

namespace {
int32_t StartPlan(RED4ext::Vector4 aStart, RED4ext::Vector4 aGoal, Navigation::PlanParams params);
}

int32_t FlightNavMap::PlanAsync(RED4ext::Vector4 aStart, RED4ext::Vector4 aGoal, bool aDirect) {
  auto params = planParams;
  params.tryDirect = aDirect;
  return StartPlan(aStart, aGoal, params);
}

int32_t FlightNavMap::PlanRaceAsync(RED4ext::Vector4 aStart, RED4ext::Vector4 aGoal, float aPreferredHeight,
                                    float aClimbCost, float aMinPassage, float aHeuristicWeight, float aTurnCost,
                                    RED4ext::Handle<FlightVehicleModel> aVehicle, bool aDetour,
                                    RED4ext::DynArray<RED4ext::Vector4> aAvoid, RED4ext::Vector4 aVelocity) {
  auto params = planParams;
  Navigation::RacePreset(params);
  // the line is shaped for the car flying it (TurnCost)
  if (aVehicle) {
    params.vehicle = aVehicle->Spec();
  }
  // slots too tight for the car's speed routed round (PlanRace) only for a trajectory follower
  if (!aDetour) {
    params.slotDetourLoss = params.slotDetourLossMin = 1e9f;
  }
  // the car's velocity: a plan for a car already flying starts the way it is going
  params.startVelocity = {aVelocity.X, aVelocity.Y, aVelocity.Z};
  // places the car hit something lately (x, y, z, radius in w): routed round like a tight slot
  for (const auto &a : aAvoid) {
    params.avoid.push_back({{a.X, a.Y, a.Z}, std::max(a.W, 1.0f), params.slotAvoidCost});
  }
  // the route's character from the Autopilot's race route fields (defaults as RacePreset)
  params.preferredHeight = aPreferredHeight;
  params.climbCost = aClimbCost;
  params.minPassage = aMinPassage;
  params.heuristicWeight = std::max(1.0f, aHeuristicWeight);
  params.turnCost = std::max(0.0f, aTurnCost);
  return StartPlan(aStart, aGoal, params);
}

int32_t FlightNavMap::PlanTripAsync(RED4ext::Vector4 aStart, RED4ext::Vector4 aGoal, float aPreferredHeight, float aClimbCost,
                                    float aMinPassage, float aHeuristicWeight, float aTurnCost,
                                    RED4ext::Handle<FlightVehicleModel> aVehicle, RED4ext::DynArray<RED4ext::Vector4> aAvoid,
                                    RED4ext::Vector4 aVelocity, float aHorizon, float aMargin, float aMinTrip) {
  auto params = planParams;
  Navigation::RacePreset(params);
  if (aVehicle) {
    params.vehicle = aVehicle->Spec();
  }
  params.startVelocity = {aVelocity.X, aVelocity.Y, aVelocity.Z};
  for (const auto &a : aAvoid) {
    params.avoid.push_back({{a.X, a.Y, a.Z}, std::max(a.W, 1.0f), params.slotAvoidCost});
  }
  params.preferredHeight = aPreferredHeight;
  params.climbCost = aClimbCost;
  params.minPassage = aMinPassage;
  params.heuristicWeight = std::max(1.0f, aHeuristicWeight);
  params.turnCost = std::max(0.0f, aTurnCost);
  Navigation::AheadParams ahead;
  ahead.horizon = std::max(0.0f, aHorizon);
  ahead.margin = std::max(0.0f, aMargin);
  ahead.minTrip = aMinTrip;
  Navigation::Vec3 start{aStart.X, aStart.Y, aStart.Z}, goal{aGoal.X, aGoal.Y, aGoal.Z};
  auto request = std::make_shared<Request>();
  int32_t id;
  {
    std::lock_guard guard(requestsLock);
    id = nextRequest++;
    requests[id] = request;
    ahead.restGuess = restSeconds;
  }
  std::thread([request, start, goal, params, ahead]() {
    auto leg = Navigation::PlanFirstLeg(Navigation::NavMap::Get(), start, goal, params, ahead);
    LogPlan(start, goal, leg.plan);
    const bool partial = leg.plan.partial;
    {
      std::lock_guard guard(requestsLock);
      request->result = leg.plan;
      request->joinS = leg.joinS;
      request->ready = true;
    }
    if (!partial) {
      return;
    }
    auto rest = Navigation::PlanRest(Navigation::NavMap::Get(), leg, goal, params);
    LogPlan(leg.joinPosition, goal, rest);
    std::lock_guard guard(requestsLock);
    restSeconds = 0.5f * restSeconds + 0.5f * float(rest.milliseconds / 1000.0);
    request->rest = std::move(rest);
    request->restReady = true;
  }).detach();
  return id;
}

bool FlightNavMap::IsRestReady(int32_t aId) {
  std::lock_guard guard(requestsLock);
  auto it = requests.find(aId);
  return it != requests.end() && it->second->restReady;
}

RED4ext::DynArray<RED4ext::Vector4> FlightNavMap::TakeRest(int32_t aId) {
  std::shared_ptr<Request> request;
  {
    std::lock_guard guard(requestsLock);
    auto it = requests.find(aId);
    if (it == requests.end() || !it->second->restReady) {
      return {};
    }
    request = it->second;
    requests.erase(it);
  }
  lastPlanInfo = request->rest.info;
  lastPlanPartial = false;
  return ToScript(request->rest);
}

int32_t FlightNavMap::PlanRejoinAsync(RED4ext::Vector4 aCar, RED4ext::Vector4 aVelocity, RED4ext::DynArray<RED4ext::Vector4> aLine,
                                      float aJoinAhead, RED4ext::Handle<FlightVehicleModel> aVehicle,
                                      RED4ext::DynArray<RED4ext::Vector4> aAvoid) {
  auto params = planParams;
  Navigation::RacePreset(params);
  if (aVehicle) {
    params.vehicle = aVehicle->Spec();
  }
  params.startVelocity = {aVelocity.X, aVelocity.Y, aVelocity.Z};
  for (const auto &a : aAvoid) {
    params.avoid.push_back({{a.X, a.Y, a.Z}, std::max(a.W, 1.0f), params.slotAvoidCost});
  }
  std::vector<Navigation::Vec3> line;
  for (const auto &p : aLine) {
    line.push_back({p.X, p.Y, p.Z});
  }
  Navigation::Vec3 car{aCar.X, aCar.Y, aCar.Z};
  auto request = std::make_shared<Request>();
  int32_t id;
  {
    std::lock_guard guard(requestsLock);
    id = nextRequest++;
    requests[id] = request;
  }
  std::thread([request, car, line, aJoinAhead, params]() {
    auto result = Navigation::PlanRejoin(Navigation::NavMap::Get(), car, line, aJoinAhead, params);
    spdlog::info("[NavMap] {}", result.info);
    std::lock_guard guard(requestsLock);
    request->result = std::move(result);
    request->ready = true;
  }).detach();
  return id;
}

namespace {
int32_t StartPlan(RED4ext::Vector4 aStart, RED4ext::Vector4 aGoal, Navigation::PlanParams params) {
  Navigation::Vec3 start{aStart.X, aStart.Y, aStart.Z}, goal{aGoal.X, aGoal.Y, aGoal.Z};
  auto request = std::make_shared<Request>();
  int32_t id;
  {
    std::lock_guard guard(requestsLock);
    id = nextRequest++;
    requests[id] = request;
  }
  std::thread([request, start, goal, params]() {
    auto result = Navigation::PlanRace(Navigation::NavMap::Get(), start, goal, params,
                                              [&](const Navigation::PlanResult &aAttempt) { LogPlan(start, goal, aAttempt); });
    std::lock_guard guard(requestsLock);
    request->result = std::move(result);
    request->ready = true;
  }).detach();
  return id;
}
} // namespace

bool FlightNavMap::IsPlanReady(int32_t aId) {
  std::lock_guard guard(requestsLock);
  auto it = requests.find(aId);
  return it != requests.end() && it->second->ready;
}

RED4ext::DynArray<RED4ext::Vector4> FlightNavMap::TakePlan(int32_t aId) {
  std::shared_ptr<Request> request;
  {
    std::lock_guard guard(requestsLock);
    auto it = requests.find(aId);
    if (it == requests.end() || !it->second->ready) {
      return {};
    }
    request = it->second;
    // a first leg's request stays for its rest (TakeRest)
    if (!request->result.partial) {
      requests.erase(it);
    }
  }
  lastPlanInfo = request->result.info;
  lastPlanPartial = request->result.partial;
  lastPlanJoin = request->joinS;
  return ToScript(request->result);
}

RED4ext::CString FlightNavMap::GetLastPlanInfo() { return {lastPlanInfo.c_str()}; }

bool FlightNavMap::GetLastPlanPartial() { return lastPlanPartial; }

float FlightNavMap::GetLastPlanJoin() { return lastPlanJoin; }

void FlightNavMap::SetTargetOverride(RED4ext::Vector4 aTarget) {
  targetOverride = aTarget;
  hasTargetOverride = true;
}

void FlightNavMap::ClearTargetOverride() { hasTargetOverride = false; }

bool FlightNavMap::HasTargetOverride() { return hasTargetOverride; }

RED4ext::Vector4 FlightNavMap::GetTargetOverride() { return targetOverride; }

int32_t FlightNavMap::GetHelipadCount() { return int32_t(Navigation::Helipads::Get().All().size()); }

RED4ext::Vector4 FlightNavMap::GetHelipadPosition(int32_t aIndex) {
  const auto *pad = PadAt(aIndex);
  return pad ? RED4ext::Vector4{pad->x, pad->y, pad->z, 1.0f} : RED4ext::Vector4{};
}

float FlightNavMap::GetHelipadYaw(int32_t aIndex) {
  const auto *pad = PadAt(aIndex);
  return pad ? pad->yaw : 0.0f;
}

float FlightNavMap::GetHelipadRadius(int32_t aIndex) {
  const auto *pad = PadAt(aIndex);
  return pad ? pad->radius : 0.0f;
}

float FlightNavMap::GetHelipadClearance(int32_t aIndex) {
  const auto *pad = PadAt(aIndex);
  return pad ? Finite(pad->clearance) : 0.0f;
}

RED4ext::CString FlightNavMap::GetHelipadId(int32_t aIndex) {
  const auto *pad = PadAt(aIndex);
  return {pad ? pad->id.c_str() : ""};
}

RED4ext::CString FlightNavMap::GetHelipadName(int32_t aIndex) {
  const auto *pad = PadAt(aIndex);
  return {pad ? pad->name.c_str() : ""};
}

int32_t FlightNavMap::FindNearestHelipad(RED4ext::Vector4 aPosition, float aMaxDistance) {
  return Navigation::Helipads::Get().Nearest(aPosition.X, aPosition.Y, aMaxDistance);
}

RED4ext::DynArray<int32_t> FlightNavMap::FindHelipadsInRange(RED4ext::Vector4 aPosition, float aRadius) {
  RED4ext::DynArray<int32_t> out;
  for (auto i : Navigation::Helipads::Get().InRange(aPosition.X, aPosition.Y, aRadius)) {
    out.EmplaceBack(i);
  }
  return out;
}

int32_t FlightNavMap::GetGateCount() { return int32_t(Navigation::Gates::Get().All().size()); }

RED4ext::Vector4 FlightNavMap::GetGatePosition(int32_t aIndex) {
  const auto *gate = GateAt(aIndex);
  return gate ? RED4ext::Vector4{gate->x, gate->y, gate->z, 1.0f} : RED4ext::Vector4{};
}

float FlightNavMap::GetGateYaw(int32_t aIndex) {
  const auto *gate = GateAt(aIndex);
  return gate ? gate->yaw : 0.0f;
}

RED4ext::DynArray<int32_t> FlightNavMap::FindGatesInRange(RED4ext::Vector4 aPosition, float aRadius) {
  RED4ext::DynArray<int32_t> out;
  for (auto i : Navigation::Gates::Get().InRange(aPosition.X, aPosition.Y, aRadius)) {
    out.EmplaceBack(i);
  }
  return out;
}

void FlightNavMap::SetPlanParameters(float aPreferredHeight, float aClimbCost, float aUnderCost,
                                     float aHeuristicWeight) {
  planParams.preferredHeight = aPreferredHeight;
  planParams.climbCost = aClimbCost;
  planParams.underCost = aUnderCost;
  planParams.heuristicWeight = std::max(1.0f, aHeuristicWeight);
}
