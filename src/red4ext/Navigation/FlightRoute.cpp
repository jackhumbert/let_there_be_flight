#include "FlightRoute.hpp"
#include "MapSpace.hpp"
#include "NavMap.hpp"

#include <algorithm>
#include <cmath>

using Navigation::Vec3;

namespace {
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
  if (aPoints.size < 3) {
    return aPoints;
  }
  return ToScript(Navigation::RoundCorners(FromScript(aPoints), aCut, aShare, aSmallest, aStep,
                                           Navigation::MapSpace(Navigation::NavMap::Get())));
}

RED4ext::DynArray<RED4ext::Vector4> FlightRoute::SmoothJoin(RED4ext::DynArray<RED4ext::Vector4> aPoints, int32_t aJoin,
                                                             int32_t aLo, int32_t aHi, float aRadius, int32_t aPasses) {
  if (aJoin < 0 || aJoin >= int32_t(aPoints.size)) {
    return aPoints;
  }
  return ToScript(Navigation::SmoothJoin(FromScript(aPoints), aJoin, aLo, aHi, aRadius, aPasses,
                                         Navigation::MapSpace(Navigation::NavMap::Get())));
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
