#pragma once

// The route the Autopilot follows, as script sees it: a handle over Navigation::Route (Route.hpp), which
// holds the points and their distances and does the geometry the follower asks every frame (the point
// at a distance, the point some metres on from the car, the bend ahead). This converts script vectors.
// The car's progress (the segment it is on) stays with the Autopilot and is passed in.

#include "Navigation/Route.hpp"

#include <RED4ext/Scripting/IScriptable.hpp>
#include <RED4ext/Scripting/Natives/Generated/Vector4.hpp>
#include <RedLib.hpp>

#include <vector>

struct FlightRoute : RED4ext::IScriptable {
  Navigation::Route route;

  // the route through `points` (empty for fewer than two)
  static RED4ext::Handle<FlightRoute> Create(RED4ext::DynArray<RED4ext::Vector4> aPoints);
  // The planner's straight legs with each corner replaced by a quadratic Bezier arc, cut back up to
  // `cut` metres and at most `share` of each leg, kept only where the arc is free on the nav map.
  // The cut steps down by `step` until the arc is clear or it is below `smallest`; the corner
  // stays sharp then.
  static RED4ext::DynArray<RED4ext::Vector4> RoundCorners(RED4ext::DynArray<RED4ext::Vector4> aPoints, float aCut,
                                                          float aShare, float aSmallest, float aStep);
  // `points` with those from index lo to hi within `radius` of point `join` eased towards their
  // neighbours, `passes` times; a point moves only where both its segments stay free on the map.
  static RED4ext::DynArray<RED4ext::Vector4> SmoothJoin(RED4ext::DynArray<RED4ext::Vector4> aPoints, int32_t aJoin,
                                                        int32_t aLo, int32_t aHi, float aRadius, int32_t aPasses);

  int32_t Size() { return route.Size(); }
  RED4ext::Vector4 Point(int32_t aIndex);
  float Distance(int32_t aIndex);
  float Length() { return route.Length(); }
  RED4ext::DynArray<RED4ext::Vector4> Points();

  // The segment nearest p from segment `from` over the next `count` (the car's segment advances).
  int32_t Nearest(RED4ext::Vector4 aP, int32_t aFrom, int32_t aCount);
  // the point on segment `segment` nearest p
  RED4ext::Vector4 Project(RED4ext::Vector4 aP, int32_t aSegment);
  // distance from q to the route from `segment` on, over `ahead` metres past the segment's start
  float DistanceTo(RED4ext::Vector4 aQ, int32_t aSegment, float aAhead);
  // point at route distance s (clamped to the ends)
  RED4ext::Vector4 At(float aS);
  // point `along` metres on from `from`, which lies on segment `segment`
  RED4ext::Vector4 Ahead(int32_t aSegment, RED4ext::Vector4 aFrom, float aAlong);
  // the route from `from` (on segment `segment`) is free on the nav map for `along` metres
  bool ClearAhead(int32_t aSegment, RED4ext::Vector4 aFrom, float aAlong);
  // Signed horizontal curvature around the lookahead: the heading change between the stretch from
  // half to the whole of `ahead` and the stretch from there to one and a half, over the distance
  // between (positive turning right).
  float Turn(int32_t aSegment, RED4ext::Vector4 aFrom, float aAhead);
  // curvature over the first part of the lookahead, 3D: towards the bend's centre, 1 / radius long
  RED4ext::Vector4 Curvature(int32_t aSegment, RED4ext::Vector4 aFrom, float aAhead);
  // the direction at route distance s, taken over `span` either side (zero where that is too short)
  RED4ext::Vector4 Tangent(float aS, float aSpan);
  // the bend at route distance s over `span` either side: towards the centre, 1 / radius long
  RED4ext::Vector4 Bend(float aS, float aSpan);
  // Route distance from `from` (on segment `segment`) to the next point within 300 m where the
  // direction `span` before and after it differs by more than 25 degrees (a big number for none).
  float SharpBend(int32_t aSegment, RED4ext::Vector4 aFrom, float aSpan);

  RTTI_IMPL_TYPEINFO(FlightRoute);
  RTTI_IMPL_ALLOCATOR();
};

RTTI_DEFINE_CLASS(FlightRoute, {
  RTTI_METHOD(Create);
  RTTI_METHOD(RoundCorners);
  RTTI_METHOD(SmoothJoin);
  RTTI_METHOD(Size);
  RTTI_METHOD(Point);
  RTTI_METHOD(Distance);
  RTTI_METHOD(Length);
  RTTI_METHOD(Points);
  RTTI_METHOD(Nearest);
  RTTI_METHOD(Project);
  RTTI_METHOD(DistanceTo);
  RTTI_METHOD(At);
  RTTI_METHOD(Ahead);
  RTTI_METHOD(ClearAhead);
  RTTI_METHOD(Turn);
  RTTI_METHOD(Curvature);
  RTTI_METHOD(Tangent);
  RTTI_METHOD(Bend);
  RTTI_METHOD(SharpBend);
});
