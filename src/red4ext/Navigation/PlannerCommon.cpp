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

namespace {
constexpr float kGuideBucket = 16.0f; // the city's lanes are dense: a bigger square holds hundreds
uint64_t GuideKey(int32_t aBx, int32_t aBy) { return (uint64_t(uint32_t(aBx)) << 32) | uint32_t(aBy); }
} // namespace

GuideField::GuideField(const PlanParams &aParams)
    : low(aParams.guideLow), high(aParams.guideHigh), radius(aParams.guideRadius), ramp(std::max(aParams.guideRamp, 0.1f)),
      reach(aParams.guideRadius + std::max(aParams.guideRamp, 0.1f)) {
  if (aParams.guideCost <= 0.0f || aParams.guide.size() < 2) {
    return;
  }
  for (size_t i = 0; i + 1 < aParams.guide.size(); ++i) {
    const Vec3 a = aParams.guide[i], b = aParams.guide[i + 1];
    if (!std::isfinite(a.x) || !std::isfinite(b.x)) {
      continue; // a break between two paths
    }
    const auto index = uint32_t(segments.size());
    segments.push_back({a, b});
    const int32_t x0 = int32_t(std::floor((std::min(a.x, b.x) - reach) / kGuideBucket));
    const int32_t x1 = int32_t(std::floor((std::max(a.x, b.x) + reach) / kGuideBucket));
    const int32_t y0 = int32_t(std::floor((std::min(a.y, b.y) - reach) / kGuideBucket));
    const int32_t y1 = int32_t(std::floor((std::max(a.y, b.y) + reach) / kGuideBucket));
    for (int32_t by = y0; by <= y1; ++by) {
      for (int32_t bx = x0; bx <= x1; ++bx) {
        buckets[GuideKey(bx, by)].push_back(index);
      }
    }
  }
}

// aEach(across, height of the path there) for every segment within reach of (aX, aY)
template <typename F> void GuideField::Near(float aX, float aY, F &&aEach) const {
  auto it = buckets.find(GuideKey(int32_t(std::floor(aX / kGuideBucket)), int32_t(std::floor(aY / kGuideBucket))));
  if (it == buckets.end()) {
    return;
  }
  for (uint32_t i : it->second) {
    const Seg &s = segments[i];
    const float dx = s.b.x - s.a.x, dy = s.b.y - s.a.y, l2 = dx * dx + dy * dy;
    const float t = l2 > 0.0f ? std::clamp(((aX - s.a.x) * dx + (aY - s.a.y) * dy) / l2, 0.0f, 1.0f) : 0.0f;
    const float across = std::hypot(aX - (s.a.x + dx * t), aY - (s.a.y + dy * t));
    if (across <= reach) {
      aEach(across, s.a.z + (s.b.z - s.a.z) * t);
    }
  }
}

float GuideField::Off(float aX, float aY, float aB, float aT) const {
  float best = 1.0f;
  const float top = std::min(aT, kTopCap);
  Near(aX, aY, [&](float aAcross, float aZ) {
    const float lo = aZ + low, hi = aZ + high;
    const float dz = std::max({0.0f, lo - top, aB - hi});
    const float off = std::clamp((aAcross - radius) / ramp, 0.0f, 1.0f) + std::clamp(dz / ramp, 0.0f, 1.0f);
    best = std::min(best, off);
  });
  return best;
}

bool GuideField::BandAt(float aX, float aY, float aZ, float &aLo, float &aHi) const {
  float bestDz = kInf;
  Near(aX, aY, [&](float aAcross, float aZ0) {
    if (aAcross > radius) {
      return;
    }
    const float lo = aZ0 + low, hi = aZ0 + high;
    const float dz = std::max({0.0f, lo - aZ, aZ - hi});
    if (dz < bestDz) {
      bestDz = dz;
      aLo = lo;
      aHi = hi;
    }
  });
  return std::isfinite(bestDz);
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

} // namespace Navigation::detail
