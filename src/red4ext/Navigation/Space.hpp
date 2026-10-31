#pragma once

// What the follower asks of free space: whether a point is free and how much of a straight stretch
// is. The nav map answers it in the game (MapSpace); tests answer it with boxes.

#include "Planner.hpp"

namespace Navigation {

struct Space {
  virtual ~Space() = default;
  virtual bool IsFree(const Vec3 &aP) const = 0;
  // the fraction of aFrom -> aTo that is free before the first blocked point (1 = clear all the way,
  // 0 = aFrom itself is blocked), as NavMap::SegmentClear
  virtual float SegmentClear(const Vec3 &aFrom, const Vec3 &aTo) const = 0;
};

} // namespace Navigation
