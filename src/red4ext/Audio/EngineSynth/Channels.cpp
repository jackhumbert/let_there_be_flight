#include "Channels.hpp"

#include "Audio/Acoustics/Engine.hpp"

#include <fmod.hpp>
#include <fmod_errors.h>
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

bool Check(FMOD_RESULT aResult, const char *aWhat) {
  if (aResult != FMOD_OK) {
    spdlog::error("[EngineSynth] {}: FMOD error {} - {}", aWhat, int(aResult), FMOD_ErrorString(aResult));
    return false;
  }
  return true;
}

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
  FMOD::DSP *source = nullptr;
  FMOD::Channel *channel = nullptr;
  // the thruster in the voice pool
  uint64_t voice = 0;
};

std::unordered_map<std::string, Thruster> thrusters;
FMOD::System *core = nullptr;
FMOD::ChannelGroup *group = nullptr;
// the synth's cost, over every block of every channel
std::atomic<uint64_t> blocks{0}, frames{0}, nanos{0}, maxNanos{0};
std::atomic<int> sampleRate{48000};
// the character and the sound, for the mixer to read without a lock
std::atomic<uint32_t> characterBits{CharacterBits(Character{})};
std::atomic<uint32_t> soundIndex{uint32_t(Tuning{}.sound)};

FMOD_RESULT F_CALL Process(FMOD_DSP_STATE *aState, unsigned int aLength, const FMOD_DSP_BUFFER_ARRAY *,
                           FMOD_DSP_BUFFER_ARRAY *aOut, FMOD_BOOL, FMOD_DSP_PROCESS_OPERATION aOperation) {
  void *userData = nullptr;
  aState->functions->getuserdata(aState, &userData);
  auto *g = static_cast<Generator *>(userData);
  if (aOperation == FMOD_DSP_PROCESS_QUERY) {
    // a generator, stereo with the same sound in both as the event's loops: the spatializer folds the
    // channels into one object (a mono source came out 6 dB under the event), and the echoes pan
    // their walls left and right
    if (aOut) {
      aOut->speakermode = FMOD_SPEAKERMODE_STEREO;
      aOut->buffernumchannels[0] = 2;
      aOut->bufferchannelmask[0] = FMOD_CHANNELMASK_STEREO;
    }
    return FMOD_OK;
  }
  float *out = aOut->buffers[0];
  if (g && g->slot >= 0 && aLength <= g->mono.size()) {
    Acoustics::Engine::ReadDry(g->slot, g->mono.data(), aLength);
    for (unsigned int i = 0; i < aLength; ++i) {
      out[2 * i] = out[2 * i + 1] = g->mono[i];
    }
    return FMOD_OK;
  }
  if (!g || !g->synth || aLength > g->mono.size()) {
    std::memset(out, 0, 2 * aLength * sizeof(float));
    return FMOD_OK;
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
  g->synth->Render(g->mono.data(), aLength);
  for (unsigned int i = 0; i < aLength; ++i) {
    out[2 * i] = out[2 * i + 1] = g->mono[i];
  }
  const uint64_t ns = uint64_t(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - t0).count());
  blocks.fetch_add(1, std::memory_order_relaxed);
  frames.fetch_add(aLength, std::memory_order_relaxed);
  nanos.fetch_add(ns, std::memory_order_relaxed);
  if (ns > maxNanos.load(std::memory_order_relaxed)) {
    maxNanos.store(ns, std::memory_order_relaxed);
  }
  return FMOD_OK;
}

FMOD_DSP_DESCRIPTION *Description() {
  static FMOD_DSP_DESCRIPTION description = [] {
    FMOD_DSP_DESCRIPTION d{};
    d.pluginsdkversion = FMOD_PLUGIN_SDK_VERSION;
    std::strncpy(d.name, "LTBF engine synth", sizeof(d.name) - 1);
    d.version = 1;
    d.numinputbuffers = 0;
    d.numoutputbuffers = 1;
    d.process = Process;
    return d;
  }();
  return &description;
}

void Release(Thruster &t) {
  if (t.channel) {
    t.channel->stop();
  }
  if (t.source) {
    t.source->release();
  }
  // the engine lets go of the slot once the channel no longer reads it
  if (t.generator && t.generator->slot >= 0) {
    Acoustics::Engine::Release(t.generator->slot);
  }
  t = {};
}

} // namespace

bool Start(FMOD::System *aCore, FMOD::ChannelGroup *aGroup, const std::string &aId, float aPitch, uint64_t aCar,
           bool aPlayer) {
  Stop(aId);
  core = aCore;
  group = aGroup;
  Voices::Init(core);
  int rate = 48000;
  core->getSoftwareFormat(&rate, nullptr, nullptr);
  sampleRate.store(rate);
  Thruster t;
  t.generator = std::make_unique<Generator>();
  const uint32_t seed = uint32_t(std::hash<std::string>()(aId));
  // the player's thrusters share the acoustics engine; a ninth, or an engine FMOD refused, plays its own
  // synth like an NPC car's
  if (aPlayer && Acoustics::Engine::Start(core, group)) {
    t.generator->slot = Acoustics::Engine::Acquire(aPitch, seed);
  }
  if (t.generator->slot < 0) {
    t.generator->synth = std::make_unique<Synth>(float(rate), aPitch, seed);
  }
  bool ok = Check(core->createDSP(Description(), &t.source), "createDSP") &&
            Check(t.source->setUserData(t.generator.get()), "setUserData") &&
            Check(core->playDSP(t.source, group, true, &t.channel), "playDSP");
  t.voice = std::hash<std::string>()(aId);
  ok = ok && Voices::Register(t.voice, aPlayer ? Voices::Kind::PlayerThruster : Voices::Kind::NpcThruster,
                              aPlayer ? 0 : aCar, true, t.channel,
                              std::pow(10.0f, Acoustics::Engine::kChannelGainDb / 20.0f));
  if (!ok) {
    Release(t);
    return false;
  }
  static bool once = false;
  if (!once) {
    once = true;
    int n = 0;
    t.channel->getNumDSPs(&n);
    std::string chain;
    for (int i = 0; i < n; ++i) {
      FMOD::DSP *dsp = nullptr;
      char name[33] = {};
      if (t.channel->getDSP(i, &dsp) == FMOD_OK) {
        dsp->getInfo(name, nullptr, nullptr, nullptr, nullptr);
      }
      chain += fmt::format("{}{}", i ? " <- " : "", name);
    }
    spdlog::info("[EngineSynth] channel chain (output first): {} <- synth, at {} Hz", chain, rate);
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
