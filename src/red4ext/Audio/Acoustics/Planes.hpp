#pragma once

// Planes fitted to the PhysX hits (docs/design/acoustics.md, Planes): the hits of the last second sit in
// a world space cache, and each side of the car (floor, ceiling, left, right) takes the plane most of
// its hits agree on, so a pole or a sign cannot stand in for a wall and a cast that finds nothing
// (collision not streamed in) does not drop the wall. Pure maths; no game or FMOD types.

#include "Probe.hpp"

#include <optional>
#include <vector>

namespace Acoustics::Planes {

struct Hit {
  Vec3 position, normal;
  float time; // s, on any clock the caller keeps
};

// A surface: a point on it and its unit normal, pointing toward the car.
struct Plane {
  Vec3 point, normal;
  int inliers;
};

enum Side { Floor, Ceiling, Left, Right, kSides };

// Which side of the car a direction (from the car to a hit) is: within 45 degrees of straight down or
// up the floor and ceiling, else left or right of the car by aRightDir.
Side SideOf(const Vec3 &aDirection, const Vec3 &aRightDir);

struct Settings {
  float maxAge = 1.0f;         // s
  float maxDistance = 150.0f;  // m from the car
  float inlierDistance = 1.0f; // m: a hit this near a seed's plane agrees with it
  float normalAgree = 0.866f;  // cos 30 degrees
  int minInliers = 4;          // fewer agreeing hits are a pole or a sign
  size_t capacity = 1024;
};

class Cache {
public:
  explicit Cache(const Settings &aSettings = {});

  // The oldest hit goes when the cache is full (no allocation).
  void Add(const Hit &aHit);
  // Drops hits older than maxAge at aNow or further than maxDistance from aCar.
  void Expire(float aNow, const Vec3 &aCar);

  // The plane most of a side's hits agree on, if enough do: each hit on the side is tried as a seed,
  // the one with most agreeing hits (nearest the car on a tie, then the earliest) gives the plane, whose
  // normal is the agreeing hits' mean and point their mean position.
  std::optional<Plane> Fit(Side aSide, const Vec3 &aCar, const Vec3 &aRightDir) const;

  size_t Size() const { return count; }

private:
  const Hit &At(size_t aIndex) const { return hits[(start + aIndex) % hits.size()]; }

  Settings settings;
  std::vector<Hit> hits; // a ring, allocated once
  size_t start = 0, count = 0;
};

} // namespace Acoustics::Planes
