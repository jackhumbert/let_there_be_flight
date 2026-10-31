#include "Reflections.hpp"

#include <algorithm>
#include <cmath>

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

} // namespace

Params Targets(const Features &aFeatures, const Vec3 &aSource, const Vec3 &aListener, const Tuning &aTuning) {
  auto capped = [](float d, float cap) { return std::isinf(d) ? cap : std::min(d, cap); };
  const float direct = Length(aListener, aSource);
  auto echo = [&](float aDistance, Vec3 aNormal, float aReflectivity) {
    Vec3 image{aSource.x + 2 * aDistance * aNormal.x, aSource.y + 2 * aDistance * aNormal.y,
               aSource.z + 2 * aDistance * aNormal.z};
    float via = Length(aListener, image);
    Echo e;
    e.delay = (via - direct) / kSpeedOfSound;
    // spreading relative to the dry sound heard at a few metres; nothing beyond the probe's range
    e.gain = aDistance < aTuning.range
                 ? aReflectivity * aTuning.gain * (direct + aTuning.nearDistance) / (via + aTuning.nearDistance)
                 : 0.0f;
    return e;
  };
  const Vec3 &r = aFeatures.rightDir;
  const float floor = capped(aFeatures.floor, kDistanceCap), ceiling = capped(aFeatures.ceiling, kDistanceCap);
  const float left = capped(aFeatures.left, kDistanceCap), right = capped(aFeatures.right, kDistanceCap);
  Params p;
  p.floor = echo(floor, {0, 0, -1}, aTuning.groundReflectivity);
  p.ceiling = echo(ceiling, {0, 0, 1}, aTuning.wallReflectivity);
  p.left = echo(left, {-r.x, -r.y, -r.z}, aTuning.wallReflectivity);
  p.right = echo(right, r, aTuning.wallReflectivity);
  // flutter only where both sides are there
  auto flutter = [&](float aApart) {
    return Clamp01((aTuning.flutterFar - aApart) / (aTuning.flutterFar - aTuning.flutterNear));
  };
  p.floorFeedback = aTuning.flutterFeedback * std::sqrt(aTuning.wallReflectivity * aTuning.groundReflectivity) *
                    flutter(floor + ceiling);
  p.wallFeedback = aTuning.flutterFeedback * std::sqrt(aTuning.wallReflectivity) * flutter(left + right);
  // Eyring with mean free path = 4V/S: open sky absorbs everything
  const float enclosure = Clamp01(aFeatures.enclosure);
  const float freePath = capped(aFeatures.meanFreePath, kFreePathCap);
  const float escape = 1 - enclosure;
  const float alpha = std::min(escape + (1 - escape) * aTuning.wallAbsorption, 0.99f);
  p.rt60 = std::clamp(0.04f * freePath / -std::log(1 - alpha), aTuning.rt60Min, aTuning.rt60Max);
  // the lower half of the sphere is nearly always ground: only enclosure above that is room
  p.wet = aTuning.wet * aTuning.gain * Clamp01((enclosure - 0.5f) / 0.5f);
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
  started = false;
  floorSlew = ceilingSlew = leftSlew = rightSlew = -1;
  fdnGainAge = 0;
  energy = {};
  tail = 0;
}

void Reflections::SetTargets(const Params &aParams) {
  // delays in samples
  auto delay = [&](const Echo &e) { return double(std::max(e.delay, tuning.minDelay)) * sampleRate; };
  const int n = started ? rampSamples : 0;
  floorDelay.To(delay(aParams.floor), n);
  floorGain.To(aParams.floor.gain, n);
  ceilingDelay.To(delay(aParams.ceiling), n);
  ceilingGain.To(aParams.ceiling.gain, n);
  leftDelay.To(delay(aParams.left), n);
  leftGain.To(aParams.left.gain, n);
  rightDelay.To(delay(aParams.right), n);
  rightGain.To(aParams.right.gain, n);
  floorFeedback.To(aParams.floorFeedback, n);
  wallFeedback.To(aParams.wallFeedback, n);
  rt60.To(aParams.rt60, n);
  wet.To(aParams.wet, n);
  if (!started) {
    // the first set takes effect at once
    for (Ramp *r : {&floorDelay, &floorGain, &ceilingDelay, &ceilingGain, &leftDelay, &leftGain, &rightDelay,
                    &rightGain, &floorFeedback, &wallFeedback, &rt60, &wet}) {
      r->Next();
    }
    started = true;
  }
}

double Reflections::Slew(double &aState, double aTarget) const {
  if (aState < 0) {
    aState = aTarget;
  } else {
    const double step = tuning.maxDelayRate; // samples per sample
    aState += std::clamp(aTarget - aState, -step, step);
  }
  return aState;
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

    float a = 0, b = 0, l = 0, r = 0, revL = 0, revR = 0;
    const double fd = Slew(floorSlew, floorDelay.Next()), cd = Slew(ceilingSlew, ceilingDelay.Next());
    const double ld = Slew(leftSlew, leftDelay.Next()), rd = Slew(rightSlew, rightDelay.Next());
    const float fg = float(floorGain.Next()), cg = float(ceilingGain.Next());
    const float lg = float(leftGain.Next()), rg = float(rightGain.Next());
    const float ffb = float(floorFeedback.Next()), wfb = float(wallFeedback.Next());
    const float decay = float(rt60.Next()), send = float(wet.Next());    if (started) {
      // each side's echo, and the other side's echo bounced back off it
      a = fg * source.Read(write, fd) + ffb * ceiling.Read(write, fd);
      b = cg * source.Read(write, cd) + ffb * floor.Read(write, cd);
      l = lg * source.Read(write, ld) + wfb * right.Read(write, ld);
      r = rg * source.Read(write, rd) + wfb * left.Read(write, rd);

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
      out[0] = in[0] + mid + tuning.pan * l + tuning.panOpposite * r + revL;
      out[1] = in[1] + mid + tuning.panOpposite * l + tuning.pan * r + revR;
      for (int c = 2; c < aChannels; ++c) {
        out[c] = in[c] + mid + 0.5f * (l + r) + 0.5f * (revL + revR);
      }
    } else {
      out[0] = in[0] + mid + 0.5f * (tuning.pan + tuning.panOpposite) * (l + r) + 0.5f * (revL + revR);
    }

    energy.dry += double(x) * x;
    energy.floor += double(a) * a;
    energy.ceiling += double(b) * b;
    energy.walls += double(l + r) * (l + r);
    energy.reverb += double(revL + revR) * (revL + revR);
    const float wetNow = mid + l + r + revL + revR;
    tail += (wetNow * wetNow - tail) * tailCoefficient;
  }
}

} // namespace Acoustics
