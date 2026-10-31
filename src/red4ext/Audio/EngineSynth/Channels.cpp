#include "Channels.hpp"

#include "Audio/Acoustics/Acoustics.hpp"

#include <fmod.hpp>
#include <fmod_errors.h>
#include <fmod_studio.hpp>
#include <spdlog/spdlog.h>

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

// the Object Spatializer as the event has it: Studio's linear-squared rolloff from 0 to 100 m (an
// inverse one from 1 m took 15 dB off at the thrusters' few metres from the camera)
constexpr float kMinDistance = 0.0f, kMaxDistance = 100.0f;
// on the channel's fader (after the echoes, so the stems still match the event's): the event reaches
// the speakers this much louder than the same sound from a channel, from somewhere in Studio's
// spatializer path not found (measured 4.5 to 5.0 dB in game, docs/history.md)
constexpr float kChannelGainDb = 4.7f;

bool Check(FMOD_RESULT aResult, const char *aWhat) {
  if (aResult != FMOD_OK) {
    spdlog::error("[EngineSynth] {}: FMOD error {} - {}", aWhat, int(aResult), FMOD_ErrorString(aResult));
    return false;
  }
  return true;
}

// what the generator DSP plays: the synth, and the newest inputs from the game thread
struct Generator {
  std::unique_ptr<Synth> synth;
  std::mutex lock;
  Inputs inputs;
  bool fresh = false;
  std::vector<float> mono = std::vector<float>(8192);
};

struct Thruster {
  std::unique_ptr<Generator> generator;
  FMOD::DSP *source = nullptr, *acoustics = nullptr, *pan = nullptr;
  FMOD::Channel *channel = nullptr;
};

std::unordered_map<std::string, Thruster> thrusters;
FMOD::System *core = nullptr;
FMOD::ChannelGroup *group = nullptr;
// the synth's cost, over every block of every channel
std::atomic<uint64_t> blocks{0}, frames{0}, nanos{0}, maxNanos{0};
std::atomic<int> sampleRate{48000};
// Character as bits (cone, distortion, darkness, grit), for the mixer to read without a lock
std::atomic<uint32_t> characterBits{0xF};

uint32_t Bits(const Character &c) {
  return uint32_t(c.cone) | uint32_t(c.distortion) << 1 | uint32_t(c.darkness) << 2 | uint32_t(c.grit) << 3;
}

Character FromBits(uint32_t b) {
  return {bool(b & 1), bool(b & 2), bool(b & 4), bool(b & 8)};
}

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
  g->synth->SetCharacter(FromBits(characterBits.load(std::memory_order_relaxed)));
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

float Dot(const Vec3 &a, const Vec3 &b) {
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

// in the listener's own axes, FMOD's right-handed way (x right, y up, z back) as the system is set up
FMOD_VECTOR Relative(const Vec3 &aVector, const Frame &aListener) {
  return {Dot(aVector, aListener.right), Dot(aVector, aListener.up), -Dot(aVector, aListener.forward)};
}

// the game's world (z up) as FlightAudio hands it to FMOD
FMOD_VECTOR World(const Vec3 &v) {
  return {v.x, v.z, -v.y};
}

void Release(Thruster &t) {
  // a DSP still on a channel is not released: off first, then the channel stopped
  if (t.channel) {
    for (FMOD::DSP *dsp : {t.pan, t.acoustics}) {
      if (dsp) {
        t.channel->removeDSP(dsp);
      }
    }
    t.channel->stop();
  }
  for (FMOD::DSP *dsp : {t.pan, t.acoustics, t.source}) {
    if (dsp) {
      dsp->release();
    }
  }
  t = {};
}

} // namespace

