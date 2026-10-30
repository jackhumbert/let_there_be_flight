#include "PlannerCommon.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <limits>
#include <memory>
#include <mutex>
#include <queue>
#include <spdlog/fmt/fmt.h>
#include <spdlog/spdlog.h>
#include <unordered_map>

namespace Navigation::detail {

GapM GapAt(NavMap::View &aView, int32_t aCx, int32_t aCy, uint32_t aGap) {
  const uint16_t *g;
  uint32_t n = aView.Column(aCx, aCy, g);
  if (aGap >= n) {
    return {kInf, -kInf};
  }
  return {aView.Map().VoxelZ(g[2 * aGap]), aView.Map().VoxelZ(g[2 * aGap + 1])};
}

float Preferred(GapM aGap, float aHeight) {
  float z = aGap.b + aHeight;
  if (std::isfinite(aGap.t)) {
    z = std::min(z, 0.5f * (aGap.b + aGap.t));
  }
  return z;
}

bool Anchor(NavMap::View &aView, Vec3 aP, int32_t &aCx, int32_t &aCy, uint32_t &aGap) {
  const NavMap &map = aView.Map();
  int32_t c0x, c0y;
  if (!map.CellOf(aP.x, aP.y, c0x, c0y)) {
    return false;
  }
  float best = kInf;
  for (int32_t r = 0; r <= 6 && !std::isfinite(best); ++r) {
    for (int32_t dy = -r; dy <= r; ++dy) {
      for (int32_t dx = -r; dx <= r; ++dx) {
        if (std::max(std::abs(dx), std::abs(dy)) != r) {
          continue;
        }
        const uint16_t *g;
        uint32_t n = aView.Column(c0x + dx, c0y + dy, g);
        for (uint32_t i = 0; i < n; ++i) {
          float b = map.VoxelZ(g[2 * i]), t = map.VoxelZ(g[2 * i + 1]);
          float d = aP.z < b ? b - aP.z : (aP.z >= t ? aP.z - t : 0.0f);
          d += float(r) * map.CellSize();
          if (d < best) {
            best = d;
            aCx = c0x + dx;
            aCy = c0y + dy;
            aGap = i;
          }
        }
      }
    }
  }
  return std::isfinite(best);
}

float Clear(const NavMap &aMap, Vec3 aA, Vec3 aB) { return aMap.SegmentClear(aA.x, aA.y, aA.z, aB.x, aB.y, aB.z); }

bool ClearWide(const NavMap &aMap, Vec3 aA, Vec3 aB, float aSide) {
  if (Clear(aMap, aA, aB) < 1.0f) {
    return false;
  }
  float dx = aB.x - aA.x, dy = aB.y - aA.y;
  float len = std::hypot(dx, dy);
  if (aSide <= 0.0f || len < 1e-3f) {
    return true;
  }
  float px = -dy / len * aSide, py = dx / len * aSide;
  for (float s : {1.0f, -1.0f}) {
    Vec3 a{aA.x + s * px, aA.y + s * py, aA.z}, b{aB.x + s * px, aB.y + s * py, aB.z};
    if (Clear(aMap, a, b) < 1.0f) {
      return false;
    }
  }
  return true;
}


float Distance(Vec3 aA, Vec3 aB) {
  float dx = aB.x - aA.x, dy = aB.y - aA.y, dz = aB.z - aA.z;
  return std::sqrt(dx * dx + dy * dy + dz * dz);
}


float LongestBlocked(const NavMap &aMap, const std::vector<Vec3> &aPoints, float aSkip) {
  float run = 0.0f, longest = 0.0f, carry = 0.0f, along = 0.0f;
  for (size_t i = 0; i + 1 < aPoints.size(); ++i) {
    Vec3 a = aPoints[i], b = aPoints[i + 1];
    float len = Distance(a, b);
    for (float s = carry; s <= len; s += kGrazeStep) {
      float t = len > 1e-4f ? s / len : 0.0f;
      bool free = along + s < aSkip || aMap.IsFree(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t);
      run = free ? 0.0f : run + kGrazeStep;
      longest = std::max(longest, run);
      carry = s + kGrazeStep - len;
    }
    along += len;
  }
  return longest;
}

bool LegFree(const NavMap &aMap, Vec3 aA, Vec3 aB) {
  return Clear(aMap, aA, aB) >= 1.0f || LongestBlocked(aMap, {aA, aB}) <= kGrazeAllowed;
}

bool LegRunFree(const NavMap &aMap, const std::vector<Vec3> &aPts, size_t aI) {
  if (!LegFree(aMap, aPts[aI], aPts[aI + 1])) {
    return false;
  }
  size_t from = aI > 0 ? aI - 1 : aI, to = std::min(aI + 2, aPts.size() - 1);
  return LongestBlocked(aMap, std::vector<Vec3>(aPts.begin() + from, aPts.begin() + to + 1)) <= kGrazeAllowedLine;
}

float ClearSides(const NavMap &aMap, Vec3 aA, Vec3 aB, float aSide) {
  float clear = Clear(aMap, aA, aB);
  float dx = aB.x - aA.x, dy = aB.y - aA.y;
  float len = std::hypot(dx, dy);
  if (aSide <= 0.0f || len < 1e-3f || clear < 1.0f) {
    return clear;
  }
  float px = -dy / len * aSide, py = dx / len * aSide;
  for (float s : {1.0f, -1.0f}) {
    clear = std::min(clear, Clear(aMap, {aA.x + s * px, aA.y + s * py, aA.z}, {aB.x + s * px, aB.y + s * py, aB.z}));
  }
  return clear;
}

std::vector<Vec3> Respace(const NavMap &aMap, const std::vector<Vec3> &aPoints, float aSpacing) {
  std::vector<float> along(aPoints.size(), 0.0f);
  for (size_t i = 1; i < aPoints.size(); ++i) {
    along[i] = along[i - 1] + Distance(aPoints[i - 1], aPoints[i]);
  }
  if (aPoints.size() < 3 || along.back() < aSpacing) {
    return aPoints;
  }
  int steps = std::max(2, int(std::ceil(along.back() / aSpacing)));
  std::vector<Vec3> even{aPoints.front()};
  std::vector<float> evenAt{0.0f};
  size_t j = 1;
  for (int k = 1; k < steps; ++k) {
    float at = along.back() * float(k) / float(steps);
    while (j + 1 < aPoints.size() && along[j] < at) {
      ++j;
    }
    float t = along[j] > along[j - 1] ? (at - along[j - 1]) / (along[j] - along[j - 1]) : 0.0f;
    const Vec3 &a = aPoints[j - 1], &b = aPoints[j];
    even.push_back({a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t});
    evenAt.push_back(at);
  }
  even.push_back(aPoints.back());
  evenAt.push_back(along.back());
  std::vector<Vec3> out{even.front()};
  size_t old = 1;
  for (size_t k = 1; k < even.size(); ++k) {
    bool free = LegFree(aMap, even[k - 1], even[k]);
    while (old < aPoints.size() && along[old] <= evenAt[k - 1]) {
      ++old;
    }
    if (!free) {
      for (size_t i = old; i < aPoints.size() && along[i] < evenAt[k]; ++i) {
        out.push_back(aPoints[i]);
      }
    }
    out.push_back(even[k]);
  }
  return out;
}

std::vector<Vec3> Resample(const std::vector<Vec3> &aPoints, float aSpacing) {
  std::vector<Vec3> out{aPoints.front()};
  for (size_t i = 0; i + 1 < aPoints.size(); ++i) {
    Vec3 a = aPoints[i], b = aPoints[i + 1];
    int steps = std::max(1, int(std::ceil(Distance(a, b) / aSpacing)));
    for (int k = 1; k <= steps; ++k) {
      float t = float(k) / float(steps);
      out.push_back({a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t});
    }
  }
  return out;
}

// Racing line: gradient descent on the summed squared second differences of the points (their
// bend), each move kept only where both legs to the neighbours stay free, with lineSide to spare
// or at least no less than before. Points ahead of a corner move out and the apex moves
// in, so the bend spreads over all the room there is. Coarse spacings first (the bend of a long
// corner spreads over a few points per pass there), then finer ones down to lineSpacing.
// The heights the line may take in a gap, at aAlong of aLength metres along it: lineFloorClearance
// over the floor (but within lineFloorEnds of either end, where it takes off and lands) and
// lineCeilingClearance under anything overhead; the middle of the gap where it has not room for
// both. The ceiling was only a push of linePush a pass, which the smoothing undid: west-east went
// under a skybridge a metre from its underside, one point left out of line with the rest, and the
// speed plan braked from 96 to 39 m/s for that kink (2026-10-27).

} // namespace Navigation::detail
