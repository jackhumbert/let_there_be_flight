#pragma once

// FlightComponent.FindGround in C++ (docs/design/hover-fly-native.md, step 3): the same four rays, one straight down
// from each wheel emitter (Flight::GroundRayEnd) but starting over it and seeing only the static world
// (Flight::GroundRayStart, so a bike's emitters under the road's surface still find it; the script's start at the
// emitters and see cars too), through the same engine calls the script makes
// (gameSpatialQueriesSystem.SyncRaycastByQueryFilter with VehicleBlocker and Water, TraceResult.IsValid), averaged by
// Flight::GroundFromHits. It reads the component's script fields by name (fl_tire, fr_tire, bl_tire, br_tire, sqs,
// distance) and, as the script does, writes distance only when the ground is found.

#include "Flight/Modes.hpp"

#include <RED4ext/Scripting/IScriptable.hpp>
#include <RED4ext/Scripting/Natives/Generated/Vector4.hpp>
#include <RedLib.hpp>

class FlightComponent;

namespace FlightGroundNative {

// The ground under aComponent (a FlightComponent); not found when a field or an engine function is missing (warned
// once) or any ray misses. aEmitters, when given, gets the four wheel emitters' mean position (world), the rays' start;
// aHits each ray, its emitter and its hit (world: front left, front right, back left, back right).
Flight::Ground Find(RED4ext::IScriptable *aComponent, float aMaxHoverHeight, Flight::Vec3 *aEmitters = nullptr,
                    Flight::Hit *aHits = nullptr);

// Each thruster on the ground under it, from Find's four hits, into aComponent->skidContacts (their scrapes and the
// Drone skid read them): its clearance (the ray's length less its emitter's height over the car's lowest point: a
// thruster's box is open at the bottom, so on the ground it takes no contact and this is how its touch is known), the
// ground's normal, how fast the thruster slides along it, what the ground is and the hit. A contact's load is left as it
// was. aPoints, when given, gets each as the skid's point. False before the car's shape is measured.
bool Touches(FlightComponent *aComponent, const Flight::StatsState &aState, const Flight::Hit (&aHits)[4],
             Flight::SkidPoint *aPoints = nullptr);

} // namespace FlightGroundNative

// For scripts and tests: the native ground next to the script's (tests compare them in game).
struct FlightGround : RED4ext::IScriptable {
  // the normal in X Y Z and the distance in W; W is -1 when nothing is found
  static RED4ext::Vector4 Find(RED4ext::Handle<RED4ext::IScriptable> aComponent);

  RTTI_IMPL_TYPEINFO(FlightGround);
  RTTI_IMPL_ALLOCATOR();
};

RTTI_DEFINE_CLASS(FlightGround, { RTTI_METHOD(Find); });
