// Hover & Fly along lines over real ground (tools/flightsim/TerrainRuns, docs/design/hover-fly-terrain.md): the
// ground profile and its tags, the sampler, the driver, a run's metrics and its stop at a wall, on worlds built here.
#include "check.hpp"

#include "../tools/flightsim/TerrainCheck.hpp"
#include "../tools/flightsim/TerrainRuns.hpp"

#include <cmath>

using namespace FlightSim;

namespace {
void Square(Colliders &aC, float aX0, float aY0, float aX1, float aY1, float aZ) {
  aC.Add({aX0, aY0, aZ}, {aX1, aY0, aZ}, {aX1, aY1, aZ});
  aC.Add({aX0, aY0, aZ}, {aX1, aY1, aZ}, {aX0, aY1, aZ});
}

// a wall across Y at y, 400 m wide and aHeight high (rays hit both faces)
void Wall(Colliders &aC, float aY, float aHeight) {
  aC.Add({-200, aY, 0}, {200, aY, 0}, {200, aY, aHeight});
  aC.Add({-200, aY, 0}, {200, aY, aHeight}, {-200, aY, aHeight});
}

Line Along(float aY0, float aY1) {
  Line l;
  l.name = "test";
  l.a = {0, aY0, 0};
  l.b = {0, aY1, 0};
  return l;
}

Setup Assist() {
  Setup s;
  s.mode = Mode::Standard;
  return s;
}
} // namespace

TEST(terrain_profile_kerb) {
  // a kerb 0.15 m high across the line, 10 m deep: up once and down once, not flat
  Colliders c;
  Square(c, -500, -500, 500, 500, 0.0f);
  Square(c, -500, 50, 500, 60, 0.15f);
  c.Build();
  Profile p = GroundProfile(c, {0, 0, 0}, {0, 150, 0});
  CHECK_NEAR(p.usable, 150.0f, 0.01f);
  CHECK(p.kerbs == 2);
  CHECK(p.steps == 0);
  CHECK_NEAR(p.stepMax, 0.15f, 1e-3f);
  std::string t = p.Tags();
  CHECK(t.find("kerbs") != std::string::npos);
  CHECK(t.find("flat") == std::string::npos);
  Colliders open;
  Square(open, -500, -500, 500, 500, 0.0f);
  open.Build();
  CHECK(GroundProfile(open, {0, 0, 0}, {0, 150, 0}).Tags() == "flat");
}

TEST(terrain_profile_cut_at_wall) {
  Colliders c;
  Square(c, -500, -500, 500, 500, 0.0f);
  Wall(c, 60.2f, 3.0f);
  c.Build();
  Profile p = GroundProfile(c, {0, 0, 0}, {0, 150, 0});
  CHECK_NEAR(p.usable, 60.0f, 0.01f);
  CHECK(p.Tags().find("wall") != std::string::npos);
}

TEST(terrain_profile_crest_and_drop) {
  // a roof edge: 4 m of drop at 80 m
  Colliders c;
  Square(c, -500, -500, 500, 80, 4.0f);
  Square(c, -500, 80, 500, 500, 0.0f);
  c.Build();
  Profile p = GroundProfile(c, {0, 0, 0}, {0, 150, 0});
  CHECK(p.drops == 1);
  CHECK(p.dip > 1.5f);
  CHECK(p.Tags().find("drop") != std::string::npos);
}

TEST(terrain_sample_lines_in_region) {
  Colliders c;
  Square(c, -200, -200, 200, 200, 0.0f);
  Wall(c, 0.0f, 3.0f);
  c.Build();
  std::vector<Line> lines = SampleLines(c, "box", 10, 150.0f, 60.0f, 7);
  CHECK(lines.size() == 10);
  for (const Line &l : lines) {
    CHECK(l.region == "box");
    // none crosses the wall at y 0, and none is shorter than asked
    CHECK((l.a.y > 0) == (l.b.y > 0));
    CHECK(std::hypot(l.b.x - l.a.x, l.b.y - l.a.y) >= 60.0f - 0.01f);
    CHECK(std::fabs(l.a.x) <= 200 - kSampleInset && std::fabs(l.a.y) <= 200 - kSampleInset);
  }
}

