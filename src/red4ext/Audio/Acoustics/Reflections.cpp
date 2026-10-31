#include "Reflections.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace Acoustics {

namespace {

constexpr float kSpeedOfSound = 343.0f;
constexpr float kFdnMs[8] = {29.7f, 37.1f, 41.1f, 43.7f, 53.3f, 59.9f, 67.1f, 73.3f};
// the probe's caps: a miss counts as a surface this far away
constexpr float kDistanceCap = 250.0f;
constexpr float kFreePathCap = 200.0f;
// the FDN's decay is worked out again every this many samples
constexpr int kFdnGainSamples = 64;
// longest echo: two capped distances and back, with room for the smear
constexpr float kMaxDelaySeconds = 4 * kDistanceCap / kSpeedOfSound;

float Length(const Vec3 &a, const Vec3 &b) {
  float x = a.x - b.x, y = a.y - b.y, z = a.z - b.z;
  return std::sqrt(x * x + y * y + z * z);
}

float Clamp01(float x) {
  return std::clamp(x, 0.0f, 1.0f);
}

float Dot(const Vec3 &a, const Vec3 &b) {
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

} // namespace

Params Targets(const Features &aFeatures, const Vec3 &aSource, const Vec3 &aListener, const Tuning &aTuning,
               const Vec3 &aVelocity) {
  auto capped = [](float d, float cap) { return std::isinf(d) ? cap : std::min(d, cap); };
  const float direct = Length(aListener, aSource);
  auto echo = [&](float aDistance, Vec3 aNormal, float aReflectivity) {
    Vec3 image{aSource.x + 2 * aDistance * aNormal.x, aSource.y + 2 * aDistance * aNormal.y,
               aSource.z + 2 * aDistance * aNormal.z};
    float via = Length(aListener, image);
    Echo e;
    e.delay = (via - direct) / kSpeedOfSound;
    // the car and listener moving at v past a still surface: the image moves at v - 2 (v.n) n, so
    // listener minus image changes at 2 (v.n) n; the direct path stays as it is
    const Vec3 toListener{aListener.x - image.x, aListener.y - image.y, aListener.z - image.z};
    e.rate = via > 1e-3f ? 2 * Dot(aVelocity, aNormal) * Dot(toListener, aNormal) / via / kSpeedOfSound : 0.0f;
    // spreading relative to the dry sound heard at a few metres; nothing beyond the probe's range
    e.gain = aDistance < aTuning.range
                 ? aReflectivity * aTuning.gain * (direct + aTuning.nearDistance) / (via + aTuning.nearDistance)
                 : 0.0f;
    return e;
  };
  const Vec3 &r = aFeatures.rightDir;
  const float floor = capped(aFeatures.floor, kDistanceCap), ceiling = capped(aFeatures.ceiling, kDistanceCap);
  const float left = capped(aFeatures.left, kDistanceCap), right = capped(aFeatures.right, kDistanceCap);
  // a surface's own reflectivity where its material is known (live hits), else the defaults
  auto known = [](float aReflectivity, float aDefault) { return aReflectivity >= 0 ? aReflectivity : aDefault; };
  const float rFloor = known(aFeatures.floorReflectivity, aTuning.groundReflectivity);
  const float rCeiling = known(aFeatures.ceilingReflectivity, aTuning.wallReflectivity);
  const float rLeft = known(aFeatures.leftReflectivity, aTuning.wallReflectivity);
  const float rRight = known(aFeatures.rightReflectivity, aTuning.wallReflectivity);
  Params p;
  p.floor = echo(floor, {0, 0, -1}, rFloor);
  p.ceiling = echo(ceiling, {0, 0, 1}, rCeiling);
  p.left = echo(left, {-r.x, -r.y, -r.z}, rLeft);
  p.right = echo(right, r, rRight);
  // back along every other fan ray that hit: the patch of wall it found, seen from the car
  for (int k = 2; k < kNearRays; ++k) {
    const float d = aFeatures.nearDistance[k];
    if (k == aFeatures.leftRay || k == aFeatures.rightRay || !(d < aTuning.range)) {
      continue;
    }
    p.scatter[k] = echo(d, aFeatures.nearDir[k], known(aFeatures.nearReflectivity[k], aTuning.wallReflectivity));
    p.scatter[k].gain *= aTuning.scatter;
  }
  // flutter only where both sides are there
  auto flutter = [&](float aApart) {
    return Clamp01((aTuning.flutterFar - aApart) / (aTuning.flutterFar - aTuning.flutterNear));
  };
  p.floorFeedback = aTuning.flutterFeedback * std::sqrt(rFloor * rCeiling) * flutter(floor + ceiling);
  // the square root of the walls' mean reflectivity, as tuned (render.py: sqrt of the wall default)
  p.wallFeedback = aTuning.flutterFeedback * std::sqrt(std::sqrt(rLeft * rRight)) * flutter(left + right);
  // Eyring with mean free path = 4V/S: open sky absorbs everything
  const float enclosure = Clamp01(aFeatures.enclosure);
  const float freePath = capped(aFeatures.meanFreePath, kFreePathCap);
  const float escape = 1 - enclosure;
  const float alpha = std::min(escape + (1 - escape) * aTuning.wallAbsorption, 0.99f);
  p.rt60 = std::clamp(0.04f * freePath / -std::log(1 - alpha), aTuning.rt60Min, aTuning.rt60Max);
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

float Reflections::Voice::Air(float x) {
  if (air >= 1.0f) {
    return x;
  }
  airState += air * (x - airState);
  return airState;
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
    fdnDelay[j] = uint32_t(std::lround(kFdnMs[j] * 1e-3f * sampleRate));
    fdn[j].Resize(fdnDelay[j] + 1);
  }
  Reset();
}

void Reflections::Reset() {
  for (Line *line : {&source, &floor, &ceiling, &left, &right}) {
    std::fill(line->buffer.begin(), line->buffer.end(), 0.0f);
  }
  for (auto &line : fdn) {
    std::fill(line.buffer.begin(), line.buffer.end(), 0.0f);
  }
  fdnAirState = {};
  started = false;
  for (Voice *v : {&floorVoice, &ceilingVoice, &leftVoice, &rightVoice}) {
    *v = {};
  }
  for (Voice &v : scatterVoices) {
    v = {};
  }
  fdnGainAge = 0;
  energy = {};
  tail = 0;
}

void Reflections::SetTargets(const Params &aParams) {
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
  };
  fdnAir = aParams.air ? 1.0f - std::exp(-2.0f * std::numbers::pi_v<float> * 5000.0f / sampleRate) : 1.0f;
  set(floorVoice, aParams.floor);
  set(ceilingVoice, aParams.ceiling);
  set(leftVoice, aParams.left);
  set(rightVoice, aParams.right);
  for (int k = 2; k < kNearRays; ++k) {
    set(scatterVoices[k], aParams.scatter[k]);
  }
  floorFeedback.To(aParams.floorFeedback, n);
  wallFeedback.To(aParams.wallFeedback, n);
  rt60.To(aParams.rt60, n);
  wet.To(aParams.wet, n);
  if (!started) {
    // the first set takes effect at once
    for (Ramp *r : {&floorFeedback, &wallFeedback, &rt60, &wet}) {
      r->Next();
    }
    started = true;
  }
}

