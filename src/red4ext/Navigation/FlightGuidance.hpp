#pragma once

// FlightGuidance: the flight route shown to the player, for other mods to draw (a navigation mod's
// path on the minimap or in the world; docs/design/guidance.md, src/redscript/Navigation/FlightGuidance.reds).
// While the player's vehicle flies with something tracked it holds the line from the car to the
// destination: the Autopilot's own route while it is engaged, else one planned for flying by hand
// (Navigation::Guidance). Updated once a frame by FlightSystem; read from any thread.
//
// Detecting LTBF from another mod: the class FlightGuidance exists (redscript
// `@if(ModuleExists("LetThereBeFlight"))`, or CRTTISystem::GetClass("FlightGuidance") from a plugin).

#include <RED4ext/Common.hpp>
#include <RED4ext/RTTITypes.hpp>
#include <RED4ext/Scripting/IScriptable.hpp>
#include <RED4ext/Scripting/Natives/Generated/Vector4.hpp>
#include <RedLib.hpp>

struct FlightGuidance : RED4ext::IScriptable {
  // The player's vehicle is flying (flight on, not only mounted).
  static bool IsFlying();
  // There is a flight route to show: flying, something tracked, and a route planned to it.
  static bool HasRoute();
  // The route is the Autopilot's own (it is engaged); false when planned for flying by hand, or none.
  static bool IsAutopilotRoute();
  // The route's world points (metres, W 1), car end first: the point on the line nearest the car,
  // then the line's points after it, about 5 m apart, to the destination end (which is the planner's
  // goal: over the destination, or above a landing pad's deck). Empty when HasRoute is false.
  static RED4ext::DynArray<RED4ext::Vector4> GetRoute();
  // Changes whenever the route is replaced (a new plan, a rejoin, the Autopilot's replan), not as
  // the car moves along it. 0 before the first.
  static int32_t GetRouteRevision();
  // m from the car to the route (how far it has strayed); 0 without a route.
  static float GetDistanceToRoute();

  // For C++ (FlightSystem): one frame of guidance, and everything forgotten when the world detaches.
  static void Update(float aTimeDelta);
  static void Reset();

  RTTI_IMPL_TYPEINFO(FlightGuidance);
  RTTI_IMPL_ALLOCATOR();
};

RTTI_DEFINE_CLASS(FlightGuidance, {
  RTTI_METHOD(IsFlying);
  RTTI_METHOD(HasRoute);
  RTTI_METHOD(IsAutopilotRoute);
  RTTI_METHOD(GetRoute);
  RTTI_METHOD(GetRouteRevision);
  RTTI_METHOD(GetDistanceToRoute);
});
