#pragma once

// FlightNavMap: script access to Navigation::NavMap (src/redscript/Navigation/FlightNavMap.reds).

#include <RED4ext/Common.hpp>
#include <RED4ext/RTTITypes.hpp>
#include <RED4ext/Scripting/IScriptable.hpp>
#include <RED4ext/Scripting/Natives/Generated/Vector2.hpp>
#include <RED4ext/Scripting/Natives/Generated/Vector4.hpp>
#include <RedLib.hpp>

struct FlightNavMap : RED4ext::IScriptable {
  static bool IsLoaded();
  // Half height of the car cylinder the bake grew solids by: a gap's bottom is this far above the
  // surface under it.
  static float GetCarHalfHeight();
  // Gaps of the column at the position, bottom up, as (bottom, top); top is 1e9 for open sky.
  static RED4ext::DynArray<RED4ext::Vector2> GetGaps(RED4ext::Vector4 aPosition);
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
  static void SetPlanParameters(float aPreferredHeight, float aClimbCost, float aUnderCost, float aHeuristicWeight);

  // Same search on a worker thread: returns a request id (> 0). IsPlanReady polls it; TakePlan
  // returns the points once (empty if it failed or is unknown) and forgets the request, with
  // its info string in GetLastPlanInfo.
  static int32_t PlanAsync(RED4ext::Vector4 aStart, RED4ext::Vector4 aGoal, bool aDirect);
  // PlanAsync with the racing line (Planner.hpp): the Autopilot's Race option
  static int32_t PlanRaceAsync(RED4ext::Vector4 aStart, RED4ext::Vector4 aGoal, float aPreferredHeight, float aClimbCost,
                               float aMinPassage, float aHeuristicWeight);
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

  RTTI_IMPL_TYPEINFO(FlightNavMap);
  RTTI_IMPL_ALLOCATOR();
};

RTTI_DEFINE_CLASS(FlightNavMap, {
  RTTI_METHOD(IsLoaded);
  RTTI_METHOD(GetCarHalfHeight);
  RTTI_METHOD(GetGaps);
  RTTI_METHOD(IsFree);
  RTTI_METHOD(GetFloor);
  RTTI_METHOD(GetCeiling);
  RTTI_METHOD(SegmentClear);
  RTTI_METHOD(Plan);
  RTTI_METHOD(GetLastPlanInfo);
  RTTI_METHOD(SetPlanParameters);
  RTTI_METHOD(PlanAsync);
  RTTI_METHOD(PlanRaceAsync);
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
});
