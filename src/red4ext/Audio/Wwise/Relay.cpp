#include "Relay.hpp"

namespace Wwise {

namespace {

// how deep this thread is inside a call we passed on. Another mod that put its callbacks in front of ours and passes
// to ours (as ours pass to it) would send an ID neither knows round the two for ever: a call that comes back to us
// from our own passing on goes no further.
thread_local int passing = 0;

struct Passing {
  Passing() { ++passing; }
  ~Passing() { --passing; }
};

} // namespace

bool Relay::Bind(AkPlayingID aId, Source *aSource) {
  if (aId == 0 || !aSource) {
    return false;
  }
  for (Slot &slot : slots) {
    AkPlayingID free = 0;
    if (slot.id.load(std::memory_order_relaxed) == 0) {
      // the source first, so the audio thread never sees the ID without it
      slot.source.store(aSource, std::memory_order_release);
      if (slot.id.compare_exchange_strong(free, aId, std::memory_order_acq_rel)) {
        return true;
      }
    }
  }
  return false;
}

void Relay::Unbind(AkPlayingID aId) {
  if (aId == 0) {
    return;
  }
  for (Slot &slot : slots) {
    AkPlayingID id = aId;
    if (slot.id.compare_exchange_strong(id, 0, std::memory_order_acq_rel)) {
      slot.source.store(nullptr, std::memory_order_release);
    }
  }
}

Source *Relay::Find(AkPlayingID aId) const {
  if (aId == 0) {
    return nullptr;
  }
  for (const Slot &slot : slots) {
    if (slot.id.load(std::memory_order_acquire) == aId) {
      return slot.source.load(std::memory_order_acquire);
    }
  }
  return nullptr;
}

void Relay::Execute(AkPlayingID aId, AkAudioBuffer *aBuffer) {
  if (Source *source = Find(aId)) {
    ours.fetch_add(1, std::memory_order_relaxed);
    const uint32_t n = aBuffer->uMaxFrames, channels = aBuffer->channelConfig & 0xFF;
    frames.store(n, std::memory_order_relaxed);
    config.store(aBuffer->channelConfig, std::memory_order_relaxed);
    source->Render(aBuffer->pData, n, channels ? channels : 1);
    aBuffer->uValidFrames = uint16_t(n);
    aBuffer->eState = AK_DataReady;
    return;
  }
  if (passing > 0) {
    loops.fetch_add(1, std::memory_order_relaxed);
    return;
  }
  if (const ExecuteFn fn = passExecute.load(std::memory_order_relaxed)) {
    passed.fetch_add(1, std::memory_order_relaxed);
    const Passing p;
    fn(aId, aBuffer);
    return;
  }
  // nobody's: the buffer is left as Wwise gave it (no data), as the plugin does with no callback
  unanswered.fetch_add(1, std::memory_order_relaxed);
}

void Relay::Format(AkPlayingID aId, AkAudioFormat &aFormat) {
  if (const Source *source = Find(aId)) {
    formats.fetch_add(1, std::memory_order_relaxed);
    aFormat.sampleRate = rate.load(std::memory_order_relaxed);
    aFormat.channelConfig = ChannelConfig(source->Channels());
    aFormat.bits = kFloatBits;
    return;
  }
  if (passing > 0) {
    return;
  }
  if (const FormatFn fn = passFormat.load(std::memory_order_relaxed)) {
    const Passing p;
    fn(aId, aFormat);
  }
}

float Relay::Gain(AkPlayingID aId) {
  // our gain is in our samples
  if (Find(aId) || passing > 0) {
    return 1.0f;
  }
  if (const GainFn fn = passGain.load(std::memory_order_relaxed)) {
    const Passing p;
    return fn(aId);
  }
  return 1.0f;
}

bool Relay::Observe(const Callbacks &aCurrent, const Callbacks &aOurs) {
  if (aCurrent.execute == aOurs.execute && aCurrent.format == aOurs.format && aCurrent.gain == aOurs.gain) {
    return false;
  }
  // whoever set them last (the game, or a mod) gets what is not ours; a half-set triple keeps the half that is not ours
  passExecute.store(aCurrent.execute == aOurs.execute ? passExecute.load() : aCurrent.execute);
  passFormat.store(aCurrent.format == aOurs.format ? passFormat.load() : aCurrent.format);
  passGain.store(aCurrent.gain == aOurs.gain ? passGain.load() : aCurrent.gain);
  observedChanges.fetch_add(1, std::memory_order_relaxed);
  return true;
}

Callbacks Relay::Passed() const {
  return {passExecute.load(), passFormat.load(), passGain.load()};
}

Relay::Stats Relay::Read() const {
  Stats s;
  s.ours = ours.load();
  s.passed = passed.load();
  s.unanswered = unanswered.load();
  s.loops = loops.load();
  s.formats = formats.load();
  s.frames = frames.load();
  s.config = config.load();
  s.observedChanges = observedChanges.load();
  return s;
}

Relay &TheRelay() {
  static Relay relay;
  return relay;
}

Callbacks Trampolines() {
  return {[](AkPlayingID aId, AkAudioBuffer *aBuffer) { TheRelay().Execute(aId, aBuffer); },
          [](AkPlayingID aId, AkAudioFormat &aFormat) { TheRelay().Format(aId, aFormat); },
          [](AkPlayingID aId) { return TheRelay().Gain(aId); }};
}

} // namespace Wwise
