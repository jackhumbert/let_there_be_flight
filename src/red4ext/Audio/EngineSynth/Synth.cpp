#include "Synth.hpp"
#include "Curves.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <string>

namespace EngineSynth {

namespace {

constexpr int kControlSamples = 32;
constexpr float kSmoothSeconds = 0.02f;

float Db(float aDb) {
  return std::pow(10.0f, aDb / 20.0f);
}

float Semitones(float aSt) {
  return std::exp2(aSt / 12.0f);
}

// a band-limited saw's correction at a step (PolyBLEP)
float Blep(double t, double dt) {
  if (t < dt) {
    t /= dt;
    return float(t + t - t * t - 1.0);
  }
  if (t > 1.0 - dt) {
    t = (t - 1.0) / dt;
    return float(t * t + t + t + 1.0);
  }
  return 0.0f;
}

const AxisCurves *const kAxes[5] = {&kSurge, &kLift, &kPitch, &kRoll, &kSway};

// FMOD's distortion: k = 2a / (1 - a), (1 + k) x / (1 + k |x|): a gain of 1 + k for quiet sound,
// loud sound pressed towards 1
float Distort(float x, float aLevel) {
  if (aLevel <= 0) {
    return x;
  }
  const float a = std::min(aLevel, 0.98f);
  const float k = 2 * a / (1 - a);
  return (1 + k) * x / (1 + k * std::abs(x));
}

} // namespace

void Synth::Svf::Set(float aFrequency, float aQ, float aSampleRate) {
  const float g = std::tan(std::numbers::pi_v<float> * std::clamp(aFrequency, 10.0f, 0.45f * aSampleRate) / aSampleRate);
  k = 1.0f / aQ;
  a1 = 1.0f / (1.0f + g * (g + k));
  a2 = g * a1;
  a3 = g * a2;
}

void Synth::Svf::Run(float aIn, float &aLow, float &aBand) {
  const float v3 = aIn - ic2;
  const float v1 = a1 * ic1 + a2 * v3;
  const float v2 = ic2 + a2 * ic1 + a3 * v3;
  ic1 = 2 * v1 - ic1;
  ic2 = 2 * v2 - ic2;
  aLow = v2;
  aBand = v1;
}

bool ParseCharacter(const char *aText, Character &aCharacter) {
  const std::string text = aText ? aText : "";
  if (text == "fmod") {
    aCharacter = {};
    return true;
  }
  if (text == "clean") {
    aCharacter = {false, false, false, false};
    return true;
  }
  size_t at = 0;
  while (at < text.size()) {
    const size_t end = std::min(text.find(',', at), text.size());
    const std::string item = text.substr(at, end - at);
    at = end + 1;
    const size_t eq = item.find('=');
    if (eq == std::string::npos) {
      return false;
    }
    const std::string name = item.substr(0, eq);
    const bool on = item.substr(eq + 1) != "0";
    bool *field = name == "cone"         ? &aCharacter.cone
                  : name == "distortion" ? &aCharacter.distortion
                  : name == "darkness"   ? &aCharacter.darkness
                  : name == "grit"       ? &aCharacter.grit
                                         : nullptr;
    if (!field) {
      return false;
    }
    *field = on;
  }
  return true;
}

float Synth::Stage(float x, float aLevel, float &aEnvelope) const {
  if (tuning.character.distortion || aLevel <= 0) {
    return Distort(x, aLevel);
  }
  // the stage's gain at the sound's level (a peak follower), applied without bending the wave
  const float level = std::abs(x);
  aEnvelope += (level - aEnvelope) * (level > aEnvelope ? envelopeAttack : envelopeRelease);
  const float e = std::max(aEnvelope, 1e-6f);
  return x * Distort(e, aLevel) / e;
}

void Synth::SetCharacter(const Character &aCharacter) {
  if (!(aCharacter == tuning.character)) {
    tuning.character = aCharacter;
    insideForFilters = -1;
  }
}

void Synth::EngineFilters() {
  if (tuning.character.darkness) {
    engineLowPass.LowPass(kEngineLowPass(now.inside) * carPitch, kEngineLowPassQ, sampleRate);
    engineHighShelf.Shelf(true, kEngineHighShelf.frequency * carPitch, kEngineHighShelf.gain, kEngineHighShelf.q,
                          sampleRate);
  } else {
    engineLowPass.LowPass(8000.0f, 0.707f, sampleRate);
    engineHighShelf.Shelf(true, 800.0f, -4.0f, 0.707f, sampleRate);
  }
  insideForFilters = now.inside;
}

void Synth::Biquad::Peak(float aFrequency, float aGain, float aQ, float aSampleRate) {
  const float A = std::pow(10.0f, aGain / 40.0f);
  const float w = 2 * std::numbers::pi_v<float> * std::min(aFrequency, 0.45f * aSampleRate) / aSampleRate;
  const float alpha = std::sin(w) / (2 * aQ);
  const float a0 = 1 + alpha / A;
  b0 = (1 + alpha * A) / a0;
  b1 = -2 * std::cos(w) / a0;
  b2 = (1 - alpha * A) / a0;
  a1 = b1;
  a2 = (1 - alpha / A) / a0;
}

void Synth::Biquad::LowPass(float aFrequency, float aQ, float aSampleRate) {
  const float w = 2 * std::numbers::pi_v<float> * std::min(aFrequency, 0.45f * aSampleRate) / aSampleRate;
  const float alpha = std::sin(w) / (2 * aQ), c = std::cos(w);
  const float a0 = 1 + alpha;
  b0 = (1 - c) / 2 / a0;
  b1 = (1 - c) / a0;
  b2 = b0;
  a1 = -2 * c / a0;
  a2 = (1 - alpha) / a0;
}

void Synth::Biquad::Shelf(bool aHigh, float aFrequency, float aGain, float aQ, float aSampleRate) {
  const float A = std::pow(10.0f, aGain / 40.0f);
  const float w = 2 * std::numbers::pi_v<float> * std::min(aFrequency, 0.45f * aSampleRate) / aSampleRate;
  const float c = std::cos(w), alpha = std::sin(w) / (2 * aQ), r = 2 * std::sqrt(A) * alpha;
  // RBJ's low shelf; the high shelf mirrors it (the cosine's sign flipped)
  const float sign = aHigh ? -1.0f : 1.0f;
  const float a0 = (A + 1) + sign * (A - 1) * c + r;
  b0 = A * ((A + 1) - sign * (A - 1) * c + r) / a0;
  b1 = sign * 2 * A * ((A - 1) - sign * (A + 1) * c) / a0;
  b2 = A * ((A + 1) - sign * (A - 1) * c - r) / a0;
  a1 = -sign * 2 * ((A - 1) + sign * (A + 1) * c) / a0;
  a2 = ((A + 1) + sign * (A - 1) * c - r) / a0;
}

float Synth::Biquad::Run(float x) {
  const float y = b0 * x + z1;
  z1 = b1 * x - a1 * y + z2;
  z2 = b2 * x - a2 * y;
  return y;
}

Synth::Synth(float aSampleRate, float aPitch, uint32_t aSeed, const Tuning &aTuning)
    : sampleRate(aSampleRate), carPitch(aPitch), tuning(aTuning), rng(aSeed * 2654435761u + 12345u) {
  rng = rng ? rng : 1;
  detune = tuning.spread * (2.0f * (float(Noise() * 0.5f + 0.5f)) - 1.0f);
  smooth = 1.0f - std::exp(-float(kControlSamples) / (kSmoothSeconds * sampleRate));
  // the event's pitch moves everything in it, its EQs too
  for (int v = 0; v < 5; ++v) {
    Axis &a = axes[v];
    for (auto &p : a.phase) {
      p = double(Noise() * 0.5f + 0.5f);
    }
    for (const Peak &band : kAxes[v]->eq) {
      if (band.frequency > 0) {
        a.eq[a.bands++].Peak(band.frequency * carPitch, band.gain, band.q, sampleRate);
      }
    }
  }
  for (int b = 0; b < 3; ++b) {
    engineEq[b].Peak(kEngineEq[b].frequency * carPitch, kEngineEq[b].gain, kEngineEq[b].q, sampleRate);
  }
  envelopeAttack = 1.0f - std::exp(-1.0f / (0.002f * sampleRate));
  envelopeRelease = 1.0f - std::exp(-1.0f / (0.05f * sampleRate));
  engineLowShelf.Shelf(false, kEngineLowShelf.frequency * carPitch, kEngineLowShelf.gain, kEngineLowShelf.q,
                       sampleRate);
  Control();
}

float Synth::Noise() {
  rng ^= rng << 13;
  rng ^= rng >> 17;
  rng ^= rng << 5;
  return float(int32_t(rng)) / 2147483648.0f;
}

void Synth::Set(const Inputs &aInputs) {
  target = aInputs;
}

void Synth::Control() {
  auto ease = [&](float &aNow, float aTarget) { aNow += (aTarget - aNow) * smooth; };
  ease(now.speed, target.speed);
  ease(now.surge, target.surge);
  ease(now.lift, target.lift);
  ease(now.pitch, target.pitch);
  ease(now.roll, target.roll);
  ease(now.sway, target.sway);
  ease(now.yaw, target.yaw);
  ease(now.brake, target.brake);
  ease(now.damage, target.damage);
  ease(now.inside, target.inside);
  ease(now.scrape, target.scrape);
  ease(now.water, target.water);
  ease(now.volume, target.volume);
  ease(now.cone, target.cone);

  const float speed = std::max(0.0f, now.speed);
  // what speed drives besides pitch: the distortion and the low-pass
  const float gritSpeed = tuning.character.grit ? speed : 40.0f;
  const float axis[5] = {now.surge, now.lift, now.pitch, now.roll, now.sway};
  const float damageSt = kDamagePitch(now.damage);
  for (int v = 0; v < 5; ++v) {
    const AxisCurves &c = *kAxes[v];
    Axis &a = axes[v];
    const float st = c.pitch(axis[v]) + c.speedPitch(speed) + damageSt + detune;
    a.frequency = c.fundamental * carPitch * Semitones(st);
    a.preGain = Db(c.preLevel(axis[v]) + tuning.source);
    a.drive = std::clamp(c.drive(gritSpeed) + c.axisDrive(axis[v]), 0.0f, 1.0f);
    a.gain = Db(c.level(axis[v]));
    a.send = Db(c.send + tuning.level[v]);
    const float cutoff = c.cutoff(gritSpeed);
    a.hasLowPass = cutoff > 0;
    if (a.hasLowPass) {
      a.lowPass.LowPass(cutoff * carPitch, 0.33f, sampleRate);
    }
  }
  engineDrive = kEngineDrive(gritSpeed);
  if (std::abs(now.inside - insideForFilters) > 0.01f) {
    EngineFilters();
  }
  coneGain = tuning.character.cone ? Db(kConeSend(now.inside) + kConeLevel(now.cone)) : 0.0f;
  yawGain = Db(kYawBand(now.yaw)) - 1.0f;
  liftGain = Db(kLiftBand(now.lift)) - 1.0f;
  yawBand.Set(540.0f * carPitch, 1.0f, sampleRate);
  liftBand.Set(220.0f * carPitch, 1.0f, sampleRate);

  // air: how hard the thrusters push, and the speed
  const float thrust = std::min(1.0f, std::abs(now.surge) + std::abs(now.lift) + 0.5f * (std::abs(now.pitch) +
                                      std::abs(now.roll) + std::abs(now.sway) + std::abs(now.yaw)));
  airGain = Db(tuning.level[Air] - 12.0f * (1.0f - thrust)) * std::min(1.0f, 0.3f + speed / 100.0f);
  air.Set(300.0f + 1500.0f * thrust + 6.0f * speed, 0.7f, sampleRate);

  brakeGain = Db(kBrakeLevel(now.brake) + kBrakeSpeedLevel(speed) + kBrakeFader + tuning.level[Brake]);
  brakeHigh.Set(kBrakeBand(now.brake), 2.0f, sampleRate);
  brakeLow.Set(2960.0f, 3.0f, sampleRate);
  tremoloRate = kBrakeTremolo(speed);

  whirlFrequency = kWhirlFundamental * Semitones(kWhirlPitch(speed));
  whirlGain = Db(kDamageLevel(now.damage) + kWhirlVolume + tuning.level[Damage]);

  scrapeGain = Db(kScrapeLevel(now.scrape) + kScrapeSpeedLevel(speed) + kScrapeSend + tuning.level[Scrape]);
  scrapeRate = (20.0f + 200.0f * now.scrape) * Semitones(kScrapePitch(speed)) / sampleRate;
  scrapeBand.Set(1650.0f * Semitones(kScrapePitch(speed)), 1.5f, sampleRate);

  masterGain = Db(kInsideLevel(now.inside) + tuning.master) * std::max(0.0f, now.volume);
  waterCutoff = kWaterCutoff(now.water);
  water.Set(waterCutoff, 0.707f, sampleRate);
}

void Synth::Render(float *aOut, uint32_t aFrames, float *aVoices) {
  const double detuneRatio = std::exp2(double(tuning.detune) / 12.0);
  for (uint32_t s = 0; s < aFrames; ++s) {
    if (controlLeft-- <= 0) {
      controlLeft = kControlSamples - 1;
      Control();
    }
    float part[kVoices] = {};
    float engine = 0;
    for (int v = 0; v < 5; ++v) {
      Axis &a = axes[v];
      const AxisCurves &c = *kAxes[v];
      const double dt[3] = {a.frequency / sampleRate, a.frequency * detuneRatio / sampleRate, a.frequency / sampleRate};
      float x = 0;
      for (int o = 0; o < 2; ++o) {
        x += float(2.0 * a.phase[o] - 1.0) - Blep(a.phase[o], dt[o]);
      }
      x = 0.5f * x + tuning.sine * float(std::sin(2.0 * std::numbers::pi * a.phase[2]));
      for (int o = 0; o < 3; ++o) {
        a.phase[o] += dt[o];
        a.phase[o] -= std::floor(a.phase[o]);
      }
      x *= a.preGain;
      for (int b = 0; b < a.bands; ++b) {
        x = a.eq[b].Run(x);
      }
      if (v == Surge) {
        float yLow, yBand, lLow, lBand;
        yawBand.Run(x, yLow, yBand);
        liftBand.Run(x, lLow, lBand);
        x += yawGain * yBand + liftGain * lBand;
      }
      x = Stage(x, a.drive, a.envelope[0]) * a.gain;
      x = Stage(x, c.drive2, a.envelope[1]);
      if (a.hasLowPass) {
        x = a.lowPass.Run(x);
      }
      part[v] = a.send * x;
      engine += part[v];
    }
    // the Engine return; each voice's part scaled by what the return did to the sum (for the fit)
    float e = engine;
    for (auto &b : engineEq) {
      e = b.Run(e);
    }
    e = Stage(e, engineDrive, engineEnvelope);
    e = engineLowShelf.Run(engineHighShelf.Run(engineLowPass.Run(e)));
    e *= 1.0f + coneGain;
    const float returnScale = std::abs(engine) > 1e-9f ? e / engine : 0.0f;
    for (int v = 0; v < 5; ++v) {
      part[v] *= returnScale;
    }
    // pink noise (Paul Kellet's filter) through the air band
    const float white = Noise();
    pink[0] = 0.99886f * pink[0] + white * 0.0555179f;
    pink[1] = 0.99332f * pink[1] + white * 0.0750759f;
    pink[2] = 0.96900f * pink[2] + white * 0.1538520f;
    pink[3] = 0.86650f * pink[3] + white * 0.3104856f;
    pink[4] = 0.55000f * pink[4] + white * 0.5329522f;
    pink[5] = -0.7616f * pink[5] - white * 0.0168980f;
    const float p = 0.11f * (pink[0] + pink[1] + pink[2] + pink[3] + pink[4] + pink[5] + pink[6] + white * 0.5362f);
    pink[6] = white * 0.115926f;
    {
      float low, band;
      air.Run(p, low, band);
      part[Air] = airGain * band;
    }
    if (brakeGain > 1e-5f) {
      float hl, hb, ll, lb;
      brakeHigh.Run(white, hl, hb);
      brakeLow.Run(white, ll, lb);
      tremoloPhase += tremoloRate / sampleRate;
      tremoloPhase -= std::floor(tremoloPhase);
      const float tremolo = 0.65f + 0.35f * std::sin(2.0f * std::numbers::pi_v<float> * tremoloPhase);
      part[Brake] = brakeGain * tremolo * (hb + 0.5f * lb);
    }
    if (whirlGain > 1e-5f) {
      whirlPhase += whirlFrequency / sampleRate;
      whirlPhase -= std::floor(whirlPhase);
      const double w = 2.0 * std::numbers::pi * whirlPhase;
      part[Damage] = whirlGain * float(0.6 * std::sin(w) + 0.3 * std::sin(2 * w) + 0.1 * std::sin(3 * w));
    }
    if (scrapeGain > 1e-5f) {
      // a click now and then, each a short decaying burst
      if (Noise() * 0.5f + 0.5f < scrapeRate) {
        click = 1.0f;
      }
      click *= 0.995f;
      float low, band;
      scrapeBand.Run(white * click, low, band);
      part[Scrape] = scrapeGain * band;
    }
    float sum = 0;
    for (int v = 0; v < kVoices; ++v) {
      sum += part[v];
    }
    float out = sum;
    if (waterCutoff < 20000.0f) {
      float low, band;
      water.Run(sum, low, band);
      out = low;
    }
    aOut[s] = masterGain * out;
    if (aVoices) {
      for (int v = 0; v < kVoices; ++v) {
        aVoices[size_t(s) * kVoices + v] = masterGain * part[v];
      }
    }
  }
}

} // namespace EngineSynth
