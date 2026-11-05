#pragma once

// The route the Autopilot follows: its points, the distance of each from the start, and the
// geometry the follower asks of it every frame (the point at a distance, the point some metres on
// from the car, the bend ahead). The car's progress (the segment it is on) stays with the
// Autopilot and is passed in. Pure code.

#include "Planner.hpp"
#include "Space.hpp"

#include <cstdint>
#include <vector>

namespace Navigation {

struct Route {
  std::vector<Vec3> points;
  std::vector<float> lengths; // route distance of each point from the start

  // the route through aPoints (empty for fewer than two)
  static Route Make(std::vector<Vec3> aPoints);

  int32_t Size() const { return int32_t(points.size()); }
  Vec3 Point(int32_t aIndex) const;
  float Distance(int32_t aIndex) const;
  float Length() const { return lengths.empty() ? 0.0f : lengths.back(); }

  // The segment nearest p from segment `from` over the next `count` (the car's segment advances).
  int32_t Nearest(const Vec3 &aP, int32_t aFrom, int32_t aCount) const;
  // the point on segment `segment` nearest p
  Vec3 Project(const Vec3 &aP, int32_t aSegment) const;
  // distance from q to the route from `segment` on, over `ahead` metres past the segment's start
  float DistanceTo(const Vec3 &aQ, int32_t aSegment, float aAhead) const;
  // point at route distance s (clamped to the ends)
  Vec3 At(float aS) const;
  // point `along` metres on from `from`, which lies on segment `segment`
  Vec3 Ahead(int32_t aSegment, const Vec3 &aFrom, float aAlong) const;
  // the route from `from` (on segment `segment`) is free in aSpace for `along` metres
  bool ClearAhead(int32_t aSegment, const Vec3 &aFrom, float aAlong, const Space &aSpace) const;
  // Signed horizontal curvature around the lookahead: the heading change between the stretch from
  // half to the whole of `ahead` and the stretch from there to one and a half, over the distance
  // between (positive turning right).
  float Turn(int32_t aSegment, const Vec3 &aFrom, float aAhead) const;
  // curvature over the first part of the lookahead, 3D: towards the bend's centre, 1 / radius long
  Vec3 Curvature(int32_t aSegment, const Vec3 &aFrom, float aAhead) const;
  // the direction at route distance s, taken over `span` either side (zero where that is too short)
  Vec3 Tangent(float aS, float aSpan) const;
  // the bend at route distance s over `span` either side: towards the centre, 1 / radius long
  Vec3 Bend(float aS, float aSpan) const;
  // Route distance from `from` (on segment `segment`) to the next point within 300 m where the
  // direction `span` before and after it differs by more than 25 degrees (a big number for none).
  float SharpBend(int32_t aSegment, const Vec3 &aFrom, float aSpan) const;
};

// The planner's straight legs with each corner replaced by a quadratic Bezier arc of nine points, cut
// back min(aShare of each leg, aCut) metres; where the arc is not clear in aSpace the cut steps down by
// aStep until it is, and below aSmallest the corner stays sharp. Fewer than three points come back as
// they are. (The Autopilot's RoundCorners passes raceCornerRound,
// raceRoundShare, raceRoundMin and raceRoundStep.)
std::vector<Vec3> RoundCorners(const std::vector<Vec3> &aPoints, float aCut, float aShare, float aSmallest,
                               float aStep, const Space &aSpace);
// Two legs joined at index aJoin eased into each other: aPasses times, each point aLo..aHi (at least 1,
// at most the last but one) within aRadius of the join point moves halfway to its neighbours' middle
// where both its segments stay clear in aSpace. An aJoin outside the points leaves them as they are.
// (The script's FlightRoute.SmoothJoin.)
std::vector<Vec3> SmoothJoin(std::vector<Vec3> aPoints, int32_t aJoin, int32_t aLo, int32_t aHi, float aRadius,
                             int32_t aPasses, const Space &aSpace);

} // namespace Navigation
