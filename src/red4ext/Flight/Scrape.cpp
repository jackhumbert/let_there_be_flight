#include "Flight/Scrape.hpp"

#include <algorithm>
#include <cmath>

namespace Flight {

namespace {
struct Prefix {
  const char *name;
  Surface surface;
};
// first match wins: the car's own chassis and characters before the families they would fall in
constexpr Prefix kPrefixes[] = {
    {"vehicle_chassis", Surface::None}, {"character", Surface::None},  {"metal", Surface::Metal},
    {"default_material", Surface::Hard}, {"asphalt", Surface::Hard},   {"concrete", Surface::Hard},
    {"stone", Surface::Hard},            {"brick", Surface::Hard},     {"tiles", Surface::Hard},
    {"plaster", Surface::Hard},          {"ceramic", Surface::Hard},   {"glass", Surface::Glass},
    {"plexiglass", Surface::Glass},      {"neon", Surface::Glass},     {"fabrics", Surface::Soft},
    {"carpet", Surface::Soft},
    {"wood", Surface::Wood},             {"plastic", Surface::Soft},   {"dirt", Surface::Soft},
    {"sand", Surface::Soft},             {"grass", Surface::Soft},     {"mud", Surface::Soft},
    {"trash", Surface::Soft},            {"cardboard", Surface::Soft}, {"upholstery", Surface::Soft},
    {"leather", Surface::Soft},          {"soft", Surface::Soft},      {"water", Surface::None},
};

float Clamp01(float x) { return std::fmin(std::fmax(x, 0.0f), 1.0f); }
} // namespace

Surface SurfaceOf(std::string_view aPhysmat) {
  for (const Prefix &p : kPrefixes)
    if (aPhysmat.rfind(p.name, 0) == 0)
      return p.surface;
  // a material not in the table scrapes as a hard one
  return aPhysmat.empty() ? Surface::None : Surface::Hard;
}

const char *SurfaceName(Surface aSurface) {
  switch (aSurface) {
  case Surface::Hard:
    return "hard";
  case Surface::Metal:
    return "metal";
  case Surface::Glass:
    return "glass";
  case Surface::Wood:
    return "wood";
  case Surface::Soft:
    return "soft";
  case Surface::None:
    break;
  }
  return "none";
}

void ScrapeTracker::Add(const ScrapeContact &aContact, const ScrapeParams &aParams) {
  if (aContact.surface == Surface::None || !(aContact.slide >= aParams.minSlide))
    return;
  const float target = Clamp01(aContact.slide / aParams.fullSlide) *
                       (0.5f + 0.5f * Clamp01(aContact.impulse / aParams.fullImpulse));
  Scrape *best = nullptr;
  float bestDistance = aParams.merge;
  for (Scrape &s : scrapes) {
    if (!s.active || s.surface != aContact.surface)
      continue;
    const float d = Length(Sub(s.local, aContact.local));
    if (d <= bestDistance) {
      best = &s;
      bestDistance = d;
    }
  }
  if (best) {
    best->local = aContact.local;
    best->normal = aContact.normal;
    best->slide = aContact.slide;
    // several contacts a frame: the hardest
    best->target = best->since == 0.0f ? std::fmax(best->target, target) : target;
    best->since = 0.0f;
    return;
  }
  // a new scrape: in a free slot, else in the weakest's
  Scrape *slot = nullptr;
  for (Scrape &s : scrapes)
    if (!s.active) {
      slot = &s;
      break;
    }
  if (!slot) {
    slot = std::min_element(std::begin(scrapes), std::end(scrapes),
                            [](const Scrape &a, const Scrape &b) { return a.intensity < b.intensity; });
  }
  *slot = {};
  slot->active = true;
  slot->id = nextId++;
  slot->local = aContact.local;
  slot->normal = aContact.normal;
  slot->surface = aContact.surface;
  slot->slide = aContact.slide;
  slot->target = target;
}

void ScrapeTracker::Step(float aDt, const ScrapeParams &aParams) {
  for (Scrape &s : scrapes) {
    if (!s.active)
      continue;
    s.age += aDt;
    const bool held = s.since <= aParams.hold;
    const float goal = held ? s.target : 0.0f;
    const float rate = goal > s.intensity ? aParams.rise : aParams.fall;
    s.intensity += (goal - s.intensity) * (1.0f - std::exp(-rate * aDt));
    s.since += aDt;
    if (!held && s.intensity < aParams.minIntensity)
      s = {};
  }
}

void ScrapeTracker::Clear() {
  for (Scrape &s : scrapes)
    s = {};
}

void SkidScrapes::Step(const SkidContact (&aContacts)[kThrusters], float aDt, const SkidScrapeParams &aParams) {
  for (int i = 0; i < kThrusters; ++i) {
    const SkidContact &c = aContacts[i];
    SkidScrape &s = scrapes[i];
    // a bike's two tyres each fill two of the four (front left and right are one): one mark each
    bool repeat = false;
    for (int j = 0; j < i; ++j)
      repeat = repeat || (aContacts[j].valid && Length(Sub(aContacts[j].ground, c.ground)) < 0.05f);
    if (c.valid && !repeat && c.surface != Surface::None && c.clearance <= aParams.touch &&
        c.slide >= aParams.minSlide) {
      if (!s.active || s.surface != c.surface) {
        s.active = true;
        s.id = nextId++;
        s.surface = c.surface;
      }
      s.ground = c.ground;
      s.normal = c.normal;
      s.slide = c.slide;
      s.since = 0.0f;
      const float target = Clamp01(c.slide / aParams.fullSlide) *
                           (0.5f + 0.5f * Clamp01((aParams.touch - c.clearance) / aParams.fullPress));
      s.intensity += (target - s.intensity) * Clamp01(aParams.rise * aDt);
    } else if (s.active) {
      s.since += aDt;
      s.intensity -= s.intensity * Clamp01(aParams.fall * aDt);
      if (s.since > aParams.hold)
        s = {};
      else if (c.valid)
        s.ground = c.ground;
    }
  }
}

void SkidScrapes::Clear() {
  for (SkidScrape &s : scrapes)
    s = {};
}

const Scrape *ScrapeTracker::Loudest() const {
  const Scrape *best = nullptr;
  for (const Scrape &s : scrapes)
    if (s.active && (!best || s.intensity > best->intensity))
      best = &s;
  return best;
}

} // namespace Flight