bool Start(FMOD::Studio::System *aStudio, const std::string &aId, float aPitch) {
  Stop(aId);
  if (!core && !Check(aStudio->getCoreSystem(&core), "getCoreSystem")) {
    return false;
  }
  if (!group) {
    FMOD::Studio::Bus *bus = nullptr;
    if (!Check(aStudio->getBus("bus:/Engine", &bus), "bus:/Engine") || !Check(bus->lockChannelGroup(), "lockChannelGroup")) {
      return false;
    }
    aStudio->flushCommands();
    if (!Check(bus->getChannelGroup(&group), "getChannelGroup")) {
      return false;
    }
  }
  int rate = 48000;
  core->getSoftwareFormat(&rate, nullptr, nullptr);
  sampleRate.store(rate);
  Thruster t;
  t.generator = std::make_unique<Generator>();
  t.generator->synth = std::make_unique<Synth>(float(rate), aPitch, uint32_t(std::hash<std::string>()(aId)));
  bool ok = Check(core->createDSP(Description(), &t.source), "createDSP") &&
            Check(t.source->setUserData(t.generator.get()), "setUserData") &&
            Check(core->playDSP(t.source, group, true, &t.channel), "playDSP");
  // the echoes just after the synth, the spatializer at the output end after the fader
  if (ok && (t.acoustics = Acoustics::CreateDsp(core))) {
    ok = Check(t.channel->addDSP(FMOD_CHANNELCONTROL_DSP_TAIL, t.acoustics), "addDSP acoustics");
  }
  ok = ok && Check(core->createDSPByType(FMOD_DSP_TYPE_OBJECTPAN, &t.pan), "createDSPByType") &&
       Check(t.pan->setParameterInt(FMOD_DSP_OBJECTPAN_3D_ROLLOFF, FMOD_DSP_PAN_3D_ROLLOFF_LINEARSQUARED), "rolloff") &&
       // our range, not the attenuation range Studio would pass (1..20 m by default)
       Check(t.pan->setParameterBool(FMOD_DSP_OBJECTPAN_OVERRIDE_RANGE, true), "override range") &&
       Check(t.pan->setParameterFloat(FMOD_DSP_OBJECTPAN_3D_MIN_DISTANCE, kMinDistance), "min distance") &&
       Check(t.pan->setParameterFloat(FMOD_DSP_OBJECTPAN_3D_MAX_DISTANCE, kMaxDistance), "max distance") &&
       Check(t.pan->setParameterInt(FMOD_DSP_OBJECTPAN_3D_EXTENT_MODE, FMOD_DSP_PAN_3D_EXTENT_MODE_OFF), "extent") &&
       Check(t.channel->addDSP(FMOD_CHANNELCONTROL_DSP_HEAD, t.pan), "addDSP pan") &&
       Check(t.channel->setVolume(std::pow(10.0f, kChannelGainDb / 20.0f)), "setVolume") &&
       Check(t.channel->setPaused(false), "setPaused");
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

void Update(const std::string &aId, const Frame &aThruster, const Frame &aListener, const Inputs &aInputs) {
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
  const Vec3 d{aThruster.position.x - aListener.position.x, aThruster.position.y - aListener.position.y,
               aThruster.position.z - aListener.position.z};
  FMOD_DSP_PARAMETER_3DATTRIBUTES_MULTI attributes{};
  attributes.numlisteners = 1;
  attributes.weight[0] = 1.0f;
  attributes.relative[0].position = Relative(d, aListener);
  attributes.relative[0].forward = Relative(aThruster.forward, aListener);
  attributes.relative[0].up = Relative(aThruster.up, aListener);
  attributes.absolute.position = World(aThruster.position);
  attributes.absolute.forward = World(aThruster.forward);
  attributes.absolute.up = World(aThruster.up);
  t.pan->setParameterData(FMOD_DSP_OBJECTPAN_3D_POSITION, &attributes, sizeof(attributes));
}

void Stop(const std::string &aId) {
  auto it = thrusters.find(aId);
  if (it == thrusters.end()) {
    return;
  }
  Release(it->second);
  thrusters.erase(it);
}

void StopAll() {
  for (auto &[_, t] : thrusters) {
    Release(t);
  }
  thrusters.clear();
}

void Pause(bool aPaused) {
  for (auto &[_, t] : thrusters) {
    t.channel->setPaused(aPaused);
  }
}

void SetCharacter(const Character &aCharacter) {
  characterBits.store(Bits(aCharacter));
}

Character GetCharacter() {
  return FromBits(characterBits.load());
}

bool Playing(const std::string &aId) {
  return thrusters.contains(aId);
}

std::string Summary() {
  const Character c = GetCharacter();
  return fmt::format("synth on {} thrusters, character cone={:d},distortion={:d},darkness={:d},grit={:d}",
                     thrusters.size(), c.cone, c.distortion, c.darkness, c.grit);
}

std::string State() {
  const uint64_t b = blocks.load(), f = frames.load(), ns = nanos.load();
  const double perBlockUs = b ? 1e-3 * double(ns) / double(b) : 0.0;
  // the share of one core a channel takes, playing in real time
  const double share = f ? double(ns) * 1e-9 * sampleRate.load() / double(f) : 0.0;
  const Character c = GetCharacter();
  return fmt::format("{{\"channels\":{},\"blocks\":{},\"blockUs\":{:.1f},\"blockUsMax\":{:.1f},\"coreShare\":{:.4f},"
                     "\"character\":\"cone={:d},distortion={:d},darkness={:d},grit={:d}\"}}",
                     thrusters.size(), b, perBlockUs, 1e-3 * double(maxNanos.load()), share, c.cone, c.distortion,
                     c.darkness, c.grit);
}

} // namespace EngineSynth::Channels