TEST(terrain_driver_holds_line_and_speed) {
  // a diagonal line over a flat floor at 80 km/h: on the line, at the speed, at the hover height, untouched
  Colliders c;
  Square(c, -500, -500, 500, 500, 0.0f);
  c.Build();
  Line l;
  l.name = "diag";
  l.a = {-100, -100, 0};
  l.b = {100, 100, 0};
  Profile p = GroundProfile(c, l.a, l.b);
  Run r = FlyLine(Assist(), c, l, p, 80.0f / 3.6f, 1.0f);
  CHECK(!r.frames.empty());
  CHECK_NEAR(r.Get("speed"), 80.0f / 3.6f, 2.0f);
  CHECK(r.Get("contacts") == 0.0f);
  CHECK(r.Get("blocked") == 0.0f);
  CHECK(r.Get("height_p95") < 0.1f);
  CHECK(std::isnan(r.Get("lag"))); // nothing to follow
  CHECK(r.Get("pitch_wobble") < 1.0f);
  float off = 0.0f;
  for (const Frame &f : r.frames)
    off = std::max(off, std::fabs(f.position.x - f.position.y) / std::sqrt(2.0f));
  CHECK(off < 1.0f);
  // it went all the way
  CHECK(r.frames.back().position.y > 95.0f);
}

TEST(terrain_ramp_followed) {
  // a ramp 3 m up over 30 m, then a plateau: the car ends at its hover height over the plateau
  Colliders c;
  Square(c, -500, -500, 500, 50, 0.0f);
  c.Add({-500, 50, 0}, {500, 50, 0}, {500, 80, 3});
  c.Add({-500, 50, 0}, {500, 80, 3}, {-500, 80, 3});
  Square(c, -500, 80, 500, 500, 3.0f);
  c.Build();
  Line l = Along(0, 200);
  Profile p = GroundProfile(c, l.a, l.b);
  CHECK(p.slopeP95 > 0.09f);
  Run r = FlyLine(Assist(), c, l, p, 40.0f / 3.6f, 1.0f);
  CHECK(r.Get("blocked") == 0.0f);
  const Frame &end = r.frames.back();
  CHECK(end.position.y > 190.0f);
  CHECK_NEAR(end.groundDistance, end.hoverHeight, 0.3f);
  // it follows the ramp late, but not by seconds, and pitched with the ramp is no wobble
  CHECK(r.Get("lag") > 0.0f && r.Get("lag") < 1.5f);
  CHECK(r.Get("pitch_wobble") < 3.0f);
}

TEST(terrain_steep_ramp_climbed_clear) {
  // a city on-ramp: its foot bends from flat to 20% over 30 m, then 20% up to 12 m, climbed at 80 km/h. Damped
  // against the ground's own vertical speed the car keeps near its hover height and clear of the ramp; damped on its
  // own vertical speed it settled under the hover height all the way up and its body scraped
  Colliders c;
  Square(c, -500, -500, 500, 60, 0.0f);
  auto strip = [&](float aY0, float aZ0, float aY1, float aZ1) {
    c.Add({-500, aY0, aZ0}, {500, aY0, aZ0}, {500, aY1, aZ1});
    c.Add({-500, aY0, aZ0}, {500, aY1, aZ1}, {-500, aY1, aZ1});
  };
  auto foot = [](float y) { return 0.2f / 60.0f * (y - 60.0f) * (y - 60.0f); };
  for (float y = 60.0f; y < 90.0f; y += 2.0f)
    strip(y, foot(y), y + 2.0f, foot(y + 2.0f));
  strip(90.0f, 1.5f, 142.5f, 12.0f);
  Square(c, -500, 142.5f, 500, 500, 12.0f);
  c.Build();
  Line l = Along(0, 200);
  Profile p = GroundProfile(c, l.a, l.b);
  Run r = FlyLine(Assist(), c, l, p, 80.0f / 3.6f, 1.0f);
  CHECK(r.Get("blocked") == 0.0f);
  CHECK(r.Get("contacts") == 0.0f);
  CHECK(r.Get("height_under_p95") < 0.45f);
  CHECK(r.Get("clear_min") > 0.05f);
  Setup old = Assist();
  old.standardGains.hoverGroundRate = 0.0f;
  old.standardGains.hoverP = 6.0f;
  old.standardGains.hoverD = 5.0f;
  Run before = FlyLine(old, c, l, p, 80.0f / 3.6f, 1.0f);
  CHECK(before.Get("height_under_p95") > 0.6f);
  CHECK(before.Get("clear_min") < 0.0f);
}

