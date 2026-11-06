// The acoustics probe's live sphere (src/red4ext/Audio/Acoustics/Probe): the rays bunched where the car is going, each
// weighed back so the enclosure and the mean free path stay the whole sphere's; live hits used, misses the nav map's.
#include "check.hpp"
#include "nav_fixture.hpp"

#include "Audio/Acoustics/Probe.hpp"

#include <cmath>

using namespace Acoustics;

namespace {
// 256 x 256 m, floor at 0, nothing standing on it
const Navigation::NavMap &Open() {
  static Navigation::NavMap map;
  static bool loaded = [] {
    NavFixture::Spec spec;
    return map.Load(NavFixture::Make(spec), "open");
  }();
  CHECK(loaded);
  return map;
}

float Dot(const Vec3 &a, const Vec3 &b) { return a.x * b.x + a.y * b.y + a.z * b.z; }

// the share of the weight on rays with Dot(dir, aAxis) > aCos
float WeightShare(const std::vector<Vec3> &aDir, const std::vector<float> &aWeight, const Vec3 &aAxis, float aCos) {
  double in = 0, all = 0;
  for (size_t i = 0; i < aDir.size(); ++i) {
    all += aWeight[i];
    in += Dot(aDir[i], aAxis) > aCos ? aWeight[i] : 0.0;
  }
  return float(in / all);
}
} // namespace

TEST(probe_sphere_rays_at_rest_uniform) {
  std::vector<Vec3> dir;
  std::vector<float> weight;
  Probe::SphereRays(992, {1, 0, 0}, 0.0f, 0.0f, dir, weight);
  CHECK(dir.size() == 992 && weight.size() == 992);
  Vec3 mean{};
  for (size_t i = 0; i < dir.size(); ++i) {
    CHECK_NEAR(Dot(dir[i], dir[i]), 1.0, 1e-4);
    CHECK_NEAR(weight[i], 1.0, 1e-4);
    mean = {mean.x + dir[i].x, mean.y + dir[i].y, mean.z + dir[i].z};
  }
  CHECK(std::sqrt(Dot(mean, mean)) / 992 < 0.01f);
  // a uniform sphere: a quarter of it within 60 degrees of any axis
  std::vector<float> ones(dir.size(), 1.0f);
  CHECK_NEAR(WeightShare(dir, ones, {1, 0, 0}, 0.5f), 0.25, 0.01);
}

TEST(probe_sphere_rays_bunch_ahead_weights_restore_the_sphere) {
  const Vec3 ahead{0.6f, 0.8f, 0.0f};
  std::vector<Vec3> dir;
  std::vector<float> weight;
  Probe::SphereRays(992, ahead, 60.0f, 0.4f, dir, weight);
  CHECK_NEAR(Probe::SphereBias(60.0f), 0.35, 1e-6);
  std::vector<float> ones(dir.size(), 1.0f);
  // the rays: well over a quarter within 60 degrees of ahead, few behind
  const float rays = WeightShare(dir, ones, ahead, 0.5f);
  std::printf("  %.2f of the rays within 60 deg of ahead at 60 m/s (0.25 uniform)\n", rays);
  CHECK(rays > 0.45f);
  CHECK(WeightShare(dir, ones, {-ahead.x, -ahead.y, -ahead.z}, 0.5f) < 0.1f);
  // weighted, any part of the sphere has its own share again: ahead, behind, below, a side; within 4% (the few rays
  // behind each stand for a wide patch, so how they fall round the axis moves a half sphere's share by about 3%)
  CHECK_NEAR(WeightShare(dir, weight, ahead, 0.5f), 0.25, 0.01);
  CHECK_NEAR(WeightShare(dir, weight, {-ahead.x, -ahead.y, -ahead.z}, 0.5f), 0.25, 0.01);
  CHECK_NEAR(WeightShare(dir, weight, {0, 0, -1}, 0.0f), 0.5, 0.04);
  CHECK_NEAR(WeightShare(dir, weight, {0.8f, -0.6f, 0}, 0.0f), 0.5, 0.04);
  double total = 0;
  for (float w : weight)
    total += w;
  CHECK_NEAR(total, 992.0, 0.01);
}

TEST(probe_live_sphere_hits_and_misses) {
  Probe probe(Open());
  const Vec3 at{128, 128, 5}, forward{0, 1, 0}, up{0, 0, 1};
  const Features map = probe(at, forward, up);
  CHECK(map.sphereLive == -1);
  CHECK_NEAR(map.enclosure, map.enclosureMap, 1e-6);

  LiveHits live{};
  live.origin = at;
  live.distance.fill(-1.0f);
  Probe::SphereRays(992, {1, 0, 0}, 0.0f, 0.0f, live.sphereDir, live.sphereWeight);
  // every ray hit 10 m out: enclosed, a 10 m mean free path
  live.sphereDistance.assign(live.sphereDir.size(), 10.0f);
  Features f = probe(at, forward, up, &live);
  CHECK(f.sphereLive == 992);
  CHECK_NEAR(f.enclosure, 1.0, 1e-5);
  CHECK_NEAR(f.meanFreePath, 10.0, 1e-4);
  CHECK_NEAR(f.enclosureMap, map.enclosureMap, 1e-6);
  // every ray missed (not streamed in, say): the nav map answers, near enough its own 48 rays' figure
  live.sphereDistance.assign(live.sphereDir.size(), -1.0f);
  f = probe(at, forward, up, &live);
  CHECK(f.sphereLive == 0);
  std::printf("  all live misses: enclosure %.3f against the map's %.3f\n", f.enclosure, f.enclosureMap);
  CHECK_NEAR(f.enclosure, f.enclosureMap, 0.05);
}

TEST(probe_live_sphere_bunched_still_the_whole_sphere) {
  // the floor live 5 m below, nothing above: half the sphere encloses, however the rays are bunched
  Probe probe(Open());
  const Vec3 at{128, 128, 5}, forward{0, 1, 0}, up{0, 0, 1};
  LiveHits live{};
  live.origin = at;
  live.distance.fill(-1.0f);
  Probe::SphereRays(992, {0.7071f, 0, -0.7071f}, 80.0f, 0.0f, live.sphereDir, live.sphereWeight);
  live.sphereDistance.resize(live.sphereDir.size());
  for (size_t i = 0; i < live.sphereDir.size(); ++i) {
    // a ray reaching the floor further than 200 m counts as open
    const float down = -live.sphereDir[i].z;
    live.sphereDistance[i] = down > 0.025f ? 5.0f / down : -1.0f;
  }
  // the misses fall back on the open map, which has no ceiling either: the upper half stays open
  const Features f = probe(at, forward, up, &live);
  std::printf("  diving at 80 m/s: enclosure %.3f (0.5 the whole sphere's)\n", f.enclosure);
  CHECK_NEAR(f.enclosure, 0.5, 0.04);
  // unweighted, the bunching would read it as more enclosed
  LiveHits flat = live;
  flat.sphereWeight.clear();
  CHECK(probe(at, forward, up, &flat).enclosure > 0.6f);
}
