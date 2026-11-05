#pragma once

// What the Autopilot asks of the world (docs/design/native-ports.md, M6): free space (Space), the
// nav map's columns, the landing pads and gates, and the two engine rays its landing checks cast.
// In the game the nav map and the engine answer it (MapWorld, rays from FlightWorld); offline and in
// tests the nav map or boxes do.

#include "Gates.hpp"
#include "Helipads.hpp"
#include "NavMap.hpp"
#include "Space.hpp"

#include <cstdint>
#include <utility>
#include <vector>

namespace Navigation {

struct World : Space {
  // the gaps of the column at (aP.x, aP.y), bottom up; top +infinity for open sky. Empty: outside
  // the map, no data, or nothing reachable (blocked).
  virtual std::vector<Gap> Gaps(const Vec3 &aP) const = 0;
  // the car cylinder's half height the solids were grown by: a gap's bottom is this far above the
  // surface under it
  virtual float CarHalfHeight() const = 0;
  virtual const std::vector<Helipad> &Pads() const = 0;
  virtual const std::vector<Gate> &GateList() const = 0;
  // a ray in the Vehicle collision group (cars, not the ground): true when it hits one
  virtual bool VehicleRay(const Vec3 &aFrom, const Vec3 &aTo) const = 0;
  // a ray against the static world, terrain and vehicle blockers: true and the first hit's position
  // when it hits. Where the world has not streamed in, nothing is hit.
  virtual bool GroundRay(const Vec3 &aFrom, const Vec3 &aTo, Vec3 &aHit) const = 0;
  // the nav map is loaded (FlightNavMap.IsLoaded): without it the Autopilot plans nothing ("no nav map")
  virtual bool Loaded() const { return true; }
  // the nav map behind the world, for the trajectory's room caps (FlightTrajectory.Build); nullptr: none
  virtual const NavMap *Map() const { return nullptr; }
};

// The first gap of aP's column whose top is above aP (the one aP is in, or the next one up), as the
// Autopilot's GapAt; false when there is none.
bool GapAt(const World &aWorld, const Vec3 &aP, Gap &aGap);

// Pads by distance in the horizontal: the nearest within aMaxDistance (or -1), and those within
// aRadius nearest first. Helipads::Nearest and InRange are these over the loaded pads.
int32_t NearestPad(const std::vector<Helipad> &aPads, float aX, float aY, float aMaxDistance);
std::vector<int32_t> PadsInRange(const std::vector<Helipad> &aPads, float aX, float aY, float aRadius);
std::vector<int32_t> GatesInRange(const std::vector<Gate> &aGates, float aX, float aY, float aRadius);

// An axis-aligned box, min and max corner, corners inclusive.
using Box = std::pair<Vec3, Vec3>;

// The first t in [0, 1] at which a + (b - a) t enters the box, or a negative number when it misses.
float RayBox(const Vec3 &aFrom, const Vec3 &aTo, const Box &aBox);

// The nav map, pads and gates as the world. Its rays hit the boxes in vehicles and ground (empty in
// the game, where FlightWorld casts them through the engine instead; offline, parked cars and the
// solids the rays should see).
struct MapWorld : World {
  MapWorld(const NavMap &aMap, const std::vector<Helipad> &aPads, const std::vector<Gate> &aGates)
      : map(aMap), pads(aPads), gates(aGates) {}

  bool IsFree(const Vec3 &aP) const override { return map.IsFree(aP.x, aP.y, aP.z); }
  float SegmentClear(const Vec3 &aFrom, const Vec3 &aTo) const override {
    return map.SegmentClear(aFrom.x, aFrom.y, aFrom.z, aTo.x, aTo.y, aTo.z);
  }
  std::vector<Gap> Gaps(const Vec3 &aP) const override { return map.GetGaps(aP.x, aP.y); }
  float CarHalfHeight() const override { return map.CarHalfHeight(); }
  const std::vector<Helipad> &Pads() const override { return pads; }
  const std::vector<Gate> &GateList() const override { return gates; }
  bool VehicleRay(const Vec3 &aFrom, const Vec3 &aTo) const override;
  bool GroundRay(const Vec3 &aFrom, const Vec3 &aTo, Vec3 &aHit) const override;
  bool Loaded() const override { return map.IsLoaded(); }
  const NavMap *Map() const override { return map.IsLoaded() ? &map : nullptr; }

  const NavMap &map;
  const std::vector<Helipad> &pads;
  const std::vector<Gate> &gates;
  std::vector<Box> vehicles;
  std::vector<Box> ground;
};

} // namespace Navigation
