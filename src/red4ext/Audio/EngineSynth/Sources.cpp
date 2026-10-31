#include "Sources.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <numbers>

namespace EngineSynth {

namespace {

constexpr float kPi = std::numbers::pi_v<float>;
// the drone's RMS (two saws at half level and a sine at 0.5): every source is scaled to it
constexpr float kDroneRms = 0.54f;

// bullroarer: the measured harmonics against the fundamental (dB), the fundamental against the drone's
// (an octave up: the recordings sat near 140 Hz), each thruster's slat up to this many semitones off
// the middle one (10 to 25 Hz apart at 140 Hz: the range heard as roughness)
constexpr float kRoarHarmonics[8] = {0, -10, -10, -12, -15, -19, -23, -25};
constexpr float kRoarRatio = 2.0f;
constexpr float kSlatSpreadSt = 1.5f;
// a spin's pitch glides from 1 down to this as the cord twists; spins last 5 to 10 s; the stop and
// restart take this long; the orbit's swell and Doppler
constexpr float kGlideFloor = 0.62f;
constexpr float kStopSeconds = 0.35f;
constexpr float kOrbitDepth = 0.45f, kOrbitDepth2 = 0.15f, kDoppler = 0.03f;
// the air round the slat: about 10 dB under the tone (the recordings' harmonic to noise)
constexpr float kRoarNoiseDb = -10.0f;

// fan: blade-pass at this many times the drone's fundamental (8 blades on its shaft), the shaft hum
// and the blade-pass harmonics (dB), two turbine partials, the wobble (cents, dB) and the rush
constexpr float kBladeRatio = 8.0f;
constexpr float kFanLevels[6] = {-16, 0, -5, -11, -17, -20}; // hum, BPF x1..x4, (both turbines)
constexpr float kTurbineRatio[2] = {2.37f, 3.71f};
constexpr float kWobbleCents = 8.0f, kWobbleDb = 2.0f;
constexpr float kRushDb = -8.0f;

float Db(float aDb) {
  return std::pow(10.0f, aDb / 20.0f);
}

uint32_t Hash(uint32_t a) {
  a ^= a >> 16;
  a *= 0x7feb352d;
  a ^= a >> 15;
  a *= 0x846ca68b;
  a ^= a >> 16;
  return a ? a : 1;
}

float Norm(const float *aDb, int aCount) {
  float sum = 0;
  for (int i = 0; i < aCount; ++i) {
    sum += Db(aDb[i]) * Db(aDb[i]) / 2;
  }
  return kDroneRms / std::sqrt(sum);
}

} // namespace

const char *SoundName(Sound aSound) {
  switch (aSound) {
  case Sound::Bullroarer:
    return "bullroarer";
  case Sound::Fan:
    return "fan";
  default:
    return "drone";
  }
}

bool ParseSound(const char *aText, Sound &aSound) {
  for (int s = 0; s < kSounds; ++s) {
    if (aText && std::strcmp(aText, SoundName(Sound(s))) == 0) {
      aSound = Sound(s);
      return true;
    }
  }
  return false;
}

float Source::Uniform() {
  rng ^= rng << 13;
  rng ^= rng >> 17;
  rng ^= rng << 5;
  return float(int32_t(rng)) / 2147483648.0f;
}

// an Ornstein-Uhlenbeck step of unit variance and time scale aTime
float Source::NoiseStep(float &aState, float aSeconds, float aTime) {
  const float a = std::exp(-aSeconds / aTime);
  aState = a * aState + std::sqrt(1 - a * a) * 1.7320508f * Uniform();
  return aState;
}

void Source::Init(float aSampleRate, uint32_t aThruster, uint32_t aVoice) {
  rate = aSampleRate;
  rng = Hash(aThruster * 977u + 13u);
  slat = std::exp2(kSlatSpreadSt * Uniform() / 12.0f);
  rng = Hash(aThruster * 977u + 101u * (aVoice + 1));
  period = 5.0f + 2.5f * (Uniform() + 1.0f);
  twist = period * (0.5f * Uniform() + 0.5f);
  orbit = 0.5f * Uniform() + 0.5f;
  phase = 0.5 * Uniform() + 0.5;
  roarNorm = Norm(kRoarHarmonics, 8);
  for (int h = 0; h < 8; ++h) {
    roarAmp[h] = Db(kRoarHarmonics[h]);
  }
  float fan[7] = {kFanLevels[0], kFanLevels[1], kFanLevels[2], kFanLevels[3], kFanLevels[4], kFanLevels[5],
                  kFanLevels[5]};
  fanNorm = Norm(fan, 7);
}

void Source::Control(Sound aSound, const SourceControl &c, float aSeconds) {
  const float push = std::clamp(c.push, 0.0f, 1.0f), brake = std::clamp(c.brake, 0.0f, 1.0f);
  const float damage = std::clamp(c.damage, 0.0f, 0.25f) * 4.0f; // 0..1
  float bandHz = 1000, bandQ = 0.7f;
  if (aSound == Sound::Bullroarer) {
    // the orbit: faster held harder, uneven when damaged
    orbitRate = 1.3f + 1.2f * push + 0.8f * damage * Uniform();
    orbit += orbitRate * aSeconds;
    orbit -= std::floor(orbit);
    // the cord: the spin slows as it twists, stops and restarts the other way; braking twists it faster,
    // damage makes a restart hang
    if (stopTime < 0) {
      twist += aSeconds * (1.0f + 1.5f * brake);
      if (twist > period) {
        stopTime = 0;
      }
    }
    const float stopLength = kStopSeconds * (1.0f + 3.0f * damage * (0.5f * Uniform() + 0.5f));
    if (stopTime >= 0) {
      const float before = stopTime;
      stopTime += aSeconds;
      if (before < 0.5f * stopLength && stopTime >= 0.5f * stopLength) {
        twist = 0;
        period = (5.0f + 2.5f * (Uniform() + 1.0f)) * (1.0f - 0.4f * brake);
      }
      stop = 0.5f + 0.5f * std::cos(2 * kPi * std::min(stopTime / stopLength, 1.0f));
      if (stopTime >= stopLength) {
        stopTime = -1;
        stop = 1;
      }
    }
    const float glide = kGlideFloor + (1 - kGlideFloor) * std::exp(-twist / (0.45f * period));
    const float target = c.frequency * kRoarRatio * slat * glide * std::max(stop, 0.05f);
    spinHz += (target - spinHz) * (1 - std::exp(-aSeconds / 0.12f));
    const float o = 2 * kPi * orbit;
    roarHz = spinHz * (1 + kDoppler * std::cos(o));
    const float swell = std::max(0.0f, 1 + kOrbitDepth * std::sin(o) + kOrbitDepth2 * std::sin(2 * o + 1));
    roarGain = roarNorm * swell * stop * std::pow(glide, 0.8f);
    noiseGain = kDroneRms * Db(kRoarNoiseDb) * swell * stop;
    bandHz = 3 * roarHz;
  } else if (aSound == Sound::Fan) {
    // the rotor spools with inertia; braking winds it down
    const float target = c.frequency * kBladeRatio * std::sqrt(slat) * (1 - 0.25f * brake);
    bpf += (target - bpf) * (1 - std::exp(-aSeconds / 0.25f));
    wobblePitch = NoiseStep(wobblePitch, aSeconds, 0.25f);
    for (int i = 0; i < 2; ++i) {
      NoiseStep(turbinePitch[i], aSeconds, 0.18f);
    }
    for (int i = 0; i < 6; ++i) {
      const float level = kFanLevels[i] + (i == 0 ? 48.0f * damage * 0.25f : 0.0f) +
                          kWobbleDb * NoiseStep(wobbleLevel[i], aSeconds, 0.2f);
      harmonicGain[i] = fanNorm * Db(level) * (1 - 0.5f * brake);
    }
    rushGain = kDroneRms * Db(kRushDb + 6 * push + 6 * brake);
    bpfNow = bpf * std::exp2(kWobbleCents * wobblePitch / 1200.0f);
    for (int i = 0; i < 2; ++i) {
      turbineHz[i] = bpf * kTurbineRatio[i] * std::exp2(2 * kWobbleCents * turbinePitch[i] / 1200.0f);
    }
    noiseGain = rushGain;
    bandHz = 1.5f * bpf;
    bandQ = 0.5f;
  }
  // the noise band, unit RMS for white noise of unit variance
  const float hz = std::clamp(bandHz, 20.0f, 0.45f * rate);
  const float g = std::tan(kPi * hz / rate);
  k = 1 / bandQ;
  a1 = 1 / (1 + g * (g + k));
  a2 = g * a1;
  a3 = g * a2;
  bandGain = std::sqrt(rate * bandQ / (kPi * hz));
}

float Source::Next(Sound aSound) {
  // the noise band
  const float v3 = 1.7320508f * Uniform() - ic2;
  const float v1 = a1 * ic1 + a2 * v3;
  const float v2 = ic2 + a2 * ic1 + a3 * v3;
  ic1 = 2 * v1 - ic1;
  ic2 = 2 * v2 - ic2;
  const float noise = noiseGain * k * v1 * bandGain;

  if (aSound == Sound::Bullroarer) {
    phase += roarHz / rate;
    phase -= std::floor(phase);
    const float x = 2 * kPi * float(phase), s1 = std::sin(x), c2 = 2 * std::cos(x);
    // sin(k x) by the recurrence s(k) = 2 cos(x) s(k - 1) - s(k - 2), while under Nyquist
    float prev = 0, now = s1, sum = 0;
    for (int h = 0; h < 8 && float(h + 1) * roarHz < 0.45f * rate; ++h) {
      sum += roarAmp[h] * now;
      const float next = c2 * now - prev;
      prev = now;
      now = next;
    }
    return roarGain * sum + noise;
  }
  if (aSound == Sound::Fan) {
    phase += bpfNow / rate;
    phase -= std::floor(phase);
    const float x = 2 * kPi * float(phase);
    // the shaft turns once for kBladeRatio blade passes
    shaftPhase += bpfNow / (kBladeRatio * rate);
    shaftPhase -= std::floor(shaftPhase);
    float out = harmonicGain[0] * std::sin(2 * kPi * float(shaftPhase));
    const float s1 = std::sin(x), c2 = 2 * std::cos(x);
    float prev = 0, now = s1;
    for (int h = 1; h <= 4 && float(h) * bpf < 0.45f * rate; ++h) {
      out += harmonicGain[h] * now;
      const float next = c2 * now - prev;
      prev = now;
      now = next;
    }
    for (int i = 0; i < 2; ++i) {
      if (turbineHz[i] < 0.45f * rate) {
        turbinePhase[i] += turbineHz[i] / rate;
        turbinePhase[i] -= std::floor(turbinePhase[i]);
        out += harmonicGain[5] * std::sin(2 * kPi * float(turbinePhase[i]));
      }
    }
    return out + noise;
  }
  return 0;
}

} // namespace EngineSynth
