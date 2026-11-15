#pragma once

// A single producer, single consumer ring of mono samples between the mixer's DSPs (docs/design/audio.md,
// Rings): the engine's bed writes a block, an echo voice reads it one block later. The buffer is allocated
// in the constructor and nothing waits. The ring starts holding aPrefill zeros, so the reader is that
// many samples behind the writer from the first block on.
//
// A Read that finds too little fills the rest with zeros and counts an underrun; a Write that finds no
// room drops what does not fit (the newest) and counts an overrun.

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <vector>

class Ring {
public:
  // aCapacityPow2: samples held at most (a power of two)
  explicit Ring(uint32_t aCapacityPow2, uint32_t aPrefill)
      : buffer(aCapacityPow2, 0.0f), mask(aCapacityPow2 - 1), write(aPrefill) {}

  // producer
  void Write(const float *aIn, uint32_t aCount) {
    const uint32_t w = write.load(std::memory_order_relaxed), r = read.load(std::memory_order_acquire);
    const uint32_t room = uint32_t(buffer.size()) - (w - r);
    const uint32_t n = std::min(aCount, room);
    if (n < aCount) {
      overruns.fetch_add(1, std::memory_order_relaxed);
    }
    for (uint32_t i = 0; i < n; ++i) {
      buffer[(w + i) & mask] = aIn[i];
    }
    write.store(w + n, std::memory_order_release);
  }

  // consumer: aCount samples to aOut; what is missing is zeros
  uint32_t Read(float *aOut, uint32_t aCount) {
    const uint32_t r = read.load(std::memory_order_relaxed), w = write.load(std::memory_order_acquire);
    const uint32_t n = std::min(aCount, w - r);
    for (uint32_t i = 0; i < n; ++i) {
      aOut[i] = buffer[(r + i) & mask];
    }
    if (n < aCount) {
      std::memset(aOut + n, 0, size_t(aCount - n) * sizeof(float));
      underruns.fetch_add(1, std::memory_order_relaxed);
    }
    read.store(r + n, std::memory_order_release);
    return n;
  }

  // consumer: what a Read could return now
  uint32_t Available() const { return write.load(std::memory_order_acquire) - read.load(std::memory_order_relaxed); }
  // consumer: drops the oldest aCount samples (a reader that was paused while the writer went on)
  void Skip(uint32_t aCount) { read.store(read.load(std::memory_order_relaxed) + std::min(aCount, Available()), std::memory_order_release); }
  // producer: makes the fill aPrefill zeros again, for a stream that starts afresh (the reader may be
  // mid-read: it gets zeros or old samples for a block, nothing unsafe)
  void Resync(uint32_t aPrefill) {
    const uint32_t r = read.load(std::memory_order_acquire);
    for (uint32_t i = 0; i < aPrefill; ++i) {
      buffer[(r + i) & mask] = 0.0f;
    }
    write.store(r + aPrefill, std::memory_order_release);
  }

  uint64_t Underruns() const { return underruns.load(std::memory_order_relaxed); }
  uint64_t Overruns() const { return overruns.load(std::memory_order_relaxed); }

private:
  std::vector<float> buffer;
  uint32_t mask;
  std::atomic<uint32_t> read{0}, write;
  std::atomic<uint64_t> underruns{0}, overruns{0};
};
