// The scrape tracker (src/red4ext/Flight/Scrape.hpp): the surfaces of the game's materials, a scrape held between the
// bursts of contacts the game reports, and let go after them.
#include "check.hpp"

#include "Flight/Scrape.hpp"

#include <cmath>

using namespace Flight;

namespace {
constexpr float kDt = 1.0f / 60.0f;

ScrapeContact Contact(Vec3 aLocal, float aSlide, Surface aSurface = Surface::Hard, float aLoad = 1.0f) {
  ScrapeContact c;
  c.local = aLocal;
  c.normal = {0, 0, 1};
  c.slide = aSlide;
  c.load = aLoad;
  c.surface = aSurface;
  return c;
}

int Active(const ScrapeTracker &t) {
  int n = 0;
  for (const Scrape &s : t)
    n += s.active ? 1 : 0;
  return n;
}
} // namespace

TEST(scrape_surfaces) {
  // the materials seen in game, 2026-11-06
  CHECK(SurfaceOf("default_material.physmat") == Surface::Hard);
  CHECK(SurfaceOf("asphalt.physmat") == Surface::Hard);
  CHECK(SurfaceOf("metal_hollow.physmat") == Surface::Metal);
  CHECK(SurfaceOf("metal_painted.physmat") == Surface::Metal);
  CHECK(SurfaceOf("metal_car_electronics.physmat") == Surface::Metal);
  CHECK(SurfaceOf("glass.physmat") == Surface::Glass);
  CHECK(SurfaceOf("wood_hollow.physmat") == Surface::Wood);
  CHECK(SurfaceOf("dirt.physmat") == Surface::Soft);
  CHECK(SurfaceOf("grass.physmat") == Surface::Grass);
  CHECK(SurfaceOf("mud.physmat") == Surface::Soft);
  CHECK(SurfaceOf("vehicle_chassis.physmat") == Surface::None);
  CHECK(SurfaceOf("character_flesh.physmat") == Surface::None);
  CHECK(SurfaceOf("") == Surface::None);
  CHECK(SurfaceOf("something_new.physmat") == Surface::Hard);
  // seen in the city's collision (cp2077_tools surface_survey.py, 2026-11-07)
  CHECK(SurfaceOf("plexiglass.physmat") == Surface::Glass);
  CHECK(SurfaceOf("neon.physmat") == Surface::Glass);
  CHECK(SurfaceOf("fabrics.physmat") == Surface::Soft);
  CHECK(SurfaceOf("carpet.physmat") == Surface::Soft);
}

TEST(scrape_held_between_bursts) {
  // a contact every 0.25 s along a kerb: one scrape, steady, never let go between them
  ScrapeTracker t;
  const ScrapeParams p;
  float lowest = 1.0f;
  for (int i = 0; i < 180; ++i) {
    if (i % 15 == 0)
      t.Add(Contact({-1.2f, 0.5f, -0.2f}, 8.0f), p);
    t.Step(kDt, p);
    if (i > 30)
      lowest = std::fmin(lowest, t.Loudest() ? t.Loudest()->intensity : 0.0f);
  }
  CHECK(Active(t) == 1);
  CHECK(lowest > 0.5f);
  // let go: it fades and ends within a second
  for (int i = 0; i < 60; ++i)
    t.Step(kDt, p);
  CHECK(Active(t) == 0);
}

TEST(scrape_two_sides_and_surfaces) {
  // the left pod on a kerb and the right one on the road are two scrapes; so are two surfaces at one place
  ScrapeTracker t;
  const ScrapeParams p;
  t.Add(Contact({-1.2f, 1.0f, -0.2f}, 8.0f, Surface::Metal), p);
  t.Add(Contact({1.2f, 1.0f, -0.2f}, 8.0f, Surface::Hard), p);
  t.Add(Contact({1.2f, 1.1f, -0.2f}, 8.0f, Surface::Hard), p);
  t.Add(Contact({1.2f, 1.1f, -0.2f}, 8.0f, Surface::Soft), p);
  t.Step(kDt, p);
  CHECK(Active(t) == 3);
}

TEST(scrape_slow_and_none_left_out) {
  ScrapeTracker t;
  const ScrapeParams p;
  t.Add(Contact({0, 0, 0}, 0.01f), p);
  t.Add(Contact({0, 0, 0}, 8.0f, Surface::None), p);
  t.Step(kDt, p);
  CHECK(Active(t) == 0);
  // harder and faster is more intense
  ScrapeTracker a, b;
  a.Add(Contact({0, 0, 0}, 4.0f, Surface::Hard, 0.1f), p);
  b.Add(Contact({0, 0, 0}, 12.0f, Surface::Hard, 2.0f), p);
  // within the hold, while both are held at their contacts'
  for (int i = 0; i < 10; ++i) {
    a.Step(kDt, p);
    b.Step(kDt, p);
  }
  CHECK(b.Loudest()->intensity > 2.0f * a.Loudest()->intensity);
  CHECK(b.Loudest()->id != 0);
}

