#include "Channels.hpp"

#include "Audio/Acoustics/Engine.hpp"
#include "Audio/Out/Out.hpp"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstring>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace EngineSynth::Channels {

namespace {

// what the generator DSP plays: the synth and the newest inputs from the game thread, or for the
// player's thrusters the dry sound of the acoustics engine's slot (Acoustics/Engine), which renders it
struct Generator {
  std::unique_ptr<Synth> synth;
  int slot = -1;
  std::mutex lock;
  Inputs inputs;
  bool fresh = false;
  std::vector<float> mono = std::vector<float>(8192);
};

struct Thruster {
  std::unique_ptr<Generator> generator;
  std::unique_ptr<Out::Voice> out;
  // the thruster in the voice pool
  uint64_t voice = 0;
};

std::unordered_map<std::string, Thruster> thrusters;
// the synth's cost, over every block of every channel
std::atomic<uint64_t> blocks{0}, frames{0}, nanos{0}, maxNanos{0};
std::atomic<int> sampleRate{48000};
// the character and the sound, for the mixer to read without a lock
std::atomic<uint32_t> characterBits{CharacterBits(Character{})};
std::atomic<uint32_t> soundIndex{uint32_t(Tuning{}.sound)};

// the generator: the synth, or the slot's dry sound
void Render(void *aUser, float *aOut, uint32_t aFrames, uint32_t) {
  auto *g = static_cast<Generator *>(aUser);
  if (g->slot >= 0 && aFrames <= g->mono.size()) {
    Acoustics::Engine::ReadDry(g->slot, aOut, aFrames);
    return;
  }
  if (!g->synth || aFrames > g->mono.size()) {
    std::memset(aOut, 0, aFrames * sizeof(float));
    return;
  }
  {
    std::unique_lock guard(g->lock, std::try_to_lock);
    if (guard.owns_lock() && g->fresh) {
      g->synth->Set(g->inputs);
      g->fresh = false;
    }
  }
  g->synth->SetCharacter(CharacterFromBits(characterBits.load(std::memory_order_relaxed)));
  g->synth->SetSound(Sound(soundIndex.load(std::memory_order_relaxed)));
  const auto t0 = std::chrono::steady_clock::now();
  g->synth->Render(aOut, aFrames);
  const uint64_t ns = uint64_t(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - t0).count());
  blocks.fetch_add(1, std::memory_order_relaxed);
  frames.fetch_add(aFrames, std::memory_order_relaxed);
  nanos.fetch_add(ns, std::memory_order_relaxed);
  if (ns > maxNanos.load(std::memory_order_relaxed)) {
    maxNanos.store(ns, std::memory_order_relaxed);
  }
}

void Release(Thruster &t) {
  // the voice first: once it is gone its generator is not run again
  t.out.reset();
  // the engine lets go of the slot once the voice no longer reads it
  if (t.generator && t.generator->slot >= 0) {
    Acoustics::Engine::Release(t.generator->slot);
  }
  t = {};
}

} // namespace

bool Start(const std::string &aId, float aPitch, uint64_t aCar, bool aPlayer) {
  Stop(aId);
  const int rate = Out::Rate();
  sampleRate.store(rate);
  Thruster t;
  t.generator = std::make_unique<Generator>();
  const uint32_t seed = uint32_t(std::hash<std::string>()(aId));
  // the player's thrusters share the acoustics engine; a ninth, or an engine the backend refused, plays its own synth
  // like an NPC car's
  if (aPlayer && Acoustics::Engine::Start()) {
    t.generator->slot = Acoustics::Engine::Acquire(aPitch, seed);
  }
  if (t.generator->slot < 0) {
    t.generator->synth = std::make_unique<Synth>(float(rate), aPitch, seed);
  }
  Out::Options options;
  options.bus = Buses::Bus::Engine;
  // an NPC car's sound is low-passed while blocked
  options.lowpass = !aPlayer;
  options.name = aPlayer ? "LTBF thruster" : "LTBF NPC thruster";
  t.out = Out::Play(Render, t.generator.get(), options);
  t.voice = std::hash<std::string>()(aId);
  const bool ok = t.out && Voices::Register(t.voice, aPlayer ? Voices::Kind::PlayerThruster : Voices::Kind::NpcThruster,
                                            aPlayer ? 0 : aCar, true, t.out.get(),
                                            std::pow(10.0f, Acoustics::Engine::kChannelGainDb / 20.0f));
  if (!ok) {
    Release(t);
    return false;
  }
  static bool once = false;
  if (!once) {
    once = true;
    spdlog::info("[EngineSynth] thruster voices at {} Hz ({})", rate, Out::Summary());
  }
  thrusters[aId] = std::move(t);
  return true;
}

