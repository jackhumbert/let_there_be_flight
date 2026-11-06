#pragma once

// A Wwise voice's processing after its generator (Out.hpp): what FMOD's channel did for it. The gain (the voice's,
// its bus's, the rolloff's) ramps over each block from the last block's to the newest, as FMOD's fader does, and a
// one-pole low-pass (an NPC car's occlusion, FMOD's "lowpass simple") runs when asked for. Game-free, for the audio
// thread: no locks, no allocation.

#include <cstdint>

namespace Out {

class Mix {
public:
  static constexpr int kMaxChannels = 2;

  // aData: aFrames of each of aChannels, channel c at aData + c * aFrames. aGain linear; aCutoffHz <= 0 for no
  // low-pass. The first block starts at aGain (no ramp up from nothing).
  void Process(float *aData, uint32_t aFrames, uint32_t aChannels, float aRate, float aGain, float aCutoffHz);
  // the next block starts afresh: no ramp from the last gain, the filter empty
  void Reset() { started = false; }

private:
  bool started = false;
  float gain = 0;
  float low[kMaxChannels] = {};
};

// FMOD's limiter as the Engine group had it (Buses::LimiterFor), for the Wwise backend, which has no bus of ours to put
// one on: the gain that holds a signal's peaks under the ceiling, down at once, back up over the release. The engine's
// bed runs it on the sum of what it hands the player's thruster and echo voices and scales them all by it.
class Limiter {
public:
  // aGain[i] for aIn[i]
  void Process(const float *aIn, float *aGain, uint32_t aFrames, float aRate, float aCeilingDb, float aReleaseMs);
  float Gain() const { return gain; }

private:
  float gain = 1;
};

} // namespace Out
