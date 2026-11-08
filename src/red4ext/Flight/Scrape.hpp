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
// splinters (Wood), dust and a rut (Soft: dirt, sand, mud), torn turf and a streak of flattened grass (Grass). The
// engine synth's scrape models are in this order (EngineSynth::ScrapeModelOf).
enum class Surface : uint8_t { None, Hard, Metal, Glass, Wood, Soft, Grass };

// The surface of a .physmat name the game reports, by prefix. Roads are default_material (and sometimes asphalt);
// kerbs, rails and street furniture metal_hollow, metal_painted; other cars metal_car_*, plastic_car_*. None for a
// character or the car's own chassis.
Surface SurfaceOf(std::string_view aPhysmat);
const char *SurfaceName(Surface aSurface);

// A surface's repeating features under a scrape, by its .physmat name: their spacing along the slide (m) and how hard
// each strikes against a grit impact (EngineSynth::ScrapeInputs::period): a grating's bars, a railing's posts, tiles,
// planks, bricks, a concrete road's slab joints. None (0) for a surface without (asphalt, dirt, the default). The
// spacings are the usual ones of such things, not read from the world's meshes.
struct Period {
  float spacing = 0.0f, strength = 0.0f;
};
Period PeriodOf(std::string_view aPhysmat);

// One contact as the game reports it: where on the car (its frame), the surface's normal (world), the slide speed
// along the surface (m/s), the contact's impulse and the load it puts on the car (its push as a share of the car's
// weight: 1 holds the whole car up).
struct ScrapeContact {
  Vec3 local, normal;
  float slide = 0.0f, impulse = 0.0f, load = 0.0f;
  Surface surface = Surface::None;
  uint64_t material = 0; // the physics material's CName hash, for the log
};

struct ScrapeParams {
  // s a scrape is held after its last contact; a scrape of one contact so far is an impact (a hit on a lamp post),
  // held impactHold: held as long, a hit rang on as a scrape
  float hold = 0.3f, impactHold = 0.08f;
  // m/s a contact must slide to scrape (barely moving: a crawl scrapes as surely as a skid); it fades in over fadeIn
  // m/s above that. A slow scrape is no quieter for being slow: the loudness is mostly the press, the slide adding the
  // rest (slowShare of full at a crawl)
  float minSlide = 0.02f, fadeIn = 0.05f, slowShare = 0.75f;
  float merge = 1.0f;       // m: a contact this near a scrape on the car, of its surface, renews it
  float fullSlide = 12.0f;  // m/s of slide for full intensity
  // the load for full press, and the share of full a contact with no load has (a scrape's loudness is mostly how hard it
  // presses)
  float fullLoad = 1.0f, lightShare = 0.25f;
  float rise = 25.0f;       // 1/s the intensity rises to the contact's
  // 1/s it falls once the scrape is let go: at 5 a brief touch rang on for most of a second
  float fall = 12.0f;
  float minIntensity = 0.02f;
};

struct Scrape {
  bool active = false;
  uint32_t id = 0;     // new for each scrape, so an effect can follow it
  Vec3 local, normal;  // where on the car (its frame), the surface's normal (world)
  Surface surface = Surface::None;
  uint64_t material = 0; // the last contact's physics material (CName hash)
  float slide = 0.0f;      // m/s, the last contact's
  float load = 0.0f;       // the last contacts' load (a share of the car's weight; the most of a frame's)
  float target = 0.0f;     // the intensity the last contact asked for
  float intensity = 0.0f;  // 0..1, smoothed
  float since = 0.0f;      // s since the last contact
  float age = 0.0f;        // s since it began
  uint32_t contacts = 0;   // how many contacts it has had (one: an impact so far)
  // in contact: from a contact until its hold runs out (its effects show only then)
  bool touching = false;
};

// How fast a point on the car (aLocal, the car's frame) slides along a surface of normal aNormal (world): the car's
// velocity plus its spin about the centre at that point, less the part into the surface. A zero normal gives the
// point's whole speed.
float SlideAt(const Vec3 &aVelocity, const Vec3 &aSpin, const Quat &aOrientation, const Vec3 &aLocal,
              const Vec3 &aNormal);

// Which of aCount emitters (world) a sound at aAt plays from: the nearest, so one scrape is never heard from two
// emitters at once (the same sound a metre apart combs). -1 without emitters.
int NearestEmitter(const Vec3 &aAt, const Vec3 *aEmitters, int aCount);

class ScrapeTracker {
public:
  static constexpr int kMax = 4;