void Update(const std::string &aId, const Frame &aThruster, const Inputs &aInputs) {
  auto it = thrusters.find(aId);
  if (it == thrusters.end()) {
    return;
  }
  Thruster &t = it->second;
  {
    std::lock_guard guard(t.generator->lock);
    t.generator->inputs = aInputs;
    t.generator->fresh = true;
  }
  if (t.generator->slot >= 0) {
    Acoustics::Engine::Update(t.generator->slot, aInputs);
  }
  Voices::Move(t.voice, aThruster.position, aThruster.forward, aThruster.up, aInputs.volume);
}

void Stop(const std::string &aId) {
  auto it = thrusters.find(aId);
  if (it == thrusters.end()) {
    return;
  }
  Voices::Unregister(it->second.voice);
  Release(it->second);
  thrusters.erase(it);
}

void StopAll() {
  for (auto &[_, t] : thrusters) {
    Voices::Unregister(t.voice);
    Release(t);
  }
  thrusters.clear();
  Acoustics::Engine::Stop();
}

void SetCharacter(const Character &aCharacter) {
  characterBits.store(CharacterBits(aCharacter));
  Acoustics::Engine::SetCharacter(aCharacter);
}

Character GetCharacter() {
  return CharacterFromBits(characterBits.load());
}

void SetSound(Sound aSound) {
  soundIndex.store(uint32_t(aSound));
  Acoustics::Engine::SetSound(aSound);
}

Sound GetSound() {
  return Sound(soundIndex.load());
}

bool Playing(const std::string &aId) {
  return thrusters.contains(aId);
}

std::string Summary() {
  const Character c = GetCharacter();
  return fmt::format("synth on {} thrusters, sound {}, character cone={:d},distortion={:d},darkness={:d},grit={:d}",
                     thrusters.size(), SoundName(GetSound()), c.cone, c.distortion, c.darkness, c.grit);
}

namespace {
// the channels' own synths and the acoustics engine's, counted together
Acoustics::Engine::Cost TotalCost() {
  const Acoustics::Engine::Cost e = Acoustics::Engine::GetCost();
  return {blocks.load() + e.blocks, frames.load() + e.frames, nanos.load() + e.nanos,
          std::max(maxNanos.load(), e.maxNanos)};
}
} // namespace

double CoreShare() {
  const auto cost = TotalCost();
  return cost.frames ? double(cost.nanos) * 1e-9 * sampleRate.load() / double(cost.frames) : 0.0;
}

std::string State() {
  const auto cost = TotalCost();
  const double perBlockUs = cost.blocks ? 1e-3 * double(cost.nanos) / double(cost.blocks) : 0.0;
  const double share = CoreShare();
  const Character c = GetCharacter();
  return fmt::format("{{\"channels\":{},\"blocks\":{},\"blockUs\":{:.1f},\"blockUsMax\":{:.1f},\"coreShare\":{:.4f},"
                     "\"sound\":\"{}\",\"character\":\"cone={:d},distortion={:d},darkness={:d},grit={:d}\",\"engine\":{}}}",
                     thrusters.size(), cost.blocks, perBlockUs, 1e-3 * double(cost.maxNanos), share,
                     SoundName(GetSound()), c.cone, c.distortion, c.darkness, c.grit, Acoustics::Engine::State());
}

} // namespace EngineSynth::Channels
