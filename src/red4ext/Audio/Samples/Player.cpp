#include "Player.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iterator>

namespace Samples {

namespace {

uint32_t U32(const std::vector<uint8_t> &b, size_t at) {
  uint32_t v;
  std::memcpy(&v, b.data() + at, 4);
  return v;
}

uint16_t U16(const std::vector<uint8_t> &b, size_t at) {
  uint16_t v;
  std::memcpy(&v, b.data() + at, 2);
  return v;
}

} // namespace

bool ParseWave(const std::vector<uint8_t> &aBytes, Wave &aOut) {
  if (aBytes.size() < 12 || std::memcmp(aBytes.data(), "RIFF", 4) != 0 || std::memcmp(aBytes.data() + 8, "WAVE", 4) != 0) {
    return false;
  }
  uint16_t format = 0, channels = 0, bits = 0;
  uint32_t rate = 0;
  size_t at = 12;
  while (at + 8 <= aBytes.size()) {
    const uint32_t size = U32(aBytes, at + 4);
    const size_t body = at + 8;
    if (body + size > aBytes.size()) {
      return false;
    }
    if (std::memcmp(aBytes.data() + at, "fmt ", 4) == 0 && size >= 16) {
      format = U16(aBytes, body);
      channels = U16(aBytes, body + 2);
      rate = U32(aBytes, body + 4);
      bits = U16(aBytes, body + 14);
    } else if (std::memcmp(aBytes.data() + at, "data", 4) == 0) {
      const bool pcm16 = format == 1 && bits == 16, float32 = format == 3 && bits == 32;
      if (!(pcm16 || float32) || channels == 0 || rate == 0) {
        return false;
      }
      const size_t bytesPerFrame = size_t(channels) * (bits / 8), frames = size / bytesPerFrame;
      aOut.rate = float(rate);
      aOut.mono.assign(frames, 0.0f);
      for (size_t f = 0; f < frames; ++f) {
        float sum = 0;
        for (uint16_t c = 0; c < channels; ++c) {
          const size_t p = body + f * bytesPerFrame + size_t(c) * (bits / 8);
          if (pcm16) {
            int16_t v;
            std::memcpy(&v, aBytes.data() + p, 2);
            sum += float(v) / 32768.0f;
          } else {
            float v;
            std::memcpy(&v, aBytes.data() + p, 4);
            sum += v;
          }
        }
        aOut.mono[f] = sum / float(channels);
      }
      return true;
    }
    at = body + size + (size & 1);
  }
  return false;
}

bool LoadWave(const std::filesystem::path &aPath, Wave &aOut) {
  std::ifstream in(aPath, std::ios::binary);
  if (!in) {
    return false;
  }
  const std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
  return ParseWave(bytes, aOut);
}

float Tremolo(double aCycles, float aDuty, float aSquare, float aPhase) {
  const float duty = std::clamp(aDuty, 0.01f, 1.0f);
  double whole = aCycles + aPhase;
  const float x = float(whole - std::floor(whole));
  if (x >= duty) {
    return 0.0f;
  }
  // up and down over the on part, flattened at the top as aSquare goes to 1
  const float triangle = 1.0f - std::abs(2.0f * x / duty - 1.0f);
  return std::min(1.0f, triangle / std::max(1.0f - std::clamp(aSquare, 0.0f, 1.0f), 0.01f));
}

float Distort(float aSample, float aLevel) {
  const float drive = 1.0f + 15.0f * std::clamp(aLevel, 0.0f, 1.0f);
  return std::tanh(drive * aSample) / std::tanh(drive);
}

Player::Player(const Wave &aWave, float aOutputRate, bool aLoop) : wave(aWave), outputRate(aOutputRate), loop(aLoop) {}

void Player::Set(float aRatio, bool aTremolo, float aTremoloHz, float aDuty, float aSquare, float aPhase,
                 bool aDistortion, float aDistortionLevel) {
  ratio.store(aRatio, std::memory_order_relaxed);
  tremolo.store(aTremolo, std::memory_order_relaxed);
  tremoloHz.store(aTremoloHz, std::memory_order_relaxed);
  duty.store(aDuty, std::memory_order_relaxed);
  square.store(aSquare, std::memory_order_relaxed);
  phase.store(aPhase, std::memory_order_relaxed);
  distortion.store(aDistortion, std::memory_order_relaxed);
  distortionLevel.store(aDistortionLevel, std::memory_order_relaxed);
}

void Player::Render(float *aOut, uint32_t aFrames) {
  const size_t n = wave.mono.size();
  if (n < 2 || ended.load(std::memory_order_relaxed)) {
    std::memset(aOut, 0, aFrames * sizeof(float));
    return;
  }
  const double step = double(ratio.load(std::memory_order_relaxed)) * wave.rate / outputRate;
  const bool trem = tremolo.load(std::memory_order_relaxed), dist = distortion.load(std::memory_order_relaxed);
  const float hz = tremoloHz.load(std::memory_order_relaxed), d = duty.load(std::memory_order_relaxed),
              sq = square.load(std::memory_order_relaxed), ph = phase.load(std::memory_order_relaxed),
              level = distortionLevel.load(std::memory_order_relaxed);
  const double dt = 1.0 / outputRate;
  for (uint32_t i = 0; i < aFrames; ++i) {
    if (position >= double(n - 1)) {
      if (!loop) {
        std::memset(aOut + i, 0, (aFrames - i) * sizeof(float));
        ended.store(true, std::memory_order_relaxed);
        return;
      }
      position = std::fmod(position, double(n - 1));
    }
    const size_t k = size_t(position);
    const float f = float(position - double(k));
    float v = wave.mono[k] + f * (wave.mono[k + 1] - wave.mono[k]);
    if (trem) {
      v *= Tremolo(cycles, d, sq, ph);
    }
    if (dist) {
      v = Distort(v, level);
    }
    aOut[i] = v;
    position += step;
    cycles += hz * dt;
  }
}

} // namespace Samples
