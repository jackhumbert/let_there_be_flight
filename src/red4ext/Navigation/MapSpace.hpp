#pragma once

// The nav map as the follower's free space (kept apart from Space.hpp so tests need no NavMap).

#include "NavMap.hpp"
#include "Space.hpp"

namespace Navigation {

struct MapSpace final : Space {
  explicit MapSpace(const NavMap &aMap) : map(aMap) {}
  bool IsFree(const Vec3 &aP) const override { return map.IsFree(aP.x, aP.y, aP.z); }
  float SegmentClear(const Vec3 &aFrom, const Vec3 &aTo) const override {
    return map.SegmentClear(aFrom.x, aFrom.y, aFrom.z, aTo.x, aTo.y, aTo.z);
  }
  const NavMap &map;
};

} // namespace Navigation
