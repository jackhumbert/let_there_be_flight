#include "FlightNavMap.hpp"
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

void LogPlan(Navigation::Vec3 aStart, Navigation::Vec3 aGoal, const Navigation::PlanResult &aResult) {
  spdlog::info("[NavMap] plan ({:.0f}, {:.0f}, {:.0f}) -> ({:.0f}, {:.0f}, {:.0f}): {}", aStart.x, aStart.y,
               aStart.z, aGoal.x, aGoal.y, aGoal.z, aResult.info);
}
} // namespace

bool FlightNavMap::IsLoaded() { return Navigation::NavMap::Get().IsLoaded(); }

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

int32_t FlightNavMap::PlanAsync(RED4ext::Vector4 aStart, RED4ext::Vector4 aGoal, bool aDirect) {
  auto params = planParams;
  params.tryDirect = aDirect;
  Navigation::Vec3 start{aStart.X, aStart.Y, aStart.Z}, goal{aGoal.X, aGoal.Y, aGoal.Z};
  auto request = std::make_shared<Request>();
  int32_t id;
  {
    std::lock_guard guard(requestsLock);
    id = nextRequest++;
    requests[id] = request;
  }
  std::thread([request, start, goal, params]() {
    auto result = Navigation::Plan(Navigation::NavMap::Get(), start, goal, params);
    LogPlan(start, goal, result);
    std::lock_guard guard(requestsLock);
    request->result = std::move(result);
    request->ready = true;
  }).detach();
  return id;
}

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

void FlightNavMap::SetPlanParameters(float aPreferredHeight, float aClimbCost, float aUnderCost,
                                     float aHeuristicWeight) {
  planParams.preferredHeight = aPreferredHeight;
  planParams.climbCost = aClimbCost;
  planParams.underCost = aUnderCost;
  planParams.heuristicWeight = std::max(1.0f, aHeuristicWeight);
}