  // A contact this frame: renews the nearest scrape of its surface within merge, else begins one (in a free slot or
  // the weakest's). Contacts sliding under minSlide or of no surface are left out.
  void Add(const ScrapeContact &aContact, const ScrapeParams &aParams);
  // Each frame: every scrape's slide is its point's now (SlideAt), not its last contact's, which come in bursts and
  // held the sound's speed while the car slowed or spun.
  void Follow(const Vec3 &aVelocity, const Vec3 &aSpin, const Quat &aOrientation);
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
// the ground's normal, how far the thruster's bottom is off it (m), how fast the thruster moves along it (m/s), what
// the ground is (its ray's physics material) and the load the skid carries there (a share of the car's weight,
// Flight::DroneSkidPoints' aLoads: a quarter each at rest on four).
struct SkidContact {
  bool valid = false;
  Vec3 ground, normal;
  float clearance = 0.0f, slide = 0.0f;
  Surface surface = Surface::Hard;
  float load = 0.0f;
  uint64_t material = 0; // the ray's physics material (CName hash; 0 none)
};

struct SkidScrapeParams {
  float touch = 0.06f;    // m of clearance or less that touches the ground
  float minSlide = 0.02f;  // m/s the ground point must slide to scrape (fading in as ScrapeParams')
  // s a scrape is held after its thruster lifts off or stops: on rough ground (the Badlands) a thruster touched for
  // half a second at a time, and held 0.15 s each touch began a new trail, too short to see
  float hold = 0.4f;
  // its loudness (SkidScrape.intensity): full at fullSlide carrying fullLoad, lightShare of that carrying nothing,
  // slowShare as loud at a crawl (as ScrapeParams'); rising and falling at the tracker's rates
  float fullSlide = 12.0f;  // m/s
  float fullLoad = 0.5f, lightShare = 0.25f;
  float fadeIn = 0.05f, slowShare = 0.75f;
  // a thruster outside the skid (TouchLoad): carrying fullLoad sunk sinkForFull m into the ground under touch
  float sinkForFull = 0.1f;
  float rise = 25.0f, fall = 12.0f;  // 1/s (fall: as ScrapeParams')
  // s a scrape's effects stay after its thruster last touched: they bridge a rough road's short hops, and end over a
  // longer one (shown that long they were left in the air)
  float effectsBridge = 0.15f;
};

// A thruster scraping the ground: where (the ground under it, world), the ground's normal, how fast it slides and on
// what. Onto another surface it is a new scrape (a new id), so its effects change with the ground.
struct SkidScrape {
  bool active = false;
  uint32_t id = 0;  // new for each scrape, so an effect can follow it
  Surface surface = Surface::Hard;
  uint64_t material = 0; // its ground's physics material (CName hash)
  Vec3 ground, normal;
  float slide = 0.0f;      // m/s along the ground
  float load = 0.0f;       // the share of the car's weight it carries
  float since = 0.0f;      // s since it last touched and slid
  float intensity = 0.0f;  // 0..1, smoothed: how loud it is (the engine synth's scrape voice)
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

// A thruster's load where the Drone skid does not carry it (any other mode): from how far it sinks under the touch
// line into the ground, none at touch, the skid's full load sunk sinkForFull.
float TouchLoad(float aClearance, const SkidScrapeParams &aParams);

// Which of a trail's three depths (0 light, 1 as drawn, 2 deep; tools/fx/scrape_effects.py's DEPTHS) a scrape lays,
// by its press (ScrapeFxInputs.press, 0..1): the band it is in, moved only once the press has been past the band's
// edge by margin for dwell s, so a press wavering at an edge does not cut the trail into short pieces. Each move is a
// new segment: the trail's width is fixed in its effect (a parameter changed the whole line).
struct TrailDepth {
  static constexpr float kEdges[2] = {0.35f, 0.75f};
  float margin = 0.08f, dwell = 0.2f;
  int band = -1;     // -1 before the first press
  int pending = -1;  // the band the press is in, while it waits out the dwell
  float waited = 0.0f;
  // the press this frame; true when the band moved (the first press sets it at once)
  bool Step(float aPress, float aDt);
};

// What a scrape's effects are given (their input parameters, tools/fx/scrape_effects.py), from what it does:
// force, how much the scrape throws (its intensity times 2: 1 is the effect as drawn, at an intensity of a half);
// press, its load over a full load; speed, its slide over a full slide. Press and speed are 0..1, the range of the
// curves the effects map them through, as the game's own vehicle effects are given theirs.
struct ScrapeFxInputs {
  float force = 0.0f, press = 0.0f, speed = 0.0f;
};
ScrapeFxInputs FxInputsOf(float aIntensity, float aLoad, float aFullLoad, float aSlide, float aFullSlide);

} // namespace Flight
