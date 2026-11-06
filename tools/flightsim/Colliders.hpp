#pragma once

// The city's collision offline, as triangles, for rays: what the car's FindGround and its underside meet in game
// (docs/design/hover-fly-terrain.md). A region file comes from tools/flightsim/region_pack.py (the sector collision
// that blocks vehicles less what the plugin moves away, LTBF's proxy colliders, and the terrain as triangles); tests
// build worlds in memory with Add.
//
// Region file, little endian: "LTBFREG1", uint32 version 1, uint32 triangle count n, float32 bounds x0 y0 z0 x1 y1
// z1, n x 9 float32 (corners a, b, c), n uint8 kinds (Kind).
//
// The rays hit both faces of a triangle, and the normal faces the ray's start.

#include "Flight/ScriptMath.hpp"

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace FlightSim {

using Flight::Vec3;

class Colliders {
public:
  enum Kind : uint8_t { Collision = 0, Terrain = 1, Proxy = 2 };

  struct Hit {
    Vec3 position, normal;
    float t = 0; // 0..1 along the ray
    Kind kind = Collision;
  };

  // Reads a region file and builds the grid; false, and aError says why, when it cannot.
  bool Load(const std::string &aPath, std::string *aError = nullptr);
  // A triangle for a world built in memory; Build before casting.
  void Add(Vec3 aA, Vec3 aB, Vec3 aC, Kind aKind = Collision);
  // Bins the triangles on a grid of aCell m squares (each in every square its bounds touch).
  void Build(float aCell = 4.0f);

  // The first hit along aFrom..aTo. The squares searched are those under the ray's bounds, so a long slanted ray costs
  // more than a short or a vertical one.
  bool Ray(Vec3 aFrom, Vec3 aTo, Hit &aHit) const;

  size_t Size() const { return tris.size(); }
  // Triangle i's corners and kind.
  void Triangle(size_t aIndex, Vec3 &aA, Vec3 &aB, Vec3 &aC, Kind &aKind) const {
    const Tri &t = tris[aIndex];
    aA = t.a;
    aB = Flight::Add(t.a, t.e1);
    aC = Flight::Add(t.a, t.e2);
    aKind = t.kind;
  }
  Vec3 Min() const { return lo; }
  Vec3 Max() const { return hi; }

private:
  struct Tri {
    Vec3 a, e1, e2; // a corner and the two edges from it
    Kind kind;
  };
  std::vector<Tri> tris;
  Vec3 lo, hi;
  float cell = 4.0f;
  int32_t nx = 0, ny = 0;
  std::vector<uint32_t> starts, items; // per square, its triangles: items[starts[i] .. starts[i + 1]]
};

// A ray against the ground for the Sim: true, the hit and its normal when it hits.
using GroundRay = std::function<bool(Vec3 aFrom, Vec3 aTo, Vec3 &aHit, Vec3 &aNormal)>;
GroundRay RayOn(const Colliders &aColliders);

} // namespace FlightSim
