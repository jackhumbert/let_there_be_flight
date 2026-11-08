#pragma once

#include <cstdint>

// The scrape's sound (ScrapeSynth, a voice of its own per thruster: ScrapeChannels): a small physical model of a
// thruster dragged along a surface, in place of filtered noise clicks, which sounded like white noise whatever the
// surface.
//
// The contact's excitation, all of it driven by the slide speed:
// - the texture: the surface's roughness passing under the contact. A surface's bumps are fewer the finer they are, so
//   it is pink noise, between the largest bumps' rate and the contact's own width's (both a number of Hz per m/s, so
//   the whole band slides up as the slide quickens, as a grind does), under a slow wobble of the contact force;
// - grit: the surface's bigger bumps, impacts at a rate per metre slid, each a random strength (most light, a few hard),
//   struck harder the faster they are hit (ImpactScale);
// - stick-slip: friction grabbing and letting go, pulses at a rate that rises with the slide, jittered (metal and
//   glass squeal, wood creaks), and gone as the slide quickens past the surface's stickFade (StickAmount): a slow
//   scrape squeals, a fast one slides;
// - the surface's repeating features (ScrapeInputs::period: a grating's bars, a railing's posts, tiles, planks, slab
//   joints): a strike every so many metres, a buzz or a rattle rising with the slide;
// - chatter: past 6 m/s the contact leaves the surface now and then for a few milliseconds and lands with a hit
//   (ChatterRate);
// - grains: loose ground (dirt, sand) crunching, short ringing ticks at random pitches.
// The excitation rings the surface's own resonances (ScrapeModel's modes: metal and glass long and uneven, wood short
// and low), the thruster pod's (kPodModes, LTBF's steel housing, the same on every car) and, through the pod, the car's
// own chassis (ChassisModes, from its size and mass), and the texture is heard straight too. ScrapeSynth sets its
// level from the scrape's intensity and slide (Curves.hpp) and ScrapeModel's level.
// Each voice (a thruster's) is detuned a little by its seed, so two thrusters scraping at once at one speed do not
// play the same pitches and beat against each other.

namespace EngineSynth {

// A two-pole resonator: rings at a frequency (Hz) for a decay (s to -60 dB) when struck
struct Resonator {
  float a1 = 0, a2 = 0, gain = 0, y1 = 0, y2 = 0;
  void Set(float aFrequency, float aDecay, float aGain, float aSampleRate);
  float Run(float x) {
    const float y = gain * x + a1 * y1 - a2 * y2;
    y2 = y1;
    y1 = y;
    return y;
  }
};

struct Mode {
  float frequency, decay, gain; // Hz, s to -60 dB, relative
};

// A surface's scrape (Flight::Surface's order)
struct ScrapeModel {
  float texture, textureLevel;                               // the texture's cutoff (Hz per m/s), its level straight out
  float gritPerMetre, gritLevel;                             // grit impacts per m slid, their strength
  float stickBase, stickPerSpeed, stickJitter, stickLevel;   // stick-slip: Hz, Hz per m/s, jitter (fraction), strength
  float grainsPerMetre, grainLow, grainHigh, grainDecay;     // crunch: grains per m, their pitches (Hz), decay (s)
  Mode modes[6];
  int modeCount;
  float body;  // how much the thruster pod and chassis ring
  float level; // dB
  float stickFade = 1.5f; // m/s at which the stick-slip is down to half (StickAmount)
};

// How hard an impact strikes at a slide (m/s), against one at 4 m/s: a bump struck faster hits harder (its amplitude
// with the speed), held to 0.3 at a crawl, where the asperities still snap as they stick and tear, and to 3 at most.
float ImpactScale(float aSlide);
// How much stick-slip there is at a slide (m/s): friction falls as the slide quickens until the contact slides
// steadily; 1 at rest, a half at aFade, a tenth at three times aFade.
float StickAmount(float aSlide, float aFade);
// How often (per second) the contact bounces off the surface at a slide (m/s): none under 6 m/s, three more a second
// for each m/s past it.
float ChatterRate(float aSlide);
const ScrapeModel &ScrapeModelOf(int aSurface);

// The thruster pod's resonances (a steel housing)
inline constexpr Mode kPodModes[] = {{165, 0.30f, 1.0f}, {390, 0.26f, 0.8f}, {720, 0.22f, 0.7f},
                                     {1130, 0.18f, 0.55f}, {1640, 0.14f, 0.45f}, {2310, 0.11f, 0.35f}};

// A car's chassis, for what a scrape rings in it: the chassis collision shape's extent (m: across, along, up) and the
// car's mass (kg). A zero size or mass is an average car's.
struct Chassis {
  float width = 1.9f, length = 4.5f, height = 1.4f, mass = 1600.0f;
  bool operator==(const Chassis &) const = default;
};

// The six lowest bending modes of the chassis's floor pan, the panel a scrape drags along: steel plates between the
// stiffeners (bays a fixed share of the car's length, so a bigger car's are bigger), as thick as a quarter of the car's
// mass spread over its surface, simply supported (f_mn = pi / 2 * c * t * ((m / a)^2 + (n / b)^2), c the plate wave
// constant of steel), stiffened by their ribs and curvature (a factor on a flat plate's), damped as deadened steel
// (a loss factor: decay 2.2 / (loss * f)). Each mode's gain falls as 1 / (m n). A 1600 kg car of 4.5 m rings from about
// 90 Hz, a truck lower, a bike higher.
void ChassisModes(const Chassis &aChassis, Mode (&aModes)[6]);

class ScrapeVoice {
public:
  explicit ScrapeVoice(uint32_t aSeed = 1);
  void Prepare(float aSampleRate);
  // Each control step: the surface (Flight::Surface as a number), the slide (m/s), the scrape's intensity (0..1) and
  // the surface's repeating features: their spacing (m, 0 none) and how hard each strikes
  void Set(int aSurface, float aSlide, float aIntensity, float aPeriod = 0.0f, float aPeriodStrength = 0.0f);
  // the car the thruster is on: its chassis's resonances
  void SetChassis(const Chassis &aChassis);
  // The next sample, before Synth's gain
  float Next();
  // this voice's detune, a factor on every frequency (about 1)
  float Spread() const { return spread; }

private:
  float Noise();      // -1..1
  float Uniform();    // 0..1
  void Grain(float aStrength);

