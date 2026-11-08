// The scrape tracker (src/red4ext/Flight/Scrape.hpp): the surfaces of the game's materials, a scrape held between the
// bursts of contacts the game reports, and let go after them.
#include "check.hpp"

#include "Flight/Scrape.hpp"

using namespace Flight;

namespace {
constexpr float kDt = 1.0f / 60.0f;

ScrapeContact Contact(Vec3 aLocal, float aSlide, Surface aSurface = Surface::Hard, float aImpulse = 400.0f) {
  ScrapeContact c;
  c.local = aLocal;
  c.normal = {0, 0, 1};
  c.slide = aSlide;
  c.impulse = aImpulse;
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
  t.Add(Contact({0, 0, 0}, 0.5f), p);
  t.Add(Contact({0, 0, 0}, 8.0f, Surface::None), p);
  t.Step(kDt, p);
  CHECK(Active(t) == 0);
  // harder and faster is more intense
  ScrapeTracker a, b;
  a.Add(Contact({0, 0, 0}, 4.0f, Surface::Hard, 50.0f), p);
  b.Add(Contact({0, 0, 0}, 12.0f, Surface::Hard, 800.0f), p);
  for (int i = 0; i < 30; ++i) {
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
  // pressed into the road at full slide the scrape is loud, only touching at half the slide a quarter as loud; it
  // fades once the thruster lifts off
  const SkidScrapeParams p;
  auto loudness = [&](float aClearance, float aSlide) {
    SkidScrapes s;
    SkidContact c[SkidScrapes::kThrusters];
    c[0].valid = true;
    c[0].normal = {0, 0, 1};
    c[0].clearance = aClearance;
    c[0].slide = aSlide;
    for (int i = 0; i < 30; ++i)
      s.Step(c, kDt, p);
    return s.begin()->intensity;
  };
  CHECK_NEAR(loudness(p.touch - p.fullPress, p.fullSlide), 1.0f, 0.01f);
  CHECK_NEAR(loudness(p.touch, 0.5f * p.fullSlide), 0.25f, 0.01f);
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
