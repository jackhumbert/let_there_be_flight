#include "Engine.hpp"

#include "Acoustics.hpp"
#include "Audio/Voices/Ring.hpp"
#include "Reflections.hpp"
#include "Stems.hpp"

#include <fmod.hpp>
#include <fmod_errors.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstring>
#include <memory>
#include <mutex>
#include <vector>

namespace Acoustics::Engine {

namespace {

// the most frames one mixer block may have here (buffers are sized for it at Start)
constexpr uint32_t kBlock = 8192;
constexpr uint32_t kRingSize = 8192;
// the rings' prefill: one mixer block at 1024 frames, 21 ms at 48 kHz
constexpr uint32_t kPrefill = 1024;
// the echoes fade in and out over this long, as the bus DSP's did
constexpr float kFadeSeconds = 0.05f;
// the echo voices' ids in the voice pool
constexpr uint64_t kEchoId = 0xEC40000000000000ull;
// no rolloff: the range is far past any listener distance, the spreading is in the echo's gain
constexpr float kEchoMin = 1000.0f, kEchoMax = 1001.0f;

enum SlotState : int { Free, Ready, Active, Releasing };

struct Slot {
  std::atomic<int> state{Free};
  std::unique_ptr<EngineSynth::Synth> synth;
  // the newest inputs from the game thread
  std::mutex lock;
  EngineSynth::Inputs inputs;
  bool fresh = false;
  Ring dry{kRingSize, kPrefill};
};

struct EchoVoice {
  Ring ring{kRingSize, kPrefill};
  std::vector<float> mono = std::vector<float>(kBlock);
  FMOD::DSP *dsp = nullptr;
  FMOD::Channel *channel = nullptr;
  bool registered = false;
};

struct Mixer {
  explicit Mixer(int aRate) : rate(aRate), reflections(float(aRate), int(std::lround(float(aRate) / kProbeHz))) {
    for (auto &s : streams) {
      s.assign(kBlock, 0.0f);
    }
    for (auto *v : {&dry, &mono, &fade, &stereoDry, &stereoWet}) {
      v->assign(kBlock * 2, 0.0f);
    }
  }
  int rate;
  std::array<Slot, kSlots> slots;
  std::array<EchoVoice, kEchoes> echoes;
  Reflections reflections;
  Tuning tuning;
  FMOD::DSP *bedDsp = nullptr;
  FMOD::Channel *bedChannel = nullptr;
  // the bed's state (mixer thread)
  uint32_t seq = 0;
  float amount = 0;
  bool idle = true;
  // floor, ceiling, left, right, scatter right, scatter left, reverb left, reverb right
  std::array<std::vector<float>, 8> streams;
  std::vector<float> dry, mono, fade, stereoDry, stereoWet;
  Stems::EventCursor stems;
};

std::unique_ptr<Mixer> mixer;
std::atomic<uint32_t> characterBits{EngineSynth::CharacterBits(EngineSynth::Character{})};
std::atomic<uint32_t> soundIndex{uint32_t(EngineSynth::Tuning{}.sound)};
// the synth's cost over every block rendered here
std::atomic<uint64_t> blocks{0}, frames{0}, nanos{0}, maxNanos{0};
// the bed's time for the echoes and the reverb (ProcessStreams), and its blocks
std::atomic<uint64_t> streamBlocks{0}, streamNanos{0};

bool Check(FMOD_RESULT aResult, const char *aWhat) {
  if (aResult != FMOD_OK) {
    spdlog::error("[Engine] {}: FMOD error {} - {}", aWhat, int(aResult), FMOD_ErrorString(aResult));
    return false;
  }
  return true;
}

// a ring read by a DSP that may have been paused while its writer went on: what is more than a block
// beyond the prefill is stale and dropped
void ReadCaughtUp(Ring &aRing, float *aOut, uint32_t aFrames) {
  const uint32_t keep = aFrames + kPrefill, available = aRing.Available();
  if (available > keep + aFrames) {
    aRing.Skip(available - keep);
  }
  aRing.Read(aOut, aFrames);
}

// both outputs of a generator: stereo, the same sound in both
void QueryStereo(FMOD_DSP_BUFFER_ARRAY *aOut) {
  if (aOut) {
    aOut->speakermode = FMOD_SPEAKERMODE_STEREO;
    aOut->buffernumchannels[0] = 2;
    aOut->bufferchannelmask[0] = FMOD_CHANNELMASK_STEREO;
  }
}

// The bed: the slots' synths, the engine on their sum, the rings.
FMOD_RESULT F_CALL BedProcess(FMOD_DSP_STATE *aState, unsigned int aLength, const FMOD_DSP_BUFFER_ARRAY *,
                              FMOD_DSP_BUFFER_ARRAY *aOut, FMOD_BOOL, FMOD_DSP_PROCESS_OPERATION aOperation) {
  if (aOperation == FMOD_DSP_PROCESS_QUERY) {
    QueryStereo(aOut);
    return FMOD_OK;
  }
  void *userData = nullptr;
  aState->functions->getuserdata(aState, &userData);
  auto *m = static_cast<Mixer *>(userData);
  float *out = aOut->buffers[0];
  if (!m || aLength > kBlock) {
    std::memset(out, 0, 2 * aLength * sizeof(float));
    return FMOD_OK;
  }
  const uint32_t n = aLength;
  float *dry = m->dry.data(), *mono = m->mono.data();
  std::fill(dry, dry + n, 0.0f);

  // the thrusters
  const EngineSynth::Character character = EngineSynth::CharacterFromBits(characterBits.load(std::memory_order_relaxed));
  const auto sound = EngineSynth::Sound(soundIndex.load(std::memory_order_relaxed));
  for (Slot &s : m->slots) {
    int state = s.state.load(std::memory_order_acquire);
    if (state == Free) {
      continue;
    }
    if (state == Releasing) {
      // the game thread frees the synth, not the mixer
      s.state.store(Free, std::memory_order_release);
      continue;
    }
    if (state == Ready) {
      s.dry.Resync(kPrefill);
      s.state.store(Active, std::memory_order_release);
    }
    {
      std::unique_lock guard(s.lock, std::try_to_lock);
      if (guard.owns_lock() && s.fresh) {
        s.synth->Set(s.inputs);
        s.fresh = false;
      }
    }
    s.synth->SetCharacter(character);
    s.synth->SetSound(sound);
    const auto t0 = std::chrono::steady_clock::now();
    s.synth->Render(mono, n);
    const uint64_t ns =
        uint64_t(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - t0).count());
    blocks.fetch_add(1, std::memory_order_relaxed);
    frames.fetch_add(n, std::memory_order_relaxed);
    nanos.fetch_add(ns, std::memory_order_relaxed);
    if (ns > maxNanos.load(std::memory_order_relaxed)) {
      maxNanos.store(ns, std::memory_order_relaxed);
    }
    s.dry.Write(mono, n);
    for (uint32_t i = 0; i < n; ++i) {
      dry[i] += mono[i];
    }
  }

