// Reflectivity by band (src/red4ext/Audio/Acoustics/Materials, BandSplit in Reflections): the table, the
// mid band unchanged from the single value it replaced, the band filter on sines, and the air absorbing
// a long echo's highs more than a short one's. Levels are RMS over the last 0.5 s, so the filters are settled.
#include "check.hpp"

#include "Audio/Acoustics/Materials.hpp"
#include "Audio/Acoustics/Reflections.hpp"

#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

using namespace Acoustics;

namespace {

constexpr float kRate = 48000.0f;
constexpr float kPi = 3.14159265f;

bool Is(const std::optional<Bands> &aActual, float aLow, float aMid, float aHigh) {
  return aActual && std::fabs(aActual->low - aLow) < 1e-6f && std::fabs(aActual->mid - aMid) < 1e-6f &&
         std::fabs(aActual->high - aHigh) < 1e-6f;
}

std::vector<float> Sine(float aHz, int aSamples) {
  std::vector<float> x(aSamples);
  for (int i = 0; i < aSamples; ++i) {
    x[i] = std::sin(2.0f * kPi * aHz * float(i) / kRate);
  }
  return x;
}

// RMS of the last aSeconds of aX
double TailRms(const std::vector<float> &aX, float aSeconds) {
  const size_t n = size_t(aSeconds * kRate), from = aX.size() - n;
  double sum = 0;
  for (size_t i = from; i < aX.size(); ++i) {
    sum += double(aX[i]) * aX[i];
  }
  return std::sqrt(sum / double(n));
}

double Db(double aRatio) {
  return 20.0 * std::log10(aRatio);
}

// a sine through the split with this tilt, its level out against in
double BandDb(float aHz, float a0, float a1, float a2) {
  BandSplit split(kRate);
  const float tilt[3] = {a0, a1, a2};
  split.Set(tilt);
  std::vector<float> in = Sine(aHz, int(kRate)), out(in.size());
  for (size_t i = 0; i < in.size(); ++i) {
    out[i] = split.Run(in[i]);
  }
  return Db(TailRms(out, 0.5f) / TailRms(in, 0.5f));
}

// the floor echo's RMS (dB re 1) of an 8 kHz sine with the air on, aDelay s behind the direct sound
double AirEchoDb(float aDelay) {
  Tuning tuning;
  tuning.airAbsorption = true;
  Reflections reflections(kRate, 480, tuning);
  Params p;
  p.floor.delay = aDelay;
  p.floor.gain = 0.5f;
  p.air = true;
  reflections.SetTargets(p);
  const int total = int(2 * kRate);
  const std::vector<float> in = Sine(8000.0f, total);
  std::vector<float> floor(total), ceiling(total), left(total), right(total), sr(total), sl(total), rl(total), rr(total);
  reflections.ProcessStreams(in.data(), {floor.data(), ceiling.data(), left.data(), right.data(), sr.data(), sl.data(), rl.data(), rr.data()},
                             uint32_t(total));
  return Db(TailRms(floor, 0.5f));
}

} // namespace

TEST(materials_table) {
  CHECK(Is(Materials::Find("concrete_rough.physmat"), 0.90f, 0.85f, 0.80f));
  CHECK(Is(Materials::Find("glass_window.physmat"), 0.80f, 0.95f, 0.95f));
  CHECK(!Materials::Find("xyz.physmat").has_value());
}

TEST(materials_mid_unchanged) {
  const struct {
    const char *name;
    float mid;
  } old[] = {{"glass", 0.95f}, {"metal", 0.9f},   {"concrete", 0.85f}, {"stone", 0.85f},    {"brick", 0.85f},
             {"asphalt", 0.75f}, {"plastic", 0.7f}, {"wood", 0.6f},     {"water", 0.8f},     {"dirt", 0.45f},
             {"mud", 0.4f},    {"sand", 0.35f},   {"trash", 0.3f},      {"cardboard", 0.3f}, {"grass", 0.3f},
             {"leaves", 0.2f}, {"foliage", 0.2f}, {"fabric", 0.2f},     {"upholstery", 0.2f}};
  for (const auto &entry : old) {
    const auto bands = Materials::Find(std::string(entry.name) + "_x.physmat");
    CHECK(bands.has_value());
    if (bands) {
      CHECK_NEAR(bands->mid, entry.mid, 1e-6);
    }
  }
}

TEST(materials_band_identity) {
  BandSplit split(kRate);
  const float tilt[3] = {1, 1, 1};
  split.Set(tilt);
  uint32_t state = 12345;
  for (int i = 0; i < 4800; ++i) {
    state = state * 1664525u + 1013904223u;
    const float x = float(state >> 8) / 8388608.0f - 1.0f;
    CHECK_NEAR(split.Run(x), x, 1e-6);
  }
}

TEST(materials_band_low_only) {
  // one pole at 250 Hz: -0.2 dB at 50 Hz, about -30 dB at 8 kHz
  CHECK(BandDb(50.0f, 1, 0, 0) > -1.0);
  CHECK(BandDb(8000.0f, 1, 0, 0) < -20.0);
}

TEST(materials_band_high_only) {
  // x minus the 2.5 kHz one pole: -1.9 dB at 8 kHz, about -34 dB at 50 Hz
  CHECK(BandDb(8000.0f, 0, 0, 1) > -2.5);
  CHECK(BandDb(50.0f, 0, 0, 1) < -20.0);
}

TEST(materials_air_absorption_by_path) {
  // paths of 6.4 and 208.8 m: the one-pole cutoffs 14.6 and 3.6 kHz leave -0.8 and -7.3 dB at 8 kHz
  const double shortEcho = AirEchoDb(0.01f), longEcho = AirEchoDb(0.6f);
  std::printf("  materials_air_absorption_by_path: short %.2f dB, long %.2f dB, difference %.2f dB\n", shortEcho, longEcho,
              shortEcho - longEcho);
  CHECK(shortEcho - longEcho >= 4.0);
}
