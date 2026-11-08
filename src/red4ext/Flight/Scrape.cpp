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

// the slide's part of a scrape's loudness: faded in above minSlide, then slowShare rising to 1 at fullSlide
float SlideShare(float aSlide, float aMin, float aFadeIn, float aSlowShare, float aFull) {
  const float fade = aFadeIn > 0.0f ? Clamp01((aSlide - aMin) / aFadeIn) : 1.0f;
  return fade * (aSlowShare + (1.0f - aSlowShare) * Clamp01(aSlide / aFull));
}

// the press's part: aLightShare with no load, rising to 1 at aFull
float PressShare(float aLoad, float aFull, float aLightShare) {
  return aLightShare + (1.0f - aLightShare) * Clamp01(aFull > 0.0f ? aLoad / aFull : 1.0f);
}
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
  const float target = SlideShare(aContact.slide, aParams.minSlide, aParams.fadeIn, aParams.slowShare, aParams.fullSlide) *
                       PressShare(aContact.load, aParams.fullLoad, aParams.lightShare);
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
    best->load = best->since == 0.0f ? std::fmax(best->load, aContact.load) : aContact.load;
    best->target = best->since == 0.0f ? std::fmax(best->target, target) : target;
    best->since = 0.0f;
    best->touching = true;
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
  slot->load = aContact.load;
  slot->target = target;
  slot->touching = true;
}

float SlideAt(const Vec3 &aVelocity, const Vec3 &aSpin, const Quat &aOrientation, const Vec3 &aLocal,
              const Vec3 &aNormal) {
  const Vec3 point = Add(aVelocity, Cross(aSpin, Rotate(aOrientation, aLocal)));
  const float nn = Dot(aNormal, aNormal);
  const Vec3 along = nn > 1e-8f ? Sub(point, Mul(aNormal, Dot(point, aNormal) / nn)) : point;
  const float slide = Length(along);
  return std::isfinite(slide) ? slide : 0.0f;
}

int NearestEmitter(const Vec3 &aAt, const Vec3 *aEmitters, int aCount) {
  int best = -1;
  float bestDistance = 0.0f;
  for (int i = 0; i < aCount; ++i) {
    const Vec3 d = Sub(aEmitters[i], aAt);
    const float distance = Dot(d, d);
    if (best < 0 || distance < bestDistance) {
      best = i;
      bestDistance = distance;
    }
  }
  return best;
}

void ScrapeTracker::Follow(const Vec3 &aVelocity, const Vec3 &aSpin, const Quat &aOrientation) {
  for (Scrape &s : scrapes)
    if (s.active)
      s.slide = SlideAt(aVelocity, aSpin, aOrientation, s.local, s.normal);
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
    if (!held)
      s.touching = false;
    if (!held && s.intensity < aParams.minIntensity)
      s = {};
  }
}

void ScrapeTracker::Release(Surface aSurface, const ScrapeParams &aParams) {
  for (Scrape &s : scrapes)
    if (s.active && s.surface == aSurface) {
      s.touching = false;
      s.since = std::fmax(s.since, aParams.hold + 1e-3f);
    }
}

void ScrapeMixer::Add(const Vec3 &aAt, float aIntensity, Surface aSurface, float aSlide) {
  if (!(aIntensity > 0.0f))
    return;
  if (!loudest.any || aIntensity > loudest.intensity) {
    loudest.any = true;
    loudest.intensity = aIntensity;
    loudest.surface = aSurface;
    loudest.slide = aSlide;
  }
  power += aIntensity * aIntensity;
  weighted = Flight::Add(weighted, Mul(aAt, aIntensity));
  weight += aIntensity;
}

ScrapeMix ScrapeMixer::Result() const {
  ScrapeMix m = loudest;
  if (!m.any)
    return m;
  m.intensity = Clamp01(std::sqrt(power));
  m.at = Mul(weighted, 1.0f / weight);
  return m;
}

int ChooseEmitter(const Vec3 &aAt, const Vec3 *aEmitters, int aCount, int aCurrent, float aMargin) {
  const int nearest = NearestEmitter(aAt, aEmitters, aCount);
  if (nearest < 0 || aCurrent < 0 || aCurrent >= aCount || aCurrent == nearest)
    return nearest;
  const float toCurrent = Length(Sub(aEmitters[aCurrent], aAt));
  const float toNearest = Length(Sub(aEmitters[nearest], aAt));
  return toNearest + aMargin < toCurrent ? nearest : aCurrent;
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
      s.load = c.load;
      s.since = 0.0f;
      const float target = SlideShare(c.slide, aParams.minSlide, aParams.fadeIn, aParams.slowShare, aParams.fullSlide) *
                           PressShare(c.load, aParams.fullLoad, aParams.lightShare);
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

ScrapeFxInputs FxInputsOf(float aIntensity, float aLoad, float aFullLoad, float aSlide, float aFullSlide) {
  ScrapeFxInputs in;
  in.force = 2.0f * Clamp01(aIntensity);
  in.press = aFullLoad > 0.0f ? Clamp01(aLoad / aFullLoad) : 0.0f;
  in.speed = aFullSlide > 0.0f ? Clamp01(aSlide / aFullSlide) : 0.0f;
  return in;
}

bool TrailDepth::Step(float aPress, float aDt) {
  auto bandOf = [](float aP) { return aP < kEdges[0] ? 0 : (aP < kEdges[1] ? 1 : 2); };
  if (band < 0) {
    band = bandOf(aPress);
    pending = -1;
    return true;
  }
  // past an edge of the current band by the margin
  const float low = band > 0 ? kEdges[band - 1] - margin : -1.0f;
  const float high = band < 2 ? kEdges[band] + margin : 2.0f;
  const int wanted = aPress < low || aPress > high ? bandOf(aPress) : band;
  if (wanted == band) {
    pending = -1;
    waited = 0.0f;
    return false;
  }
  if (wanted != pending) {
    pending = wanted;
    waited = 0.0f;
  }
  waited += aDt;
  if (waited < dwell)
    return false;
  band = wanted;
  pending = -1;
  waited = 0.0f;
  return true;
}

const Scrape *ScrapeTracker::Loudest() const {
  const Scrape *best = nullptr;
  for (const Scrape &s : scrapes)
    if (s.active && (!best || s.intensity > best->intensity))
      best = &s;
  return best;
}

} // namespace Flight
