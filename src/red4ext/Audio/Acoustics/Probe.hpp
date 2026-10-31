#pragma once

// Probe: what the space round the car is like for sound, read from the nav map (docs/design/acoustics.md).
//
// Rays are marched in 1 m steps through the free gaps; the map's solids are grown by the car's shape,
// so a hit is pushed back out along the ray by that ellipsoid's radius. Columns with no data
// (outside the city) count as open, an empty column inside the map as solid. A ray starting inside
// the grown floor (a car on the ground) hits only after it has been in free space.
//
// The port of tools/acoustics/probe.py, which stays the reference. Not thread safe: one Probe per
// thread (it holds a NavMap::View).

#include "Navigation/NavMap.hpp"

#include <array>
#include <optional>

namespace Acoustics {

struct Vec3 {
  float x = 0, y = 0, z = 0;
};

// The near rays, in NearRays() order: straight down, straight up, the right fan, the left fan.
constexpr int kFanRays = 15; // 5 yaws x 3 pitches, each side
constexpr int kNearRays = 2 + 2 * kFanRays;

// Distances in metres to the real surfaces; +infinity: nothing within range.
struct Features {
  float floor;          // straight down, from the gap the car is in
  float ceiling;        // straight up, from the same gap
  float left;           // nearest hit in a fan round the car's horizontal left
  float right;          // the same to the right
  float enclosure;      // fraction of the sphere's rays that hit within range
  float meanFreePath;   // mean distance of those hits
  Vec3 rightDir;        // the horizontal right vector the fans were cast round
  // with live hits: each surface's reflectivity from its material (-1: none known), how many of the
  // near rays a live hit answered, and the nav map's own distances for comparison
  float floorReflectivity = -1, ceilingReflectivity = -1, leftReflectivity = -1, rightReflectivity = -1;
  int liveRays = 0;
  float floorMap = 0, ceilingMap = 0, leftMap = 0, rightMap = 0;
  int leftRay = -1, rightRay = -1; // the nearest ray of each fan (NearRays index)
  // every near ray: its direction, distance (live where answered) and reflectivity (-1: unknown), for
  // the scattered echoes
  std::array<Vec3, kNearRays> nearDir{};
  std::array<float, kNearRays> nearDistance{}, nearReflectivity{};
};

// The game's own collision along the near rays (cast by the scripts, FlightAudio.reds), from origin:
// distance to the hit (< 0: no hit within the cast's range) and the hit's reflectivity (< 0: unknown).
struct LiveHits {
  Vec3 origin;
  std::array<float, kNearRays> distance;
  std::array<float, kNearRays> reflectivity;
};

class Probe {
public:
  static constexpr int kSphereRays = 48;
  static constexpr float kMaxDistance = 200.0f;
  static constexpr float kStep = 1.0f;

  explicit Probe(const Navigation::NavMap &aMap);

  // The near rays' directions for a car with these axes (world, z up).
  static std::array<Vec3, kNearRays> NearRays(const Vec3 &aForward, const Vec3 &aUp);

  // aForward and aUp: the car's axes (world, z up). aLive, if given, answers the near rays it hit;
  // the nav map answers the rest (a live miss may only be collision not streamed in yet).
  Features operator()(const Vec3 &aPos, const Vec3 &aForward, const Vec3 &aUp, const LiveHits *aLive = nullptr);

private:
  bool Free(float aX, float aY, float aZ);
  float March(const Vec3 &aOrigin, const Vec3 &aDir);
  void FloorCeiling(const Vec3 &aPos, float &aFloor, float &aCeiling);
  // the nearest of a fan's rays (aFirst: its first index in NearRays), live where answered
  float Fan(const Vec3 &aOrigin, const std::array<Vec3, kNearRays> &aRays, int aFirst, const LiveHits *aLive,
            float &aMapNearest, float &aReflectivity, int &aRay, int &aLiveRays, Features &aFeatures);

  const Navigation::NavMap &map;
  // the View keeps every tile it touched alive: started afresh now and then (the map's own cache
  // keeps the recent ones inflated)
  std::optional<Navigation::NavMap::View> view;
  int probes = 0;
  std::array<Vec3, kSphereRays> sphere;
};

} // namespace Acoustics
