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

// Distances in metres to the real surfaces; +infinity: nothing within range.
struct Features {
  float floor;          // straight down, from the gap the car is in
  float ceiling;        // straight up, from the same gap
  float left;           // nearest hit in a fan round the car's horizontal left
  float right;          // the same to the right
  float enclosure;      // fraction of the sphere's rays that hit within range
  float meanFreePath;   // mean distance of those hits
  Vec3 rightDir;        // the horizontal right vector the fans were cast round
};

class Probe {
public:
  static constexpr int kSphereRays = 48;
  static constexpr int kFanRays = 15; // 5 yaws x 3 pitches, each side
  static constexpr float kMaxDistance = 200.0f;
  static constexpr float kStep = 1.0f;

  explicit Probe(const Navigation::NavMap &aMap);

  // aForward and aUp: the car's axes (world, z up).
  Features operator()(const Vec3 &aPos, const Vec3 &aForward, const Vec3 &aUp);

private:
  bool Free(float aX, float aY, float aZ);
  float March(const Vec3 &aOrigin, const Vec3 &aDir);
  void FloorCeiling(const Vec3 &aPos, float &aFloor, float &aCeiling);
  float Fan(const Vec3 &aOrigin, const Vec3 &aSide);

  const Navigation::NavMap &map;
  // the View keeps every tile it touched alive: started afresh now and then (the map's own cache
  // keeps the recent ones inflated)
  std::optional<Navigation::NavMap::View> view;
  int probes = 0;
  std::array<Vec3, kSphereRays> sphere;
};

} // namespace Acoustics
