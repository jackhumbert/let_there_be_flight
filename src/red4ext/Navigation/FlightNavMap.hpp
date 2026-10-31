#pragma once

// FlightNavMap: script access to Navigation::NavMap (src/redscript/Navigation/FlightNavMap.reds).

#include <RED4ext/Common.hpp>
#include <RED4ext/RTTITypes.hpp>
#include <RED4ext/Scripting/IScriptable.hpp>
#include <RED4ext/Scripting/Natives/Generated/Vector2.hpp>
#include <RED4ext/Scripting/Natives/Generated/Vector4.hpp>
#include <RedLib.hpp>

#include "FlightVehicleModel.hpp"

struct FlightNavMap : RED4ext::IScriptable {
  static bool IsLoaded();
  // Half height of the car cylinder the bake grew solids by: a gap's bottom is this far above the
  // surface under it.
  static float GetCarHalfHeight();
  // Gaps of the column at the position, bottom up, as (bottom, top); top is 1e9 for open sky.
  static RED4ext::DynArray<RED4ext::Vector2> GetGaps(RED4ext::Vector4 aPosition);
  static RED4ext::DynArray<RED4ext::Vector4> GetOutlines(RED4ext::Vector4 aCentre, float aRadius,
                                                         RED4ext::DynArray<float> aHeights);
  static RED4ext::DynArray<RED4ext::Vector4> GetNavEdges(RED4ext::Vector4 aCentre, float aRadius, float aZMin,
                                                         float aZMax);
  static RED4ext::DynArray<RED4ext::Vector4> GetNavBlocks(RED4ext::Vector4 aCentre, float aRadius, float aZMin,
                                                          float aZMax);
  static bool IsFree(RED4ext::Vector4 aPosition);
  // Bottom / top of the gap containing the position; -1e9 when the position is blocked.
  static float GetFloor(RED4ext::Vector4 aPosition);
  static float GetCeiling(RED4ext::Vector4 aPosition);
  // Fraction of the way from a to b that is free before the first blocked sample.
  static float SegmentClear(RED4ext::Vector4 aFrom, RED4ext::Vector4 aTo);

  // Route from start to goal as car-centre corner points (empty when there is none). Runs on
  // the calling thread. GetLastPlanInfo describes the last call (method, length, time).
  static RED4ext::DynArray<RED4ext::Vector4> Plan(RED4ext::Vector4 aStart, RED4ext::Vector4 aGoal, bool aDirect);
  static RED4ext::CString GetLastPlanInfo();
  // the last plan TakePlan returned is a first leg (PlanTripAsync): it ends short of the goal, and the
  // rest joins it GetLastPlanJoin metres along
  static bool GetLastPlanPartial();
  static float GetLastPlanJoin();
  static void SetPlanParameters(float aPreferredHeight, float aClimbCost, float aUnderCost, float aHeuristicWeight);

  // Same search on a worker thread: returns a request id (> 0). IsPlanReady polls it; TakePlan
  // returns the points once (empty if it failed or is unknown) and forgets the request, with
  // its info string in GetLastPlanInfo.
  static int32_t PlanAsync(RED4ext::Vector4 aStart, RED4ext::Vector4 aGoal, bool aDirect);
  // PlanAsync with the Race's route character (RacePreset), slot detours and the start velocity: the
  // Autopilot's plan
  static int32_t PlanRaceAsync(RED4ext::Vector4 aStart, RED4ext::Vector4 aGoal, float aPreferredHeight, float aClimbCost,
                               float aMinPassage, float aHeuristicWeight, float aTurnCost,
                               RED4ext::Handle<FlightVehicleModel> aVehicle, bool aDetour,
                               RED4ext::DynArray<RED4ext::Vector4> aAvoid, RED4ext::Vector4 aVelocity);
  // A free roam leg: PlanRaceAsync flying lower and round buildings rather than over them: the line's
  // lineHeightFree / lineHeightWavelength replaced (aHeightFree, aHeightWavelength), overCost per
  // metre over a building (aOverCost), and no doorway layer (it chooses the corridor without that
  // cost; a leg of a kilometre or so plans in under a second without it)
  static int32_t PlanFreeRoamAsync(RED4ext::Vector4 aStart, RED4ext::Vector4 aGoal, float aPreferredHeight, float aClimbCost,
                                   float aMinPassage, float aHeuristicWeight, float aTurnCost,
                                   RED4ext::Handle<FlightVehicleModel> aVehicle, RED4ext::DynArray<RED4ext::Vector4> aAvoid,
                                   RED4ext::Vector4 aVelocity, float aHeightFree, float aHeightWavelength, float aOverCost);
  // PlanRaceAsync planned ahead (Navigation/PlanAhead.hpp): a first leg aHorizon metres along the
  // doorway route (trips over aMinTrip), ready for TakePlan at once, and the rest from where the
  // car will be on it aMargin seconds after the rest's plan is expected, planned straight after on
  // the same thread: IsRestReady / TakeRest. A shorter trip is one plan, as PlanRaceAsync.
  static int32_t PlanTripAsync(RED4ext::Vector4 aStart, RED4ext::Vector4 aGoal, float aPreferredHeight, float aClimbCost,
                               float aMinPassage, float aHeuristicWeight, float aTurnCost,
                               RED4ext::Handle<FlightVehicleModel> aVehicle, RED4ext::DynArray<RED4ext::Vector4> aAvoid,
                               RED4ext::Vector4 aVelocity, float aHorizon, float aMargin, float aMinTrip);
  static bool IsRestReady(int32_t aId);
  // the rest's line (it starts at the join), once; empty when it failed. Its info in GetLastPlanInfo.
  static RED4ext::DynArray<RED4ext::Vector4> TakeRest(int32_t aId);
  // back onto the route's line after leaving it: a connector from aCar (flying at aVelocity) to the
  // line joinAhead metres on, then the rest of aLine (the line from about the car's place on it)
  static int32_t PlanRejoinAsync(RED4ext::Vector4 aCar, RED4ext::Vector4 aVelocity, RED4ext::DynArray<RED4ext::Vector4> aLine,
                                 float aJoinAhead, RED4ext::Handle<FlightVehicleModel> aVehicle,
                                 RED4ext::DynArray<RED4ext::Vector4> aAvoid);
  static bool IsPlanReady(int32_t aId);
  static RED4ext::DynArray<RED4ext::Vector4> TakePlan(int32_t aId);

