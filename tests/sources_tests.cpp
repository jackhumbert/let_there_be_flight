// The synth's sources (src/red4ext/Audio/EngineSynth/Sources): names, levels against the drone, the
// bullroarer's glide and reversal and its slats, the fan's brightness.
#include "check.hpp"

#include "Audio/EngineSynth/Synth.hpp"

#include <cmath>
#include <vector>

using namespace EngineSynth;

namespace {
constexpr float kRate = 48000;

struct Measure {
  double rms = 0, crossings = 0; // crossings: upward zero crossings a second
  bool finite = true;
};

Measure Render(Sound aSound, float aSeconds, uint32_t aSeed = 3) {
  Tuning tuning;
  tuning.sound = aSound;
  Synth synth(kRate, 1.0f, aSeed, tuning);
  Inputs in;
  in.speed = 40;
  in.surge = 1;
  synth.Set(in);
  std::vector<float> out(size_t(kRate * aSeconds));
  synth.Render(out.data(), uint32_t(kRate * 0.5f)); // settle
  synth.Render(out.data(), uint32_t(out.size()));
  Measure m;
  double sum = 0;
  for (size_t i = 0; i < out.size(); ++i) {
    m.finite = m.finite && std::isfinite(out[i]);
    sum += double(out[i]) * out[i];
    m.crossings += (i > 0 && out[i - 1] < 0 && out[i] >= 0) ? 1 : 0;
  }
  m.rms = std::sqrt(sum / double(out.size()));
  m.crossings /= aSeconds;
  return m;
}
} // namespace

TEST(sources_names) {
  for (int s = 0; s < kSounds; ++s) {
    Sound parsed = Sound::Drone;
    CHECK(ParseSound(SoundName(Sound(s)), parsed));
    CHECK(parsed == Sound(s));
  }
  Sound sound = Sound::Fan;
  CHECK(!ParseSound("jet", sound));
  CHECK(sound == Sound::Fan);
  CHECK(Tuning{}.sound == Sound::Drone);
}

TEST(sources_levels_near_the_drone) {
  const Measure drone = Render(Sound::Drone, 10);
  for (Sound s : {Sound::Bullroarer, Sound::Fan}) {
    const Measure m = Render(s, 10);
    CHECK(m.finite);
    CHECK_NEAR(20.0 * std::log10(m.rms / drone.rms), 0.0, 6.0);
  }
}

TEST(sources_fan_brighter_than_the_drone) {
  CHECK(Render(Sound::Fan, 4).crossings > 2.0 * Render(Sound::Drone, 4).crossings);
}

TEST(sources_bullroarer_glides_and_reverses) {
  Source source;
  source.Init(kRate, 7, 0);
  const SourceControl control{70.0f, 1.0f, 0.0f, 0.0f};
  const float block = 32 / kRate;
  // wait for a reversal, then follow the next spin
  float t = 0;
  bool stopped = false;
  for (; t < 12 && !stopped; t += block) {
    source.Control(Sound::Bullroarer, control, block);
    stopped = source.BullroarerStop() < 0.1f;
  }
  CHECK(stopped);
  for (float until = t + 1; t < until && source.BullroarerStop() < 1; t += block) {
    source.Control(Sound::Bullroarer, control, block);
  }
  CHECK(source.BullroarerStop() == 1.0f);
  // a spin starts near twice the drone's pitch (times the slat) and glides down by up to 38%
  std::vector<float> hz;
  for (float until = t + 4; t < until; t += block) {
    source.Control(Sound::Bullroarer, control, block);
    hz.push_back(source.BullroarerHz());
  }
  float early = 0, late = 0;
  const size_t n = hz.size();
  for (size_t i = n / 16; i < n / 8; ++i) {
    early += hz[i];
  }
  for (size_t i = n - n / 16; i < n; ++i) {
    late += hz[i];
  }
  early /= float(n / 8 - n / 16);
  late /= float(n / 16);
  CHECK(early > 0.8f * 140 * source.Slat());
  CHECK(late < 0.92f * early);
  CHECK(late > 0.55f * early);
}

TEST(sources_bullroarer_slats_differ) {
  // each thruster's slat a different width: the car's four bullroarers beat
  float lo = 10, hi = 0;
  for (uint32_t thruster = 1; thruster <= 4; ++thruster) {
    Source s;
    s.Init(kRate, thruster, 0);
    CHECK(std::abs(12 * std::log2(s.Slat())) <= 1.5f);
    lo = std::min(lo, s.Slat());
    hi = std::max(hi, s.Slat());
    // the five voices of one thruster share its slat
    Source other;
    other.Init(kRate, thruster, 3);
    CHECK(other.Slat() == s.Slat());
  }
  CHECK(12 * std::log2(hi / lo) > 0.5f);
}
