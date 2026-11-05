// Where the Autopilot lands (src/red4ext/Navigation/Landing), worked from FlightModeAutopilot.reds.
#include "check.hpp"

#include "nav_fixture.hpp"
#include "Navigation/Landing.hpp"

#include <cmath>

using namespace Navigation;

namespace {
// 256 x 256 m, floor at 0; a 20 m building (100..160, 100..160); a 1.5 m kerb block (40..44, 60..90)
const NavMap &Fixture() {
  static NavMap map;
  static bool loaded = [] {
    NavFixture::Spec spec;
    spec.blocks.push_back({100, 100, 160, 160, 20});
    spec.blocks.push_back({40, 60, 44, 90, 1.5f});
    return map.Load(NavFixture::Make(spec), "fixture");
  }();
  CHECK(loaded);
  return map;
}

const std::vector<Helipad> kNoPads;
const std::vector<Gate> kNoGates;

// MapWorld counting the rays cast
struct CountingWorld : MapWorld {
  using MapWorld::MapWorld;
  mutable int rays = 0;
  bool VehicleRay(const Vec3 &aFrom, const Vec3 &aTo) const override {
    ++rays;
    return MapWorld::VehicleRay(aFrom, aTo);
  }
  bool GroundRay(const Vec3 &aFrom, const Vec3 &aTo, Vec3 &aHit) const override {
    ++rays;
    return MapWorld::GroundRay(aFrom, aTo, aHit);
  }
};

// a parked car at (x, y) on the floor
Box Car(float aX, float aY) { return {{aX - 2.2f, aY - 1.0f, 0.0f}, {aX + 2.2f, aY + 1.0f, 1.5f}}; }

// The script's FindLandingSpotWithRoom as written: every check on every candidate, nearest kept.
bool ScriptSpot(const World &aWorld, const AutopilotParams &aP, const Vec3 &aTarget, float aRoom, const KeepClear &aKeep,
                GroundSpot &aOut) {
  Gap gap{};
  if (!GapAt(aWorld, aTarget, gap) || aTarget.z - gap.bottom > 4.0f) {
    return false;
  }
  const float level = gap.bottom;
  for (int r = 0; r <= aP.landingSearch; ++r) {
    const int count = r == 0 ? 1 : 6 * r;
    float best = 1000000.0f;
    for (int i = 0; i < count; ++i) {
      const float angle = 6.2831853f * float(i) / float(count);
      const Vec3 c{aTarget.x + 2.0f * float(r) * std::cos(angle), aTarget.y + 2.0f * float(r) * std::sin(angle),
                   level + 1.0f};
      Gap g{};
      if (GapAt(aWorld, c, g) && std::fabs(g.bottom - level) < 3.0f && g.top - g.bottom > aRoom &&
          IsFlat(aWorld, c, g.bottom, aP.landingFlat) && ShaftClear(aWorld, c, g.bottom, aRoom, aP.landingShaft) &&
          !SpotTaken(aWorld, {c.x, c.y, g.bottom}) && SpotSupported(aWorld, {c.x, c.y, g.bottom}, aP.landingFootprint) &&
          (aKeep.radius <= 0.0f || std::hypot(c.x - aKeep.at.x, c.y - aKeep.at.y) >= aKeep.radius)) {
        const float dx = c.x - aTarget.x, dy = c.y - aTarget.y;
        const float d = dx * dx + dy * dy;
        if (d < best) {
          best = d;
          aOut = {{c.x, c.y, g.bottom}, g.top};
        }
      }
    }
    if (best < 1000000.0f) {
      return true;
    }
  }
  return false;
}
} // namespace

TEST(landing_spot_open_floor) {
  MapWorld world(Fixture(), kNoPads, kNoGates);
  AutopilotParams p;
  GroundSpot out;
  // a pin on open ground: the pin's own column, the floor half the car's height up, open sky
  CHECK(FindLandingSpot(world, p, {61, 61, 1}, {}, out));
  CHECK_NEAR(out.spot.x, 61.0, 1e-4);
  CHECK_NEAR(out.spot.y, 61.0, 1e-4);
  CHECK_NEAR(out.spot.z, 1.0, 1e-4);
  CHECK(std::isinf(out.top));
  // on the roof
  CHECK(FindLandingSpot(world, p, {130, 130, 21}, {}, out));
  CHECK_NEAR(out.spot.z, 21.0, 1e-4);
  // a point in the air: no spot, hover there
  CHECK(!FindLandingSpot(world, p, {61, 61, 30}, {}, out));
}

