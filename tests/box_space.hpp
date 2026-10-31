#pragma once

// Free space for tests: axis-aligned boxes that are blocked (corners inclusive), the rest free.
// Exact, so expected values can be worked by hand.

#include "Navigation/Space.hpp"

#include <algorithm>
#include <cmath>
#include <utility>
#include <vector>

struct BoxSpace : Navigation::Space {
  using Vec3 = Navigation::Vec3;
  std::vector<std::pair<Vec3, Vec3>> boxes; // min and max corner of each blocked box

  bool IsFree(const Vec3 &aP) const override {
    for (const auto &[lo, hi] : boxes) {
      if (aP.x >= lo.x && aP.x <= hi.x && aP.y >= lo.y && aP.y <= hi.y && aP.z >= lo.z && aP.z <= hi.z) {
        return false;
      }
    }
    return true;
  }

  // the slab test of a + (b - a) t, t in [0, 1], against each box; the smallest entry t of those hit
  float SegmentClear(const Vec3 &aFrom, const Vec3 &aTo) const override {
    float best = 1.0f;
    for (const auto &[lo, hi] : boxes) {
      const float a[3] = {aFrom.x, aFrom.y, aFrom.z}, b[3] = {aTo.x, aTo.y, aTo.z};
      const float mn[3] = {lo.x, lo.y, lo.z}, mx[3] = {hi.x, hi.y, hi.z};
      float tmin = 0.0f, tmax = 1.0f;
      bool hit = true;
      for (int k = 0; k < 3 && hit; ++k) {
        const float d = b[k] - a[k];
        if (std::fabs(d) < 1e-9f) {
          hit = a[k] >= mn[k] && a[k] <= mx[k];
          continue;
        }
        float t1 = (mn[k] - a[k]) / d, t2 = (mx[k] - a[k]) / d;
        if (t1 > t2) {
          std::swap(t1, t2);
        }
        tmin = std::max(tmin, t1);
        tmax = std::min(tmax, t2);
        hit = tmin <= tmax;
      }
      if (hit) {
        best = std::min(best, tmin);
      }
    }
    return best;
  }
};