TEST(terrain_run_stops_at_wall) {
  // flown with a profile that missed the wall (another world's), the run stops at the wall: a block 4 m high, taller
  // than the car's centre, so the underside's rays (cast from the centre's height down) see its top as a ceiling
  Colliders open, walled;
  Square(open, -500, -500, 500, 500, 0.0f);
  Square(walled, -500, -500, 500, 500, 0.0f);
  Square(walled, -200, 60, 200, 500, 4.0f);
  Wall(walled, 60.0f, 4.0f);
  open.Build();
  walled.Build();
  Line l = Along(0, 150);
  Run r = FlyLine(Assist(), walled, l, GroundProfile(open, l.a, l.b), 40.0f / 3.6f, 1.0f);
  CHECK(r.Get("blocked") == 1.0f);
  CHECK(r.frames.back().position.y < 65.0f);
}

TEST(terrain_ground_check_finds_missing_collision) {
  // a recording over a floor at 0 whose rays found a 0.6 m box the region lacks under one wheel, and nothing under
  // another (water, in game): both are listed where they are; the rest agree to the centimetre
  Colliders c;
  Square(c, -100, -100, 100, 100, 0.0f);
  c.Build();
  TerrainRecording rec;
  for (int i = 0; i < 20; ++i) {
    Recorded row;
    row.position = {0, float(i), 1.0f};
    row.forward = {0, 1, 0};
    row.right = {1, 0, 0};
    rec.rows.push_back(row);
    std::array<RecordedRay, 4> rays{};
    const Vec3 offsets[4] = {{-0.8f, 1.4f, -0.3f}, {0.8f, 1.4f, -0.3f}, {-0.8f, -1.4f, -0.3f}, {0.8f, -1.4f, -0.3f}};
    for (int k = 0; k < 4; ++k) {
      Vec3 o = Flight::Add(row.position, offsets[k]);
      rays[k] = {o, {o.x, o.y, k == 1 ? 0.6f : 0.0f}, {0, 0, 1}, k != 3};
    }
    rec.rays.push_back(rays);
  }
  GroundReport g = GroundCheck(rec, c, Car{});
  CHECK(g.rays == 80);
  CHECK(g.both == 60);
  CHECK(g.regionOnly == 20);
  CHECK_NEAR(g.dzP50, 0.0f, 1e-3f);
  CHECK_NEAR(g.dzMax, 0.6f, 1e-3f);
  CHECK(!g.spots.empty());
  CHECK_NEAR(g.wheel.x, 0.8f, 1e-3f);
  CHECK_NEAR(g.wheel.y, 1.4f, 1e-3f);
  CHECK_NEAR(g.wheel.z, -0.3f, 1e-3f);
  // the generic car's bottom (-0.6) 1 m over the floor at its centre: 0.4 m clear, never touching
  CHECK_NEAR(g.gameClearanceMin, 0.4f, 1e-3f);
  CHECK(g.gameTouches == 0);
}

TEST(terrain_profile_needs_a_car_wide_gap) {
  // two walls 2 m apart along the line from 50 m on: the line is cut where the gap starts, not run through it, and a
  // line starting in the gap is none
  Colliders c;
  Square(c, -500, -500, 500, 500, 0.0f);
  for (float x : {-1.0f, 1.0f}) {
    c.Add({x, 50, 0}, {x, 150, 0}, {x, 150, 3});
    c.Add({x, 50, 0}, {x, 150, 3}, {x, 50, 3});
  }
  c.Build();
  Profile p = GroundProfile(c, {0, 0, 0}, {0, 150, 0});
  CHECK(p.usable < 50.0f && p.usable > 45.0f);
  CHECK(GroundProfile(c, {0, 60, 0}, {0, 150, 0}).usable == 0.0f);
}