  // the echoes and the reverb on their sum, the newest Params without waiting
  const float target = Acoustics::Level();
  const bool wanted = target > 0 || m->amount > 0;
  if (!wanted) {
    m->idle = true;
  } else if (m->idle) {
    m->reflections.Reset();
    m->seq = 0;
    m->idle = false;
  }
  Params params;
  if (wanted && Acoustics::NewestParams(m->seq, params)) {
    m->reflections.SetTargets(params);
  }
  auto &s = m->streams;
  const bool running = wanted && m->seq != 0;
  if (running) {
    const auto t0 = std::chrono::steady_clock::now();
    m->reflections.ProcessStreams(dry, {s[0].data(), s[1].data(), s[2].data(), s[3].data(), s[4].data(), s[5].data(),
                                        s[6].data(), s[7].data()}, n);
    streamNanos.fetch_add(uint64_t(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - t0).count()),
                          std::memory_order_relaxed);
    streamBlocks.fetch_add(1, std::memory_order_relaxed);
    // fade to the level, per sample
    const float step = 1.0f / (kFadeSeconds * float(m->rate));
    for (uint32_t i = 0; i < n; ++i) {
      m->amount += std::clamp(target - m->amount, -step, step);
      m->fade[i] = m->amount;
    }
    for (auto &stream : s) {
      for (uint32_t i = 0; i < n; ++i) {
        stream[i] *= m->fade[i];
      }
    }
  } else {
    for (auto &stream : s) {
      std::fill(stream.begin(), stream.begin() + n, 0.0f);
    }
  }
  for (int k = 0; k < kEchoes; ++k) {
    m->echoes[k].ring.Write(s[k].data(), n);
  }
  for (uint32_t i = 0; i < n; ++i) {
    out[2 * i] = s[6][i];
    out[2 * i + 1] = s[7][i];
  }

  // the stems: the dry sum, and the dry sum with the echoes and the reverb panned as Reflections does
  if (Stems::Recording()) {
    const Tuning &t = m->tuning;
    float *sd = m->stereoDry.data(), *sw = m->stereoWet.data();
    for (uint32_t i = 0; i < n; ++i) {
      const float mid = s[0][i] + s[1][i], l = s[2][i] + s[5][i], r = s[3][i] + s[4][i];
      sd[2 * i] = sd[2 * i + 1] = dry[i];
      sw[2 * i] = dry[i] + mid + t.pan * l + t.panOpposite * r + s[6][i];
      sw[2 * i + 1] = dry[i] + mid + t.panOpposite * l + t.pan * r + s[7][i];
    }
  }
  Stems::AddEvent(m->stems, m->stereoDry.data(), m->stereoWet.data(), n, 2);
  return FMOD_OK;
}