TEST(scrape_skid_thrusters) {
  // the front left thruster touches and slides at 8 m/s, the others hover 0.3 m up
  SkidScrapes s;
  const SkidScrapeParams p;
  SkidContact c[SkidScrapes::kThrusters];
  auto frame = [&](float aX, float aFrontClearance, float aSlide = 8.0f) {
    for (int i = 0; i < SkidScrapes::kThrusters; ++i) {
      c[i].valid = true;
      c[i].ground = {aX + (i % 2 ? 1.0f : -1.0f), i < 2 ? 1.5f : -1.5f, 0.0f};
      c[i].normal = {0, 0, 1};
      c[i].clearance = i == 0 ? aFrontClearance : 0.3f;
      c[i].slide = aSlide;
    }
    s.Step(c, kDt, p);
  };
  float x = 0.0f;
  for (int i = 0; i < 10; ++i)
    frame(x += 8.0f * kDt, 0.01f);
  int active = 0;
  for (const SkidScrape &k : s)
    active += k.active ? 1 : 0;
  CHECK(active == 1);
  const SkidScrape &front = *s.begin();
  CHECK(front.active);
  CHECK_NEAR(front.slide, 8.0f, 0.01f);
  CHECK_NEAR(front.ground.x, x - 1.0f, 1e-4f);
  const uint32_t id = front.id;
  // lifted off, held a moment, then let go
  frame(x += 8.0f * kDt, 0.2f);
  CHECK(front.active && front.id == id);
  for (int i = 0; i < 30; ++i)
    frame(x += 8.0f * kDt, 0.2f);
  CHECK(!front.active);
  // resting on the ground without sliding is no scrape
  for (int i = 0; i < 10; ++i)
    frame(x, 0.0f, 0.0f);
  CHECK(!front.active);
}

TEST(scrape_skid_bike_one_mark_per_tyre) {
  // a bike's front and back tyres, each filling two of the four contacts, both touching and sliding: two scrapes
  SkidScrapes s;
  SkidContact c[SkidScrapes::kThrusters];
  for (int i = 0; i < SkidScrapes::kThrusters; ++i) {
    c[i].valid = true;
    c[i].ground = {0.0f, i < 2 ? 0.8f : -0.8f, 0.0f};
    c[i].normal = {0, 0, 1};
    c[i].slide = 8.0f;
  }
  s.Step(c, kDt, {});
  int active = 0;
  for (const SkidScrape &k : s)
    active += k.active ? 1 : 0;
  CHECK(active == 2);
}

TEST(scrape_skid_surface_change) {
  // a thruster sliding from the road onto a metal plate: a new scrape (its effects change), and on no surface (water)
  // none at all
  SkidScrapes s;
  SkidContact c[SkidScrapes::kThrusters];
  c[0].valid = true;
  c[0].normal = {0, 0, 1};
  c[0].slide = 8.0f;
  s.Step(c, kDt, {});
  const SkidScrape &k = *s.begin();
  CHECK(k.active && k.surface == Surface::Hard);
  const uint32_t road = k.id;
  c[0].surface = Surface::Metal;
  s.Step(c, kDt, {});
  CHECK(k.active && k.surface == Surface::Metal && k.id != road);
  c[0].surface = Surface::None;
  for (int i = 0; i < 30; ++i)
    s.Step(c, kDt, {});
  CHECK(!k.active);
}

