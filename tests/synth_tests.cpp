// The engine synth (src/red4ext/Audio/EngineSynth): it renders finite sound, louder with the surge held.
#include "check.hpp"

#include "Audio/EngineSynth/Synth.hpp"

#include <cmath>
#include <vector>

namespace {
float Rms(EngineSynth::Synth &aSynth, uint32_t aFrames) {
  std::vector<float> out(aFrames * 2);
  aSynth.Render(out.data(), aFrames);
  double sum = 0;
  for (float x : out) {
    if (!std::isfinite(x)) {
      return -1;
    }
    sum += double(x) * x;
  }
  return float(std::sqrt(sum / out.size()));
}
} // namespace

TEST(synth_finite_and_louder_with_surge) {
  EngineSynth::Synth idle(48000, 1, 1), surge(48000, 1, 1);
  EngineSynth::Inputs in;
  in.speed = 40;
  idle.Set(in);
  in.surge = 1;
  surge.Set(in);
  Rms(idle, 48000);
  Rms(surge, 48000);
  const float quiet = Rms(idle, 48000), loud = Rms(surge, 48000);
  CHECK(quiet >= 0 && loud >= 0);
  CHECK(loud > quiet);
}

TEST(synth_character_clean_by_default) {
  const EngineSynth::Character byDefault;
  CHECK(!byDefault.cone && !byDefault.distortion && !byDefault.darkness && !byDefault.grit);
  EngineSynth::Character c;
  CHECK(EngineSynth::ParseCharacter("fmod", c));
  CHECK(c.cone && c.distortion && c.darkness && c.grit);
  CHECK(EngineSynth::ParseCharacter("clean", c));
  CHECK(c == byDefault);
  CHECK(EngineSynth::ParseCharacter("darkness=1", c));
  CHECK(c.darkness && !c.cone);
  CHECK(!EngineSynth::ParseCharacter("loud=1", c));
}

namespace {
// the scrape voice alone over aSeconds: its RMS, and the energy share in a band (Hz) by a plain DFT at a few bins
struct ScrapeHeard {
  float rms = 0, spread = 1;
  std::vector<float> samples;
};
ScrapeHeard RunScrape(int aSurface, float aSlide, float aIntensity, float aSeconds = 0.5f, uint32_t aSeed = 3) {
  EngineSynth::ScrapeVoice v(aSeed);
  v.Prepare(48000);
  ScrapeHeard h;
  h.spread = v.Spread();
  const int n = int(aSeconds * 48000);
  double sum = 0;
  for (int i = 0; i < n; ++i) {
    if (i % 64 == 0)
      v.Set(aSurface, aSlide, aIntensity);
    const float x = v.Next();
    h.samples.push_back(x);
    sum += double(x) * x;
  }
  h.rms = float(std::sqrt(sum / n));
  return h;
}

float BandPower(const std::vector<float> &aX, float aFrequency) {
  double re = 0, im = 0;
  for (size_t i = 0; i < aX.size(); ++i) {
    const double w = 2.0 * 3.14159265358979 * aFrequency * double(i) / 48000.0;
    re += aX[i] * std::cos(w);
    im += aX[i] * std::sin(w);
  }
  return float((re * re + im * im) / double(aX.size()));
}

// the spectral centroid (Hz) over 48 log-spaced frequencies from 100 Hz to 10 kHz, each the mean of three bins a third
// of a step apart (so one resonance does not stand for a band)
float Centroid(const std::vector<float> &aX) {
  double weighted = 0, total = 0;
  for (int i = 0; i < 48; ++i) {
    const double f = 100.0 * std::pow(100.0, i / 47.0);
    double p = 0;
    for (int k = -1; k <= 1; ++k)
      p += BandPower(aX, float(f * std::pow(100.0, k / (3.0 * 47.0))));
    // per log band, the power density times the band's width
    weighted += p * f * f;
    total += p * f;
  }
  return float(weighted / total);
}
} // namespace

TEST(scrape_voice_finite_and_silent_at_rest) {
  // every surface renders finite sound while sliding; standing still there is no grit, stick-slip or grains, and only a
  // low rumble of texture
  for (int s = 0; s <= 5; ++s) {
    const ScrapeHeard sliding = RunScrape(s, 6.0f, 0.5f);
    CHECK(std::isfinite(sliding.rms) && sliding.rms > 0.0f);
    CHECK(RunScrape(s, 0.0f, 0.5f).rms < sliding.rms);
  }
}

TEST(scrape_voice_metal_rings_at_its_modes) {
  // metal's first mode (1210 Hz, as this voice is detuned) stands well over a frequency between its modes
  const ScrapeHeard metal = RunScrape(2, 6.0f, 0.5f, 1.0f);
  CHECK(BandPower(metal.samples, 1210.0f * metal.spread) > 10.0f * BandPower(metal.samples, 1800.0f * metal.spread));
}

TEST(scrape_voice_pitch_follows_slide) {
  // every surface's sound climbs with the slide, as the roughness passes under the contact faster: its spectral centre
  // at 12 m/s is well over its centre at 1.5 m/s, about twice (the surface's resonances stay where they are, and a slow
  // scrape's impacts are sharp cracks)
  for (int s = 1; s <= 5; ++s) {
    const float slow = Centroid(RunScrape(s, 1.5f, 0.4f, 1.0f).samples);
    const float fast = Centroid(RunScrape(s, 12.0f, 0.4f, 1.0f).samples);
    CHECK(fast > 1.6f * slow);
  }
}

TEST(scrape_voice_resonator_level_any_pitch) {
  // driven by white noise a resonator's level is its gain, low or high, long or short: a low mode does not ring over a
  // high one
  for (float f : {110.0f, 850.0f, 5200.0f})
    for (float decay : {0.01f, 0.5f}) {
      EngineSynth::Resonator r;
      r.Set(f, decay, 1.0f, 48000.0f);
      uint32_t rng = 12345;
      double in = 0, out = 0;
      for (int i = 0; i < 96000; ++i) {
        rng ^= rng << 13;
        rng ^= rng >> 17;
        rng ^= rng << 5;
        const float x = float(int32_t(rng)) / 2147483648.0f;
        const float y = r.Run(x);
        if (i >= 48000) {
          in += double(x) * x;
          out += double(y) * y;
        }
      }
      CHECK_NEAR(10.0 * std::log10(out / in), 0.0, 1.5);
    }
}

TEST(scrape_voice_seeds_detune) {
  // two thrusters' voices are a little apart in pitch (so they do not beat), and never far
  EngineSynth::ScrapeVoice a(1), b(2);
  a.Prepare(48000);
  b.Prepare(48000);
  CHECK(a.Spread() != b.Spread());
  for (uint32_t seed = 0; seed < 64; ++seed) {
    EngineSynth::ScrapeVoice v(seed);
    v.Prepare(48000);
    CHECK(v.Spread() >= 0.97f && v.Spread() <= 1.03f);
  }
}

TEST(scrape_voice_resonator_decays) {
  // struck once, a resonator rings at its frequency and falls 60 dB in its decay
  EngineSynth::Resonator r;
  r.Set(1000.0f, 0.1f, 1.0f, 48000.0f);
  float first = 0, later = 0;
  for (int i = 0; i < 4800 * 2; ++i) {
    const float y = std::fabs(r.Run(i == 0 ? 1.0f : 0.0f));
    if (i < 480)
      first = std::fmax(first, y);
    if (i >= 4800 && i < 4800 + 480)
      later = std::fmax(later, y);
  }
  CHECK(later < first * 0.002f && later > first * 0.0002f);
}
