#include "FlightRoute.hpp"
#include "NavMap.hpp"

#include <algorithm>
#include <cmath>

using Navigation::Vec3;

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

Vec3 ToVec(const RED4ext::Vector4 &v) { return {v.X, v.Y, v.Z}; }
RED4ext::Vector4 ToPoint(const Vec3 &v) { return {v.x, v.y, v.z, 1.0f}; }
RED4ext::Vector4 ToDirection(const Vec3 &v) { return {v.x, v.y, v.z, 0.0f}; }

std::vector<Vec3> FromScript(const RED4ext::DynArray<RED4ext::Vector4> &aPoints) {
  std::vector<Vec3> out;
  out.reserve(aPoints.size);
  for (const auto &p : aPoints) {
    out.push_back(ToVec(p));
  }
  return out;
}

RED4ext::DynArray<RED4ext::Vector4> ToScript(const std::vector<Vec3> &aPoints) {
  RED4ext::DynArray<RED4ext::Vector4> out;
  out.Reserve(uint32_t(aPoints.size()));
  for (const auto &p : aPoints) {
    out.EmplaceBack(p.x, p.y, p.z, 1.0f);
  }
  return out;
}

bool Clear(const Vec3 &a, const Vec3 &b) {
  return Navigation::NavMap::Get().SegmentClear(a.x, a.y, a.z, b.x, b.y, b.z) >= 1.0f;
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

RED4ext::Handle<FlightRoute> FlightRoute::Create(RED4ext::DynArray<RED4ext::Vector4> aPoints) {
  auto handle = RED4ext::MakeHandle<FlightRoute>();
  if (aPoints.size < 2) {
    return handle;
  }
  handle->points = FromScript(aPoints);
  handle->lengths.reserve(handle->points.size());
  float total = 0.0f;
  for (size_t i = 0; i < handle->points.size(); ++i) {
    if (i > 0) {
      total += Dist(handle->points[i - 1], handle->points[i]);
    }
    handle->lengths.push_back(total);
  }
  return handle;
}

RED4ext::DynArray<RED4ext::Vector4> FlightRoute::RoundCorners(RED4ext::DynArray<RED4ext::Vector4> aPoints, float aCut,
                                                               float aShare, float aSmallest, float aStep) {
  auto in = FromScript(aPoints);
  size_t n = in.size();
  if (n < 3) {
    return aPoints;
  }
  std::vector<Vec3> out{in[0]};
  for (size_t i = 1; i + 1 < n; ++i) {
    const Vec3 &a = in[i - 1], &b = in[i], &c = in[i + 1];
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
      bool clear = Clear(out.back(), arc[0]);
      for (int k = 0; clear && k < 8; ++k) {
        clear = Clear(arc[k], arc[k + 1]);
      }
      if (clear) {
        out.insert(out.end(), std::begin(arc), std::end(arc));
        rounded = true;
      }
    }
    if (!rounded) {
      out.push_back(b);
    }
  }
  out.push_back(in[n - 1]);
  return ToScript(out);
}

RED4ext::DynArray<RED4ext::Vector4> FlightRoute::SmoothJoin(RED4ext::DynArray<RED4ext::Vector4> aPoints, int32_t aJoin,
                                                             int32_t aLo, int32_t aHi, float aRadius, int32_t aPasses) {
  auto route = FromScript(aPoints);
  int32_t n = int32_t(route.size());
  if (aJoin < 0 || aJoin >= n) {
    return aPoints;
  }
  Vec3 centre = route[aJoin];
  int32_t lo = std::max(aLo, 1);
  int32_t hi = std::min(aHi, n - 2);
  for (int32_t pass = 0; pass < aPasses; ++pass) {
    for (int32_t k = lo; k <= hi; ++k) {
      if (Dist(route[k], centre) < aRadius) {
        Vec3 q = route[k] * 0.5f + (route[k - 1] + route[k + 1]) * 0.25f;
        if (Clear(route[k - 1], q) && Clear(q, route[k + 1])) {
          route[k] = q;
        }
      }
    }
  }
  return ToScript(route);
}

RED4ext::Vector4 FlightRoute::Point(int32_t aIndex) {
  if (points.empty()) {
    return {0.0f, 0.0f, 0.0f, 1.0f};
  }
  return ToPoint(points[std::clamp(aIndex, 0, Size() - 1)]);
}

float FlightRoute::Distance(int32_t aIndex) {
  if (lengths.empty()) {
    return 0.0f;
  }
  return lengths[std::clamp(aIndex, 0, Size() - 1)];
}

RED4ext::DynArray<RED4ext::Vector4> FlightRoute::Points() { return ToScript(points); }

int32_t FlightRoute::Nearest(RED4ext::Vector4 aP, int32_t aFrom, int32_t aCount) {
  Vec3 p = ToVec(aP);
  int32_t n = Size();
  float best = 1e9f;
  int32_t bestSegment = aFrom;
  for (int32_t i = std::max(aFrom, 0); i < n - 1 && i <= aFrom + aCount; ++i) {
    Vec3 a = points[i], ab = points[i + 1] - a;
    float len2 = Dot(ab, ab);
    float t = len2 > 0.0001f ? std::clamp(Dot(p - a, ab) / len2, 0.0f, 1.0f) : 0.0f;
    float d = Dist(p, a + ab * t);
    if (d < best) {
      best = d;
      bestSegment = i;
    }
  }
  return bestSegment;
}

