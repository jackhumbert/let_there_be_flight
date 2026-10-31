#include "Stems.hpp"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <cstdio>
#include <limits>
#include <mutex>
#include <thread>
#include <vector>

namespace Acoustics::Stems {

namespace {

constexpr int kStems = 3;
constexpr const char *kNames[kStems] = {"dry", "wet", "bus"};
// about 11 s at 48 kHz, stereo
constexpr uint64_t kRingFrames = uint64_t(1) << 19;
constexpr uint64_t kMask = kRingFrames - 1;
constexpr uint64_t kNone = std::numeric_limits<uint64_t>::max();
// the writer saves what is this old (the mixer may still add to anything newer) every 100 ms
constexpr float kLagSeconds = 0.5f;
constexpr auto kWritePeriod = std::chrono::milliseconds(100);

std::atomic<bool> recording{false};
std::array<std::vector<float>, kStems> ring;
// mix clock of the first frame, the end of the newest frames added, and how far the writer is
std::atomic<uint64_t> startClock{kNone}, newest{0}, written{0};
std::atomic<int> rate{0};
std::atomic<double> startWall{0};
std::atomic<uint64_t> dropped{0};

// Start / Stop and the writer
std::mutex lock;
std::array<FILE *, kStems> files{};
uint64_t frames = 0;
std::string base;
// the thrusters' parameters, a row an update (WriteParams), from the game thread
std::mutex paramsLock;
FILE *params = nullptr;
bool paramsHeader = false;
bool infoWritten = false;
std::condition_variable wake;
bool stopping = false;
std::thread writer;

void Header(FILE *aFile, uint64_t aFrames, int aRate) {
  const uint32_t data = uint32_t(aFrames * 8);
  const uint16_t format = 3, channels = 2, align = 8, bits = 32; // IEEE float
  const uint32_t riff = 36 + data, fmtSize = 16, sampleRate = uint32_t(aRate), byteRate = sampleRate * align;
  std::fseek(aFile, 0, SEEK_SET);
  std::fwrite("RIFF", 1, 4, aFile);
  std::fwrite(&riff, 4, 1, aFile);
  std::fwrite("WAVEfmt ", 1, 8, aFile);
  std::fwrite(&fmtSize, 4, 1, aFile);
  std::fwrite(&format, 2, 1, aFile);
  std::fwrite(&channels, 2, 1, aFile);
  std::fwrite(&sampleRate, 4, 1, aFile);
  std::fwrite(&byteRate, 4, 1, aFile);
  std::fwrite(&align, 2, 1, aFile);
  std::fwrite(&bits, 2, 1, aFile);
  std::fwrite("data", 1, 4, aFile);
  std::fwrite(&data, 4, 1, aFile);
  std::fseek(aFile, 0, SEEK_END);
}

void WriteInfo() {
  const double wall = startWall.load();
  if (FILE *f = std::fopen((base + ".json").c_str(), "w")) {
    // startWallOfDay: seconds since midnight UTC, as lib.Recording's .wall
    std::fprintf(f, "{\"rate\":%d,\"startClock\":%llu,\"startWall\":%.6f,\"startWallOfDay\":%.6f}\n", rate.load(),
                 (unsigned long long)startClock.load(), wall, std::fmod(wall, 86400.0));
    std::fclose(f);
  }
  infoWritten = true;
}

// Write the frames up to mix clock aEnd. Under lock.
void Flush(uint64_t aEnd) {
  const uint64_t start = startClock.load();
  if (start == kNone || !files[0]) {
    return;
  }
  if (!infoWritten) {
    WriteInfo();
    written.store(start);
  }
  uint64_t from = written.load();
  if (aEnd <= from) {
    return;
  }
  std::vector<float> chunk;
  while (from < aEnd) {
    const uint64_t n = std::min(aEnd - from, kRingFrames - (from & kMask));
    for (int k = 0; k < kStems; ++k) {
      float *at = ring[k].data() + (from & kMask) * 2;
      chunk.assign(at, at + n * 2);
      std::fill(at, at + n * 2, 0.0f);
      std::fwrite(chunk.data(), sizeof(float), chunk.size(), files[k]);
    }
    from += n;
    frames += n;
  }
  written.store(from);
  for (auto *f : files) {
    Header(f, frames, rate.load());
    std::fflush(f);
  }
}

void Write() {
  std::unique_lock guard(lock);
  while (!stopping) {
    wake.wait_for(guard, kWritePeriod, [] { return stopping; });
    const uint64_t lag = uint64_t(kLagSeconds * rate.load());
    const uint64_t end = newest.load();
    if (end > lag) {
      Flush(end - lag);
    }
  }
}

} // namespace

bool Start(const std::string &aBase) {
  Stop();
  std::lock_guard guard(lock);
  for (int k = 0; k < kStems; ++k) {
    const auto path = aBase + "_" + kNames[k] + ".wav";
    files[k] = std::fopen(path.c_str(), "wb");
    if (!files[k]) {
      spdlog::warn("[Acoustics] cannot write {}", path);
      for (auto *&f : files) {
        if (f) {
          std::fclose(f);
          f = nullptr;
        }
      }
      return false;
    }
    // the header's place, filled in as the data comes
    Header(files[k], 0, 0);
    ring[k].assign(kRingFrames * 2, 0.0f);
  }
  base = aBase;
  frames = 0;
  infoWritten = false;
  startClock.store(kNone);
  newest.store(0);
  written.store(0);
  rate.store(0);
  dropped.store(0);
  stopping = false;
  {
    std::lock_guard paramsGuard(paramsLock);
    params = std::fopen((aBase + "_params.csv").c_str(), "w");
    paramsHeader = false;
  }
  recording.store(true);
  writer = std::thread(Write);
  spdlog::info("[Acoustics] stems to {}_*.wav", aBase);
  return true;
}

void WriteParams(const std::string &aId, float aPitch, float aVolume, const float *aPosition,
                 const std::vector<std::pair<std::string, float>> &aValues) {
  std::lock_guard guard(paramsLock);
  if (!params) {
    return;
  }
  if (!paramsHeader) {
    std::fputs("wall,id,start_pitch,volume,x,y,z", params);
    for (const auto &[name, _] : aValues) {
      std::fprintf(params, ",%s", name.c_str());
    }
    std::fputc('\n', params);
    paramsHeader = true;
  }
  const double wall = std::chrono::duration<double>(std::chrono::system_clock::now().time_since_epoch()).count();
  std::fprintf(params, "%.4f,%s,%.4f,%.4f,%.3f,%.3f,%.3f", wall, aId.c_str(), aPitch, aVolume, aPosition[0],
               aPosition[1], aPosition[2]);
  for (const auto &[_, value] : aValues) {
    std::fprintf(params, ",%.4f", value);
  }
  std::fputc('\n', params);
}

void Stop() {
  {
    std::lock_guard guard(lock);
    if (!recording.load()) {
      return;
    }
    recording.store(false);
    stopping = true;
  }
  wake.notify_all();
  writer.join();
  std::lock_guard guard(lock);
  Flush(newest.load());
  for (auto *&f : files) {
    std::fclose(f);
    f = nullptr;
  }
  {
    std::lock_guard paramsGuard(paramsLock);
    if (params) {
      std::fclose(params);
      params = nullptr;
    }
  }
  spdlog::info("[Acoustics] stems: {} frames to {}_*.wav, {} dropped", frames, base, dropped.load());
}

bool Recording() { return recording.load(std::memory_order_relaxed); }

namespace {

// aLength frames of aChannels appended to aTo as stereo: mono to both channels, more than two folded
// onto left and right by index
void Fold(const float *aBuffer, unsigned int aLength, int aChannels, std::vector<float> &aTo) {
  const float scale = aChannels > 2 ? 2.0f / float(aChannels) : 1.0f;
  for (unsigned int s = 0; s < aLength; ++s) {
    const float *in = aBuffer + size_t(s) * aChannels;
    float l = 0, r = 0;
    if (aChannels == 1) {
      l = r = in[0];
    } else {
      for (int c = 0; c < aChannels; ++c) {
        (c & 1 ? r : l) += scale * in[c];
      }
    }
    aTo.push_back(l);
    aTo.push_back(r);
  }
}

// aFrames stereo frames added to a stem at mix clock aAt
void Place(Stem aStem, uint64_t aAt, const float *aStereo, uint64_t aFrames) {
  // too late for the writer, or so far ahead the ring would wrap onto unwritten frames
  const uint64_t done = std::max(written.load(std::memory_order_relaxed), startClock.load(std::memory_order_relaxed));
  if (aAt < done || aAt + aFrames > done + kRingFrames) {
    dropped.fetch_add(aFrames, std::memory_order_relaxed);
    return;
  }
  float *stem = ring[int(aStem)].data();
  for (uint64_t s = 0; s < aFrames; ++s) {
    float *frame = stem + ((aAt + s) & kMask) * 2;
    frame[0] += aStereo[2 * s];
    frame[1] += aStereo[2 * s + 1];
  }
  uint64_t end = aAt + aFrames, seen = newest.load(std::memory_order_relaxed);
  while (end > seen && !newest.compare_exchange_weak(seen, end, std::memory_order_relaxed)) {
  }
}

// aIn (stereo, any length) resampled linearly to aFrames frames and placed
void PlaceResampled(Stem aStem, uint64_t aAt, const std::vector<float> &aIn, uint64_t aFrames, std::vector<float> &aScratch) {
  const size_t n = aIn.size() / 2;
  if (n == 0 || aFrames == 0) {
    return;
  }
  aScratch.resize(aFrames * 2);
  const double step = double(n) / double(aFrames);
  for (uint64_t j = 0; j < aFrames; ++j) {
    const double x = std::clamp((j + 0.5) * step - 0.5, 0.0, double(n - 1));
    const size_t i = size_t(x);
    const size_t k = std::min(i + 1, n - 1);
    const float f = float(x - double(i));
    aScratch[2 * j] = aIn[2 * i] + f * (aIn[2 * k] - aIn[2 * i]);
    aScratch[2 * j + 1] = aIn[2 * i + 1] + f * (aIn[2 * k + 1] - aIn[2 * i + 1]);
  }
  Place(aStem, aAt, aScratch.data(), aFrames);
}

// the newest mix block the bus DSP saw
std::atomic<uint64_t> mixClock{kNone};
std::atomic<uint32_t> mixLength{0};

} // namespace

EventCursor::EventCursor() : mix(kNone) {
  dry.reserve(kReserve);
  wet.reserve(kReserve);
  scratch.reserve(kReserve);
}

void AddBus(uint64_t aClock, const float *aIn, const float *aOut, unsigned int aLength, int aChannels, int aRate, bool aTest,
            std::vector<float> &aScratch) {
  if (!recording.load(std::memory_order_relaxed) || aChannels < 1) {
    return;
  }
  uint64_t none = kNone;
  if (startClock.load(std::memory_order_relaxed) == kNone) {
    int noRate = 0;
    rate.compare_exchange_strong(noRate, aRate);
    if (startClock.compare_exchange_strong(none, aClock)) {
      startWall.store(std::chrono::duration<double>(std::chrono::system_clock::now().time_since_epoch()).count());
    }
  }
  mixLength.store(aLength, std::memory_order_relaxed);
  mixClock.store(aClock, std::memory_order_relaxed);
  aScratch.clear();
  Fold(aOut, aLength, aChannels, aScratch);
  Place(aTest ? Stem::Wet : Stem::Bus, aClock, aScratch.data(), aLength);
  if (aTest) {
    aScratch.clear();
    Fold(aIn, aLength, aChannels, aScratch);
    Place(Stem::Dry, aClock, aScratch.data(), aLength);
  }
}

void AddEvent(EventCursor &aCursor, const float *aIn, const float *aOut, unsigned int aLength, int aChannels) {
  if (!recording.load(std::memory_order_relaxed) || aChannels < 1) {
    aCursor.mix = kNone;
    return;
  }
  const uint64_t mix = mixClock.load(std::memory_order_relaxed);
  if (mix == kNone) {
    return;
  }
  if (mix != aCursor.mix) {
    // a new mix block: what was read in the last one goes out at its length
    if (aCursor.mix != kNone) {
      PlaceResampled(Stem::Dry, aCursor.mix, aCursor.dry, aCursor.length, aCursor.scratch);
      PlaceResampled(Stem::Wet, aCursor.mix, aCursor.wet, aCursor.length, aCursor.scratch);
    }
    aCursor.dry.clear();
    aCursor.wet.clear();
    aCursor.mix = mix;
    aCursor.length = mixLength.load(std::memory_order_relaxed);
  }
  Fold(aIn, aLength, aChannels, aCursor.dry);
  Fold(aOut, aLength, aChannels, aCursor.wet);
}

std::string State() {
  std::lock_guard guard(lock);
  return fmt::format("{{\"recording\":{},\"frames\":{},\"dropped\":{}}}", recording.load() ? "true" : "false", frames,
                     dropped.load());
}

} // namespace Acoustics::Stems