// An echo voice: its ring, in both channels.
FMOD_RESULT F_CALL EchoProcess(FMOD_DSP_STATE *aState, unsigned int aLength, const FMOD_DSP_BUFFER_ARRAY *,
                               FMOD_DSP_BUFFER_ARRAY *aOut, FMOD_BOOL, FMOD_DSP_PROCESS_OPERATION aOperation) {
  if (aOperation == FMOD_DSP_PROCESS_QUERY) {
    QueryStereo(aOut);
    return FMOD_OK;
  }
  void *userData = nullptr;
  aState->functions->getuserdata(aState, &userData);
  auto *echo = static_cast<EchoVoice *>(userData);
  float *out = aOut->buffers[0];
  if (!echo || aLength > kBlock) {
    std::memset(out, 0, 2 * aLength * sizeof(float));
    return FMOD_OK;
  }
  ReadCaughtUp(echo->ring, echo->mono.data(), aLength);
  for (unsigned int i = 0; i < aLength; ++i) {
    out[2 * i] = out[2 * i + 1] = echo->mono[i];
  }
  return FMOD_OK;
}

FMOD_DSP_DESCRIPTION Description(const char *aName, FMOD_DSP_PROCESS_CALLBACK aProcess) {
  FMOD_DSP_DESCRIPTION d{};
  d.pluginsdkversion = FMOD_PLUGIN_SDK_VERSION;
  std::strncpy(d.name, aName, sizeof(d.name) - 1);
  d.version = 1;
  d.numinputbuffers = 0;
  d.numoutputbuffers = 1;
  d.process = aProcess;
  return d;
}

// a generator DSP playing in aGroup, userdata aData; paused when aPaused
bool Play(FMOD::System *aCore, FMOD_DSP_DESCRIPTION *aDescription, void *aData, FMOD::ChannelGroup *aGroup, bool aPaused,
          FMOD::DSP *&aDsp, FMOD::Channel *&aChannel) {
  return Check(aCore->createDSP(aDescription, &aDsp), "createDSP") && Check(aDsp->setUserData(aData), "setUserData") &&
         Check(aCore->playDSP(aDsp, aGroup, aPaused, &aChannel), "playDSP");
}

void ReleaseDsp(FMOD::DSP *&aDsp, FMOD::Channel *&aChannel) {
  if (aChannel) {
    aChannel->stop();
    aChannel = nullptr;
  }
  if (aDsp) {
    aDsp->release();
    aDsp = nullptr;
  }
}

} // namespace

bool Start(FMOD::System *aCore, FMOD::ChannelGroup *aGroup) {
  if (mixer) {
    return true;
  }
  Voices::Init(aCore);
  int rate = 48000;
  aCore->getSoftwareFormat(&rate, nullptr, nullptr);
  auto m = std::make_unique<Mixer>(rate);
  static FMOD_DSP_DESCRIPTION bed = Description("LTBF engine bed", BedProcess);
  static FMOD_DSP_DESCRIPTION echo = Description("LTBF echo", EchoProcess);
  const float gain = std::pow(10.0f, kChannelGainDb / 20.0f);
  bool ok = Play(aCore, &bed, m.get(), aGroup, false, m->bedDsp, m->bedChannel) &&
            Check(m->bedChannel->setVolume(gain), "setVolume");
  for (int k = 0; ok && k < kEchoes; ++k) {
    EchoVoice &e = m->echoes[k];
    ok = Play(aCore, &echo, &e, aGroup, true, e.dsp, e.channel) &&
         Voices::Register(kEchoId + uint64_t(k), Voices::Kind::PlayerEcho, 0, false, e.channel, gain, kEchoMin, kEchoMax);
    e.registered = ok;
  }
  if (!ok) {
    for (int k = 0; k < kEchoes; ++k) {
      if (m->echoes[k].registered) {
        Voices::Unregister(kEchoId + uint64_t(k));
      }
      ReleaseDsp(m->echoes[k].dsp, m->echoes[k].channel);
    }
    ReleaseDsp(m->bedDsp, m->bedChannel);
    return false;
  }
  mixer = std::move(m);
  spdlog::info("[Engine] bed and {} echo voices at {} Hz, rings {} frames behind", kEchoes, rate, kPrefill);
  return true;
}

void Stop() {
  if (!mixer) {
    return;
  }
  for (int k = 0; k < kEchoes; ++k) {
    Voices::Unregister(kEchoId + uint64_t(k));
    ReleaseDsp(mixer->echoes[k].dsp, mixer->echoes[k].channel);
  }
  ReleaseDsp(mixer->bedDsp, mixer->bedChannel);
  mixer.reset();
}

