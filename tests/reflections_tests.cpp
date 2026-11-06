// The space round the car (src/red4ext/Audio/Acoustics/Reflections): the late reverb's two decay times (the highs
// faster, from the materials and the air), its size, its directions by quarter of the horizon, the distant echoes, and
// the floor's echo short enough to comb with the car low.
#include "check.hpp"

#include "Audio/Acoustics/Reflections.hpp"

#include <cmath>
#include <vector>

using namespace Acoustics;

namespace {
// a street: walls at 8 m either side, the ground 3 m down, open above; enclosed 0.7, mean free path 20 m
Features Street() {
  Features f{};
  f.floor = 3;
  f.ceiling = std::numeric_limits<float>::infinity();
  f.left = f.right = 8;
  f.rightDir = {1, 0, 0};
  f.enclosure = 0.7f;
  f.meanFreePath = 20;
  for (Quarter &q : f.quarters) {
    q.share = 0.5f;
  }
  return f;
}

// energy of x above and below about 2 kHz (one-pole split), over samples [aFrom, aTo)
void BandEnergy(const std::vector<float> &x, size_t aFrom, size_t aTo, float aRate, double &aLow, double &aHigh) {
  const float k = 1.0f - std::exp(-2.0f * 3.14159265f * 2000.0f / aRate);
  float state = 0;
  aLow = aHigh = 0;
  for (size_t i = 0; i < aTo && i < x.size(); ++i) {
    state += k * (x[i] - state);
    if (i >= aFrom) {
      aLow += double(state) * state;
      aHigh += double(x[i] - state) * (x[i] - state);
    }
  }
}

// the reverb's two channels after an impulse, aSeconds long, with these Params
std::vector<float> ReverbAfterImpulse(const Params &aParams, float aSeconds, float aRate = 48000.0f) {
  Reflections r(aRate, 960);
  r.SetTargets(aParams);
  const uint32_t n = uint32_t(aSeconds * aRate);
  std::vector<float> dry(n, 0.0f), out(n);
  dry[0] = 1.0f;
  std::vector<float> s[8];
  for (auto &v : s) {
    v.assign(n, 0.0f);
  }
  std::vector<float> q[kQuarters];
  for (auto &v : q) {
    v.assign(n, 0.0f);
  }
  r.ProcessStreams(dry.data(), {s[0].data(), s[1].data(), s[2].data(), s[3].data(), s[4].data(), s[5].data(),
                                s[6].data(), s[7].data(), {q[0].data(), q[1].data(), q[2].data(), q[3].data()}},
                   n);
  for (uint32_t i = 0; i < n; ++i) {
    out[i] = s[6][i] + s[7][i];
  }
  return out;
}

Params ReverbOnly(float aRt60, float aRt60High, float aSize) {
  Params p;
  p.rt60 = aRt60;
  p.rt60High = aRt60High;
  p.wet = 1.0f;
  p.size = aSize;
  return p;
}
} // namespace

TEST(reflections_two_decay_times) {
  const Tuning t;
  Features f = Street();
  // materials unknown: the highs decay faster only through the air
  Params p = Targets(f, {0, 0, 0}, {0, 0, 0}, t);
  std::printf("  street: RT60 %.2f s, highs %.2f s\n", p.rt60, p.rt60High);
  CHECK(p.rt60High < p.rt60);
  Tuning noAir;
  noAir.airAbsorption = false;
  Params flat = Targets(f, {0, 0, 0}, {0, 0, 0}, noAir);
  CHECK_NEAR(flat.rt60High, flat.rt60, 1e-5);
  // surfaces that swallow the highs (a high band half the mid's): shorter still
  for (auto *b : {&f.floorReflectivity, &f.leftReflectivity, &f.rightReflectivity}) {
    *b = Bands{0.8f, 0.8f, 0.4f};
  }
  Params dull = Targets(f, {0, 0, 0}, {0, 0, 0}, noAir);
  std::printf("  dull walls: RT60 %.2f s, highs %.2f s\n", dull.rt60, dull.rt60High);
  CHECK(dull.rt60High < 0.8f * dull.rt60);
  CHECK(dull.rt60High >= t.rt60Min && dull.rt60 <= t.rt60Max);
}

TEST(reflections_size_by_free_path) {
  const Tuning t;
  Features f = Street();
  f.meanFreePath = 5;
  CHECK_NEAR(Targets(f, {}, {}, t).size, 0.0, 1e-6);
  f.meanFreePath = 40;
  CHECK_NEAR(Targets(f, {}, {}, t).size, 1.0, 1e-6);
  f.meanFreePath = 0.5f * (t.smallFreePath + t.largeFreePath);
  CHECK_NEAR(Targets(f, {}, {}, t).size, 0.5, 1e-5);
}

