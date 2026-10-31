#include "Model.hpp"

#include <algorithm>

namespace Wind {

namespace {
constexpr float kPi = 3.14159265358979f;
constexpr float kSqrt3 = 1.7320508075688772f;
constexpr float kSpeedOfSound = 343.0f;
// Rossiter's constants for a shallow gap: the vortices' convection speed over the flow's, and the
// phase lag
constexpr float kConvection = 0.57f, kLag = 0.25f;

float FromDb(float aDb) {
  return std::pow(10.0f, aDb / 20.0f);
}

uint32_t Nonzero(uint32_t aSeed) {
  return aSeed ? aSeed : 0x9E3779B9u;
}

// xorshift32 to a uniform in [-sqrt3, sqrt3): unit variance
float Uniform(uint32_t &aState) {
  aState ^= aState << 13;
  aState ^= aState >> 17;
  aState ^= aState << 5;
  return (float(aState >> 8) * (1.0f / 16777216.0f) * 2.0f - 1.0f) * kSqrt3;
}

float Length(const Vec &v) {
  return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

float Dot(const Vec &a, const Vec &b) {
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

// one step of an Ornstein-Uhlenbeck process of unit variance and time scale aTime
float Step(float aX, float aDt, float aTime, float aDraw) {
  const float a = std::exp(-aDt / std::max(aTime, 1e-3f));
  return a * aX + std::sqrt(std::max(0.0f, 1.0f - a * a)) * aDraw;
}
} // namespace

Field::Field(uint32_t aSeed, const FieldTuning &aTuning) : tuning(aTuning), state(Nonzero(aSeed)) {
  heading = (Draw() / kSqrt3 + 1.0f) * kPi;
  for (float &g : gust) {
    g = Draw();
  }
}

float Field::Draw() {
  return Uniform(state);
}

Vec Field::Update(const FieldInput &in) {
  const FieldTuning &t = tuning;
  const float height = std::clamp(std::isinf(in.floorDistance) ? t.maxHeight : in.floorDistance, 1.0f, t.maxHeight);
  const float open = std::clamp(in.openSky, 0.0f, 1.0f);
  const float wall = std::isinf(in.wallDistance) ? t.wallShelter : std::clamp(in.wallDistance, 0.0f, t.wallShelter);
  const float shelter = (t.canyon + (1.0f - t.canyon) * open) * (0.5f + 0.5f * wall / t.wallShelter);

  // a step in shelter is a building edge: the shear layer off it, settling over a few metres
  if (lastShelter >= 0.0f) {
    stir += t.stir * std::fabs(shelter - lastShelter);
  }
  lastShelter = shelter;
  stir *= std::exp(-in.dt * std::max(in.crossing, 5.0f) / t.stirLength);

  mean = t.refWind * std::pow(height / t.refHeight, t.exponent) * shelter;
  sigma = t.intensity * mean * (1.0f + stir);

  const float scale = std::clamp(t.scaleShare * height, t.minScale, t.maxScale);
  const float time = scale / std::max(mean + in.crossing, 1.0f);
  for (float &g : gust) {
    g = Step(g, in.dt, time, Draw());
  }
  drift = Step(drift, in.dt, t.drift, Draw());

  const float angle = heading + drift * t.driftDegrees * kPi / 180.0f;
  return {mean * std::cos(angle) + sigma * gust[0], mean * std::sin(angle) + sigma * gust[1], 0.5f * sigma * gust[2]};
}

float Spl(float v, const Tuning &t) {
  return t.refSpl + 60.0f * std::log10(std::max(v, 1e-6f) / t.refSpeed);
}

float LevelDb(float v, const Tuning &t) {
  if (v <= 0.0f) {
    return -std::numeric_limits<float>::infinity();
  }
  return std::min((Spl(v, t) - t.mixSpl) / t.compression, t.maxDb);
}

float Level(float v, const Tuning &t) {
  return v <= 0.0f ? 0.0f : FromDb(LevelDb(v, t));
}

float WhistleLock(float v, const Tuning &t) {
  if (v <= 0.0f) {
    return 0.0f;
  }
  // each mode's frequency, f = (m - lag) U / (L (M + 1/kappa))
  const float mach = v / kSpeedOfSound;
  float lock = 0.0f;
  for (int m = 1; m <= 3; ++m) {
    const float f = (float(m) - kLag) * v / (t.gapLength * (mach + 1.0f / kConvection));
    const float z = (f - t.whistleHz) / (t.lockWidth * t.whistleHz);
    lock += std::exp(-0.5f * z * z);
  }
  return std::min(lock, 1.0f);
}

Model::Model(const Tuning &aTuning) : tuning(aTuning) {}

Params Model::Update(const Input &in) const {
  const Tuning &t = tuning;
  const Vec air{in.velocity.x - in.wind.x, in.velocity.y - in.wind.y, in.velocity.z - in.wind.z};
  const float u = Length(air);

  Params p;
  p.airspeed = u;
  p.level = Level(u, t);
  p.flowHz = t.strouhal * u / t.headScale;
  p.edgeHz = t.strouhal * u / t.edgeScale;
  p.edgeLevel = p.level * FromDb(t.edgeDb);
  // crossflow: the air's share across the car
  const float rightLength = std::max(Length(in.right), 1e-6f);
  const float across = u > 1e-3f ? Dot(air, in.right) / (u * rightLength) : 0.0f;
  p.aeolianHz = t.strouhal * u / t.strutScale;
  p.aeolianLevel = p.level * FromDb(t.aeolianDb + t.slipDb * std::fabs(across));

  if (in.view == View::Fpp) {
    // mass law: the loss is 20 log10(f m) - 47 dB, 0 at f0 = 10^(47/20) / m, 6 dB more an octave above
    p.cabinHz = std::pow(10.0f, 47.0f / 20.0f) / t.panelMass;
    p.leak = FromDb(t.leakDb);
    p.boomLevel = p.level * FromDb(t.boomDb);
    p.boomHz = t.boomHz;
    p.whistleLevel = p.level * FromDb(t.whistleDb) * WhistleLock(u, t);
    p.whistleHz = t.whistleHz;
    // the air comes from the way the car moves through it: from the left when that is to the left
    const float s = std::clamp(-across, -1.0f, 1.0f);
    p.left = std::sqrt((1.0f + s) / 2.0f);
    p.right = std::sqrt((1.0f - s) / 2.0f);
  }
  return p;
}

void Render::Svf::Design(float aHz, float aQ, float aRate) {
  const float hz = std::clamp(aHz, 5.0f, 0.45f * aRate);
  const float g = std::tan(kPi * hz / aRate);
  k = 1.0f / aQ;
  a1 = 1.0f / (1.0f + g * (g + k));
  a2 = g * a1;
  a3 = g * a2;
  // white noise's power per Hz is 2 / rate; the low-pass passes about (pi / 2) hz Q of it, the
  // band-pass (pi / 2) hz / Q
  lowGain = std::sqrt(aRate / (kPi * hz * aQ));
  bandGain = std::sqrt(aRate * aQ / (kPi * hz));
}

void Render::Svf::Run(float aX, float &aLow, float &aBand) {
  const float v3 = aX - ic2;
  const float v1 = a1 * ic1 + a2 * v3;
  const float v2 = ic2 + a2 * ic1 + a3 * v3;
  ic1 = 2.0f * v1 - ic1;
  ic2 = 2.0f * v2 - ic2;
  aLow = v2 * lowGain;
  aBand = k * v1 * bandGain;
}

Render::Render(float aSampleRate, uint32_t aSeed)
    : rate(aSampleRate), state(Nonzero(aSeed)), smoothing(1.0f - std::exp(-1.0f / (0.02f * aSampleRate))) {
  Set(target);
}

void Render::Set(const Params &aParams) {
  target = aParams;
  flow.Design(target.flowHz, 0.7071f, rate);
  edge.Design(target.edgeHz, 0.6f, rate);
  aeolian.Design(target.aeolianHz, 20.0f, rate);
  boom.Design(target.boomHz, 4.0f, rate);
  whistle.Design(target.whistleHz, 40.0f, rate);
}

float Render::Noise() {
  return Uniform(state);
}

void Render::Process(float *aStereo, uint32_t aFrames) {
  // the rumble rises 12 dB an octave to its peak: a high-pass two octaves under it
  const float highCoefficient = 1.0f - std::exp(-2.0f * kPi * std::max(target.flowHz / 4.0f, 2.0f) / rate);
  const float cabinCoefficient = 1.0f - std::exp(-2.0f * kPi * std::max(target.cabinHz, 1.0f) / rate);
  // the hiss falls 12 dB an octave over its peak, as turbulent flow noise does: a one-pole low-pass
  // there on top of the band-pass's 6
  const float edgeCoefficient = 1.0f - std::exp(-2.0f * kPi * target.edgeHz / rate);
  const float goals[7] = {target.level,        target.edgeLevel,
                          target.aeolianLevel, target.boomLevel,
                          target.whistleLevel, target.cabinHz > 0 ? 1.0f : 0.0f,
                          target.leak};
  float low = 0, band = 0;
  for (uint32_t i = 0; i < aFrames; ++i) {
    for (int k = 0; k < 7; ++k) {
      gain[k] += smoothing * (goals[k] - gain[k]);
    }
    flow.Run(Noise(), low, band);
    flowHigh += highCoefficient * (low - flowHigh);
    const float rumble = low - flowHigh;
    edge.Run(Noise(), low, band);
    edgeLow += edgeCoefficient * (band - edgeLow);
    const float hiss = edgeLow;
    aeolian.Run(Noise(), low, band);
    const float tone = band;
    const float outside = gain[0] * rumble + gain[1] * hiss + gain[2] * tone;

    // the panels low-pass what reaches the cabin; the seals let some through as it is
    cabin += cabinCoefficient * (outside - cabin);
    float inside = cabin + gain[6] * outside;
    boom.Run(Noise(), low, band);
    inside += gain[3] * band;
    whistle.Run(Noise(), low, band);
    inside += gain[4] * band;

    const float out = (1.0f - gain[5]) * outside + gain[5] * inside;
    aStereo[2 * i] = aStereo[2 * i + 1] = out;
  }
}

} // namespace Wind
