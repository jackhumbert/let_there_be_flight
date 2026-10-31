#include "Occlusion.hpp"

#include <algorithm>
#include <array>
#include <cmath>

namespace Acoustics::Occlusion {

std::atomic<bool> enabled{true};

namespace {

struct SlotData {
  std::atomic<uint64_t> car{0};
  std::atomic<float> x{0}, y{0}, z{0};
  // -1 unknown, 0 open, 1 blocked
  std::atomic<int> blocked{-1};
};

std::array<SlotData, kSlots> slots;
std::atomic<int> count{0};
std::atomic<float> listenerX{0}, listenerY{0}, listenerZ{0};
std::atomic<uint64_t> casts{0};

} // namespace

float Smooth(float aValue, bool aBlocked, float aDt) {
  return aValue + ((aBlocked ? 1.0f : 0.0f) - aValue) * (1.0f - std::exp(-aDt / kSeconds));
}

float CutoffHz(float aOcclusion) {
  const float open = std::log(kOpenHz);
  return std::exp(open + aOcclusion * (std::log(kBlockedHz) - open));
}

float DuckDb(float aOcclusion) {
  return kDuckDb * aOcclusion;
}

std::vector<int> RoundRobin::Next(int aCount) {
  std::vector<int> picked;
  if (aCount <= 0) {
    next = 0;
    return picked;
  }
  if (next >= aCount) {
    next = 0;
  }
  const int n = std::min(kPerCast, aCount - next);
  for (int i = 0; i < n; ++i) {
    picked.push_back(next + i);
  }
  next = (next + n) % aCount;
  return picked;
}

void Publish(const Vec3 &aListener, const std::vector<Car> &aCars) {
  listenerX.store(aListener.x, std::memory_order_relaxed);
  listenerY.store(aListener.y, std::memory_order_relaxed);
  listenerZ.store(aListener.z, std::memory_order_relaxed);
  // what the slots held, to carry each car's result over to its new place
  const int old = count.load(std::memory_order_relaxed);
  std::array<std::pair<uint64_t, int>, kSlots> before;
  for (int s = 0; s < old; ++s) {
    before[s] = {slots[s].car.load(std::memory_order_relaxed), slots[s].blocked.load(std::memory_order_relaxed)};
  }
  const int wanted = int(std::min<size_t>(aCars.size(), kSlots));
  for (int i = 0; i < wanted; ++i) {
    int blocked = -1;
    for (int s = 0; s < old; ++s) {
      if (before[s].first == aCars[i].id) {
        blocked = before[s].second;
        break;
      }
    }
    SlotData &slot = slots[i];
    slot.car.store(aCars[i].id, std::memory_order_relaxed);
    slot.x.store(aCars[i].position.x, std::memory_order_relaxed);
    slot.y.store(aCars[i].position.y, std::memory_order_relaxed);
    slot.z.store(aCars[i].position.z, std::memory_order_relaxed);
    slot.blocked.store(blocked, std::memory_order_relaxed);
  }
  for (int s = wanted; s < old; ++s) {
    slots[s].car.store(0, std::memory_order_relaxed);
  }
  count.store(wanted, std::memory_order_release);
}

Vec3 Listener() {
  return {listenerX.load(std::memory_order_relaxed), listenerY.load(std::memory_order_relaxed),
          listenerZ.load(std::memory_order_relaxed)};
}

int Count() {
  return count.load(std::memory_order_acquire);
}

bool Slot(int aIndex, Target &aTarget) {
  if (aIndex < 0 || aIndex >= kSlots) {
    return false;
  }
  const SlotData &s = slots[aIndex];
  aTarget.car = s.car.load(std::memory_order_relaxed);
  aTarget.position = {s.x.load(std::memory_order_relaxed), s.y.load(std::memory_order_relaxed),
                      s.z.load(std::memory_order_relaxed)};
  return aTarget.car != 0;
}

void StoreBlocked(int aSlot, uint64_t aCar, bool aBlocked) {
  if (aSlot >= 0 && aSlot < kSlots && slots[aSlot].car.load(std::memory_order_relaxed) == aCar) {
    slots[aSlot].blocked.store(aBlocked ? 1 : 0, std::memory_order_relaxed);
  }
}

int Blocked(uint64_t aCar) {
  const int n = count.load(std::memory_order_acquire);
  for (int s = 0; s < n; ++s) {
    if (slots[s].car.load(std::memory_order_relaxed) == aCar) {
      return slots[s].blocked.load(std::memory_order_relaxed);
    }
  }
  return -1;
}

uint64_t Casts() {
  return casts.load(std::memory_order_relaxed);
}

void CountCast() {
  casts.fetch_add(1, std::memory_order_relaxed);
}

} // namespace Acoustics::Occlusion
