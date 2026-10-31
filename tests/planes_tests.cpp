// Planes fitted to the PhysX hits (src/red4ext/Audio/Acoustics/Planes): the side of a direction, a wall from
// noisy hits, a pole that must not win, a wall kept through a gap and too few hits. The noise comes from
// a fixed-seed generator, so the tests are the same every run.
#include "check.hpp"

#include "Audio/Acoustics/Planes.hpp"

#include <cmath>
#include <cstdint>

using namespace Acoustics;
using namespace Acoustics::Planes;

namespace {

constexpr float kPi = 3.14159265f;

// uniform in [-1, 1) from a linear congruential generator
struct Noise {
  uint32_t state;
  float Next() {
    state = state * 1664525u + 1013904223u;
    return float(state >> 8) / 8388608.0f - 1.0f;
  }
};

float DegreesBetween(const Vec3 &a, const Vec3 &b) {
  const float dot = a.x * b.x + a.y * b.y + a.z * b.z;
  return std::acos(std::fmax(-1.0f, std::fmin(1.0f, dot))) * 180.0f / kPi;
}

float DistanceTo(const Plane &aPlane, const Vec3 &aCar) {
  return (aCar.x - aPlane.point.x) * aPlane.normal.x + (aCar.y - aPlane.point.y) * aPlane.normal.y +
         (aCar.z - aPlane.point.z) * aPlane.normal.z;
}

} // namespace

TEST(planes_side_of) {
  CHECK(SideOf({0, 0, -1}, {1, 0, 0}) == Floor);
  CHECK(SideOf({0, 0, 1}, {1, 0, 0}) == Ceiling);
  CHECK(SideOf({1, 0, 0}, {1, 0, 0}) == Right);
  CHECK(SideOf({-1, 0.2f, 0}, {1, 0, 0}) == Left);
}

TEST(planes_fit_noisy) {
  Noise noise{12345};
  const float c = std::cos(30.0f * kPi / 180.0f), s = std::sin(30.0f * kPi / 180.0f);
  const Vec3 right{c, s, 0}, along{-s, c, 0};
  Cache cache;
  for (int i = 0; i < 30; ++i) {
    const float u = 5.0f * noise.Next(); // over the 10 m of wall
    const float h = 3.0f * noise.Next(); // and 6 m of height
    const float offset = 0.5f * noise.Next();
    const float tilt = 5.0f * kPi / 180.0f * noise.Next();
    const float d = 10.0f + offset;
    Hit hit;
    hit.position = {d * right.x + u * along.x, d * right.y + u * along.y, h};
    hit.normal = {-std::cos(30.0f * kPi / 180.0f + tilt), -std::sin(30.0f * kPi / 180.0f + tilt), 0};
    hit.time = 0;
    cache.Add(hit);
  }
  const auto plane = cache.Fit(Right, {0, 0, 0}, right);
  CHECK(plane.has_value());
  if (plane) {
    CHECK_NEAR(DistanceTo(*plane, {0, 0, 0}), 10.0f, 0.25f);
    CHECK(DegreesBetween(plane->normal, {-c, -s, 0}) < 3.0f);
  }
}

TEST(planes_pole_ignored) {
  Noise noise{777};
  Cache cache;
  for (int i = 0; i < 20; ++i) {
    const float y = 5.0f * noise.Next(), z = 3.0f * noise.Next(), x = 10.0f + 0.3f * noise.Next();
    cache.Add({{x, y, z}, {-1, 0, 0}, 0});
  }
  // a pole: three hits on a post 4 m away, facing the car
  cache.Add({{4, 0.1f, 0}, {-1, 0, 0}, 0});
  cache.Add({{4, 0.0f, 1}, {-1, 0, 0}, 0});
  cache.Add({{4, -0.1f, 2}, {-1, 0, 0}, 0});
  const auto plane = cache.Fit(Right, {0, 0, 0}, {1, 0, 0});
  CHECK(plane.has_value());
  if (plane) {
    CHECK_NEAR(DistanceTo(*plane, {0, 0, 0}), 10.0f, 0.35f);
    CHECK(std::fabs(DistanceTo(*plane, {0, 0, 0}) - 4.0f) > 1.0f);
  }
}

TEST(planes_wall_through_gap) {
  Cache cache;
  const Vec3 car{0, 0, 0}, right{1, 0, 0};
  // three hits on a wall every 0.05 s from 0 to 0.5 s, then collision stops answering
  for (int step = 0; step <= 10; ++step) {
    for (int k = 0; k < 3; ++k) {
      cache.Add({{10, float(k) - 1.0f, 0}, {-1, 0, 0}, 0.05f * float(step)});
    }
  }
  cache.Expire(1.2f, car); // the hits from 0.2 s on remain
  CHECK(cache.Fit(Right, car, right).has_value());
  cache.Expire(1.6f, car);
  CHECK(!cache.Fit(Right, car, right).has_value());
}

TEST(planes_too_few) {
  Cache cache;
  for (int k = 0; k < 3; ++k) {
    cache.Add({{10, float(k) - 1.0f, 0}, {-1, 0, 0}, 0});
  }
  CHECK(!cache.Fit(Right, {0, 0, 0}, {1, 0, 0}).has_value());
}
