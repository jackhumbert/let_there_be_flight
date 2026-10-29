#include "FlightNavMap.hpp"
#include "Gates.hpp"
#include "Helipads.hpp"
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

struct Request {
  bool ready = false;
  Navigation::PlanResult result;
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
                                    float aClimbCost, float aMinPassage, float aHeuristicWeight, float aTurnCost) {
  auto params = planParams;
  params.racingLine = true;
  // the route's character comes from the Autopilot (FlightModeAutopilot race route fields)
  params.preferredHeight = aPreferredHeight;
  params.climbCost = aClimbCost;
  params.minPassage = aMinPassage;
  params.heuristicWeight = std::max(1.0f, aHeuristicWeight);
  params.turnCost = std::max(0.0f, aTurnCost);
  // a climb's cost grows exponentially with its rise (ClimbCost): round the Dogtown stadium, not
  // over it (Jack, 2026-10-29); the city's small rises cost about what they did. The baked doorways
  // hold the old step costs, so they are worked out again at first use until the map is rebaked.
  params.climbScale = 30.0f;
  return StartPlan(aStart, aGoal, params);
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
    auto result = Navigation::PlanWithRetries(Navigation::NavMap::Get(), start, goal, params,
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
    requests.erase(it);
  }
  lastPlanInfo = request->result.info;
  return ToScript(request->result);
}

RED4ext::CString FlightNavMap::GetLastPlanInfo() { return {lastPlanInfo.c_str()}; }

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