  static constexpr int kGrains = 8;
  float sampleRate = 48000;
  uint32_t rng;
  int surface = -1;
  const ScrapeModel *model = nullptr;
  Resonator modes[6], pod[6], chassis[6], grains[kGrains];
  Chassis chassisOf;
  bool chassisSet = false;
  float spread = 1.0f;
  // the impacts' low-pass, its cutoff rising with the slide
  float strikeLp = 0, strikeCoeff = 1;
  int nextGrain = 0;
  // the texture: pink noise (three poles), its two low-pass stages and the high-pass under them, their coefficients;
  // the contact force's wobble
  float pink0 = 0, pink1 = 0, pink2 = 0, tex1 = 0, tex2 = 0, texLow = 0, texCoeff = 0, lowCoeff = 0;
  // and its lows a pod-sized contact hardly radiates (kRadiationLow)
  float radiated = 0, radiationCoeff = 0;
  float wobble = 0, wobbleCoeff = 0, jitter = 0;
  // per sample: the chance of a grit impact and of a grain, the stick-slip phase and its step
  float gritChance = 0, grainChance = 0, stickPhase = 0, stickStep = 0;
  // the impacts' strength by the slide, how much stick-slip there is, the repeating features' phase, step and strength
  float impactScale = 1, stickAmount = 1, periodPhase = 0, periodStep = 0, periodStrength = 0;
  // chatter: the chance per sample of leaving the surface, and the samples left until it lands
  float chatterChance = 0;
  int airborne = 0;
  float slide = 0, intensity = 0;
};

// What a thruster's scrape is this frame: how hard (Flight::Scrape's intensity, 0..1), its slide (m/s), its surface
// (Flight::Surface as a number), how much the listener is inside the car (0..1) and the car's chassis
struct ScrapeInputs {
  float intensity = 0, slide = 0;
  int surface = 0;
  // the surface's repeating features (Flight::PeriodOf its physics material): their spacing (m, 0 none), how hard
  float period = 0, periodStrength = 0;
  float inside = 0;
  Chassis chassis;
};

// A thruster's scrape as a sound of its own: ScrapeVoice at the level its intensity and slide give it (Curves.hpp:
// kScrapeLevel, kScrapeSpeedLevel, kScrapeSend; ScrapeModel's level; the synth's master and inside level), its inputs
// smoothed over 20 ms, through a gentle low-pass (kScrapeLowPass: its top octave was harsh). No game or audio backend
// types. One thread.
inline constexpr float kScrapeLowPass = 4000.0f; // Hz, 12 dB an octave over it
class ScrapeSynth {
public:
  ScrapeSynth(float aSampleRate, uint32_t aSeed);
  void Set(const ScrapeInputs &aInputs) { target = aInputs; }
  void Render(float *aOut, uint32_t aFrames);
  float Spread() const { return voice.Spread(); }

private:
  void Control();
  float sampleRate;
  ScrapeVoice voice;
  ScrapeInputs target, now;
  float smooth = 0, gain = 0;
  int controlLeft = 0;
  // the low-pass (a biquad, Q 0.707): its coefficients and state
  float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
};

} // namespace EngineSynth
