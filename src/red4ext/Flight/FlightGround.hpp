#pragma once

// FlightComponent.FindGround in C++ (docs/design/hover-fly-native.md, step 3): the same four rays, one straight down
// from each wheel emitter (Flight::GroundRayEnd), through the same engine calls the script makes
// (gameSpatialQueriesSystem.SyncRaycastByQueryFilter with VehicleBlocker and Water, TraceResult.IsValid), averaged by
// Flight::GroundFromHits. It reads the component's script fields by name (fl_tire, fr_tire, bl_tire, br_tire, sqs,
// distance) and, as the script does, writes distance only when the ground is found.

#include "Flight/Modes.hpp"

#include <RED4ext/Scripting/IScriptable.hpp>
#include <RED4ext/Scripting/Natives/Generated/Vector4.hpp>
#include <RedLib.hpp>

namespace FlightGroundNative {

// The ground under aComponent (a FlightComponent); not found when a field or an engine function is missing (warned
// once) or any ray misses. aEmitters, when given, gets the four wheel emitters' mean position (world), the rays' start;
// aHits each ray, its emitter and its hit (world: front left, front right, back left, back right).
Flight::Ground Find(RED4ext::IScriptable *aComponent, float aMaxHoverHeight, Flight::Vec3 *aEmitters = nullptr,
                    Flight::Hit *aHits = nullptr);

} // namespace FlightGroundNative

// For scripts and the bridge: the native ground next to the script's (LtbfBridge compares them in game).
struct FlightGround : RED4ext::IScriptable {
  // the normal in X Y Z and the distance in W; W is -1 when nothing is found
  static RED4ext::Vector4 Find(RED4ext::Handle<RED4ext::IScriptable> aComponent);

  RTTI_IMPL_TYPEINFO(FlightGround);
  RTTI_IMPL_ALLOCATOR();
};

RTTI_DEFINE_CLASS(FlightGround, { RTTI_METHOD(Find); });