TEST(landing_spot_off_the_kerb) {
  MapWorld world(Fixture(), kNoPads, kNoGates);
  AutopilotParams p;
  GroundSpot out;
  // next to the kerb block the floor within landingFlat is not level: the spot moves away from it
  CHECK(FindLandingSpot(world, p, {47, 75, 1}, {}, out));
  CHECK(out.spot.x >= 44.0f + p.landingFlat - 2.0f);
  CHECK(IsFlat(world, out.spot, out.spot.z, p.landingFlat));
}

TEST(landing_spot_taken_and_kept_clear) {
  MapWorld world(Fixture(), kNoPads, kNoGates);
  AutopilotParams p;
  GroundSpot out;
  world.vehicles.push_back(Car(61, 61));
  CHECK(SpotTaken(world, {61, 61, 1}));
  CHECK(FindLandingSpot(world, p, {61, 61, 1}, {}, out));
  CHECK(!SpotTaken(world, out.spot));
  CHECK(std::hypot(out.spot.x - 61, out.spot.y - 61) > 2.0);
  // away from V
  MapWorld open(Fixture(), kNoPads, kNoGates);
  CHECK(FindLandingSpot(open, p, {61, 61, 1}, {{61, 61, 1}, 10.0f}, out));
  CHECK(std::hypot(out.spot.x - 61, out.spot.y - 61) >= 10.0 - 1e-3);
}

TEST(landing_spot_supported) {
  MapWorld world(Fixture(), kNoPads, kNoGates);
  // nothing streamed in: nothing to say
  CHECK(SpotSupported(world, {61, 61, 1}, 3.0f));
  // level ground under all nine rays
  world.ground.push_back({{0, 0, -1}, {256, 256, 0}});
  CHECK(SpotSupported(world, {61, 61, 1}, 3.0f));
  // a ledge under part of the car
  world.ground.push_back({{62, 50, 0}, {70, 70, 0.8f}});
  CHECK(!SpotSupported(world, {61, 61, 1}, 3.0f));
  // an edge: some rays miss
  MapWorld edge(Fixture(), kNoPads, kNoGates);
  edge.ground.push_back({{0, 0, -1}, {62, 256, 0}});
  CHECK(!SpotSupported(edge, {61, 61, 1}, 3.0f));
}

TEST(landing_spot_as_the_script_with_fewer_rays) {
  AutopilotParams p;
  CountingWorld world(Fixture(), kNoPads, kNoGates);
  world.ground.push_back({{0, 0, -1}, {256, 256, 0}});
  world.ground.push_back({{100, 100, 0}, {160, 160, 20}});
  world.ground.push_back({{40, 60, 0}, {44, 90, 1.5f}});
  world.vehicles = {Car(61, 61), Car(66, 58), Car(55, 64), Car(48, 75), Car(130, 130)};
  world.ground.push_back({{20, 20, 0}, {30, 30, 0.9f}}); // a ramp edge: unsupported spots near it
  const Vec3 targets[] = {{61, 61, 1}, {47, 75, 1}, {130, 130, 21}, {25, 25, 1}, {101, 130, 1}, {200, 30, 1}};
  for (const auto &t : targets) {
    for (float room : {p.approachHeight + 4.0f, 6.0f}) {
      GroundSpot a{}, b{};
      world.rays = 0;
      const bool found = FindLandingSpotWithRoom(world, p, t, room, {}, a);
      const int fewer = world.rays;
      world.rays = 0;
      const bool script = ScriptSpot(world, p, t, room, {}, b);
      const int all = world.rays;
      CHECK(found == script);
      if (found && script) {
        CHECK_NEAR(a.spot.x, b.spot.x, 1e-4);
        CHECK_NEAR(a.spot.y, b.spot.y, 1e-4);
        CHECK_NEAR(a.spot.z, b.spot.z, 1e-4);
      }
      CHECK(fewer <= all);
    }
  }
  // the ring round a taken pin: the script casts rays for every flat column, this for the nearest few
  GroundSpot out;
  world.rays = 0;
  CHECK(FindLandingSpotWithRoom(world, p, {61, 61, 1}, 6.0f, {}, out));
  const int fewer = world.rays;
  world.rays = 0;
  CHECK(ScriptSpot(world, p, {61, 61, 1}, 6.0f, {}, out));
  // 133 against 367 when written
  CHECK(fewer * 2 < world.rays);
}

