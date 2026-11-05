#include "Plans.hpp"

#include <algorithm>
#include <spdlog/spdlog.h>
#include <thread>

namespace Navigation {

namespace {
void LogPlan(Vec3 aStart, Vec3 aGoal, const PlanResult &aResult) {
  spdlog::info("[NavMap] plan ({:.0f}, {:.0f}, {:.0f}) -> ({:.0f}, {:.0f}, {:.0f}): {}", aStart.x, aStart.y, aStart.z,
               aGoal.x, aGoal.y, aGoal.z, aResult.info);
}

void ApplyShape(PlanParams &aParams, const RouteShape &aShape) {
  aParams.preferredHeight = aShape.preferredHeight;
  aParams.climbCost = aShape.climbCost;
  aParams.minPassage = aShape.minPassage;
  aParams.heuristicWeight = std::max(1.0f, aShape.heuristicWeight);
  aParams.turnCost = std::max(0.0f, aShape.turnCost);
}
} // namespace

PlanParams CarPlanParams(const PlanParams &aBase, const VehicleSpec *aVehicle, const std::vector<AvoidSpot> &aAvoid,
                         Vec3 aVelocity) {
  auto params = aBase;
  RacePreset(params);
  // the line is shaped for the car flying it (TurnCost)
  if (aVehicle) {
    params.vehicle = *aVehicle;
  }
  // a plan for a car already flying starts the way it is going
  params.startVelocity = aVelocity;
  for (const auto &a : aAvoid) {
    params.avoid.push_back({a.at, std::max(a.radius, 1.0f), params.slotAvoidCost});
  }
  return params;
}

PlanParams RaceParams(const PlanParams &aBase, const RouteShape &aShape, const VehicleSpec *aVehicle, bool aDetour,
                      const std::vector<AvoidSpot> &aAvoid, Vec3 aVelocity) {
  auto params = CarPlanParams(aBase, aVehicle, aAvoid, aVelocity);
  // slots too tight for the car's speed are routed round (PlanRace) only for a trajectory follower
  if (!aDetour) {
    params.slotDetourLoss = params.slotDetourLossMin = 1e9f;
  }
  ApplyShape(params, aShape);
  return params;
}

PlanParams FreeRoamParams(const PlanParams &aBase, const RouteShape &aShape, const FreeRoamShape &aFree,
                          const VehicleSpec *aVehicle, const std::vector<AvoidSpot> &aAvoid, Vec3 aVelocity) {
  auto params = RaceParams(aBase, aShape, aVehicle, true, aAvoid, aVelocity);
  params.lineHeightFree = aFree.heightFree;
  params.lineHeightWavelength = aFree.heightWavelength;
  params.overCost = std::max(0.0f, aFree.overCost);
  // the corridor is chosen without the doorway layer: a leg of a kilometre or so plans in under a
  // second without it
  params.doorways = false;
  return params;
}

int32_t PlanQueue::Add(const std::shared_ptr<Request> &aRequest) {
  std::lock_guard guard(lock);
  const int32_t id = next++;
  requests[id] = aRequest;
  return id;
}

template <typename F> void PlanQueue::Run(F &&aWork) {
  if (threaded) {
    std::thread(std::forward<F>(aWork)).detach();
  } else {
    aWork();
  }
}

int32_t PlanQueue::Race(Vec3 aStart, Vec3 aGoal, const PlanParams &aParams) {
  auto request = std::make_shared<Request>();
  const int32_t id = Add(request);
  Run([this, request, aStart, aGoal, aParams]() {
    auto result = PlanRace(map, aStart, aGoal, aParams, [&](const PlanResult &aAttempt) { LogPlan(aStart, aGoal, aAttempt); });
    std::lock_guard guard(lock);
    request->result = std::move(result);
    request->readyAt = now + delay;
    request->ready = true;
  });
  return id;
}

int32_t PlanQueue::Trip(Vec3 aStart, Vec3 aGoal, const PlanParams &aParams, AheadParams aAhead) {
  auto request = std::make_shared<Request>();
  const int32_t id = Add(request);
  {
    std::lock_guard guard(lock);
    aAhead.restGuess = restSeconds;
  }
  Run([this, request, aStart, aGoal, aParams, aAhead]() {
    auto leg = PlanFirstLeg(map, aStart, aGoal, aParams, aAhead);
    LogPlan(aStart, aGoal, leg.plan);
    const bool partial = leg.plan.partial;
    {
      std::lock_guard guard(lock);
      request->result = leg.plan;
      request->joinS = leg.joinS;
      request->readyAt = now + delay;
      request->ready = true;
    }
    if (!partial) {
      return;
    }
    auto rest = PlanRest(map, leg, aGoal, aParams);
    LogPlan(leg.joinPosition, aGoal, rest);
    std::lock_guard guard(lock);
    restSeconds = 0.5f * restSeconds + 0.5f * float(rest.milliseconds / 1000.0);
    request->rest = std::move(rest);
    request->restAt = request->readyAt + delay;
    request->restReady = true;
  });
  return id;
}

int32_t PlanQueue::Rejoin(Vec3 aCar, std::vector<Vec3> aLine, float aJoinAhead, const PlanParams &aParams) {
  auto request = std::make_shared<Request>();
  const int32_t id = Add(request);
  Run([this, request, aCar, line = std::move(aLine), aJoinAhead, aParams]() {
    auto result = PlanRejoin(map, aCar, line, aJoinAhead, aParams);
    spdlog::info("[NavMap] {}", result.info);
    std::lock_guard guard(lock);
    request->result = std::move(result);
    request->readyAt = now + delay;
    request->ready = true;
  });
  return id;
}

bool PlanQueue::Ready(int32_t aId) const {
  std::lock_guard guard(lock);
  auto it = requests.find(aId);
  return it != requests.end() && it->second->ready && now >= it->second->readyAt;
}

bool PlanQueue::Take(int32_t aId, TakenPlan &aOut) {
  std::lock_guard guard(lock);
  auto it = requests.find(aId);
  if (it == requests.end() || !it->second->ready || now < it->second->readyAt) {
    return false;
  }
  auto request = it->second;
  if (!request->result.partial) {
    requests.erase(it);
  }
  aOut.plan = request->result;
  aOut.joinS = request->joinS;
  return true;
}

bool PlanQueue::RestReady(int32_t aId) const {
  std::lock_guard guard(lock);
  auto it = requests.find(aId);
  return it != requests.end() && it->second->restReady && now >= it->second->restAt;
}

bool PlanQueue::TakeRest(int32_t aId, PlanResult &aOut) {
  std::lock_guard guard(lock);
  auto it = requests.find(aId);
  if (it == requests.end() || !it->second->restReady || now < it->second->restAt) {
    return false;
  }
  aOut = std::move(it->second->rest);
  requests.erase(it);
  return true;
}

void PlanQueue::Drop(int32_t aId) {
  std::lock_guard guard(lock);
  requests.erase(aId);
}

size_t PlanQueue::Pending() const {
  std::lock_guard guard(lock);
  return requests.size();
}

void PlanQueue::Advance(float aSeconds) {
  std::lock_guard guard(lock);
  now += aSeconds;
}

} // namespace Navigation