TEST(scrape_skid_intensity) {
  // carrying its full load at full slide the scrape is loud, carrying nothing at half the slide a fifth as loud, at
  // rest on four (a quarter of the car each) in between; it fades once the thruster lifts off
  const SkidScrapeParams p;
  auto loudness = [&](float aLoad, float aSlide) {
    SkidScrapes s;
    SkidContact c[SkidScrapes::kThrusters];
    c[0].valid = true;
    c[0].normal = {0, 0, 1};
    c[0].load = aLoad;
    c[0].slide = aSlide;
    for (int i = 0; i < 30; ++i)
      s.Step(c, kDt, p);
    return s.begin()->intensity;
  };
  CHECK_NEAR(loudness(p.fullLoad, p.fullSlide), 1.0f, 0.01f);
  CHECK_NEAR(loudness(0.0f, 0.5f * p.fullSlide), 0.875f * 0.25f, 0.01f);
  CHECK_NEAR(loudness(0.25f, p.fullSlide), 0.625f, 0.01f);
  // heavier is louder, and a crawl pressed in is nearly as loud as a slide: at 0.1 m/s three quarters of full,
  // fading in from 0.02 m/s
  CHECK(loudness(0.5f, 6.0f) > 1.5f * loudness(0.1f, 6.0f));
  CHECK(loudness(p.fullLoad, 0.1f) > 0.7f);
  CHECK(loudness(p.fullLoad, 0.03f) < loudness(p.fullLoad, 0.1f));
  CHECK_NEAR(loudness(p.fullLoad, 0.01f), 0.0f, 1e-6f);
  SkidScrapes s;
  SkidContact c[SkidScrapes::kThrusters];
  c[0].valid = true;
  c[0].slide = p.fullSlide;
  for (int i = 0; i < 30; ++i)
    s.Step(c, kDt, p);
  const float on = s.begin()->intensity;
  c[0].clearance = 0.3f;
  for (int i = 0; i < 6; ++i)
    s.Step(c, kDt, p);
  CHECK(s.begin()->active && s.begin()->intensity < 0.7f * on);
}

TEST(scrape_slide_at_point) {
  // a car at rest spinning 2 rad/s about its up axis: its nose 2 m ahead slides 4 m/s sideways along flat ground, its
  // centre not at all; driving 5 m/s straight into a wall it slides along none of it
  const Quat level{0, 0, 0, 1};
  CHECK_NEAR(SlideAt({0, 0, 0}, {0, 0, 2}, level, {0, 2, 0}, {0, 0, 1}), 4.0f, 1e-4f);
  CHECK_NEAR(SlideAt({0, 0, 0}, {0, 0, 2}, level, {0, 0, 0}, {0, 0, 1}), 0.0f, 1e-6f);
  CHECK_NEAR(SlideAt({0, 5, 0}, {0, 0, 0}, level, {0, 2, 0}, {0, -1, 0}), 0.0f, 1e-6f);
  // turned 90 degrees about up, the car's nose points along world -X: the spin moves it along -Y
  const float h = 0.70710678f;
  CHECK_NEAR(SlideAt({3, 0, 0}, {0, 0, 2}, {0, 0, h, h}, {0, 2, 0}, {0, 0, 1}), 5.0f, 1e-3f);
}

TEST(scrape_follow_updates_slide) {
  // contacts come in bursts: between them the scrape's slide is the car's at that point now
  ScrapeTracker t;
  const ScrapeParams p;
  t.Add(Contact({0, 2, -0.5f}, 10.0f), p);
  t.Follow({3, 0, 0}, {0, 0, 0}, {0, 0, 0, 1});
  CHECK_NEAR(t.Loudest()->slide, 3.0f, 1e-4f);
  t.Follow({0, 0, 0}, {0, 0, 1}, {0, 0, 0, 1});
  CHECK_NEAR(t.Loudest()->slide, 2.0f, 1e-4f);
}

TEST(scrape_nearest_emitter) {
  // a scrape sounds from one thruster only, the nearest
  const Vec3 thrusters[4] = {{-1, 2, 0}, {1, 2, 0}, {-1, -2, 0}, {1, -2, 0}};
  CHECK(NearestEmitter({0.9f, 2.5f, -0.5f}, thrusters, 4) == 1);
  CHECK(NearestEmitter({-0.2f, -1.0f, 0}, thrusters, 4) == 2);
  CHECK(NearestEmitter({0, 0, 0}, thrusters, 0) == -1);
}

TEST(scrape_fx_inputs) {
  // what a scrape's effects are given: force twice its intensity (1, the effect as drawn, at half), press and speed
  // its load and slide over their full, held to 0..1
  ScrapeFxInputs in = FxInputsOf(0.5f, 0.25f, 0.5f, 6.0f, 12.0f);
  CHECK_NEAR(in.force, 1.0f, 1e-6f);
  CHECK_NEAR(in.press, 0.5f, 1e-6f);
  CHECK_NEAR(in.speed, 0.5f, 1e-6f);
  in = FxInputsOf(1.5f, 3.0f, 1.0f, 40.0f, 12.0f);
  CHECK_NEAR(in.force, 2.0f, 1e-6f);
  CHECK_NEAR(in.press, 1.0f, 1e-6f);
  CHECK_NEAR(in.speed, 1.0f, 1e-6f);
  CHECK_NEAR(FxInputsOf(0.0f, 0.0f, 0.0f, 0.0f, 0.0f).press, 0.0f, 1e-6f);
}