  // Destination override for the autopilot (testing, or other mods): wins over the GPS target.
  static void SetTargetOverride(RED4ext::Vector4 aTarget);
  static void ClearTargetOverride();
  static bool HasTargetOverride();
  static RED4ext::Vector4 GetTargetOverride();

  // Landing pads (Navigation::Helipads, helipads.tsv), by index 0 .. GetHelipadCount() - 1.
  // Position is the deck centre a car rests on; an index out of range gives zeros / "".
  static int32_t GetHelipadCount();
  static RED4ext::Vector4 GetHelipadPosition(int32_t aIndex);
  static float GetHelipadYaw(int32_t aIndex);
  static float GetHelipadRadius(int32_t aIndex);
  // Free height above the deck; 1e9 for open sky.
  static float GetHelipadClearance(int32_t aIndex);
  static RED4ext::CString GetHelipadId(int32_t aIndex);
  static RED4ext::CString GetHelipadName(int32_t aIndex);
  // Nearest pad within aMaxDistance metres (horizontal) of the position, or -1.
  static int32_t FindNearestHelipad(RED4ext::Vector4 aPosition, float aMaxDistance);
  // Pads within aRadius metres (horizontal), nearest first.
  static RED4ext::DynArray<int32_t> FindHelipadsInRange(RED4ext::Vector4 aPosition, float aRadius);

  // Free roam gates (Navigation::Gates, gates.tsv), by index 0 .. GetGateCount() - 1. Position is
  // the opening's centre at the height to fly it; an index out of range gives zeros.
  static int32_t GetGateCount();
  static RED4ext::Vector4 GetGatePosition(int32_t aIndex);
  // Degrees, the way through (either direction).
  static float GetGateYaw(int32_t aIndex);
  // Gates within aRadius metres (horizontal), nearest first.
  static RED4ext::DynArray<int32_t> FindGatesInRange(RED4ext::Vector4 aPosition, float aRadius);

  RTTI_IMPL_TYPEINFO(FlightNavMap);
  RTTI_IMPL_ALLOCATOR();
};

RTTI_DEFINE_CLASS(FlightNavMap, {
  RTTI_METHOD(IsLoaded);
  RTTI_METHOD(GetCarHalfHeight);
  RTTI_METHOD(GetGaps);
  RTTI_METHOD(GetOutlines);
  RTTI_METHOD(GetNavEdges);
  RTTI_METHOD(GetNavBlocks);
  RTTI_METHOD(IsFree);
  RTTI_METHOD(GetFloor);
  RTTI_METHOD(GetCeiling);
  RTTI_METHOD(SegmentClear);
  RTTI_METHOD(Plan);
  RTTI_METHOD(GetLastPlanInfo);
  RTTI_METHOD(GetLastPlanPartial);
  RTTI_METHOD(GetLastPlanJoin);
  RTTI_METHOD(PlanTripAsync);
  RTTI_METHOD(PlanFreeRoamAsync);
  RTTI_METHOD(IsRestReady);
  RTTI_METHOD(TakeRest);
  RTTI_METHOD(SetPlanParameters);
  RTTI_METHOD(PlanAsync);
  RTTI_METHOD(PlanRaceAsync);
  RTTI_METHOD(PlanRejoinAsync);
  RTTI_METHOD(IsPlanReady);
  RTTI_METHOD(TakePlan);
  RTTI_METHOD(SetTargetOverride);
  RTTI_METHOD(ClearTargetOverride);
  RTTI_METHOD(HasTargetOverride);
  RTTI_METHOD(GetTargetOverride);
  RTTI_METHOD(GetHelipadCount);
  RTTI_METHOD(GetHelipadPosition);
  RTTI_METHOD(GetHelipadYaw);
  RTTI_METHOD(GetHelipadRadius);
  RTTI_METHOD(GetHelipadClearance);
  RTTI_METHOD(GetHelipadId);
  RTTI_METHOD(GetHelipadName);
  RTTI_METHOD(FindNearestHelipad);
  RTTI_METHOD(FindHelipadsInRange);
  RTTI_METHOD(GetGateCount);
  RTTI_METHOD(GetGatePosition);
  RTTI_METHOD(GetGateYaw);
  RTTI_METHOD(FindGatesInRange);
});
