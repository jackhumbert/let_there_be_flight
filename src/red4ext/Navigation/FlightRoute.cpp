#include "FlightRoute.hpp"
#include "MapSpace.hpp"
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
} // namespace

RED4ext::Handle<FlightRoute> FlightRoute::Create(RED4ext::DynArray<RED4ext::Vector4> aPoints) {
  auto handle = RED4ext::MakeHandle<FlightRoute>();
  if (aPoints.size < 2) {
    return handle;
  }
  handle->route = Navigation::Route::Make(FromScript(aPoints));
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

RED4ext::Vector4 FlightRoute::Point(int32_t aIndex) { return ToPoint(route.Point(aIndex)); }

float FlightRoute::Distance(int32_t aIndex) { return route.Distance(aIndex); }

RED4ext::DynArray<RED4ext::Vector4> FlightRoute::Points() { return ToScript(route.points); }

int32_t FlightRoute::Nearest(RED4ext::Vector4 aP, int32_t aFrom, int32_t aCount) {
  return route.Nearest(ToVec(aP), aFrom, aCount);
}

RED4ext::Vector4 FlightRoute::Project(RED4ext::Vector4 aP, int32_t aSegment) {
  if (route.Size() < 2) {
    return aP;
  }
  return ToPoint(route.Project(ToVec(aP), aSegment));
}

float FlightRoute::DistanceTo(RED4ext::Vector4 aQ, int32_t aSegment, float aAhead) {
  return route.DistanceTo(ToVec(aQ), aSegment, aAhead);
}

RED4ext::Vector4 FlightRoute::At(float aS) { return ToPoint(route.At(aS)); }

RED4ext::Vector4 FlightRoute::Ahead(int32_t aSegment, RED4ext::Vector4 aFrom, float aAlong) {
  if (route.Size() == 0) {
    return aFrom;
  }
  return ToPoint(route.Ahead(aSegment, ToVec(aFrom), aAlong));
}

bool FlightRoute::ClearAhead(int32_t aSegment, RED4ext::Vector4 aFrom, float aAlong) {
  return route.ClearAhead(aSegment, ToVec(aFrom), aAlong, Navigation::MapSpace(Navigation::NavMap::Get()));
}

float FlightRoute::Turn(int32_t aSegment, RED4ext::Vector4 aFrom, float aAhead) {
  return route.Turn(aSegment, ToVec(aFrom), aAhead);
}

RED4ext::Vector4 FlightRoute::Curvature(int32_t aSegment, RED4ext::Vector4 aFrom, float aAhead) {
  return ToDirection(route.Curvature(aSegment, ToVec(aFrom), aAhead));
}

RED4ext::Vector4 FlightRoute::Tangent(float aS, float aSpan) { return ToDirection(route.Tangent(aS, aSpan)); }

RED4ext::Vector4 FlightRoute::Bend(float aS, float aSpan) { return ToDirection(route.Bend(aS, aSpan)); }

float FlightRoute::SharpBend(int32_t aSegment, RED4ext::Vector4 aFrom, float aSpan) {
  return route.SharpBend(aSegment, ToVec(aFrom), aSpan);
}
