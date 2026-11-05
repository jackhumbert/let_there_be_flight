#include "Route.hpp"

#include <algorithm>
#include <cmath>

namespace Navigation {

namespace {
Vec3 operator+(const Vec3 &a, const Vec3 &b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
Vec3 operator-(const Vec3 &a, const Vec3 &b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
Vec3 operator*(const Vec3 &a, float f) { return {a.x * f, a.y * f, a.z * f}; }
float Dot(const Vec3 &a, const Vec3 &b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
float Len(const Vec3 &a) { return std::sqrt(Dot(a, a)); }
float Dist(const Vec3 &a, const Vec3 &b) { return Len(a - b); }
Vec3 Normalize(const Vec3 &a) {
  float l = Len(a);
  return l > 0.0f ? a * (1.0f / l) : Vec3{0.0f, 0.0f, 0.0f};
}

// the bend from a through b to c: towards the centre, 1 / radius long (zero where a leg is no
// longer than aShortest)
Vec3 BendOf(const Vec3 &a, const Vec3 &b, const Vec3 &c, float aShortest) {
  Vec3 ab = b - a, bc = c - b;
  float l1 = Len(ab), l2 = Len(bc);
  if (l1 <= aShortest || l2 <= aShortest) {
    return {0.0f, 0.0f, 0.0f};
  }
  return (bc * (1.0f / l2) - ab * (1.0f / l1)) * (2.0f / (l1 + l2));
}
} // namespace

Route Route::Make(std::vector<Vec3> aPoints) {
  Route route;
  if (aPoints.size() < 2) {
    return route;
  }
  route.points = std::move(aPoints);
  route.lengths.reserve(route.points.size());
  float total = 0.0f;
  for (size_t i = 0; i < route.points.size(); ++i) {
    if (i > 0) {
      total += Dist(route.points[i - 1], route.points[i]);
    }
    route.lengths.push_back(total);
  }
  return route;
}

Vec3 Route::Point(int32_t aIndex) const {
  if (points.empty()) {
    return {0.0f, 0.0f, 0.0f};
  }
  return points[std::clamp(aIndex, 0, Size() - 1)];
}

float Route::Distance(int32_t aIndex) const {
  if (lengths.empty()) {
    return 0.0f;
  }
  return lengths[std::clamp(aIndex, 0, Size() - 1)];
}

int32_t Route::Nearest(const Vec3 &aP, int32_t aFrom, int32_t aCount) const {
  int32_t n = Size();
  float best = 1e9f;
  int32_t bestSegment = aFrom;
  for (int32_t i = std::max(aFrom, 0); i < n - 1 && i <= aFrom + aCount; ++i) {
    Vec3 a = points[i], ab = points[i + 1] - a;
    float len2 = Dot(ab, ab);
    float t = len2 > 0.0001f ? std::clamp(Dot(aP - a, ab) / len2, 0.0f, 1.0f) : 0.0f;
    float d = Dist(aP, a + ab * t);
    if (d < best) {
      best = d;
      bestSegment = i;
    }
  }
  return bestSegment;
}

Vec3 Route::Project(const Vec3 &aP, int32_t aSegment) const {
  int32_t n = Size();
  if (n < 2) {
    return aP;
  }
  int32_t i = std::clamp(aSegment, 0, n - 2);
  Vec3 a = points[i], ab = points[i + 1] - a;
  float len2 = Dot(ab, ab);
  float t = len2 > 0.0001f ? std::clamp(Dot(aP - a, ab) / len2, 0.0f, 1.0f) : 0.0f;
  return a + ab * t;
}

float Route::DistanceTo(const Vec3 &aQ, int32_t aSegment, float aAhead) const {
  int32_t n = Size();
  if (n < 2) {
    return 1e9f;
  }
  float best = 1e9f;
  int32_t first = std::clamp(aSegment, 0, n - 1);
  float end = lengths[first] + aAhead;
  for (int32_t i = first; i < n - 1 && lengths[i] <= end; ++i) {
    Vec3 a = points[i], ab = points[i + 1] - a;
    float len2 = Dot(ab, ab);
    float t = len2 > 0.0001f ? std::clamp(Dot(aQ - a, ab) / len2, 0.0f, 1.0f) : 0.0f;
    best = std::min(best, Dist(aQ, a + ab * t));
  }
  return best;
}

Vec3 Route::At(float aS) const {
  if (points.empty()) {
    return {0.0f, 0.0f, 0.0f};
  }
  if (aS <= 0.0f) {
    return points.front();
  }
  // the first point at or past s; s lies on the segment ending there
  auto it = std::lower_bound(lengths.begin() + 1, lengths.end(), aS);
  if (it == lengths.end()) {
    return points.back();
  }
  size_t i = size_t(it - lengths.begin()) - 1;
  float len = lengths[i + 1] - lengths[i];
  float t = len > 0.001f ? (aS - lengths[i]) / len : 0.0f;
  return points[i] + (points[i + 1] - points[i]) * t;
}

Vec3 Route::Ahead(int32_t aSegment, const Vec3 &aFrom, float aAlong) const {
  if (points.empty()) {
    return aFrom;
  }
  int32_t n = Size();
  Vec3 p = aFrom;
  float left = aAlong;
  for (int32_t i = std::max(aSegment, 0); i < n - 1; ++i) {
    const Vec3 &b = points[i + 1];
    float d = Dist(p, b);
    if (d >= left) {
      return p + Normalize(b - p) * left;
    }
    left -= d;
    p = b;
  }
  return points.back();
}

bool Route::ClearAhead(int32_t aSegment, const Vec3 &aFrom, float aAlong, const Space &aSpace) const {
  int32_t n = Size();
  Vec3 p = aFrom;
  float left = aAlong;
  for (int32_t i = std::max(aSegment, 0); i < n - 1 && left > 0.0f; ++i) {
    const Vec3 &b = points[i + 1];
    float d = Dist(p, b);
    Vec3 end = d > left ? p + Normalize(b - p) * left : b;
    if (aSpace.SegmentClear(p, end) < 1.0f) {
      return false;
    }
    left -= d;
    p = b;
  }
  return true;
}

float Route::Turn(int32_t aSegment, const Vec3 &aFrom, float aAhead) const {
  if (points.size() < 2) {
    return 0.0f;
  }
  Vec3 a = Ahead(aSegment, aFrom, 0.5f * aAhead);
  Vec3 b = Ahead(aSegment, aFrom, aAhead);
  Vec3 c = Ahead(aSegment, aFrom, 1.5f * aAhead);
  Vec3 d1{b.x - a.x, b.y - a.y, 0.0f}, d2{c.x - b.x, c.y - b.y, 0.0f};
  float l1 = Len(d1), l2 = Len(d2);
  if (l1 < 1.0f || l2 < 1.0f) {
    return 0.0f;
  }
  // the heading change from d1 to d2, anticlockwise seen from above positive
  float turn = std::atan2(d1.x * d2.y - d1.y * d2.x, d1.x * d2.x + d1.y * d2.y);
  return -turn / (0.5f * (l1 + l2));
}

Vec3 Route::Curvature(int32_t aSegment, const Vec3 &aFrom, float aAhead) const {
  if (points.size() < 2) {
    return {0.0f, 0.0f, 0.0f};
  }
  Vec3 a = Ahead(aSegment, aFrom, 0.1f * aAhead);
  Vec3 b = Ahead(aSegment, aFrom, 0.4f * aAhead);
  Vec3 c = Ahead(aSegment, aFrom, 0.7f * aAhead);
  return BendOf(a, b, c, 1.0f);
}

Vec3 Route::Tangent(float aS, float aSpan) const {
  if (points.size() < 2) {
    return {0.0f, 0.0f, 0.0f};
  }
  Vec3 ac = At(aS + aSpan) - At(aS - aSpan);
  return Len(ac) > 0.01f ? Normalize(ac) : Vec3{0.0f, 0.0f, 0.0f};
}

Vec3 Route::Bend(float aS, float aSpan) const {
  if (points.size() < 2) {
    return {0.0f, 0.0f, 0.0f};
  }
  return BendOf(At(aS - aSpan), At(aS), At(aS + aSpan), 0.5f);
}

float Route::SharpBend(int32_t aSegment, const Vec3 &aFrom, float aSpan) const {
  int32_t n = Size();
  if (aSegment < 0 || aSegment + 1 >= n) {
    return 1000000.0f;
  }
  float start = lengths[aSegment + 1] - Dist(aFrom, points[aSegment + 1]);
  for (int32_t k = aSegment + 1; k < n - 1 && lengths[k] - start < 300.0f; ++k) {
    float s = lengths[k];
    Vec3 in = points[k] - At(s - aSpan);
    Vec3 out = At(s + aSpan) - points[k];
    if (Len(in) > 0.5f && Len(out) > 0.5f && Dot(Normalize(in), Normalize(out)) < 0.906f) {
      return s - start;
    }
  }
  return 1000000.0f;
}

std::vector<Vec3> RoundCorners(const std::vector<Vec3> &aPoints, float aCut, float aShare, float aSmallest,
                               float aStep, const Space &aSpace) {
  const size_t n = aPoints.size();
  if (n < 3) {
    return aPoints;
  }
  auto clear = [&](const Vec3 &a, const Vec3 &b) { return aSpace.SegmentClear(a, b) >= 1.0f; };
  std::vector<Vec3> out{aPoints[0]};
  for (size_t i = 1; i + 1 < n; ++i) {
    const Vec3 &a = aPoints[i - 1], &b = aPoints[i], &c = aPoints[i + 1];
    bool rounded = false;
    // full cut, then smaller; below the smallest the corner stays sharp
    for (float cut = aCut; !rounded && cut >= aSmallest && aStep < 1.0f; cut *= aStep) {
      float d1 = std::min(aShare * Dist(a, b), cut);
      float d2 = std::min(aShare * Dist(b, c), cut);
      if (d1 <= 1.0f || d2 <= 1.0f) {
        continue;
      }
      Vec3 q1 = b + Normalize(a - b) * d1;
      Vec3 q2 = b + Normalize(c - b) * d2;
      Vec3 arc[9];
      for (int k = 0; k <= 8; ++k) {
        float t = float(k) / 8.0f;
        arc[k] = q1 * ((1.0f - t) * (1.0f - t)) + b * (2.0f * t * (1.0f - t)) + q2 * (t * t);
      }
      bool ok = clear(out.back(), arc[0]);
      for (int k = 0; ok && k < 8; ++k) {
        ok = clear(arc[k], arc[k + 1]);
      }
      if (ok) {
        out.insert(out.end(), std::begin(arc), std::end(arc));
        rounded = true;
      }
    }
    if (!rounded) {
      out.push_back(b);
    }
  }
  out.push_back(aPoints[n - 1]);
  return out;
}

std::vector<Vec3> SmoothJoin(std::vector<Vec3> aPoints, int32_t aJoin, int32_t aLo, int32_t aHi, float aRadius,
                             int32_t aPasses, const Space &aSpace) {
  const int32_t n = int32_t(aPoints.size());
  if (aJoin < 0 || aJoin >= n) {
    return aPoints;
  }
  auto clear = [&](const Vec3 &a, const Vec3 &b) { return aSpace.SegmentClear(a, b) >= 1.0f; };
  const Vec3 centre = aPoints[aJoin];
  const int32_t lo = std::max(aLo, 1);
  const int32_t hi = std::min(aHi, n - 2);
  for (int32_t pass = 0; pass < aPasses; ++pass) {
    for (int32_t k = lo; k <= hi; ++k) {
      if (Dist(aPoints[k], centre) < aRadius) {
        Vec3 q = aPoints[k] * 0.5f + (aPoints[k - 1] + aPoints[k + 1]) * 0.25f;
        if (clear(aPoints[k - 1], q) && clear(q, aPoints[k + 1])) {
          aPoints[k] = q;
        }
      }
    }
  }
  return aPoints;
}

} // namespace Navigation
