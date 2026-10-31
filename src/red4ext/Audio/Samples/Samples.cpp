#include "Samples.hpp"

#include "Audio/Voices/Voices.hpp"
#include "Events.hpp"

#include <fmod.hpp>
#include <fmod_errors.h>
#include <spdlog/spdlog.h>

#include <cmath>
#include <random>
#include <unordered_map>
#include <vector>

namespace Samples {

namespace {

bool Check(FMOD_RESULT aResult, const char *aWhat) {
  if (aResult != FMOD_OK) {
    spdlog::error("[Samples] {}: FMOD error {} - {}", aWhat, int(aResult), FMOD_ErrorString(aResult));
    return false;
  }
  return true;
}

struct Loaded {
  const Event *event = nullptr;
  FMOD::Sound *sound = nullptr;
  float defaultFrequency = 0;
  float lengthSeconds = 0;
};

struct Playing {
  const Event *event = nullptr;
  uint64_t voice = 0;
  FMOD::Channel *channel = nullptr;
  FMOD::DSP *tremolo = nullptr, *distortion = nullptr;
  float defaultFrequency = 0, lengthSeconds = 0;
  // seconds since the start, and how far into the sound (its own seconds, at the pitch so far)
  float time = 0, position = 0;
  float randomOffset = 0;
  RED4ext::WeakHandle<RED4ext::vehicle::BaseObject> vehicle;
  Voices::Vec3 last;
  bool oneShot = false;
};

FMOD::System *core = nullptr;
FMOD::ChannelGroup *groups[2] = {};
std::unordered_map<std::string, Loaded> loaded;
std::unordered_map<std::string, Playing> loops;
std::vector<Playing> oneShots;
uint64_t nextVoice = 1;

FMOD::ChannelGroup *Group(Bus aBus) {
  return groups[aBus == Bus::Engine ? 0 : 1];
}

void Release(Playing &p) {
  Voices::Unregister(p.voice);
  // a DSP still on a channel is not released: off first, then the channel stopped
  if (p.channel) {
    for (FMOD::DSP *dsp : {p.tremolo, p.distortion}) {
      if (dsp) {
        p.channel->removeDSP(dsp);
      }
    }
    p.channel->stop();
  }
  for (FMOD::DSP *dsp : {p.tremolo, p.distortion}) {
    if (dsp) {
      dsp->release();
    }
  }
  p = {};
}

bool Begin(const std::string &aName, const RED4ext::Handle<RED4ext::vehicle::BaseObject> &aVehicle, bool aOneShot,
           Playing &aOut) {
  const auto it = loaded.find(aName);
  if (it == loaded.end()) {
    spdlog::warn("[Samples] unknown or not loaded: {}", aName);
    return false;
  }
  const Loaded &l = it->second;
  const Event &e = *l.event;
  Playing p;
  p.event = &e;
  p.defaultFrequency = l.defaultFrequency;
  p.lengthSeconds = l.lengthSeconds;
  p.oneShot = aOneShot;
  p.vehicle = aVehicle;
  if (e.randomPitch > 0.0f) {
    static std::mt19937 generator{std::random_device{}()};
    p.randomOffset = std::uniform_real_distribution<float>(-0.5f * e.randomPitch, 0.5f * e.randomPitch)(generator);
  }
  bool ok = Check(core->playSound(l.sound, Group(e.bus), true, &p.channel), "playSound");
  if (ok && e.tremolo) {
    ok = Check(core->createDSPByType(FMOD_DSP_TYPE_TREMOLO, &p.tremolo), "createDSP tremolo") &&
         Check(p.tremolo->setParameterFloat(FMOD_DSP_TREMOLO_FREQUENCY, Value(e.tremoloHz, 0.0f)), "tremolo frequency") &&
         Check(p.tremolo->setParameterFloat(FMOD_DSP_TREMOLO_DUTY, e.duty), "tremolo duty") &&
         Check(p.tremolo->setParameterFloat(FMOD_DSP_TREMOLO_SQUARE, e.square), "tremolo square") &&
         Check(p.tremolo->setParameterFloat(FMOD_DSP_TREMOLO_PHASE, e.phase), "tremolo phase") &&
         Check(p.tremolo->setParameterFloat(FMOD_DSP_TREMOLO_DEPTH, 1.0f), "tremolo depth") &&
         Check(p.channel->addDSP(FMOD_CHANNELCONTROL_DSP_TAIL, p.tremolo), "addDSP tremolo");
  }
  if (ok && e.distortion) {
    ok = Check(core->createDSPByType(FMOD_DSP_TYPE_DISTORTION, &p.distortion), "createDSP distortion") &&
         Check(p.distortion->setParameterFloat(FMOD_DSP_DISTORTION_LEVEL, Value(e.distortionLevel, 0.0f)),
               "distortion level") &&
         Check(p.channel->addDSP(FMOD_CHANNELCONTROL_DSP_TAIL, p.distortion), "addDSP distortion");
  }
  p.voice = nextVoice++;
  ok = ok && Voices::Register(p.voice, aOneShot ? Voices::Kind::PlayerSample : Voices::Kind::Sample, 0, false, p.channel,
                              std::pow(10.0f, e.levelDb / 20.0f));
  if (!ok) {
    Release(p);
    return false;
  }
  aOut = p;
  return true;
}

// the sound's pitch, DSPs and place for this frame; false when a one-shot is over
bool Step(Playing &p, float aDt) {
  const Event &e = *p.event;
  p.time += aDt;
  const float ratio = PitchRatio(PitchSemitones(e, p.time, p.randomOffset));
  p.position += aDt * ratio;
  if (p.oneShot) {
    bool playing = true;
    p.channel->isPlaying(&playing);
    // a channel the pool has paused is still playing: it is over by its length
    if (!playing || p.position >= p.lengthSeconds) {
      return false;
    }
  }
  p.channel->setFrequency(p.defaultFrequency * ratio);
  if (p.tremolo) {
    p.tremolo->setParameterFloat(FMOD_DSP_TREMOLO_FREQUENCY, Value(e.tremoloHz, p.time));
  }
  if (p.distortion) {
    p.distortion->setParameterFloat(FMOD_DSP_DISTORTION_LEVEL, Value(e.distortionLevel, p.time));
  }
  if (auto vehicle = p.vehicle.Lock()) {
    const auto &position = vehicle->worldTransform.Position;
    p.last = {position.x.Bits * (1.0f / 131072.0f), position.y.Bits * (1.0f / 131072.0f),
              position.z.Bits * (1.0f / 131072.0f)};
  }
  Voices::Move(p.voice, p.last, {0, 1, 0}, {0, 0, 1}, 1.0f);
  return true;
}

} // namespace

bool Load(FMOD::System *aCore, FMOD::ChannelGroup *aEngine, FMOD::ChannelGroup *aWarning,
          const std::filesystem::path &aDir) {
  StopAll();
  core = aCore;
  Voices::Init(core);
  groups[0] = aEngine;
  groups[1] = aWarning;
  bool ok = true;
  for (const char *name : {"vehicle3_on", "vehicle3_off", "vehicle3_destroyed"}) {
    const Event *e = Find(name);
    Loaded l;
    l.event = e;
    const std::string path = (aDir / e->file).string();
    const FMOD_MODE mode = FMOD_2D | FMOD_CREATESAMPLE | (e->loop ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF);
    if (!Check(core->createSound(path.c_str(), mode, nullptr, &l.sound), path.c_str())) {
      ok = false;
      continue;
    }
    unsigned int ms = 0;
    l.sound->getDefaults(&l.defaultFrequency, nullptr);
    l.sound->getLength(&ms, FMOD_TIMEUNIT_MS);
    l.lengthSeconds = 0.001f * float(ms);
    loaded[name] = l;
  }
  spdlog::info("[Samples] {} sounds loaded from {}", loaded.size(), aDir.string());
  return ok;
}

void PlayOneShot(const std::string &aName, const RED4ext::Handle<RED4ext::vehicle::BaseObject> &aVehicle) {
  Playing p;
  if (Begin(aName, aVehicle, true, p)) {
    oneShots.push_back(p);
  }
}

void StartLoop(const std::string &aId, const std::string &aName,
               const RED4ext::Handle<RED4ext::vehicle::BaseObject> &aVehicle) {
  StopLoop(aId);
  Playing p;
  if (Begin(aName, aVehicle, false, p)) {
    loops[aId] = p;
  }
}

void StopLoop(const std::string &aId) {
  const auto it = loops.find(aId);
  if (it == loops.end()) {
    return;
  }
  Release(it->second);
  loops.erase(it);
}

void Frame(float aDt) {
  for (auto &[_, p] : loops) {
    Step(p, aDt);
  }
  for (size_t i = 0; i < oneShots.size();) {
    if (Step(oneShots[i], aDt)) {
      ++i;
    } else {
      Release(oneShots[i]);
      oneShots.erase(oneShots.begin() + i);
    }
  }
}

void StopAll() {
  for (auto &[_, p] : loops) {
    Release(p);
  }
  loops.clear();
  for (Playing &p : oneShots) {
    Release(p);
  }
  oneShots.clear();
  for (auto &[_, l] : loaded) {
    if (l.sound) {
      l.sound->release();
    }
  }
  loaded.clear();
}

} // namespace Samples
