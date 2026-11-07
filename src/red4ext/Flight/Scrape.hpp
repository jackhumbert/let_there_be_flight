#pragma once

// A flying car scraping the world, made steady (docs/design/scrape-effects.md): the game reports a sliding car's
// contacts in bursts, 3 to 12 in 2 s while it slid along a kerb, not one a frame, so effects renewed by contacts flicker.
// The tracker holds each scrape a little after its last contact, where it is on the car, what it is against and how
// hard, and the decal, the sparks and the sound are driven from it. Pure; no game types.

#include "Flight/ScriptMath.hpp"

#include <cstdint>
#include <string_view>

namespace Flight {

// What a surface does under a scraping car: a scratch and sparks (Hard, Metal), sparks without a scratch (Glass),
// splinters (Wood), dust (Soft).
enum class Surface : uint8_t { None, Hard, Metal, Glass, Wood, Soft };

// The surface of a .physmat name the game reports, by prefix. Roads are default_material (and sometimes asphalt);
// kerbs, rails and street furniture metal_hollow, metal_painted; other cars metal_car_*, plastic_car_*. None for a
// character or the car's own chassis.
Surface SurfaceOf(std::string_view aPhysmat);
const char *SurfaceName(Surface aSurface);

// One contact as the game reports it: where on the car (its frame), the surface's normal (world), the slide speed
// along the surface (m/s) and the contact's impulse.
struct ScrapeContact {
  Vec3 local, normal;
  float slide = 0.0f, impulse = 0.0f;
  Surface surface = Surface::None;
};

struct ScrapeParams {
  float hold = 0.3f;        // s a scrape is held after its last contact
  float minSlide = 1.0f;    // m/s a contact must slide to scrape
  float merge = 1.0f;       // m: a contact this near a scrape on the car, of its surface, renews it
  float fullSlide = 12.0f;  // m/s of slide for full intensity
  float fullImpulse = 400;  // impulse for full press
  float rise = 25.0f;       // 1/s the intensity rises to the contact's
  float fall = 5.0f;        // 1/s it falls once the scrape is let go
  float minIntensity = 0.02f;
};

struct Scrape {
  bool active = false;
  uint32_t id = 0;     // new for each scrape, so an effect can follow it
  Vec3 local, normal;  // where on the car (its frame), the surface's normal (world)
  Surface surface = Surface::None;
  float slide = 0.0f;      // m/s, the last contact's
  float target = 0.0f;     // the intensity the last contact asked for
  float intensity = 0.0f;  // 0..1, smoothed
  float since = 0.0f;      // s since the last contact
  float age = 0.0f;        // s since it began
};

class ScrapeTracker {
public:
  static constexpr int kMax = 4;

  // A contact this frame: renews the nearest scrape of its surface within merge, else begins one (in a free slot or
  // the weakest's). Contacts sliding under minSlide or of no surface are left out.
  void Add(const ScrapeContact &aContact, const ScrapeParams &aParams);
  // Ends the frame: each scrape's intensity moves toward its contact's, held for hold after the last contact, then
  // falls; a scrape faded out ends.
  void Step(float aDt, const ScrapeParams &aParams);
  void Clear();

  const Scrape *begin() const { return scrapes; }
  const Scrape *end() const { return scrapes + kMax; }
  // the most intense active scrape, null without one
  const Scrape *Loudest() const;

private:
  Scrape scrapes[kMax];
  uint32_t nextId = 1;
};

// A thruster's skid as the Drone skid found it this frame (FlightModeControl's Skid): the ground under it (world),
// the ground's normal, how far the thruster's bottom is off it (m) and how fast the thruster moves along it (m/s).
struct SkidContact {
  bool valid = false;
  Vec3 ground, normal;
  float clearance = 0.0f, slide = 0.0f;
};

struct SkidScrapeParams {
  float touch = 0.06f;    // m of clearance or less that touches the ground
  float minSlide = 1.0f;  // m/s the ground point must slide to scrape
  float hold = 0.15f;     // s a scrape is held after its thruster lifts off or stops
};

// A thruster scraping the ground: where (the ground under it, world), the ground's normal and how fast it slides.
struct SkidScrape {
  bool active = false;
  uint32_t id = 0;  // new for each scrape, so an effect can follow it
  Vec3 ground, normal;
  float slide = 0.0f;  // m/s along the ground
  float since = 0.0f;  // s since it last touched and slid
};

// The marks of the thrusters on the ground, placed where each one is, from the skid's contacts rather than the
// game's: one scrape per thruster while it touches and slides.
class SkidScrapes {
public:
  static constexpr int kThrusters = 4;

  // The latest contacts (all invalid when the skid is not running); a thruster's scrape begins, follows or is let go.
  void Step(const SkidContact (&aContacts)[kThrusters], float aDt, const SkidScrapeParams &aParams);
  void Clear();

  const SkidScrape *begin() const { return scrapes; }
  const SkidScrape *end() const { return scrapes + kThrusters; }

private:
  SkidScrape scrapes[kThrusters];
  // apart from the tracker's ids, so both can key one table of effects
  uint32_t nextId = 0x80000000u;
};

} // namespace Flight