TEST(reflections_tail_by_quarter) {
  const Tuning t;
  Features f = Street();
  // even: each quarter the same, their squares adding up to the old stereo bed's 4
  Params p = Targets(f, {}, {}, t);
  float squares = 0;
  for (float a : p.tail) {
    CHECK_NEAR(a, 1.0, 1e-5);
    squares += a * a;
  }
  CHECK_NEAR(squares, 4.0, 1e-4);
  // walls only to the +x side: the tail comes from there
  for (Quarter &q : f.quarters) {
    q.share = 0;
  }
  f.quarters[0].share = 0.9f;
  p = Targets(f, {}, {}, t);
  CHECK_NEAR(p.tail[0], 2.0, 1e-5);
  CHECK_NEAR(p.tail[1] + p.tail[2] + p.tail[3], 0.0, 1e-6);
  // nothing anywhere: even again
  f.quarters[0].share = 0;
  p = Targets(f, {}, {}, t);
  CHECK_NEAR(p.tail[2], 1.0, 1e-5);
}

TEST(reflections_distant_echo) {
  const Tuning t;
  Features f = Street();
  f.quarters[0].farDistance = 100;
  f.quarters[0].farDir = {1, 0, 0};
  f.quarters[0].farCoherence = 0.5f;
  const Params p = Targets(f, {0, 0, 0}, {0, 0, 0}, t);
  // there and back: 200 m, 0.58 s
  CHECK_NEAR(p.distant[0].delay, 200.0 / 343.0, 1e-3);
  CHECK(p.distant[0].gain > 0 && p.distant[0].gain < 0.1f);
  CHECK_NEAR(p.quarterImage[0].x, 200.0, 1e-3);
  // no facade: no echo, its tail still heard from that way
  CHECK(p.distant[1].gain == 0);
  CHECK(p.quarterImage[1].y > 0);
}

TEST(reflections_floor_comb_under_a_camera) {
  // the car 1.5 m up, a third person camera 8 m behind and 3 m up: the floor's extra path is about a metre (3 ms),
  // which the old 4 ms floor clamped; the new floor is under it
  const Tuning t;
  Features f = Street();
  f.floor = 1.5f;
  const Params p = Targets(f, {0, 0, 1.5f}, {0, -8, 3}, t);
  std::printf("  floor echo %.2f ms behind the direct sound\n", 1000 * p.floor.delay);
  CHECK(p.floor.delay > t.minDelay && p.floor.delay < 0.004f);
}

TEST(reflections_tail_highs_die_faster) {
  const float rate = 48000.0f;
  for (float size : {0.0f, 1.0f}) {
    double el, eh, ll, lh;
    // the highs at a third of the lows' RT60: their share of the tail falls
    const auto dull = ReverbAfterImpulse(ReverbOnly(1.0f, 0.33f, size), 0.7f, rate);
    BandEnergy(dull, size_t(0.05 * rate), size_t(0.15 * rate), rate, el, eh);
    BandEnergy(dull, size_t(0.45 * rate), size_t(0.65 * rate), rate, ll, lh);
    const double drop = (eh / el) / (lh / ll);
    // one RT60: the share stays about where it was
    const auto even = ReverbAfterImpulse(ReverbOnly(1.0f, 1.0f, size), 0.7f, rate);
    double el2, eh2, ll2, lh2;
    BandEnergy(even, size_t(0.05 * rate), size_t(0.15 * rate), rate, el2, eh2);
    BandEnergy(even, size_t(0.45 * rate), size_t(0.65 * rate), rate, ll2, lh2);
    const double evenDrop = (eh2 / el2) / (lh2 / ll2);
    std::printf("  size %.0f: the highs' share falls %.1f times over 0.4 s (one RT60: %.1f)\n", size, drop, evenDrop);
    // a one-pole loop filter: the RT60 slides from the lows' to the highs' over the band, as the air's absorption does,
    // so a split at 2 kHz sees a few times, not the 48 dB at Nyquist
    CHECK(drop > 2.5);
    CHECK(evenDrop < 1.5);
  }
}

TEST(reflections_quarters_carry_the_bed) {
  // the four quarters' tails together as loud as the old stereo bed (even shares)
  const float rate = 48000.0f;
  Reflections r(rate, 960);
  Params p = ReverbOnly(1.0f, 1.0f, 1.0f);
  r.SetTargets(p);
  const uint32_t n = uint32_t(0.5f * rate);
  std::vector<float> dry(n);
  uint32_t seed = 1;
  for (auto &x : dry) {
    seed = seed * 1664525u + 1013904223u;
    x = float(int32_t(seed)) / 2147483648.0f;
  }
  std::vector<float> s[8], q[kQuarters];
  for (auto &v : s) {
    v.assign(n, 0.0f);
  }
  for (auto &v : q) {
    v.assign(n, 0.0f);
  }
  r.ProcessStreams(dry.data(), {s[0].data(), s[1].data(), s[2].data(), s[3].data(), s[4].data(), s[5].data(),
                                s[6].data(), s[7].data(), {q[0].data(), q[1].data(), q[2].data(), q[3].data()}},
                   n);
  double bed = 0, quarters = 0;
  for (uint32_t i = n / 2; i < n; ++i) {
    bed += double(s[6][i]) * s[6][i] + double(s[7][i]) * s[7][i];
    for (auto &v : q) {
      quarters += double(v[i]) * v[i];
    }
  }
  std::printf("  quarters against the bed: %.2f\n", quarters / bed);
  CHECK(quarters / bed > 0.7 && quarters / bed < 1.4);
}
