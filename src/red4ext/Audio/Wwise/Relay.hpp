#pragma once

// The relay: our callbacks in front of the game's for the Audio Input plugin (docs/design/audio.md,
// Wwise). Wwise calls them on its audio thread for every Audio Input sound playing: ours answer the playing IDs bound
// to a source of ours and pass every other ID to the callbacks that were there before (the game's, for REDmod's
// sounds), so those keep playing. The game sets its callbacks only when it has REDmod sounds (the spike found none
// set without them), so it may set them after ours: Observe, once a frame, notices and keeps the new ones to pass on,
// and says ours must go back in.
//
// Game-free: installing (the setter found by Pattern, the globals read) is Link's.

#include "Types.hpp"

#include <atomic>
#include <cstdint>

namespace Wwise {

// one of our sounds. Called on Wwise's audio thread, so it must not lock or allocate; it must outlive its binding
// (Unbind returns once the audio thread is out of it, so it may go after that).
struct Source {
  virtual ~Source() = default;
  virtual uint32_t Channels() const { return 1; }
  // aFrames samples of each channel, channel c at aData + c * aFrames
  virtual void Render(float *aData, uint32_t aFrames, uint32_t aChannels) = 0;
};

struct Callbacks {
  ExecuteFn execute = nullptr;
  FormatFn format = nullptr;
  GainFn gain = nullptr;
};

class Relay {
public:
  static constexpr int kSlots = 128;

  // aSource answers aId from now on; false if aId is 0 or every slot is taken. Bind and Unbind from one thread (the
  // game's); the audio thread only reads. Unbind waits for a call into the source under way to end.
  bool Bind(AkPlayingID aId, Source *aSource);
  void Unbind(AkPlayingID aId);

  void Execute(AkPlayingID aId, AkAudioBuffer *aBuffer);
  void Format(AkPlayingID aId, AkAudioFormat &aFormat);
  float Gain(AkPlayingID aId);

  // the globals as they are now against ours (Trampolines()): true when ours are not in them, in which case the
  // callbacks found there are the ones passed to from now on, and ours must be installed again
  bool Observe(const Callbacks &aCurrent, const Callbacks &aOurs);
  Callbacks Passed() const;
  // the rate our sources render at (GetSampleRate; 48 kHz in the game)
  void SetRate(uint32_t aRate) { rate.store(aRate, std::memory_order_relaxed); }

  struct Stats {
    uint64_t ours = 0, passed = 0, unanswered = 0, loops = 0, formats = 0;
    uint32_t frames = 0, config = 0, observedChanges = 0;
  };
  Stats Read() const;

private:
  struct Slot {
    std::atomic<AkPlayingID> id{0};
    std::atomic<Source *> source{nullptr};
    // the audio thread is in the source
    std::atomic<bool> busy{false};
  };
  // the slot bound to aId, marked busy (Leave it after), or null
  Slot *Enter(AkPlayingID aId);
  static void Leave(Slot *aSlot) { aSlot->busy.store(false); }
  Slot slots[kSlots];
  std::atomic<ExecuteFn> passExecute{nullptr};
  std::atomic<FormatFn> passFormat{nullptr};
  std::atomic<GainFn> passGain{nullptr};
  std::atomic<uint32_t> rate{48000};
  std::atomic<uint64_t> ours{0}, passed{0}, unanswered{0}, loops{0}, formats{0};
  std::atomic<uint32_t> frames{0}, config{0}, observedChanges{0};
};

// the relay the installed callbacks answer through, and those callbacks
Relay &TheRelay();
Callbacks Trampolines();

} // namespace Wwise
