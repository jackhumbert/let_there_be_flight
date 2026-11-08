#include "Scrape.hpp"

#include <algorithm>
#include <cmath>

namespace EngineSynth {

namespace {
constexpr float kPi = 3.14159265f;
// how hard the texture drives the surface's modes
constexpr float kTextureDrive = 1.5f;

// Flight::Surface's order: none known, hard, metal, glass, wood, soft. First guesses at each material, to be tuned by
// ear through tools/acoustics/engine_synth. The levels set a scrape at 0.4 sliding 5.5 m/s to about -36 dB, the
// engine's level at 6 m/s there (2026-11-07).
// clang-format off
constexpr ScrapeModel kModels[] = {
    // none known: as hard
    {350, 0.6f, 150, 0.5f, 0, 0, 0, 0, 0, 0, 0, 0,
     {{850, 0.012f, 1}, {1900, 0.010f, 0.8f}, {3300, 0.008f, 0.6f}, {5200, 0.006f, 0.4f}}, 4, 0.35f, -8.9f},
    // hard (concrete, asphalt): a gritty rasp, dead stone resonances, the pod rings a little
    {350, 0.6f, 150, 0.5f, 0, 0, 0, 0, 0, 0, 0, 0,
     {{850, 0.012f, 1}, {1900, 0.010f, 0.8f}, {3300, 0.008f, 0.6f}, {5200, 0.006f, 0.4f}}, 4, 0.35f, -8.9f},
    // metal: a squeal (stick-slip) through long uneven modes, the pod ringing with it
    {700, 0.25f, 40, 0.4f, 90, 25, 0.08f, 0.8f, 0, 0, 0, 0,
     {{1210, 0.45f, 1}, {2530, 0.38f, 0.8f}, {3690, 0.30f, 0.7f}, {5170, 0.25f, 0.5f}, {6950, 0.20f, 0.4f},
      {8740, 0.15f, 0.3f}}, 6, 1.0f, -9.1f},
    // glass: a high squeak, very long pure modes, almost no texture
    {1100, 0.1f, 20, 0.3f, 160, 40, 0.04f, 0.5f, 0, 0, 0, 0,
     {{3020, 0.7f, 1}, {4810, 0.6f, 0.8f}, {6370, 0.5f, 0.6f}, {8250, 0.4f, 0.5f}, {10400, 0.35f, 0.35f}}, 5, 0.25f, -8.7f},
    // wood: knocks and a loose creak through short low modes
    {220, 0.5f, 90, 0.6f, 25, 6, 0.35f, 0.35f, 0, 0, 0, 0,
     {{190, 0.05f, 1}, {420, 0.04f, 0.8f}, {780, 0.03f, 0.6f}, {1300, 0.025f, 0.4f}, {2100, 0.02f, 0.25f}}, 5, 0.4f, -9.3f},
    // soft (dirt, sand, mud): a crunch of grains over a low rumble, a dull thud of ground
    {120, 0.5f, 0, 0, 0, 0, 0, 0, 500, 900, 4500, 0.004f,
     {{110, 0.03f, 0.6f}, {240, 0.025f, 0.4f}}, 2, 0.15f, -6.7f},
};
// clang-format on

float OnePole(float aCutoff, float aSampleRate) {
  return 1.0f - std::exp(-2.0f * kPi * std::min(aCutoff, 0.45f * aSampleRate) / aSampleRate);
}
} // namespace

void Resonator::Set(float aFrequency, float aDecay, float aGain, float aSampleRate) {
  const float f = std::min(aFrequency, 0.45f * aSampleRate);
  // the pole radius that falls 60 dB in aDecay seconds
  const float r = std::exp(-6.9078f / (std::max(aDecay, 1e-4f) * aSampleRate));
  a1 = 2.0f * r * std::cos(2.0f * kPi * f / aSampleRate);
  a2 = r * r;
  // aGain is its level for a broadband drive, whatever the decay and the pitch: white noise through the two poles comes
  // out (1 + r^2) / ((1 - r^2) ((1 + r^2)^2 - a1^2)) as strong in power (about 1 / (2 (1 - r^2) sin^2 theta)), so a gain
  // set by the decay alone let a low mode ring some 30 dB over a high one and over the texture
  const float rr = 1.0f + a2;
  gain = aGain * std::sqrt((1.0f - a2) * (rr * rr - a1 * a1) / rr);
}

const ScrapeModel &ScrapeModelOf(int aSurface) {
  return kModels[aSurface >= 0 && aSurface < int(sizeof(kModels) / sizeof(kModels[0])) ? aSurface : 0];
}

ScrapeVoice::ScrapeVoice(uint32_t aSeed) : rng(aSeed * 2654435761u + 97u) { rng = rng ? rng : 1; }

void ScrapeVoice::Prepare(float aSampleRate) {
  sampleRate = aSampleRate;
  surface = -1;
  wobbleCoeff = OnePole(6.0f, sampleRate);
  // up to 3 % either way, by the seed
  spread = 1.0f + 0.06f * (Uniform() - 0.5f);
}

float ScrapeVoice::Noise() {
  rng ^= rng << 13;
  rng ^= rng >> 17;
  rng ^= rng << 5;
  return float(int32_t(rng)) / 2147483648.0f;
}

float ScrapeVoice::Uniform() { return 0.5f * Noise() + 0.5f; }

void ScrapeVoice::Set(int aSurface, float aSlide, float aIntensity) {
  if (aSurface != surface) {
    surface = aSurface;
    model = &ScrapeModelOf(aSurface);
    for (int i = 0; i < model->modeCount; ++i)
      modes[i].Set(model->modes[i].frequency * spread, model->modes[i].decay, model->modes[i].gain, sampleRate);
    for (int i = 0; i < 6; ++i)
      body[i].Set(kBodyModes[i].frequency * spread, kBodyModes[i].decay, kBodyModes[i].gain, sampleRate);
  }
  slide = std::max(aSlide, 0.0f);
  intensity = std::clamp(aIntensity, 0.0f, 1.0f);
  // pressed harder the texture is brighter
  texCoeff = OnePole(std::max(40.0f, model->texture * slide * (0.6f + 0.4f * intensity)), sampleRate);
  // the bumps' rate under the contact (texture Hz per m/s): the impacts' brightness follows it, but each stays a sharp
  // crack at a crawl (asperities snapping; held to the bumps' rate there, a slow scrape was a dull thud, unheard)
  const float roughFrequency = std::clamp(model->texture * slide * spread, 60.0f, 0.4f * sampleRate);
  strikeCoeff = OnePole(std::max(2500.0f, 2.0f * roughFrequency), sampleRate);
  // grit at random (not at a rate that could be heard as a pitch), per metre slid; a crawl still grinds at a few dozen
  // a second, as the asperities stick and tear
  gritChance = slide > 0.01f ? model->gritPerMetre * (slide + 0.4f) / sampleRate : 0.0f;
  grainChance = model->grainsPerMetre * slide / sampleRate;
  // stick-slip down to a crawl, where a slow scrape squeals and groans most
  stickStep = slide > 0.02f ? spread * (model->stickBase + model->stickPerSpeed * slide) / sampleRate : 0.0f;
}

void ScrapeVoice::Grain(float aStrength) {
  Resonator &g = grains[nextGrain];
  nextGrain = (nextGrain + 1) % kGrains;
  const float f = spread * model->grainLow * std::pow(model->grainHigh / model->grainLow, Uniform());
  g.Set(f, model->grainDecay * (0.6f + 0.8f * Uniform()), 1.0f, sampleRate);
  g.y1 += aStrength;
}

float ScrapeVoice::Next() {
  if (!model)
    return 0.0f;
  const ScrapeModel &m = *model;
  // the contact force: around 1, wobbling slowly, never below 0
  wobble += (Noise() - wobble) * wobbleCoeff;
  const float force = std::max(0.0f, 1.0f + 2.5f * wobble);
  // the texture: noise through two low-pass stages at the slide's cutoff
  const float white = Noise();
  tex1 += (white - tex1) * texCoeff;
  tex2 += (tex1 - tex2) * texCoeff;
  const float texture = tex2 * force;
  float strike = 0.0f;
  // grit: mostly light, now and then hard
  if (Uniform() < gritChance) {
    const float u = Uniform();
    strike += m.gritLevel * u * u * u * (Noise() < 0 ? -1.0f : 1.0f) * force * 8.0f;
  }
  // stick-slip: a pulse each time the jittered phase wraps
  if (stickStep > 0.0f) {
    jitter += (Noise() - jitter) * 0.002f;
    stickPhase += stickStep * (1.0f + m.stickJitter * 8.0f * jitter);
    if (stickPhase >= 1.0f) {
      stickPhase -= 1.0f;
      strike += m.stickLevel * (0.7f + 0.3f * Uniform()) * force * 4.0f;
    }
  }
  // the impacts are as long as a bump takes to pass: dull at a crawl, sharp fast (as loud at low pitches either way)
  strikeLp += (strike - strikeLp) * strikeCoeff;
  strike = strikeLp;
  float crunch = 0.0f;
  if (grainChance > 0.0f) {
    if (Uniform() < grainChance)
      Grain((0.3f + 0.7f * Uniform()) * force);
    for (Resonator &g : grains)
      crunch += g.Run(0.0f);
  }
  // the surface's modes from the impacts and the texture, so they ring brighter as the slide quickens (driven by flat
  // noise they rang the same at any speed and held the scrape's pitch still; through a resonant band at the slide's
  // rate the band swept with the slide and was heard as phasing); the pod and chassis mostly from the impacts (driven
  // by the texture too, their low modes boomed over every surface)
  float ring = 0.0f;
  const float excite = strike + kTextureDrive * texture;
  for (int i = 0; i < m.modeCount; ++i)
    ring += modes[i].Run(excite);
  float pod = 0.0f;
  const float knock = strike + 0.1f * texture;
  for (Resonator &b : body)
    pod += b.Run(knock);
  // the grains are struck straight (their resonators take no drive), so they are set under the ground's ringing here
  return m.textureLevel * texture + ring + 0.3f * m.body * pod + 0.1f * crunch;
}

} // namespace EngineSynth
