#pragma once

#include <cstdint>

// The engine synth's scrape voice (Synth's Scrape): a small physical model of a thruster dragged along a surface, in
// place of filtered noise clicks, which sounded like white noise whatever the surface.
//
// The contact's excitation has four parts, all driven by the slide speed:
// - the texture: the surface's roughness passing under the contact, noise low-passed at a cutoff that rises with the
//   slide (a rough surface's bumps go by faster, so the scrape brightens as it quickens), under a slow wobble of the
//   contact force so it swells and catches;
// - grit: the surface's bigger bumps, impacts at a rate per metre slid, each a random strength (most light, a few hard);
// - stick-slip: friction grabbing and letting go, pulses at a rate that rises with the slide, jittered (metal and
//   glass squeal, wood creaks);
// - grains: loose ground (dirt, sand) crunching, short ringing ticks at random pitches.
// The excitation rings the surface's own resonances (ScrapeModel's modes: metal and glass long and uneven, wood short
// and low) and the thruster pod's and chassis's (kBodyModes, a steel housing), and the texture is heard straight too.
// Synth sets its level from the scrape's intensity and slide (Curves.hpp) and ScrapeModel's level.
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
};
const ScrapeModel &ScrapeModelOf(int aSurface);

// The thruster pod's and chassis's resonances (a steel housing)
inline constexpr Mode kBodyModes[] = {{165, 0.30f, 1.0f}, {390, 0.26f, 0.8f}, {720, 0.22f, 0.7f},
                                      {1130, 0.18f, 0.55f}, {1640, 0.14f, 0.45f}, {2310, 0.11f, 0.35f}};

class ScrapeVoice {
public:
  explicit ScrapeVoice(uint32_t aSeed = 1);
  void Prepare(float aSampleRate);
  // Each control step: the surface (Flight::Surface as a number), the slide (m/s) and the scrape's intensity (0..1)
  void Set(int aSurface, float aSlide, float aIntensity);
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
  Resonator modes[6], body[6], grains[kGrains];
  float spread = 1.0f;
  // the impacts' low-pass, its cutoff rising with the slide
  float strikeLp = 0, strikeCoeff = 1;
  int nextGrain = 0;
  // the texture's two low-pass stages and its coefficient, the contact force's wobble
  float tex1 = 0, tex2 = 0, texCoeff = 0, wobble = 0, wobbleCoeff = 0, jitter = 0;
  // per sample: the chance of a grit impact and of a grain, the stick-slip phase and its step
  float gritChance = 0, grainChance = 0, stickPhase = 0, stickStep = 0;
  float slide = 0, intensity = 0;
};

} // namespace EngineSynth
