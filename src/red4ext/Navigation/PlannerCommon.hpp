#pragma once

// Planner internals shared by Planner.cpp, Line.cpp and Doorways.cpp: the search grid's keys
// and gaps, and the checks that a leg or a line is free. Not for use outside the planner.

#include "Planner.hpp"

#include <cmath>
#include <cstdint>
#include <limits>
#include <vector>

namespace Navigation::detail {

inline constexpr float kInf = std::numeric_limits<float>::infinity();
inline constexpr float kTopCap = 1000.0f; // finite stand-in for open sky in the height profile

struct GapM {
  float b, t; // metres; t is +infinity for open sky
};

inline uint64_t MakeKey(int32_t aCx, int32_t aCy, uint32_t aGap, int32_t aWidth) {
  return ((uint64_t(aCy) * uint64_t(aWidth) + uint64_t(aCx)) << 8) | aGap;
}
inline int32_t KeyCx(uint64_t aKey, int32_t aWidth) { return int32_t((aKey >> 8) % uint64_t(aWidth)); }
inline int32_t KeyCy(uint64_t aKey, int32_t aWidth) { return int32_t((aKey >> 8) / uint64_t(aWidth)); }
inline uint32_t KeyGap(uint64_t aKey) { return uint32_t(aKey & 0xFF); }

inline constexpr int kTurnBase = 4; // cells a bend is judged over (turnCost)
// Longest stretch (m) of the polyline that is blocked, sampled every half metre along its whole
// length (so where the legs are cut does not change the answer).
inline constexpr float kGrazeStep = 0.5f;
inline constexpr float kGrazeAllowed = 1.0f; // m: the planner's own legs graze this much between their samples
// m, the finished line: moves are checked at SegmentClear's 1 m sampling, the line at half a metre,
// which finds a little more; still half the 3 m the bake grew solids by for the car
inline constexpr float kGrazeAllowedLine = 1.5f;

GapM GapAt(NavMap::View &aView, int32_t aCx, int32_t aCy, uint32_t aGap);

// Height the route would like to fly at in a gap: preferredHeight above the floor, or halfway up
// a gap with something overhead if that is lower.
float Preferred(GapM aGap, float aHeight);

// The gap nearest to p (containing it if possible). If p's column has nothing (the car is inside
// the inflated margin of a wall), the nearest column within a few cells is used.
bool Anchor(NavMap::View &aView, Vec3 aP, int32_t &aCx, int32_t &aCy, uint32_t &aGap);

float Clear(const NavMap &aMap, Vec3 aA, Vec3 aB);

// The leg and two parallel legs offset sideways by aSide must all be free.
bool ClearWide(const NavMap &aMap, Vec3 aA, Vec3 aB, float aSide);

float Distance(Vec3 aA, Vec3 aB);

// The longest run of blocked space along the polyline, from aSkip metres on (the stretch from a
// car under cover out to the free space is blocked on the map and is flown anyway).
float LongestBlocked(const NavMap &aMap, const std::vector<Vec3> &aPoints, float aSkip = 0.0f);

// The leg is free but for a graze no longer than the planner's own legs have.
bool LegFree(const NavMap &aMap, Vec3 aA, Vec3 aB);

// LegFree for leg aI of aPts, and the grazes either side of it taken as one run, as the finished
// line is checked (LongestBlocked over it, kGrazeAllowedLine): legs each grazing a little either
// side of a point pass alone and fail together.
bool LegRunFree(const NavMap &aMap, const std::vector<Vec3> &aPts, size_t aI);

// How much of the leg is free, the leg and its two side-offset copies taken together (0..1).
float ClearSides(const NavMap &aMap, Vec3 aA, Vec3 aB, float aSide);

// The polyline's points moved to even spacing along it (aSpacing at most, its ends kept): the height
// solve takes bends point to point, and bunched points hide a crest.
// Where a new leg is not free (it cut a corner: the take-off, a tight street), the old points
// between its ends go back in: those legs lie on the old ones.
std::vector<Vec3> Respace(const NavMap &aMap, const std::vector<Vec3> &aPoints, float aSpacing);

// Each leg of the polyline cut into equal steps of at most aSpacing, its corners kept: every new
// leg lies on an old one, so a free polyline stays free.
std::vector<Vec3> Resample(const std::vector<Vec3> &aPoints, float aSpacing);

// The line through the route's corridor (Line.cpp).
std::vector<Vec3> Line(const NavMap &aMap, const std::vector<Vec3> &aRoute, const PlanParams &aParams);

// The line made free where its smoothing left it blocked (Line.cpp): each blocked stretch, and a few
// points either side (more until it fits), replaced by aRoute (free, pulled straight) between the
// line's points at its ends, then evened out wherever the moves stay free. Legs within aSkip of the
// start (the take-off from under cover) do not count. aRepaired: the stretches replaced.
std::vector<Vec3> RepairLine(const NavMap &aMap, std::vector<Vec3> aLine, const std::vector<Vec3> &aRoute,
                             const PlanParams &aParams, float aSkip, int32_t &aRepaired);

} // namespace Navigation::detail
