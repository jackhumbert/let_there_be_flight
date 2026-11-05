#pragma once

// The Autopilot's route plans (docs/design/native-ports.md, M6): the planner's params for each kind
// of plan, and the requests they run as. In the game a PlanQueue runs each request on its own thread
// (FlightNavMap's Plan*Async); offline it plans at once and hands the plan over after a modelled
// delay, so a simulated flight waits for its plans as the game's does.

#include "NavMap.hpp"
#include "PlanAhead.hpp"
#include "Planner.hpp"
#include "Vehicle.hpp"

#include <cstdint>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace Navigation {

// The route's character from the Autopilot's race route fields (raceRouteHeight, raceClimbCost,
// raceMinPassage, raceSearchWeight, raceTurnCost; freeRoamRouteHeight for free roam).
struct RouteShape {
  float preferredHeight = 0.0f;
  float climbCost = 0.0f;
  float minPassage = 0.0f;
  float heuristicWeight = 1.0f;
  float turnCost = 0.0f;
};

// A free roam leg's line: lower and round buildings rather than over them (PlanParams'
// lineHeightFree, lineHeightWavelength and overCost).
struct FreeRoamShape {
  float heightFree = 0.0f;
  float heightWavelength = 0.0f;
  float overCost = 0.0f;
};

// A place the car hit something lately, routed round like a tight slot.
struct AvoidSpot {
  Vec3 at;
  float radius;
};

// The planner's params for a plan of the car flying at aVelocity: aBase with the Race's preset
// (RacePreset), the car's limits when given, and aAvoid.
PlanParams CarPlanParams(const PlanParams &aBase, const VehicleSpec *aVehicle, const std::vector<AvoidSpot> &aAvoid,
                         Vec3 aVelocity);
// A route to a destination (PlanRaceAsync, PlanTripAsync): CarPlanParams with the route's character.
// aDetour false: slots too tight for the car's speed are not routed round.
PlanParams RaceParams(const PlanParams &aBase, const RouteShape &aShape, const VehicleSpec *aVehicle, bool aDetour,
                      const std::vector<AvoidSpot> &aAvoid, Vec3 aVelocity);
// A free roam leg (PlanFreeRoamAsync): RaceParams with the free roam line and no doorway layer.
PlanParams FreeRoamParams(const PlanParams &aBase, const RouteShape &aShape, const FreeRoamShape &aFree,
                          const VehicleSpec *aVehicle, const std::vector<AvoidSpot> &aAvoid, Vec3 aVelocity);

// A plan taken from a request: the plan, and for a first leg (plan.partial) where its rest joins it.
struct TakenPlan {
  PlanResult plan;
  float joinS = 0.0f;
};

// Plan requests by id (> 0). Each is taken once; a first leg's request stays for its rest.
class PlanQueue {
public:
  // aThreaded: each request plans on a thread of its own (the game; the queue must outlive them, as
  // a static does). Otherwise it plans when asked and is ready aDelay seconds of Advance later, its
  // rest aDelay after that.
  PlanQueue(const NavMap &aMap, bool aThreaded, float aDelay = 0.0f) : map(aMap), threaded(aThreaded), delay(aDelay) {}

  int32_t Race(Vec3 aStart, Vec3 aGoal, const PlanParams &aParams);
  int32_t Trip(Vec3 aStart, Vec3 aGoal, const PlanParams &aParams, AheadParams aAhead);
  int32_t Rejoin(Vec3 aCar, std::vector<Vec3> aLine, float aJoinAhead, const PlanParams &aParams);

  bool Ready(int32_t aId) const;
  // the plan, once; false while it is not ready, or for an unknown id
  bool Take(int32_t aId, TakenPlan &aOut);
  bool RestReady(int32_t aId) const;
  bool TakeRest(int32_t aId, PlanResult &aOut);
  // forgets a request no longer wanted (its plan, if still running, is freed when it finishes)
  void Drop(int32_t aId);
  // requests not yet taken or dropped
  size_t Pending() const;

  // offline: the clock the delay runs on
  void Advance(float aSeconds);

private:
  struct Request {
    bool ready = false;
    PlanResult result;
    float joinS = 0.0f;
    bool restReady = false;
    PlanResult rest;
    double readyAt = 0.0, restAt = 0.0;
  };
  int32_t Add(const std::shared_ptr<Request> &aRequest);
  template <typename F> void Run(F &&aWork);

  const NavMap &map;
  const bool threaded;
  const float delay;
  double now = 0.0;
  // s the rest of a trip planned ahead took last (eased): when its join is due
  float restSeconds = 2.0f;
  mutable std::mutex lock;
  std::unordered_map<int32_t, std::shared_ptr<Request>> requests;
  int32_t next = 1;
};

} // namespace Navigation