TEST(scrape_tracker_keeps_load) {
  // a frame's contacts: the scrape keeps the hardest's load; the next frame's replaces it
  ScrapeTracker t;
  const ScrapeParams p;
  t.Add(Contact({0, 0, 0}, 6.0f, Surface::Hard, 0.3f), p);
  t.Add(Contact({0.1f, 0, 0}, 6.0f, Surface::Hard, 0.8f), p);
  CHECK_NEAR(t.Loudest()->load, 0.8f, 1e-6f);
  t.Step(kDt, p);
  t.Add(Contact({0, 0, 0}, 6.0f, Surface::Hard, 0.2f), p);
  CHECK_NEAR(t.Loudest()->load, 0.2f, 1e-6f);
}

TEST(scrape_impact_short) {
  // one contact is an impact (a lamp post): it lets go after impactHold and is gone within a few tenths of a second;
  // a contact renewed is a scrape and held the full hold
  const ScrapeParams p;
  ScrapeTracker hit;
  hit.Add(Contact({0, 2, 0}, 10.0f, Surface::Metal), p);
  for (int i = 0; i < int(p.impactHold / kDt) + 2; ++i)
    hit.Step(kDt, p);
  CHECK(hit.Loudest() && !hit.Loudest()->touching);
  for (int i = 0; i < 20; ++i)
    hit.Step(kDt, p);
  CHECK(!hit.Loudest());
  ScrapeTracker scrape;
  scrape.Add(Contact({0, 2, 0}, 10.0f), p);
  scrape.Step(kDt, p);
  scrape.Add(Contact({0, 2, 0}, 10.0f), p);
  for (int i = 0; i < int(p.impactHold / kDt) + 2; ++i)
    scrape.Step(kDt, p);
  CHECK(scrape.Loudest() && scrape.Loudest()->touching);
  // untouched past the hold it stops touching too
  for (int i = 0; i < int(p.hold / kDt) + 2; ++i)
    scrape.Step(kDt, p);
  CHECK(!scrape.Loudest() || !scrape.Loudest()->touching);
}

TEST(scrape_touch_load) {
  // a thruster outside the skid: no load at the touch line or above it, the full load sunk sinkForFull under it
  const SkidScrapeParams p;
  CHECK_NEAR(TouchLoad(p.touch + 0.1f, p), 0.0f, 1e-6f);
  CHECK_NEAR(TouchLoad(p.touch, p), 0.0f, 1e-6f);
  CHECK_NEAR(TouchLoad(p.touch - 0.5f * p.sinkForFull, p), 0.5f * p.fullLoad, 1e-5f);
  CHECK_NEAR(TouchLoad(p.touch - 2.0f * p.sinkForFull, p), p.fullLoad, 1e-6f);
}

TEST(scrape_trail_depth) {
  // the first press picks the depth at once; a press past the next edge by the margin moves it after the dwell, one
  // wavering at an edge does not
  TrailDepth d;
  CHECK(d.Step(0.5f, kDt) && d.band == 1);
  int moves = 0;
  for (int i = 0; i < 60; ++i)
    moves += d.Step(i % 2 ? 0.80f : 0.70f, kDt) ? 1 : 0; // about the edge, inside the margin
  CHECK(moves == 0 && d.band == 1);
  for (int i = 0; i < 6; ++i)
    CHECK(!d.Step(0.95f, kDt)); // pressed hard, still within the dwell
  bool moved = false;
  for (int i = 0; i < 12; ++i)
    moved = d.Step(0.95f, kDt) || moved;
  CHECK(moved && d.band == 2);
  // a brief light touch inside a hard scrape is not a new segment
  for (int i = 0; i < 6; ++i)
    CHECK(!d.Step(0.1f, kDt));
  CHECK(!d.Step(0.95f, kDt) && d.band == 2);
  // let up for longer: light
  for (int i = 0; i < 20; ++i)
    d.Step(0.1f, kDt);
  CHECK(d.band == 0);
}

TEST(scrape_periods) {
  // a grating's bars, a railing's posts, a concrete road's joints; none on asphalt or dirt
  CHECK_NEAR(PeriodOf("metal_catwalk.physmat").spacing, 0.04f, 1e-6f);
  CHECK(PeriodOf("metal_hollow.physmat").spacing > 1.0f);
  CHECK(PeriodOf("concrete.physmat").spacing > 2.0f);
  CHECK_NEAR(PeriodOf("asphalt.physmat").spacing, 0.0f, 1e-6f);
  CHECK_NEAR(PeriodOf("dirt.physmat").spacing, 0.0f, 1e-6f);
  CHECK_NEAR(PeriodOf("").strength, 0.0f, 1e-6f);
}
