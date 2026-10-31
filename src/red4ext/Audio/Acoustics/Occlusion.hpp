#pragma once

// NPC cars heard through buildings (docs/design/audio-native.md D.13), with no FMOD or game types so
// tests/ can link it. The game thread publishes the cars that hold a channel into a few slots; the PhysX
// caster (LiveRays.cpp) casts a ray from the listener to some of them each cast and stores whether
// static geometry is in the way; the game thread smooths that per car into 0 (open) .. 1 (blocked) and
// Voices turns it into a low-pass and a duck. No locks: every slot field is an atomic.

#include <atomic>
#include <cstdint>
#include <vector>

namespace Acoustics::Occlusion {

// the smoothing's time constant
inline constexpr float kSeconds = 0.1f;
// the low-pass cutoff, open and fully blocked
inline constexpr float kOpenHz = 22000.0f, kBlockedHz = 1200.0f;
// the duck, fully blocked
inline constexpr float kDuckDb = -8.0f;
// rays a cast spends on cars, and cars that can be tracked at once
inline constexpr int kPerCast = 4, kSlots = 16;
// a ray ends this far short of a car: the car's own size
inline constexpr float kCarSizeM = 3.0f;
// the longest ray
inline constexpr float kRangeM = 100.0f;

// the value moves toward 1 (blocked) or 0 (open) with time constant kSeconds
float Smooth(float aValue, bool aBlocked, float aDt);
// the cutoff for an occlusion 0..1, log-spaced between kOpenHz and kBlockedHz
float CutoffHz(float aOcclusion);
// the duck for an occlusion 0..1
float DuckDb(float aOcclusion);

// Which slots a cast takes: up to kPerCast, continuing where the last call stopped, wrapping.
class RoundRobin {
public:
  std::vector<int> Next(int aCount);

private:
  int next = 0;
};

struct Vec3 {
  float x = 0, y = 0, z = 0;
};

// a car for Publish: its id and centre (world)
struct Car {
  uint64_t id;
  Vec3 position;
};

// what the caster reads for a slot
struct Target {
  uint64_t car;
  Vec3 position;
};

// The switch (Acoustics::SetOcclusion): off, no rays are cast and every car fades back to open.
extern std::atomic<bool> enabled;

// Game thread: the listener and the cars for the casts, at most kSlots (the rest are left out). A car
// that stays in the list keeps its last result.
void Publish(const Vec3 &aListener, const std::vector<Car> &aCars);
// Caster: the listener, how many slots are in use, and a slot's car (false: emptied meanwhile).
Vec3 Listener();
int Count();
bool Slot(int aIndex, Target &aTarget);
// Caster: the result of a ray at aCar in aSlot; ignored if the slot has since gone to another car.
void StoreBlocked(int aSlot, uint64_t aCar, bool aBlocked);
// Game thread: -1 unknown, 0 open, 1 blocked
int Blocked(uint64_t aCar);

// rays cast so far, for State
uint64_t Casts();
void CountCast();

} // namespace Acoustics::Occlusion
