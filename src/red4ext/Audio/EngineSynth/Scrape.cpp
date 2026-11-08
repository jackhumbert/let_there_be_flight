#include "Scrape.hpp"

#include "Curves.hpp"
#include "Synth.hpp"

#include <algorithm>
#include <cmath>

namespace EngineSynth {

namespace {
constexpr float kPi = 3.14159265f;
// how hard the texture drives the surface's modes
constexpr float kTextureDrive = 1.5f;

// Flight::Surface's order: none known, hard, metal, glass, wood, soft, grass. First guesses at each material, to be tuned by
// ear through tools/acoustics/engine_synth. The levels set a scrape at 0.4 sliding 5.5 m/s to about -36 dB, the
// engine's level at 6 m/s there (2026-11-07).
// clang-format off
constexpr ScrapeModel kModels[] = {
    // none known: as hard
    {350, 2.0f, 150, 0.5f, 0, 0, 0, 0, 0, 0, 0, 0,
     {{850, 0.012f, 0.2f}, {1900, 0.010f, 0.16f}, {3300, 0.008f, 0.12f}, {5200, 0.006f, 0.08f}}, 4, 0.35f, -6.9f},
    // hard (concrete, asphalt): a gritty rasp, its cracks heard straight; the stone's resonances faint (rung by every
    // crack at full they were a pitched tink over the rasp), the pod rings a little
    {350, 2.0f, 150, 0.5f, 0, 0, 0, 0, 0, 0, 0, 0,
     {{850, 0.012f, 0.2f}, {1900, 0.010f, 0.16f}, {3300, 0.008f, 0.12f}, {5200, 0.006f, 0.08f}}, 4, 0.35f, -6.9f},
    // metal: a squeal (stick-slip) through long uneven modes, the pod ringing with it
    {700, 0.25f, 40, 0.4f, 90, 25, 0.08f, 0.8f, 0, 0, 0, 0,
     {{1210, 0.45f, 1}, {2530, 0.38f, 0.8f}, {3690, 0.30f, 0.7f}, {5170, 0.25f, 0.5f}, {6950, 0.20f, 0.4f},
      {8740, 0.15f, 0.3f}}, 6, 1.0f, -8.4f, 2.0f},
    // glass: a high squeak, very long pure modes, almost no texture
    {1100, 0.1f, 20, 0.3f, 160, 40, 0.04f, 0.5f, 0, 0, 0, 0,
     {{3020, 0.7f, 1}, {4810, 0.6f, 0.8f}, {6370, 0.5f, 0.6f}, {8250, 0.4f, 0.5f}, {10400, 0.35f, 0.35f}}, 5, 0.25f, -4.8f, 1.2f},
    // wood: knocks and a loose creak through short low modes
    {220, 0.5f, 90, 0.6f, 25, 6, 0.35f, 0.35f, 0, 0, 0, 0,
     {{190, 0.05f, 1}, {420, 0.04f, 0.8f}, {780, 0.03f, 0.6f}, {1300, 0.025f, 0.4f}, {2100, 0.02f, 0.25f}}, 5, 0.4f, -15.4f, 1.0f},
    // soft (dirt, sand, mud): a crunch of grains over a low rumble, a dull thud of ground
    {120, 0.5f, 0, 0, 0, 0, 0, 0, 500, 900, 4500, 0.004f,
     {{110, 0.03f, 0.6f}, {240, 0.025f, 0.4f}}, 2, 0.15f, -12.6f},
    // grass: as soft ground for now (turf over earth)
    {120, 0.5f, 0, 0, 0, 0, 0, 0, 500, 900, 4500, 0.004f,
     {{110, 0.03f, 0.6f}, {240, 0.025f, 0.4f}}, 2, 0.15f, -12.6f},
};
// clang-format on

// the chassis (ChassisModes): steel's plate wave constant sqrt(E / (12 rho (1 - nu^2))) (m/s) and density (kg/m3);
// the share of the car's mass in its panels; a bay's span against the car's length, and its limits (m); the ribs' and
// curvature's stiffening; deadened steel's loss factor; the panels' thickness limits (m)
constexpr float kSteelPlateWave = 1528.0f, kSteelDensity = 7850.0f;
constexpr float kPanelShare = 0.25f;
constexpr float kBayShare = 0.12f, kBayMin = 0.35f, kBayMax = 1.0f;
constexpr float kStiffening = 3.0f;
constexpr float kLoss = 0.05f;
constexpr float kThinnest = 0.0005f, kThickest = 0.004f;
// how much of the pod's ringing the chassis takes, through the pod's mounts
constexpr float kChassisShare = 0.7f;
// the impacts heard straight, as cracks (they were heard only through the resonators, each a short pitched ring)
constexpr float kCrack = 1.5f;
// the texture's band: the largest bumps' rate this far under the contact's (its low corner, a share of its high one)
constexpr float kTextureLow = 1.0f / 16.0f;
// a contact the size of a thruster pod radiates little under this (Hz): the texture's lows, heard
constexpr float kRadiationLow = 120.0f;
// chatter: how long the contact is off the surface (s), how much of the force is left meanwhile, how hard it lands
constexpr float kAirborneMin = 0.005f, kAirborneMax = 0.025f, kAirborneForce = 0.1f, kLanding = 4.0f;
// a repeating feature's strike against a grit impact's, and the wander in its spacing (a fraction)
constexpr float kPeriodLevel = 8.0f, kPeriodJitter = 0.005f;
// the inputs' smoothing, and the control step (samples)
constexpr float kSmoothSeconds = 0.02f;
constexpr int kControlSamples = 32;

float Db(float aDb) { return std::pow(10.0f, aDb / 20.0f); }

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

void ChassisModes(const Chassis &aChassis, Mode (&aModes)[6]) {
  const Chassis c = aChassis.width > 0.1f && aChassis.length > 0.1f && aChassis.height > 0.1f && aChassis.mass > 1.0f
                        ? aChassis
                        : Chassis{};
  const float surface = 2.0f * (c.width * c.length + c.width * c.height + c.length * c.height);
  const float thickness = std::clamp(kPanelShare * c.mass / (kSteelDensity * surface), kThinnest, kThickest);
  const float bay = std::clamp(kBayShare * c.length, kBayMin, kBayMax);
  // the bays across and along the floor
  const float a = c.width / std::max(1.0f, std::ceil(c.width / bay));
  const float b = c.length / std::max(1.0f, std::ceil(c.length / bay));
  Mode all[16];
  int n = 0;
  for (int i = 1; i <= 4; ++i)
    for (int j = 1; j <= 4; ++j) {
      const float f = kStiffening * 0.5f * kPi * kSteelPlateWave * thickness *
                      (float(i * i) / (a * a) + float(j * j) / (b * b));
      all[n++] = {f, std::min(1.0f, 2.2f / (kLoss * f)), 1.0f / float(i * j)};
    }
  std::sort(all, all + n, [](const Mode &x, const Mode &y) { return x.frequency < y.frequency; });
  for (int k = 0; k < 6; ++k)
    aModes[k] = all[k];
}

float ImpactScale(float aSlide) { return std::clamp(std::max(aSlide, 0.0f) / 4.0f, 0.3f, 3.0f); }

float StickAmount(float aSlide, float aFade) {
  if (!(aFade > 0.0f))
    return 1.0f;
  const float x = std::max(aSlide, 0.0f) / aFade;
  return 1.0f / (1.0f + x * x);
}

float ChatterRate(float aSlide) { return std::max(0.0f, aSlide - 6.0f) * 3.0f; }

ScrapeVoice::ScrapeVoice(uint32_t aSeed) : rng(aSeed * 2654435761u + 97u) { rng = rng ? rng : 1; }

void ScrapeVoice::Prepare(float aSampleRate) {
  sampleRate = aSampleRate;
  surface = -1;
  wobbleCoeff = OnePole(6.0f, sampleRate);
  radiationCoeff = OnePole(kRadiationLow, sampleRate);
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

void ScrapeVoice::Set(int aSurface, float aSlide, float aIntensity, float aPeriod, float aPeriodStrength) {
  if (aSurface != surface) {
    surface = aSurface;
    model = &ScrapeModelOf(aSurface);
    for (int i = 0; i < model->modeCount; ++i)
      modes[i].Set(model->modes[i].frequency * spread, model->modes[i].decay, model->modes[i].gain, sampleRate);
    for (int i = 0; i < 6; ++i)
      pod[i].Set(kPodModes[i].frequency * spread, kPodModes[i].decay, kPodModes[i].gain, sampleRate);
  }
  if (!chassisSet)
    SetChassis(chassisOf);
  slide = std::max(aSlide, 0.0f);
  intensity = std::clamp(aIntensity, 0.0f, 1.0f);
  // the texture's band, sliding up with the slide: its high corner the contact's width's rate (pressed harder,
  // brighter), its low one the largest bumps'
  const float high = std::max(40.0f, model->texture * slide * (0.6f + 0.4f * intensity));
  texCoeff = OnePole(high, sampleRate);
  lowCoeff = OnePole(std::max(10.0f, kTextureLow * high), sampleRate);
  impactScale = ImpactScale(slide);
  stickAmount = StickAmount(slide, model->stickFade);
  periodStep = aPeriod > 0.0f && slide > 0.01f ? slide / aPeriod / sampleRate : 0.0f;
  periodStrength = aPeriodStrength;
  chatterChance = ChatterRate(slide) / sampleRate;
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

void ScrapeVoice::SetChassis(const Chassis &aChassis) {
  if (chassisSet && aChassis == chassisOf)
    return;
  chassisOf = aChassis;
  chassisSet = true;
  Mode m[6];
  ChassisModes(aChassis, m);
  for (int i = 0; i < 6; ++i)
    chassis[i].Set(m[i].frequency * spread, m[i].decay, m[i].gain, sampleRate);
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
  // the contact force: around 1, wobbling slowly, never below 0; off the surface (chatter) almost nothing
  wobble += (Noise() - wobble) * wobbleCoeff;
  float force = std::max(0.0f, 1.0f + 2.5f * wobble);
  float strike = 0.0f;
  if (airborne > 0) {
    force *= kAirborneForce;
    if (--airborne == 0)
      strike += m.gritLevel * kLanding * impactScale * (0.6f + 0.4f * Uniform());
  } else if (chatterChance > 0.0f && Uniform() < chatterChance) {
    airborne = int((kAirborneMin + (kAirborneMax - kAirborneMin) * Uniform()) * sampleRate);
  }
  // the texture: pink noise (Paul Kellet's three poles) through two low-pass stages at the band's high corner, less
  // what lies under its low one
  const float white = Noise();
  pink0 = 0.99765f * pink0 + white * 0.0990460f;
  pink1 = 0.96300f * pink1 + white * 0.2965164f;
  pink2 = 0.57000f * pink2 + white * 1.0526913f;
  const float pink = 0.25f * (pink0 + pink1 + pink2 + white * 0.1848f);
  tex1 += (pink - tex1) * texCoeff;
  tex2 += (tex1 - tex2) * texCoeff;
  texLow += (tex2 - texLow) * lowCoeff;
  const float band = tex2 - texLow;
  radiated += (band - radiated) * radiationCoeff;
  const float texture = (band - radiated) * force;
  // grit: mostly light, now and then hard, harder the faster it is hit
  if (Uniform() < gritChance) {
    const float u = Uniform();
    strike += m.gritLevel * u * u * u * (Noise() < 0 ? -1.0f : 1.0f) * force * 8.0f * impactScale;
  }
  // stick-slip: a pulse each time the jittered phase wraps, fading as the slide quickens
  if (stickStep > 0.0f && stickAmount > 0.01f) {
    jitter += (Noise() - jitter) * 0.002f;
    stickPhase += stickStep * (1.0f + m.stickJitter * 8.0f * jitter);
    if (stickPhase >= 1.0f) {
      stickPhase -= 1.0f;
      strike += m.stickLevel * stickAmount * (0.7f + 0.3f * Uniform()) * force * 4.0f;
    }
  }
  // the surface's repeating features, struck one after another as the slide passes them
  if (periodStep > 0.0f) {
    periodPhase += periodStep * (1.0f + kPeriodJitter * Noise());
    if (periodPhase >= 1.0f) {
      periodPhase -= 1.0f;
      strike += m.gritLevel * kPeriodLevel * periodStrength * impactScale * force;
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
  // rate the band swept with the slide and was heard as phasing); the pod mostly from the impacts (driven by the
  // texture too, its low modes boomed over every surface), and the car's chassis through the pod
  float ring = 0.0f;
  const float excite = strike + kTextureDrive * texture;
  for (int i = 0; i < m.modeCount; ++i)
    ring += modes[i].Run(excite);
  float housing = 0.0f, frame = 0.0f;
  const float knock = strike + 0.1f * texture;
  for (Resonator &b : pod)
    housing += b.Run(knock);
  for (Resonator &b : chassis)
    frame += b.Run(knock);
  // the grains are struck straight (their resonators take no drive), so they are set under the ground's ringing here
  return kCrack * strike + m.textureLevel * texture + ring + 0.3f * m.body * (housing + kChassisShare * frame) +
         0.1f * crunch;
}

ScrapeSynth::ScrapeSynth(float aSampleRate, uint32_t aSeed) : sampleRate(aSampleRate), voice(aSeed) {
  voice.Prepare(sampleRate);
  smooth = 1.0f - std::exp(-float(kControlSamples) / (kSmoothSeconds * sampleRate));
  // the low-pass: RBJ's, Butterworth
  const float w = 2.0f * kPi * std::min(kScrapeLowPass, 0.45f * sampleRate) / sampleRate;
  const float alpha = std::sin(w) / (2.0f * 0.70710678f), c = std::cos(w), a0 = 1.0f + alpha;
  b0 = (1.0f - c) * 0.5f / a0;
  b1 = (1.0f - c) / a0;
  b2 = b0;
  a1 = -2.0f * c / a0;
  a2 = (1.0f - alpha) / a0;
}

void ScrapeSynth::Control() {
  auto ease = [&](float &aNow, float aTarget) { aNow += (aTarget - aNow) * smooth; };
  ease(now.intensity, target.intensity);
  ease(now.slide, target.slide);
  ease(now.inside, target.inside);
  now.surface = target.surface;
  now.period = target.period;
  now.periodStrength = target.periodStrength;
  now.chassis = target.chassis;
  voice.SetChassis(now.chassis);
  const float master = Tuning{}.master;
  gain = now.intensity > 0.001f ? Db(kScrapeLevel(now.intensity) + kScrapeSpeedLevel(now.slide) + kScrapeSend +
                                     ScrapeModelOf(now.surface).level + kInsideLevel(now.inside) + master)
                                : 0.0f;
  voice.Set(now.surface, now.slide, now.intensity, now.period, now.periodStrength);
}

void ScrapeSynth::Render(float *aOut, uint32_t aFrames) {
  for (uint32_t s = 0; s < aFrames; ++s) {
    if (controlLeft-- <= 0) {
      controlLeft = kControlSamples - 1;
      Control();
    }
    const float x = gain > 1e-5f ? gain * voice.Next() : 0.0f;
    const float y = b0 * x + z1;
    z1 = b1 * x - a1 * y + z2;
    z2 = b2 * x - a2 * y;
    aOut[s] = y;
  }
}

} // namespace EngineSynth
