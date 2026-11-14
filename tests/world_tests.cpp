// What the Autopilot asks of the world (src/red4ext/Navigation/World): gaps, pads, gates and rays.
#include "check.hpp"

#include "nav_fixture.hpp"
#include "Navigation/World.hpp"

#include <cmath>

using namespace Navigation;

namespace {
// 256 x 256 m from the origin, floor at 0, a 20 m block from (100, 100) to (140, 140)
const NavMap &Fixture() {
  static NavMap map;
  static bool loaded = [] {
    NavFixture::Spec spec;
    spec.blocks.push_back({100, 100, 140, 140, 20});
    return map.Load(NavFixture::Make(spec), "fixture");
  }();
  CHECK(loaded);
  return map;
}

Helipad Pad(float aX, float aY) { return {"", "", aX, aY, 0, 0, 5, 1e9f}; }
Gate GateAt(float aX, float aY) { return {aX, aY, 10, 0, 8, 10, 1e9f}; }
} // namespace

TEST(world_fixture_columns) {
  const auto &map = Fixture();
  // the floor's gap: the car's centre half its height above the floor, open sky above
  auto gaps = map.GetGaps(51, 51);
  CHECK(gaps.size() == 1);
  CHECK_NEAR(gaps[0].bottom, 1.0, 1e-4);
  CHECK(std::isinf(gaps[0].top));
  // on the block
  gaps = map.GetGaps(121, 121);
  CHECK(gaps.size() == 1);
  CHECK_NEAR(gaps[0].bottom, 21.0, 1e-4);
  CHECK(!map.IsFree(121, 121, 10));
  CHECK(map.IsFree(121, 121, 30));
  // outside the map
  CHECK(map.GetGaps(-10, 50).empty());
}

TEST(world_gap_at) {
  const std::vector<Helipad> pads;
  const std::vector<Gate> gates;
  MapWorld world(Fixture(), pads, gates);
  Gap g{};
  // in the gap, and below it (inside the floor): the first gap whose top is above the point
  CHECK(GapAt(world, {51, 51, 30}, g));
  CHECK_NEAR(g.bottom, 1.0, 1e-4);
  CHECK(GapAt(world, {121, 121, 5}, g));
  CHECK_NEAR(g.bottom, 21.0, 1e-4);
  // no column
  CHECK(!GapAt(world, {-10, 51, 30}, g));
  CHECK_NEAR(world.CarHalfHeight(), 1.0, 1e-6);
}

TEST(world_pads_by_distance) {
  const std::vector<Helipad> pads{Pad(100, 0), Pad(10, 0), Pad(50, 0)};
  CHECK(NearestPad(pads, 0, 0, 200) == 1);
  CHECK(NearestPad(pads, 0, 0, 5) == -1);
  // within the distance, inclusive
  CHECK(NearestPad(pads, 0, 0, 10) == 1);
  auto in = PadsInRange(pads, 0, 0, 60);
  CHECK(in.size() == 2);
  CHECK(in[0] == 1 && in[1] == 2);
  const std::vector<Gate> gates{GateAt(0, 30), GateAt(0, 20), GateAt(0, 300)};
  auto g = GatesInRange(gates, 0, 0, 100);
  CHECK(g.size() == 2);
  CHECK(g[0] == 1 && g[1] == 0);
}

TEST(world_ray_box) {
  const Box box{{-1, -1, 0}, {1, 1, 2}};
  // straight down from 6 m over it to -2 m: enters the top at z 2, t 0.5
  CHECK_NEAR(RayBox({0, 0, 6}, {0, 0, -2}, box), 0.5, 1e-6);
  // beside it
  CHECK(RayBox({3, 0, 6}, {3, 0, -2}, box) < 0.0f);
  // from inside: at once
  CHECK_NEAR(RayBox({0, 0, 1}, {0, 0, -2}, box), 0.0, 1e-6);
  // stops short of it
  CHECK(RayBox({0, 0, 6}, {0, 0, 3}, box) < 0.0f);
}

TEST(world_map_world_rays) {
  const std::vector<Helipad> pads;
  const std::vector<Gate> gates;
  MapWorld world(Fixture(), pads, gates);
  Vec3 hit{};
  // nothing to hit: the world has not streamed in
  CHECK(!world.VehicleRay({0, 0, 6}, {0, 0, -2}));
  CHECK(!world.GroundRay({0, 0, 2}, {0, 0, -6}, hit));
  world.vehicles.push_back({{-2, -1, 0}, {2, 1, 1.5f}});
  world.ground.push_back({{-50, -50, -1}, {50, 50, 0}});
  world.ground.push_back({{-5, -5, 0}, {5, 5, 0.3f}});
  CHECK(world.VehicleRay({0, 0, 6}, {0, 0, -2}));
  CHECK(!world.VehicleRay({4, 0, 6}, {4, 0, -2}));
  // the nearest of the ground boxes
  CHECK(world.GroundRay({0, 0, 2}, {0, 0, -6}, hit));
  CHECK_NEAR(hit.z, 0.3, 1e-5);
  CHECK(world.GroundRay({20, 0, 2}, {20, 0, -6}, hit));
  CHECK_NEAR(hit.z, 0.0, 1e-5);
  CHECK_NEAR(hit.x, 20.0, 1e-5);
}

TEST(world_sea_is_the_floor) {
  // the bake knows no water: a seabed 20 m below the sea's surface, open sky above it. The gap starts at the
  // surface plus the car's half height (1 m), so nothing plans through the water
  static NavMap sea;
  NavFixture::Spec spec;
  spec.z0 = -20.0f;
  spec.blocks.push_back({100, 100, 140, 140, 10}); // an island 10 m above the surface
  CHECK(sea.Load(NavFixture::Make(spec), "sea"));
  auto gaps = sea.GetGaps(50, 50);
  CHECK(gaps.size() == 1);
  if (!gaps.empty()) {
    CHECK_NEAR(gaps[0].bottom, NavMap::kSeaLevel + 1.0f, 1e-4);
    CHECK(std::isinf(gaps[0].top));
  }
  CHECK(!sea.IsFree(50, 50, -5.0f));
  CHECK(sea.IsFree(50, 50, 2.0f));
  // land above the surface keeps its own floor
  gaps = sea.GetGaps(120, 120);
  CHECK(!gaps.empty() && std::fabs(gaps[0].bottom - 11.0f) < 1e-4f);
}