RED4ext::Vector4 FlightRoute::Project(RED4ext::Vector4 aP, int32_t aSegment) {
  int32_t n = Size();
  if (n < 2) {
    return aP;
  }
  int32_t i = std::clamp(aSegment, 0, n - 2);
  Vec3 p = ToVec(aP), a = points[i], ab = points[i + 1] - a;
  float len2 = Dot(ab, ab);
  float t = len2 > 0.0001f ? std::clamp(Dot(p - a, ab) / len2, 0.0f, 1.0f) : 0.0f;
  return ToPoint(a + ab * t);
}

float FlightRoute::DistanceTo(RED4ext::Vector4 aQ, int32_t aSegment, float aAhead) {
  int32_t n = Size();
  if (n < 2) {
    return 1e9f;
  }
  Vec3 q = ToVec(aQ);
  float best = 1e9f;
  int32_t first = std::clamp(aSegment, 0, n - 1);
  float end = lengths[first] + aAhead;
  for (int32_t i = first; i < n - 1 && lengths[i] <= end; ++i) {
    Vec3 a = points[i], ab = points[i + 1] - a;
    float len2 = Dot(ab, ab);
    float t = len2 > 0.0001f ? std::clamp(Dot(q - a, ab) / len2, 0.0f, 1.0f) : 0.0f;
    best = std::min(best, Dist(q, a + ab * t));
  }
  return best;
}

Vec3 FlightRoute::AtPoint(float aS) const {
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

RED4ext::Vector4 FlightRoute::At(float aS) {
  if (points.empty()) {
    return {0.0f, 0.0f, 0.0f, 1.0f};
  }
  return ToPoint(AtPoint(aS));
}

Vec3 FlightRoute::AheadPoint(int32_t aSegment, const Vec3 &aFrom, float aAlong) const {
  int32_t n = int32_t(points.size());
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

RED4ext::Vector4 FlightRoute::Ahead(int32_t aSegment, RED4ext::Vector4 aFrom, float aAlong) {
  if (points.empty()) {
    return aFrom;
  }
  return ToPoint(AheadPoint(aSegment, ToVec(aFrom), aAlong));
}

bool FlightRoute::ClearAhead(int32_t aSegment, RED4ext::Vector4 aFrom, float aAlong) {
  int32_t n = Size();
  Vec3 p = ToVec(aFrom);
  float left = aAlong;
  for (int32_t i = std::max(aSegment, 0); i < n - 1 && left > 0.0f; ++i) {
    const Vec3 &b = points[i + 1];
    float d = Dist(p, b);
    Vec3 end = d > left ? p + Normalize(b - p) * left : b;
    if (!Clear(p, end)) {
      return false;
    }
    left -= d;
    p = b;
  }
  return true;
}

float FlightRoute::Turn(int32_t aSegment, RED4ext::Vector4 aFrom, float aAhead) {
  if (points.size() < 2) {
    return 0.0f;
  }
  Vec3 from = ToVec(aFrom);
  Vec3 a = AheadPoint(aSegment, from, 0.5f * aAhead);
  Vec3 b = AheadPoint(aSegment, from, aAhead);
  Vec3 c = AheadPoint(aSegment, from, 1.5f * aAhead);
  Vec3 d1{b.x - a.x, b.y - a.y, 0.0f}, d2{c.x - b.x, c.y - b.y, 0.0f};
  float l1 = Len(d1), l2 = Len(d2);
  if (l1 < 1.0f || l2 < 1.0f) {
    return 0.0f;
  }
  // the heading change from d1 to d2, anticlockwise seen from above positive
  float turn = std::atan2(d1.x * d2.y - d1.y * d2.x, d1.x * d2.x + d1.y * d2.y);
  return -turn / (0.5f * (l1 + l2));
}

RED4ext::Vector4 FlightRoute::Curvature(int32_t aSegment, RED4ext::Vector4 aFrom, float aAhead) {
  if (points.size() < 2) {
    return {0.0f, 0.0f, 0.0f, 0.0f};
  }
  Vec3 from = ToVec(aFrom);
  Vec3 a = AheadPoint(aSegment, from, 0.1f * aAhead);
  Vec3 b = AheadPoint(aSegment, from, 0.4f * aAhead);
  Vec3 c = AheadPoint(aSegment, from, 0.7f * aAhead);
  return ToDirection(BendOf(a, b, c, 1.0f));
}

RED4ext::Vector4 FlightRoute::Tangent(float aS, float aSpan) {
  if (points.size() < 2) {
    return {0.0f, 0.0f, 0.0f, 0.0f};
  }
  Vec3 ac = AtPoint(aS + aSpan) - AtPoint(aS - aSpan);
  return ToDirection(Len(ac) > 0.01f ? Normalize(ac) : Vec3{0.0f, 0.0f, 0.0f});
}

RED4ext::Vector4 FlightRoute::Bend(float aS, float aSpan) {
  if (points.size() < 2) {
    return {0.0f, 0.0f, 0.0f, 0.0f};
  }
  return ToDirection(BendOf(AtPoint(aS - aSpan), AtPoint(aS), AtPoint(aS + aSpan), 0.5f));
}

float FlightRoute::SharpBend(int32_t aSegment, RED4ext::Vector4 aFrom, float aSpan) {
  int32_t n = Size();
  if (aSegment < 0 || aSegment + 1 >= n) {
    return 1000000.0f;
  }
  float start = lengths[aSegment + 1] - Dist(ToVec(aFrom), points[aSegment + 1]);
  for (int32_t k = aSegment + 1; k < n - 1 && lengths[k] - start < 300.0f; ++k) {
    float s = lengths[k];
    Vec3 in = points[k] - AtPoint(s - aSpan);
    Vec3 out = AtPoint(s + aSpan) - points[k];
    if (Len(in) > 0.5f && Len(out) > 0.5f && Dot(Normalize(in), Normalize(out)) < 0.906f) {
      return s - start;
    }
  }
  return 1000000.0f;
}