void Reflections::Process(const float *aIn, float *aOut, uint32_t aFrames, int aChannels) {
  const float tailCoefficient = 1.0f - std::exp(-1.0f / (0.05f * sampleRate));
  for (uint32_t s = 0; s < aFrames; ++s) {
    const float *in = aIn + size_t(s) * aChannels;
    float *out = aOut + size_t(s) * aChannels;
    float x = 0;
    for (int c = 0; c < aChannels; ++c) {
      x += in[c];
    }
    x /= float(aChannels);
    source.buffer[write & source.mask] = x;

    float a = 0, b = 0, l = 0, r = 0, sl = 0, sr = 0, revL = 0, revR = 0;
    const double maxRate = tuning.maxDelayRate, minDelay = double(tuning.minDelay) * sampleRate;
    for (Voice *v : {&floorVoice, &ceilingVoice, &leftVoice, &rightVoice}) {
      v->Step(maxRate, minDelay);
    }
    const float ffb = float(floorFeedback.Next()), wfb = float(wallFeedback.Next());
    const float decay = float(rt60.Next()), send = float(wet.Next());
    if (started) {
      // each side's echo, and the other side's echo bounced back off it
      a = floorVoice.Air(floorVoice.Read(source, write, true, true) + ffb * floorVoice.Read(ceiling, write, false, true));
      b = ceilingVoice.Air(ceilingVoice.Read(source, write, true, true) + ffb * ceilingVoice.Read(floor, write, false, true));
      l = leftVoice.Air(leftVoice.Read(source, write, true, true) + wfb * leftVoice.Read(right, write, false, true));
      r = rightVoice.Air(rightVoice.Read(source, write, true, true) + wfb * rightVoice.Read(left, write, false, true));
      // the scattered echoes, each on its fan's side
      for (int k = 2; k < kNearRays; ++k) {
        Voice &v = scatterVoices[k];
        v.Step(maxRate, minDelay);
        if (v.level > 1e-6f) {
          (k < 2 + kFanRays ? sr : sl) += v.Air(v.Read(source, write, true, false));
        }
      }

      if (fdnGainAge-- <= 0) {
        fdnGainAge = kFdnGainSamples - 1;
        const float t = std::max(decay, 0.05f) * sampleRate;
        for (int j = 0; j < 8; ++j) {
          fdnGain[j] = std::pow(10.0f, -3.0f * float(fdnDelay[j]) / t);
        }
      }
      float back[8], sum = 0;
      for (int j = 0; j < 8; ++j) {
        back[j] = fdn[j].buffer[(write - fdnDelay[j]) & fdn[j].mask];
        sum += back[j];
      }
      // Householder: v - (2 / 8) sum(v)
      const float input = x * send / std::sqrt(8.0f);
      for (int j = 0; j < 8; ++j) {
        float v = fdnGain[j] * (back[j] - 0.25f * sum) + input;
        if (fdnAir < 1.0f) {
          fdnAirState[j] += fdnAir * (v - fdnAirState[j]);
          v = fdnAirState[j];
        }
        fdn[j].buffer[write & fdn[j].mask] = v;
        (j % 2 == 0 ? revL : revR) += 0.5f * v;
      }
    }
    floor.buffer[write & floor.mask] = a;
    ceiling.buffer[write & ceiling.mask] = b;
    left.buffer[write & left.mask] = l;
    right.buffer[write & right.mask] = r;
    ++write;

    const float mid = a + b;
    if (aChannels >= 2) {
      out[0] = in[0] + mid + tuning.pan * (l + sl) + tuning.panOpposite * (r + sr) + revL;
      out[1] = in[1] + mid + tuning.panOpposite * (l + sl) + tuning.pan * (r + sr) + revR;
      for (int c = 2; c < aChannels; ++c) {
        out[c] = in[c] + mid + 0.5f * (l + r + sl + sr) + 0.5f * (revL + revR);
      }
    } else {
      out[0] = in[0] + mid + 0.5f * (tuning.pan + tuning.panOpposite) * (l + r + sl + sr) + 0.5f * (revL + revR);
    }

    energy.dry += double(x) * x;
    energy.floor += double(a) * a;
    energy.ceiling += double(b) * b;
    energy.walls += double(l + r) * (l + r);
    energy.reverb += double(revL + revR) * (revL + revR);
    energy.scatter += double(sl + sr) * (sl + sr);
    const float wetNow = mid + l + r + sl + sr + revL + revR;
    tail += (wetNow * wetNow - tail) * tailCoefficient;
  }
}

} // namespace Acoustics
