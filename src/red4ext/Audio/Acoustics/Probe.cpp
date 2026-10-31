#include "Probe.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>

namespace Acoustics {

namespace {

constexpr float kInf = std::numeric_limits<float>::infinity();
// a fresh View after this many probes (about 10 s at 20 Hz)
constexpr int kViewProbes = 200;

Vec3 Normalized(Vec3 v) {
  float n = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
  return n > 1e-6f ? Vec3{v.x / n, v.y / n, v.z / n} : Vec3{1, 0, 0};
}

} // namespace

Probe::Probe(const Navigation::NavMap &aMap) : map(aMap) {
  // Fibonacci sphere
  const double golden = std::numbers::pi * (3.0 - std::sqrt(5.0));
  for (int i = 0; i < kSphereRays; ++i) {
    double z = 1.0 - 2.0 * (i + 0.5) / kSphereRays;
    double r = std::sqrt(1.0 - z * z);
    double a = golden * (i + 0.5);
    sphere[i] = {float(r * std::cos(a)), float(r * std::sin(a)), float(z)};
  }
}

bool Probe::Free(float aX, float aY, float aZ) {
  int32_t cx, cy;
  if (!map.CellOf(aX, aY, cx, cy)) {
    return true;
  }
  const uint16_t *gaps;
  bool hasData;
  uint32_t n = view->Column(cx, cy, gaps, &hasData);
  if (!hasData) {
    return true;
  }
  for (uint32_t g = 0; g < n; ++g) {
    if (aZ >= map.VoxelZ(gaps[2 * g]) && aZ < map.VoxelZ(gaps[2 * g + 1])) {
      return true;
    }
  }
  return false;
}

float Probe::March(const Vec3 &aOrigin, const Vec3 &aDir) {
  const int steps = int(kMaxDistance / kStep);
  bool beenFree = false;
  for (int i = 1; i <= steps; ++i) {
    float t = i * kStep;
    bool free = Free(aOrigin.x + aDir.x * t, aOrigin.y + aDir.y * t, aOrigin.z + aDir.z * t);
    if (free) {
      beenFree = true;
    } else if (beenFree) {
      float h = std::hypot(aDir.x, aDir.y);
      float shell = 1.0f / std::sqrt(std::pow(h / map.CarRadius(), 2.0f) + std::pow(aDir.z / map.CarHalfHeight(), 2.0f));
      return t - kStep / 2 + shell;
    }
  }
  return kInf;
}

void Probe::FloorCeiling(const Vec3 &aPos, float &aFloor, float &aCeiling) {
  aFloor = aCeiling = kInf;
  int32_t cx, cy;
  if (!map.CellOf(aPos.x, aPos.y, cx, cy)) {
    return;
  }
  const uint16_t *gaps;
  uint32_t n = view->Column(cx, cy, gaps);
  // the gap the car is in; a car centre closer to the ground than the grown solid (sitting on a
  // road) takes the gap just above
  float bottom = kInf, top = kInf;
  bool found = false;
  for (uint32_t g = 0; g < n; ++g) {
    float b = map.VoxelZ(gaps[2 * g]), t = map.VoxelZ(gaps[2 * g + 1]);
    if (aPos.z >= b && aPos.z < t) {
      bottom = b;
      top = t;
      found = true;
      break;
    }
    if (!found && std::isinf(bottom) && b > aPos.z && b - aPos.z < 2 * map.CarHalfHeight()) {
      bottom = b;
      top = t;
    }
  }
  if (std::isinf(bottom)) {
    return;
  }
  aFloor = std::max(0.3f, aPos.z - (bottom - map.CarHalfHeight()));
  aCeiling = std::isinf(top) ? kInf : top + map.CarHalfHeight() - aPos.z;
}

float Probe::Fan(const Vec3 &aOrigin, const Vec3 &aSide) {
  constexpr float yaws[] = {-30, -15, 0, 15, 30};
  constexpr float pitches[] = {-10, 0, 10};
  const float base = std::atan2(aSide.y, aSide.x);
  const float rad = std::numbers::pi_v<float> / 180.0f;
  float nearest = kInf;
  for (float p : pitches) {
    for (float y : yaws) {
      float a = base + y * rad, e = p * rad;
      nearest = std::min(nearest, March(aOrigin, {std::cos(e) * std::cos(a), std::cos(e) * std::sin(a), std::sin(e)}));
    }
  }
  return nearest;
}

Features Probe::operator()(const Vec3 &aPos, const Vec3 &aForward, const Vec3 &aUp) {
  if (!view || ++probes % kViewProbes == 0) {
    view.reset();
    view.emplace(map);
  }
  Features f{};
  // right = forward x up, flattened
  Vec3 right{aForward.y * aUp.z - aForward.z * aUp.y, aForward.z * aUp.x - aForward.x * aUp.z, 0};
  f.rightDir = Normalized(right);
  FloorCeiling(aPos, f.floor, f.ceiling);
  f.right = Fan(aPos, f.rightDir);
  f.left = Fan(aPos, {-f.rightDir.x, -f.rightDir.y, 0});
  int hits = 0;
  double sum = 0;
  for (const auto &d : sphere) {
    float r = March(aPos, d);
    if (!std::isinf(r)) {
      ++hits;
      sum += r;
    }
  }
  f.enclosure = float(hits) / kSphereRays;
  f.meanFreePath = hits ? float(sum / hits) : kInf;
  return f;
}

} // namespace Acoustics