TEST(landing_pad_choice) {
  std::vector<Helipad> pads{{"a", "", 61, 61, 0.5f, 90, 6, 1e9f}, {"b", "", 200, 200, 20, 0, 6, 1e9f}};
  MapWorld world(Fixture(), pads, kNoGates);
  AutopilotParams p;
  // on pad a, at its level
  CHECK(FindLandingPad(world, p, {70, 70, 1}) == 0);
  // too far above it (and padRange off)
  CHECK(FindLandingPad(world, p, {70, 70, 10}) == -1);
  // a destination on the ground within padRange of a pad
  CHECK(FindLandingPad(world, p, {120, 61, 1}) == -1);
  p.padRange = 100.0f;
  CHECK(FindLandingPad(world, p, {120, 61, 1}) == 0);
  // pad b's deck is 20 m up: on it only at its level
  CHECK(FindLandingPad(world, p, {200, 200, 21}) == 1);
}

TEST(landing_pad_heading) {
  Helipad pad{"", "", 0, 0, 0, 90, 6, 1e9f};
  // yaw 90: +Y turned to -X; arriving east the nearer way is +X
  const Vec3 h = PadHeading(pad, {1, 0, 0});
  CHECK_NEAR(h.x, 1.0, 1e-5);
  CHECK_NEAR(h.y, 0.0, 1e-5);
  const Vec3 w = PadHeading(pad, {-1, 0.2f, 0});
  CHECK_NEAR(w.x, -1.0, 1e-5);
}

TEST(landing_heading_along_the_street) {
  // a street along x between two buildings (y 40..48 open)
  static NavMap map;
  static bool loaded = [] {
    NavFixture::Spec spec;
    spec.blocks.push_back({0, 0, 256, 40, 30});
    spec.blocks.push_back({0, 48, 256, 256, 30});
    return map.Load(NavFixture::Make(spec), "street");
  }();
  CHECK(loaded);
  MapWorld world(map, kNoPads, kNoGates);
  // arriving from the north-west: along the street, the way east the arrival leans
  Vec3 h = LandingHeading(world, {101, 45, 1}, {1, -1, 0});
  CHECK_NEAR(h.x, 1.0, 1e-5);
  CHECK_NEAR(h.y, 0.0, 1e-5);
  h = LandingHeading(world, {101, 45, 1}, {-1, -1, 0});
  CHECK_NEAR(h.x, -1.0, 1e-5);
  // in the open every line is clear: the one nearest the arrival heading wins (its score adds 0.15)
  MapWorld open(Fixture(), kNoPads, kNoGates);
  h = LandingHeading(open, {61, 61, 1}, {0, 1, 0});
  CHECK_NEAR(h.y, 1.0, 1e-5);
}

TEST(landing_shift_route_end) {
  MapWorld world(Fixture(), kNoPads, kNoGates);
  std::vector<Vec3> pts;
  for (int i = 0; i <= 48; ++i) {
    pts.push_back({10.0f + 5.0f * float(i), 30, 21});
  }
  const Route route = Route::Make(pts);
  std::vector<Vec3> moved;
  // the spot moves 4 m north, its floor unchanged: the end moves 4 m, eased in over the last 150 m
  CHECK(ShiftRouteEnd(world, route, 0, {250, 34, 1}, 1.0f, 150.0f, moved));
  CHECK(moved.size() == pts.size());
  if (moved.size() == pts.size()) {
    CHECK_NEAR(moved.back().y, 34.0, 1e-4);
    CHECK_NEAR(moved.back().z, 21.0, 1e-4);
    // 150 m before the end and earlier: unmoved; halfway: half (smoothstep 0.5)
    CHECK_NEAR(moved[18].y, 30.0, 1e-4);
    CHECK_NEAR(moved[33].y, 32.0, 1e-4);
  }
  // the points up to segment + 1 never move
  CHECK(ShiftRouteEnd(world, route, 46, {250, 34, 1}, 1.0f, 150.0f, moved));
  if (moved.size() == pts.size()) {
    CHECK_NEAR(moved[47].y, 30.0, 1e-4);
    CHECK_NEAR(moved[48].y, 34.0, 1e-4);
  }
  // into the building: refused, untouched
  const std::vector<Vec3> before = moved;
  std::vector<Vec3> into;
  for (int i = 0; i <= 30; ++i) {
    into.push_back({10.0f + 3.0f * float(i), 105, 10});
  }
  CHECK(!ShiftRouteEnd(world, Route::Make(into), 0, {100, 120, 1}, 1.0f, 150.0f, moved));
  CHECK(moved.size() == before.size());
  // too short
  CHECK(!ShiftRouteEnd(world, Route::Make({{0, 0, 5}, {10, 0, 5}}), 0, {10, 4, 1}, 1.0f, 150.0f, moved));
}
