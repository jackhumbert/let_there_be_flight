// The engine synth (src/red4ext/Audio/EngineSynth): it renders finite sound, louder with the surge held.
#include "check.hpp"

#include "Audio/EngineSynth/Scrape.hpp"
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
  for (int s = 0; s <= 6; ++s) {
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
  for (int s = 1; s <= 6; ++s) {
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

using namespace EngineSynth;

TEST(scrape_chassis_modes) {
  // an average car's floor pan rings from about 90 Hz, its modes rising, the lowest ringing longest and loudest; a
  // truck's lower, a bike's higher; no size is an average car's
  Mode car[6], truck[6], bike[6], none[6];
  ChassisModes({1.9f, 4.5f, 1.4f, 1600.0f}, car);
  ChassisModes({2.5f, 8.0f, 3.5f, 10000.0f}, truck);
  ChassisModes({0.8f, 2.1f, 1.1f, 200.0f}, bike);
  ChassisModes({0, 0, 0, 0}, none);
  CHECK(car[0].frequency > 60.0f && car[0].frequency < 130.0f);
  for (int i = 1; i < 6; ++i) {
    CHECK(car[i].frequency >= car[i - 1].frequency);
    CHECK(car[i].decay <= car[i - 1].decay);
  }
  CHECK(car[0].gain >= car[5].gain);
  CHECK(truck[0].frequency < car[0].frequency);
  CHECK(bike[0].frequency > car[0].frequency);
  CHECK_NEAR(none[0].frequency, car[0].frequency, 1e-3f);
}

TEST(scrape_synth_level) {
  // the scrape's own voice: silent without a scrape, a crawl nearly as loud as a slide, and pressed harder louder
  auto rms = [](float aIntensity, float aSlide) {
    ScrapeSynth synth(48000.0f, 5);
    ScrapeInputs in;
    in.intensity = aIntensity;
    in.slide = aSlide;
    in.surface = 1;
    synth.Set(in);
    std::vector<float> out(24000);
    synth.Render(out.data(), 24000); // settles
    synth.Render(out.data(), 24000);
    double e = 0;
    for (float x : out)
      e += double(x) * x;
    return 10.0 * std::log10(e / 24000 + 1e-30);
  };
  CHECK(rms(0.0f, 6.0f) < -200.0);
  const double slide = rms(0.5f, 6.0f), crawl = rms(0.45f, 0.1f);
  CHECK(std::isfinite(slide) && slide > -80.0);
  CHECK(crawl > slide - 6.0);
  CHECK(rms(0.9f, 6.0f) > slide + 3.0);
}

TEST(scrape_synth_chassis_heard) {
  // a car's chassis rings in the scrape: a bigger, heavier car's lower (each voice detuned by its seed, Spread)
  float spread = 1.0f;
  auto sound = [&](const Chassis &aChassis) {
    ScrapeSynth synth(48000.0f, 9);
    spread = synth.Spread();
    ScrapeInputs in;
    in.intensity = 0.6f;
    in.slide = 4.0f;
    in.surface = 2;
    in.chassis = aChassis;
    synth.Set(in);
    std::vector<float> out(48000);
    synth.Render(out.data(), 48000);
    return out;
  };
  const Chassis carShape{1.9f, 4.5f, 1.4f, 1600.0f}, truckShape{2.5f, 8.0f, 3.5f, 10000.0f};
  Mode car[6], truck[6];
  ChassisModes(carShape, car);
  ChassisModes(truckShape, truck);
  const auto carSound = sound(carShape), truckSound = sound(truckShape);
  const float carLow = car[0].frequency * spread, truckLow = truck[0].frequency * spread;
  // each rings at its own lowest mode more than the other car does there
  CHECK(BandPower(carSound, carLow) > 2.0f * BandPower(truckSound, carLow));
  CHECK(BandPower(truckSound, truckLow) > 2.0f * BandPower(carSound, truckLow));
}

TEST(scrape_speed_rules) {
  // impacts harder the faster they are struck, held at a crawl and at the top; stick-slip gone as the slide quickens;
  // chatter only past 6 m/s, more the faster
  CHECK_NEAR(ImpactScale(4.0f), 1.0f, 1e-6f);
  CHECK_NEAR(ImpactScale(0.02f), 0.3f, 1e-6f);
  CHECK(ImpactScale(8.0f) > 1.9f && ImpactScale(40.0f) <= 3.0f);
  CHECK_NEAR(StickAmount(0.0f, 2.0f), 1.0f, 1e-6f);
  CHECK_NEAR(StickAmount(2.0f, 2.0f), 0.5f, 1e-6f);
  CHECK(StickAmount(6.0f, 2.0f) < 0.11f);
  CHECK_NEAR(ChatterRate(5.0f), 0.0f, 1e-6f);
  CHECK(ChatterRate(12.0f) > ChatterRate(8.0f) && ChatterRate(8.0f) > 0.0f);
}

TEST(scrape_metal_squeal_fades) {
  // metal squeals at a crawl (its stick-slip pulses at their rate, 90 Hz plus 25 per m/s) and slides quietly fast:
  // the pulse rate's share of the sound falls as the slide quickens
  auto share = [](float aSlide) {
    ScrapeSynth synth(48000.0f, 5);
    ScrapeInputs in;
    in.intensity = 0.6f;
    in.slide = aSlide;
    in.surface = 2;
    synth.Set(in);
    std::vector<float> out(48000);
    synth.Render(out.data(), 48000);
    double total = 0;
    for (float x : out)
      total += double(x) * x;
    const float rate = (90.0f + 25.0f * aSlide) * synth.Spread();
    return BandPower(out, rate) / float(total / out.size());
  };
  CHECK(share(0.5f) > 3.0f * share(8.0f));
}

TEST(scrape_period_buzz) {
  // a grating's bars (4 cm) at 4 m/s: a buzz at 100 Hz and its harmonics (each bar a click) that is not there on plain
  // metal
  auto sound = [](float aPeriod) {
    ScrapeSynth synth(48000.0f, 5);
    ScrapeInputs in;
    in.intensity = 0.6f;
    in.slide = 4.0f;
    in.surface = 2;
    in.period = aPeriod;
    in.periodStrength = 1.0f;
    synth.Set(in);
    std::vector<float> out(48000);
    synth.Render(out.data(), 48000);
    return std::pair{out, synth.Spread()};
  };
  const auto [grating, spread] = sound(0.04f);
  const auto plain = sound(0.0f).first;
  auto buzz = [](const std::vector<float> &aX) {
    return BandPower(aX, 100.0f) + BandPower(aX, 200.0f) + BandPower(aX, 300.0f);
  };
  CHECK(buzz(grating) > 4.0f * buzz(plain));
}

TEST(scrape_synth_low_pass) {
  // the scrape's top octave is turned down: glass, the brightest, has far less at 12 kHz than at 3 kHz
  ScrapeSynth synth(48000.0f, 5);
  ScrapeInputs in;
  in.intensity = 0.6f;
  in.slide = 8.0f;
  in.surface = 3;
  synth.Set(in);
  std::vector<float> out(48000);
  synth.Render(out.data(), 48000);
  CHECK(BandPower(out, 12000.0f) < 0.05f * BandPower(out, 3000.0f));
}
