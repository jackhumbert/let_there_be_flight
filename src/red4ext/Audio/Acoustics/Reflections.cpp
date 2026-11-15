#include "Reflections.hpp"

#include "Images.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <utility>

namespace Acoustics {

namespace {

constexpr float kSpeedOfSound = 343.0f;
constexpr float kFdnMs[8] = {29.7f, 37.1f, 41.1f, 43.7f, 53.3f, 59.9f, 67.1f, 73.3f};
// the small network: an underpass, a narrow street (mutually prime-ish, a quarter of the large one's)
constexpr float kFdnSmallMs[8] = {7.3f, 9.1f, 10.7f, 11.9f, 13.7f, 15.1f, 17.3f, 19.9f};
// the quarters' axes (Probe::Quarters' order)
constexpr Vec3 kQuarterAxis[kQuarters] = {{1, 0, 0}, {0, 1, 0}, {-1, 0, 0}, {0, -1, 0}};
// the probe's caps: a miss counts as a surface this far away
constexpr float kDistanceCap = 250.0f;
constexpr float kFreePathCap = 200.0f;
// the FDN's decay is worked out again every this many samples
constexpr int kFdnGainSamples = 64;
// longest echo: two capped distances and back, with room for the smear
constexpr float kMaxDelaySeconds = 4 * kDistanceCap / kSpeedOfSound;

float Clamp01(float x) {
  return std::clamp(x, 0.0f, 1.0f);
}

} // namespace

Params Targets(const Features &aFeatures, const Vec3 &aSource, const Vec3 &aListener, const Tuning &aTuning,
               const Vec3 &aVelocity) {
  auto capped = [](float d, float cap) { return std::isinf(d) ? cap : std::min(d, cap); };
  // each surface seen square on from the source, an echo off it for the listener
  auto echo = [&](float aDistance, const Vec3 &aOutward, const Bands &aReflectivity, Vec3 &aImage) {
    const Plane surface = SquareOn(aSource, aOutward, aDistance);
    aImage = Mirror(aSource, surface);
    Echo e = EchoFor(surface, aSource, aListener, aVelocity, aReflectivity.mid, aTuning, aDistance < aTuning.range);
    if (aReflectivity.mid > 0) {
      e.tilt[0] = aReflectivity.low / aReflectivity.mid;
      e.tilt[2] = aReflectivity.high / aReflectivity.mid;
    }
    return e;
  };
  // outward normals: the probe's or a fitted plane's (a hand-made Features without them is square on)
  auto outward = [](const Vec3 &aNormal, const Vec3 &aFallback) {
    return aNormal.x * aNormal.x + aNormal.y * aNormal.y + aNormal.z * aNormal.z > 0.5f ? aNormal : aFallback;
  };
  const Vec3 &r = aFeatures.rightDir;
  const Vec3 leftOut = outward(aFeatures.leftNormal, {-r.x, -r.y, -r.z}), rightOut = outward(aFeatures.rightNormal, r);
  const float floor = capped(aFeatures.floor, kDistanceCap), ceiling = capped(aFeatures.ceiling, kDistanceCap);
  const float left = capped(aFeatures.left, kDistanceCap), right = capped(aFeatures.right, kDistanceCap);
  // a surface's own reflectivity where its material is known (live hits), else the defaults, flat
  auto known = [](const Bands &aBands, float aDefault) { return aBands.mid >= 0 ? aBands : Bands{aDefault}; };
  const Bands bFloor = known(aFeatures.floorReflectivity, aTuning.groundReflectivity);
  const Bands bCeiling = known(aFeatures.ceilingReflectivity, aTuning.wallReflectivity);
  const Bands bLeft = known(aFeatures.leftReflectivity, aTuning.wallReflectivity);
  const Bands bRight = known(aFeatures.rightReflectivity, aTuning.wallReflectivity);
  const float rFloor = bFloor.mid, rCeiling = bCeiling.mid, rLeft = bLeft.mid, rRight = bRight.mid;
  Params p;
  p.floor = echo(floor, aFeatures.floorNormal, bFloor, p.floorImage);
  p.ceiling = echo(ceiling, aFeatures.ceilingNormal, bCeiling, p.ceilingImage);
  p.left = echo(left, leftOut, bLeft, p.leftImage);
  p.right = echo(right, rightOut, bRight, p.rightImage);
  // back along every other fan ray that hit: the patch of wall it found, seen from the car
  Vec3 sideSum[2]{};
  float sideGain[2]{};
  for (int k = 2; k < kNearRays; ++k) {
    const float d = aFeatures.nearDistance[k];
    if (k == aFeatures.leftRay || k == aFeatures.rightRay || !(d < aTuning.range)) {
      continue;
    }
    Vec3 image;
    p.scatter[k] = echo(d, aFeatures.nearDir[k], known(aFeatures.nearReflectivity[k], aTuning.wallReflectivity), image);
    p.scatter[k].gain *= aTuning.scatter;
    // a side's image: its echoes' images weighted by gain
    const int side = k < 2 + kFanRays ? 0 : 1;
    const float g = p.scatter[k].gain;
    sideSum[side] = {sideSum[side].x + g * image.x, sideSum[side].y + g * image.y, sideSum[side].z + g * image.z};
    sideGain[side] += g;
  }
  auto sideImage = [&](int aSide) {
    const float g = sideGain[aSide];
    return g > 0 ? Vec3{sideSum[aSide].x / g, sideSum[aSide].y / g, sideSum[aSide].z / g} : aSource;
  };
  p.scatterRightImage = sideImage(0);
  p.scatterLeftImage = sideImage(1);
  // flutter only where both sides are there
  auto flutter = [&](float aApart) {
    return Clamp01((aTuning.flutterFar - aApart) / (aTuning.flutterFar - aTuning.flutterNear));
  };
  p.floorFeedback = aTuning.flutterFeedback * std::sqrt(rFloor * rCeiling) * flutter(floor + ceiling);
  // the square root of the walls' mean reflectivity, as tuned (render.py: sqrt of the wall default)
  p.wallFeedback = aTuning.flutterFeedback * std::sqrt(std::sqrt(rLeft * rRight)) * flutter(left + right);
  // Eyring with mean free path = 4V/S: open sky absorbs everything. Two bands: the surfaces' lows (and mids) and
  // highs from the materials the near rays hit (their tilt against the mid band; 1 unknown), the highs also lost to
  // the air along the path (Sabine's 4mV: m l against -ln(1 - alpha))
  const float enclosure = Clamp01(aFeatures.enclosure);
  const float freePath = capped(aFeatures.meanFreePath, kFreePathCap);
  const float escape = 1 - enclosure;
  double lowTilt = 0, highTilt = 0;
  int tilts = 0;
  auto addTilt = [&](const Bands &aBands) {
    if (aBands.mid > 0) {
      lowTilt += aBands.low / aBands.mid;
      highTilt += aBands.high / aBands.mid;
      ++tilts;
    }
  };
  for (const Bands *b : {&aFeatures.floorReflectivity, &aFeatures.ceilingReflectivity, &aFeatures.leftReflectivity,
                         &aFeatures.rightReflectivity}) {
    addTilt(*b);
  }
  for (int k = 2; k < kNearRays; ++k) {
    addTilt(aFeatures.nearReflectivity[k]);
  }
  auto absorption = [&](double aTilt) {
    const float reflect = std::clamp(float((1 - aTuning.wallAbsorption) * aTilt), 0.0f, 0.99f);
    return std::min(escape + (1 - escape) * (1 - reflect), 0.99f);
  };
  const float alpha = absorption(tilts ? lowTilt / tilts : 1.0);
  const float alphaHigh = absorption(tilts ? highTilt / tilts : 1.0);
  p.rt60 = std::clamp(0.04f * freePath / -std::log(1 - alpha), aTuning.rt60Min, aTuning.rt60Max);
  const float airLoss = aTuning.airAbsorption ? aTuning.highAir * freePath : 0.0f;
  p.rt60High = std::clamp(0.04f * freePath / (-std::log(1 - alphaHigh) + airLoss), aTuning.rt60Min, p.rt60);
  p.size = Clamp01((freePath - aTuning.smallFreePath) / (aTuning.largeFreePath - aTuning.smallFreePath));
  // the tail by quarter, as enclosed as each is (equal with nothing anywhere)
  float shares = 0;
  for (const Quarter &q : aFeatures.quarters) {
    shares += q.share;
  }
  for (int q = 0; q < kQuarters; ++q) {
    const float w = shares > 0 ? aFeatures.quarters[q].share / shares : 1.0f / kQuarters;
    p.tail[q] = 2 * std::sqrt(w);
    // the quarter's facade: an echo off it, seen square on; with none, a point that way for its tail
    const Quarter &quarter = aFeatures.quarters[q];
    const Vec3 &axis = kQuarterAxis[q];
    p.quarterImage[q] = {aSource.x + 30 * axis.x, aSource.y + 30 * axis.y, aSource.z};
    if (!std::isinf(quarter.farDistance) && quarter.farCoherence > 0) {
      p.distant[q] = echo(quarter.farDistance, quarter.farDir, Bands{aTuning.wallReflectivity}, p.quarterImage[q]);
      p.distant[q].gain *= aTuning.distant * quarter.farCoherence;
    }
  }
  // the lower half of the sphere is mostly ground: the reverb rises from a little above that
  p.wet = aTuning.wet * aTuning.gain * Clamp01((enclosure - aTuning.wetFrom) / (1 - aTuning.wetFrom));
  p.air = aTuning.airAbsorption;
  return p;
}

void Reflections::Ramp::To(double aTarget, int aSamples) {
  left = std::max(aSamples, 1);
  step = (aTarget - value) / double(left);
}

double Reflections::Ramp::Next() {
  if (left > 0) {
    value += step;
    --left;
  }
  return value;
}

void Reflections::Line::Resize(uint32_t aMinSize) {
  uint32_t size = 1;
  while (size < aMinSize) {
    size <<= 1;
  }
  buffer.assign(size, 0.0f);
  mask = size - 1;
}

float Reflections::Line::Read(uint32_t aWrite, double aDelay) const {
  // aWrite - aDelay = i + f, kept in integers so a long run loses no precision
  const uint32_t whole = uint32_t(aDelay);
  const float f = float(1.0 - (aDelay - double(whole)));
  const uint32_t i = aWrite - whole - 1;
  auto at = [&](uint32_t k) { return buffer[(i + k) & mask] * (1 - f) + buffer[(i + k + 1) & mask] * f; };
  return 0.5f * at(0) + 0.25f * at(uint32_t(-1)) + 0.25f * at(1);
}

float Reflections::Line::ReadLinear(uint32_t aWrite, double aDelay) const {
  const uint32_t whole = uint32_t(aDelay);
  const float f = float(1.0 - (aDelay - double(whole)));
  const uint32_t i = aWrite - whole - 1;
  return buffer[i & mask] * (1 - f) + buffer[(i + 1) & mask] * f;
}

void Reflections::Voice::Set(double aDelay, double aRate, float aGain, int aInterval, const Reflections &aOwner) {
  Tap &a = taps[active];
  if (!started || a.fade <= 0 || gain.value < 1e-5) {
    // the first, or nothing audible to move from: at once
    taps[active] = {aDelay, aRate, 1.0f, 0.0f};
    taps[1 - active] = {};
    gain.To(aGain, started ? aInterval : 0);
    if (!started) {
      gain.Next();
      started = true;
    }
    return;
  }
  gain.To(aGain, aInterval);
  const double jump = aOwner.tuning.jumpSeconds * aOwner.sampleRate + std::abs(aRate) * aInterval;
  if (std::abs(aDelay - a.delay) > jump) {
    // another reflection: the other tap fades in at it, this one fades out where it was going
    const float step = 1.0f / std::max(1.0f, aOwner.tuning.fadeSeconds * aOwner.sampleRate);
    a.fadeStep = -step;
    active = 1 - active;
    taps[active] = {aDelay, aRate, 0.0f, step};
    return;
  }
  // at its rate, steered to be where the target will be at the end of the interval
  a.rate = (aDelay + aRate * aInterval - a.delay) / double(aInterval);
}

void Reflections::Voice::Step(double aMaxRate, double aMinDelay) {
  level = float(gain.Next());
  for (Tap &t : taps) {
    if (t.fade <= 0 && t.fadeStep <= 0) {
      continue;
    }
    t.delay = std::max(aMinDelay, t.delay + std::clamp(t.rate, -aMaxRate, aMaxRate));
    if (t.fadeStep != 0) {
      t.fade = std::clamp(t.fade + t.fadeStep, 0.0f, 1.0f);
      if (t.fade == 0.0f || t.fade == 1.0f) {
        t.fadeStep = 0;
      }
    }
  }
}

BandSplit::BandSplit(float aRate)
    : lowCoefficient(1.0f - std::exp(-2.0f * std::numbers::pi_v<float> * 250.0f / aRate)),
      highCoefficient(1.0f - std::exp(-2.0f * std::numbers::pi_v<float> * 2500.0f / aRate)) {}

void BandSplit::Set(const float aTilt[3]) {
  flat = true;
  for (int i = 0; i < 3; ++i) {
    tilt[i] = aTilt[i];
    flat = flat && aTilt[i] == 1.0f;
  }
}

float BandSplit::Run(float x) {
  if (flat) {
    return x;
  }
  lowState += lowCoefficient * (x - lowState);
  highState += highCoefficient * (x - highState);
  const float low = lowState, high = x - highState, mid = x - low - high;
  return tilt[0] * low + tilt[1] * mid + tilt[2] * high;
}

float Reflections::Voice::Filter(float x) {
  if (air < 1.0f) {
    airState += air * (x - airState);
    x = airState;
  }
  return bands.Run(x);
}

float Reflections::Voice::Read(const Line &aLine, uint32_t aWrite, bool aWithGain, bool aSmear) const {
  float sum = 0;
  for (const Tap &t : taps) {
    if (t.fade > 0) {
      sum += t.fade * (aSmear ? aLine.Read(aWrite, t.delay) : aLine.ReadLinear(aWrite, t.delay));
    }
  }
  return aWithGain ? level * sum : sum;
}

Reflections::Reflections(float aSampleRate, int aRampSamples, const Tuning &aTuning)
    : sampleRate(aSampleRate), rampSamples(aRampSamples), tuning(aTuning) {
  const uint32_t echoSize = uint32_t(kMaxDelaySeconds * sampleRate) + 4;
  for (Line *line : {&source, &floor, &ceiling, &left, &right}) {
    line->Resize(echoSize);
  }
  for (int j = 0; j < 8; ++j) {
    for (auto [net, ms] : {std::pair{&small, kFdnSmallMs[j]}, std::pair{&large, kFdnMs[j]}}) {
      net->delay[j] = uint32_t(std::lround(ms * 1e-3f * sampleRate));
      net->lines[j].Resize(net->delay[j] + 1);
    }
  }
  Reset();
}

void Reflections::Reset() {
  for (Line *line : {&source, &floor, &ceiling, &left, &right}) {
    std::fill(line->buffer.begin(), line->buffer.end(), 0.0f);
  }
  for (Fdn *net : {&small, &large}) {
    for (auto &line : net->lines) {
      std::fill(line.buffer.begin(), line.buffer.end(), 0.0f);
    }
    net->state = {};
  }
  started = false;
  for (Voice *v : {&floorVoice, &ceilingVoice, &leftVoice, &rightVoice}) {
    *v = {};
    v->bands = BandSplit(sampleRate);
  }
  for (Voice &v : scatterVoices) {
    v = {};
    v.bands = BandSplit(sampleRate);
  }
  for (Voice &v : farVoices) {
    v = {};
    v.bands = BandSplit(sampleRate);
  }
  fdnGainAge = 0;
  energy = {};
  tail = 0;
}

void Reflections::SetTargets(const Params &aParams) {
  // a target not a number would poison the lines: refused, and which one kept (Engine::State)
  auto finite = [](const Echo &e) { return std::isfinite(e.delay) && std::isfinite(e.rate) && std::isfinite(e.gain); };
  const char *bad = !finite(aParams.floor) ? "floor" : !finite(aParams.ceiling) ? "ceiling"
                    : !finite(aParams.left) ? "left" : !finite(aParams.right) ? "right" : nullptr;
  for (int k = 2; !bad && k < kNearRays; ++k) {
    bad = finite(aParams.scatter[k]) ? nullptr : "scatter";
  }
  for (int q = 0; !bad && q < kQuarters; ++q) {
    bad = !finite(aParams.distant[q]) ? "distant" : !std::isfinite(aParams.tail[q]) ? "tail" : nullptr;
  }
  if (!bad) {
    bad = !std::isfinite(aParams.floorFeedback) || !std::isfinite(aParams.wallFeedback) ? "feedback"
          : !std::isfinite(aParams.rt60) || !std::isfinite(aParams.rt60High) ? "rt60"
          : !std::isfinite(aParams.wet) ? "wet" : !std::isfinite(aParams.size) ? "size" : nullptr;
  }
  if (bad) {
    ++badParams;
    badParam = bad;
    return;
  }
  // delays in samples
  auto delay = [&](const Echo &e) { return double(std::max(e.delay, tuning.minDelay)) * sampleRate; };
  const int n = started ? rampSamples : 0;
  // air absorption, air and the surfaces together: about 16 kHz near, 8 kHz by a 60 m path, 4 kHz by 180 m
  auto air = [&](const Echo &e) {
    if (!aParams.air) {
      return 1.0f;
    }
    const float path = e.delay * kSpeedOfSound + tuning.nearDistance;
    const float cutoff = 16000.0f / (1.0f + path / 60.0f);
    return 1.0f - std::exp(-2.0f * std::numbers::pi_v<float> * cutoff / sampleRate);
  };
  auto set = [&](Voice &v, const Echo &e) {
    v.Set(delay(e), e.rate, e.gain, rampSamples, *this);
    v.air = air(e);
    v.bands.Set(e.tilt);
  };
  set(floorVoice, aParams.floor);
  set(ceilingVoice, aParams.ceiling);
  set(leftVoice, aParams.left);
  set(rightVoice, aParams.right);
  for (int k = 2; k < kNearRays; ++k) {
    set(scatterVoices[k], aParams.scatter[k]);
  }
  for (int q = 0; q < kQuarters; ++q) {
    set(farVoices[q], aParams.distant[q]);
    tailRamps[q].To(aParams.tail[q], n);
  }
  floorFeedback.To(aParams.floorFeedback, n);
  wallFeedback.To(aParams.wallFeedback, n);
  rt60.To(aParams.rt60, n);
  rt60High.To(aParams.rt60High, n);
  wet.To(aParams.wet, n);
  size.To(aParams.size, n);
  if (!started) {
    // the first set takes effect at once
    for (Ramp *r : {&floorFeedback, &wallFeedback, &rt60, &rt60High, &wet, &size}) {
      r->Next();
    }
    for (Ramp &r : tailRamps) {
      r.Next();
    }
    started = true;
  }
}

Reflections::Sample Reflections::Tick(float x) {
  source.buffer[write & source.mask] = x;
  float a = 0, b = 0, l = 0, r = 0, sl = 0, sr = 0, revL = 0, revR = 0;
  const double maxRate = tuning.maxDelayRate, minDelay = double(tuning.minDelay) * sampleRate;
  for (Voice *v : {&floorVoice, &ceilingVoice, &leftVoice, &rightVoice}) {
    v->Step(maxRate, minDelay);
  }
  const float ffb = float(floorFeedback.Next()), wfb = float(wallFeedback.Next());
  const float decay = float(rt60.Next()), decayHigh = float(rt60High.Next()), send = float(wet.Next());
  // clamped: a ramp to 0 can end a rounding below it, and the large network's share is its square root (a NaN there
  // filled the networks and silenced the Engine bus driving into a tunnel)
  const float large01 = std::clamp(float(size.Next()), 0.0f, 1.0f);
  std::array<float, kQuarters> distant{}, tailOut{}, tailAmp{};
  for (int q = 0; q < kQuarters; ++q) {
    tailAmp[q] = float(tailRamps[q].Next());
  }
  if (started) {
    // each side's echo, and the other side's echo bounced back off it; the floor's read unsmeared, so its echo and
    // the direct sound comb as they do near the ground
    a = floorVoice.Filter(floorVoice.Read(source, write, true, false) + ffb * floorVoice.Read(ceiling, write, false, true));
    b = ceilingVoice.Filter(ceilingVoice.Read(source, write, true, true) + ffb * ceilingVoice.Read(floor, write, false, true));
    l = leftVoice.Filter(leftVoice.Read(source, write, true, true) + wfb * leftVoice.Read(right, write, false, true));
    r = rightVoice.Filter(rightVoice.Read(source, write, true, true) + wfb * rightVoice.Read(left, write, false, true));
    // the scattered echoes, each on its fan's side
    for (int k = 2; k < kNearRays; ++k) {
      Voice &v = scatterVoices[k];
      v.Step(maxRate, minDelay);
      if (v.level > 1e-6f) {
        (k < 2 + kFanRays ? sr : sl) += v.Filter(v.Read(source, write, true, false));
      }
    }

    // the distant echoes, one a quarter
    for (int q = 0; q < kQuarters; ++q) {
      Voice &v = farVoices[q];
      v.Step(maxRate, minDelay);
      if (v.level > 1e-6f) {
        distant[q] = v.Filter(v.Read(source, write, true, true));
      }
    }

    if (fdnGainAge-- <= 0) {
      fdnGainAge = kFdnGainSamples - 1;
      // each line's loop: the lows' and the highs' gain over its delay, as a one-pole between them
      const float t = std::max(decay, 0.05f) * sampleRate;
      const float tHigh = std::max(std::min(decayHigh, decay), 0.05f) * sampleRate;
      for (Fdn *net : {&small, &large}) {
        for (int j = 0; j < 8; ++j) {
          const float low = std::pow(10.0f, -3.0f * float(net->delay[j]) / t);
          const float high = std::pow(10.0f, -3.0f * float(net->delay[j]) / tHigh);
          net->low[j] = low;
          net->pole[j] = (low - high) / (low + high);
        }
      }
    }
    // the input shared between the two networks by the space's size, at equal power
    const float input = x * send / std::sqrt(8.0f);
    std::array<float, 8> lines{};
    for (auto [net, share] : {std::pair{&small, std::sqrt(1 - large01)}, std::pair{&large, std::sqrt(large01)}}) {
      float back[8], sum = 0;
      for (int j = 0; j < 8; ++j) {
        back[j] = net->lines[j].buffer[(write - net->delay[j]) & net->lines[j].mask];
        sum += back[j];
      }
      for (int j = 0; j < 8; ++j) {
        // Householder: v - (2 / 8) sum(v), through the loop filter, then the input
        const float loop = back[j] - 0.25f * sum;
        net->state[j] = net->pole[j] * net->state[j] + net->low[j] * (1 - net->pole[j]) * loop;
        const float v = net->state[j] + share * input;
        net->lines[j].buffer[write & net->lines[j].mask] = v;
        lines[j] += v;
      }
    }
    for (int j = 0; j < 8; ++j) {
      (j % 2 == 0 ? revL : revR) += 0.5f * lines[j];
    }
    // each quarter's tail: two of the lines, as enclosed as the quarter is. The lines all take the same input, so the
    // old bed's four-line sums added partly in phase: sqrt(2) a pair keeps the tail as loud (reflections_tests)
    for (int q = 0; q < kQuarters; ++q) {
      tailOut[q] = tailAmp[q] * 0.70710678f * (lines[q] + lines[q + 4]);
    }
  }
  floor.buffer[write & floor.mask] = a;
  ceiling.buffer[write & ceiling.mask] = b;
  left.buffer[write & left.mask] = l;
  right.buffer[write & right.mask] = r;
  ++write;

  energy.dry += double(x) * x;
  energy.floor += double(a) * a;
  energy.ceiling += double(b) * b;
  energy.walls += double(l + r) * (l + r);
  energy.reverb += double(revL + revR) * (revL + revR);
  energy.scatter += double(sl + sr) * (sl + sr);
  float farSum = 0;
  for (float v : distant) {
    farSum += v;
  }
  energy.distant += double(farSum) * farSum;
  const float wetNow = a + b + l + r + sl + sr + revL + revR + farSum;
  if (!std::isfinite(wetNow)) {
    // anything not a number would stay in the delay lines and silence the bus: start again from silence, keeping
    // which part went first
    ++resets;
    resetPart = !std::isfinite(a + b) ? "floor or ceiling" : !std::isfinite(l + r) ? "walls"
                : !std::isfinite(sl + sr) ? "scatter" : !std::isfinite(farSum) ? "distant" : "reverb";
    Reset();
    return {};
  }
  tail += (wetNow * wetNow - tail) * (1.0f - std::exp(-1.0f / (0.05f * sampleRate)));
  Sample out{a, b, l, r, sl, sr, revL, revR};
  out.distant = distant;
  out.tail = tailOut;
  return out;
}

void Reflections::Process(const float *aIn, float *aOut, uint32_t aFrames, int aChannels) {
  for (uint32_t s = 0; s < aFrames; ++s) {
    const float *in = aIn + size_t(s) * aChannels;
    float *out = aOut + size_t(s) * aChannels;
    float x = 0;
    for (int c = 0; c < aChannels; ++c) {
      x += in[c];
    }
    x /= float(aChannels);
    const Sample p = Tick(x);
    float farSum = 0;
    for (float v : p.distant) {
      farSum += v;
    }
    const float mid = p.floor + p.ceiling + farSum, l = p.left, r = p.right, sl = p.scatterLeft, sr = p.scatterRight;
    if (aChannels >= 2) {
      out[0] = in[0] + mid + tuning.pan * (l + sl) + tuning.panOpposite * (r + sr) + p.reverbLeft;
      out[1] = in[1] + mid + tuning.panOpposite * (l + sl) + tuning.pan * (r + sr) + p.reverbRight;
      for (int c = 2; c < aChannels; ++c) {
        out[c] = in[c] + mid + 0.5f * (l + r + sl + sr) + 0.5f * (p.reverbLeft + p.reverbRight);
      }
    } else {
      out[0] = in[0] + mid + 0.5f * (tuning.pan + tuning.panOpposite) * (l + r + sl + sr) +
               0.5f * (p.reverbLeft + p.reverbRight);
    }
  }
}

void Reflections::ProcessStreams(const float *aDryMono, const Streams &aStreams, uint32_t aFrames) {
  for (uint32_t s = 0; s < aFrames; ++s) {
    const Sample p = Tick(aDryMono[s]);
    aStreams.floor[s] = p.floor;
    aStreams.ceiling[s] = p.ceiling;
    aStreams.left[s] = p.left;
    aStreams.right[s] = p.right;
    aStreams.scatterRight[s] = p.scatterRight;
    aStreams.scatterLeft[s] = p.scatterLeft;
    aStreams.reverbLeft[s] = p.reverbLeft;
    aStreams.reverbRight[s] = p.reverbRight;
    for (int q = 0; q < kQuarters; ++q) {
      if (aStreams.quarter[q]) {
        aStreams.quarter[q][s] = p.distant[q] + p.tail[q];
      }
    }
  }
}

} // namespace Acoustics