int Acquire(float aPitch, uint32_t aSeed) {
  if (!mixer) {
    return -1;
  }
  for (int i = 0; i < kSlots; ++i) {
    Slot &s = mixer->slots[i];
    if (s.state.load(std::memory_order_acquire) != Free) {
      continue;
    }
    // the mixer has let go of a Free slot: its old synth goes, the new one is built here
    s.synth = std::make_unique<EngineSynth::Synth>(float(mixer->rate), aPitch, aSeed);
    {
      std::lock_guard guard(s.lock);
      s.fresh = false;
    }
    s.state.store(Ready, std::memory_order_release);
    return i;
  }
  return -1;
}

void Update(int aSlot, const EngineSynth::Inputs &aInputs) {
  if (!mixer || aSlot < 0 || aSlot >= kSlots) {
    return;
  }
  Slot &s = mixer->slots[aSlot];
  std::lock_guard guard(s.lock);
  s.inputs = aInputs;
  s.fresh = true;
}

void Release(int aSlot) {
  if (!mixer || aSlot < 0 || aSlot >= kSlots) {
    return;
  }
  mixer->slots[aSlot].state.store(Releasing, std::memory_order_release);
}

void ReadDry(int aSlot, float *aOut, uint32_t aFrames) {
  if (!mixer || aSlot < 0 || aSlot >= kSlots) {
    std::memset(aOut, 0, aFrames * sizeof(float));
    return;
  }
  ReadCaughtUp(mixer->slots[aSlot].dry, aOut, aFrames);
}

void Frame(const Voices::Vec3 &aListener) {
  if (!mixer) {
    return;
  }
  for (Slot &s : mixer->slots) {
    if (s.state.load(std::memory_order_acquire) == Free && s.synth) {
      s.synth.reset();
    }
  }
  Params p;
  const bool have = Acoustics::LastParams(p);
  const float level = have ? Acoustics::Level() : 0.0f;
  float scatterRight = 0, scatterLeft = 0;
  for (int k = 2; k < kNearRays; ++k) {
    (k < 2 + kFanRays ? scatterRight : scatterLeft) += p.scatter[k].gain;
  }
  struct Placed {
    Vec3 image;
    float gain;
  };
  const Placed placed[kEchoes] = {{p.floorImage, p.floor.gain},
                                  {p.ceilingImage, p.ceiling.gain},
                                  {p.leftImage, p.left.gain},
                                  {p.rightImage, p.right.gain},
                                  {p.scatterRightImage, scatterRight},
                                  {p.scatterLeftImage, scatterLeft}};
  for (int k = 0; k < kEchoes; ++k) {
    const Vec3 &image = placed[k].image;
    // facing the listener, up the world's
    Voices::Vec3 forward{aListener.x - image.x, aListener.y - image.y, aListener.z - image.z};
    const float length = std::sqrt(forward.x * forward.x + forward.y * forward.y + forward.z * forward.z);
    forward = length > 1e-4f ? Voices::Vec3{forward.x / length, forward.y / length, forward.z / length}
                             : Voices::Vec3{0, 1, 0};
    Voices::Move(kEchoId + uint64_t(k), {image.x, image.y, image.z}, forward, {0, 0, 1}, placed[k].gain * level);
  }
}

void SetCharacter(const EngineSynth::Character &c) {
  characterBits.store(EngineSynth::CharacterBits(c));
}

void SetSound(EngineSynth::Sound aSound) {
  soundIndex.store(uint32_t(aSound));
}

Cost GetCost() {
  return {blocks.load(), frames.load(), nanos.load(), maxNanos.load()};
}

std::string State() {
  int used = 0;
  uint64_t dryUnderruns = 0, dryOverruns = 0, echoUnderruns = 0, echoOverruns = 0;
  if (mixer) {
    for (const Slot &s : mixer->slots) {
      used += s.state.load() != Free;
      dryUnderruns += s.dry.Underruns();
      dryOverruns += s.dry.Overruns();
    }
    for (const EchoVoice &e : mixer->echoes) {
      echoUnderruns += e.ring.Underruns();
      echoOverruns += e.ring.Overruns();
    }
  }
  const uint64_t sb = streamBlocks.load();
  return fmt::format("{{\"running\":{},\"slots\":{},\"dryUnderruns\":{},\"dryOverruns\":{},\"echoUnderruns\":{},"
                     "\"echoOverruns\":{},\"streamUs\":{:.1f}}}",
                     mixer ? "true" : "false", used, dryUnderruns, dryOverruns, echoUnderruns, echoOverruns,
                     sb ? 1e-3 * double(streamNanos.load()) / double(sb) : 0.0);
}

} // namespace Acoustics::Engine
