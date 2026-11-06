#include "Mix.hpp"

#include <algorithm>
#include <cmath>

namespace Out {

void Mix::Process(float *aData, uint32_t aFrames, uint32_t aChannels, float aRate, float aGain, float aCutoffHz) {
  const uint32_t channels = std::min<uint32_t>(aChannels, kMaxChannels);
  if (!started) {
    started = true;
    gain = aGain;
    for (float &l : low) {
      l = 0;
    }
  }
  const float from = gain, step = aFrames ? (aGain - from) / float(aFrames) : 0.0f;
  // the one-pole's coefficient, 1 for a cutoff at or past Nyquist (no filtering)
  const bool filter = aCutoffHz > 0 && aCutoffHz < 0.5f * aRate;
  const float k = filter ? 1.0f - std::exp(-2.0f * 3.14159265f * aCutoffHz / aRate) : 1.0f;
  for (uint32_t c = 0; c < channels; ++c) {
    float *x = aData + size_t(c) * aFrames;
    float l = filter ? low[c] : 0.0f;
    for (uint32_t i = 0; i < aFrames; ++i) {
      float v = x[i];
      if (filter) {
        l += k * (v - l);
        v = l;
      }
      x[i] = v * (from + step * float(i + 1));
    }
    low[c] = filter ? l : 0.0f;
  }
  gain = aGain;
}

void Limiter::Process(const float *aIn, float *aGain, uint32_t aFrames, float aRate, float aCeilingDb, float aReleaseMs) {
  const float ceiling = std::pow(10.0f, aCeilingDb / 20.0f);
  const float release = 1.0f - std::exp(-1.0f / std::max(aReleaseMs * 1e-3f * aRate, 1.0f));
  for (uint32_t i = 0; i < aFrames; ++i) {
    const float peak = std::abs(aIn[i]);
    const float target = peak > ceiling ? ceiling / peak : 1.0f;
    gain = target < gain ? target : gain + release * (target - gain);
    aGain[i] = gain;
  }
}

} // namespace Out
