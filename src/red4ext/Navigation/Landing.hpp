#pragma once

// Where the Autopilot lands (docs/design/native-ports.md, M6 step 4a): a spot on the ground near a
// destination with room to come straight down into, a landing pad, the heading to land on, and the
// route's end moved over when the spot turns out taken. FlightModeAutopilot.reds' FindLandingSpot,
// ShaftClear, SpotTaken, IsFlat, SpotSupported, FindLandingPad, PadHeading, LandingHeading and
// ShiftRouteEnd, over a World.

#include "AutopilotParams.hpp"
#include "Route.hpp"
#include "World.hpp"

#include <cstdint>
#include <vector>

namespace Navigation {

// A point the spot keeps away from (FlightSummonSystem: V), radius 0 for none.
struct KeepClear {
  Vec3 at{};
  float radius = 0.0f;
};

// A spot found: where a resting car's centre sits (the bake's floor), and the top of its gap.
struct GroundSpot {
  Vec3 spot{};
  float top = 0.0f;
};

// The columns on rings 2 m apart out to aShaft around aC, eight directions, are open from aFloor up
// at least aRoom.
bool ShaftClear(const World &aWorld, const Vec3 &aC, float aFloor, float aRoom, float aShaft);
// A vehicle in the spot (floor height, car centre at rest): vertical rays in the Vehicle group from
// 6 m above to 2 m below, at the centre and on rings of 2 and 4 m, eight directions.
bool SpotTaken(const World &aWorld, const Vec3 &aSpot);
// The columns out to aFlat around aC (rings 2 m apart, the last at aFlat, eight directions) have
// their floor within half a metre of aFloor.
bool IsFlat(const World &aWorld, const Vec3 &aC, float aFloor, float aFlat);
// The game's collision under the spot: rays down at its centre and aFootprint out in eight
// directions. All nine hitting within 0.6 m of each other: room to land. None hitting: the world has
// not streamed in, nothing to say (true). Otherwise (an edge, a ledge): false.
bool SpotSupported(const World &aWorld, const Vec3 &aSpot, float aFootprint);

// A destination on the ground: the nearest column within landingSearch rings of 2 m that is at the
// destination's level, open aRoom above, flat, with a clear shaft, no vehicle in it, the game's
// collision under it and away from aKeep. False for a destination in the air (more than 4 m over its
// floor). Each ring is checked on the nav map first and the rays cast only for its candidates
// nearest first, until one passes: the script's result with far fewer rays.
bool FindLandingSpotWithRoom(const World &aWorld, const AutopilotParams &aParams, const Vec3 &aTarget, float aRoom,
                             const KeepClear &aKeep, GroundSpot &aOut);
// First with room to come straight down from the approach height (a spot with little room over it
// can be under a canopy), then with 6 m.
bool FindLandingSpot(const World &aWorld, const AutopilotParams &aParams, const Vec3 &aTarget, const KeepClear &aKeep,
                     GroundSpot &aOut);

// The landing pad for a destination: the one it is on (within padSnapDistance, at its level within
// padSnapLevel), else with padRange set and the destination on the ground, the nearest within
// padRange; -1 for none.
int32_t FindLandingPad(const World &aWorld, const AutopilotParams &aParams, const Vec3 &aTarget);
// A pad's heading to land on: its yaw (+Y turned by yaw), of its two ways the one nearer aArrival.
Vec3 PadHeading(const Helipad &aPad, const Vec3 &aArrival);
// The heading to land on at a spot: along the most open of eight lines through it (a street, a
// plaza's long side), of its two ways the one nearer the arrival heading; the arrival heading when
// nothing is open.
Vec3 LandingHeading(const World &aWorld, const Vec3 &aSpot, const Vec3 &aArrival);

// The route's end moved over to a new landing spot aTo (the end keeping its height above the floor:
// aSpotZ is the old spot's): the last aSpan metres past segment aSegment + 1 shifted by the offset,
// eased in. False, and aMoved untouched, when the route is too short or a moved point is not free.
bool ShiftRouteEnd(const World &aWorld, const Route &aRoute, int32_t aSegment, const Vec3 &aTo, float aSpotZ,
                   float aSpan, std::vector<Vec3> &aMoved);

} // namespace Navigation
