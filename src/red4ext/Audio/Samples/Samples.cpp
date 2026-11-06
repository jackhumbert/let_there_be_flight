#include "Samples.hpp"

#include "Audio/Out/Out.hpp"
#include "Audio/Voices/Voices.hpp"
#include "Events.hpp"
#include "Player.hpp"

#include <spdlog/spdlog.h>

#include <cmath>
#include <memory>
#include <random>
#include <unordered_map>
#include <vector>

namespace Samples {

namespace {

struct Playing {
  const Event *event = nullptr;
  uint64_t voice = 0;
  std::unique_ptr<Player> player;
  std::unique_ptr<Out::Voice> out;
  float lengthSeconds = 0;
  // seconds since the start, and how far into the sound (its own seconds, at the pitch so far)
  float time = 0, position = 0;
  float randomOffset = 0;
  RED4ext::WeakHandle<RED4ext::vehicle::BaseObject> vehicle;
  Voices::Vec3 last;
  bool oneShot = false;
};

std::unordered_map<std::string, Wave> loaded;
std::unordered_map<std::string, Playing> loops;
std::vector<Playing> oneShots;
uint64_t nextVoice = 1;

void Render(void *aUser, float *aOut, uint32_t aFrames, uint32_t) {
  static_cast<Player *>(aUser)->Render(aOut, aFrames);
}

void Release(Playing &p) {
  Voices::Unregister(p.voice);
  // the voice before its player: once the voice is gone the player is not run again
  p.out.reset();
  p = {};
}

// the pitch, the tremolo and the distortion at the sound's time
void Drive(Playing &p) {
  const Event &e = *p.event;
  const float ratio = PitchRatio(PitchSemitones(e, p.time, p.randomOffset));
  p.player->Set(ratio, e.tremolo, Value(e.tremoloHz, p.time), e.duty, e.square, e.phase, e.distortion,
                Value(e.distortionLevel, p.time));
}

bool Begin(const std::string &aName, const RED4ext::Handle<RED4ext::vehicle::BaseObject> &aVehicle, bool aOneShot,
           Playing &aOut) {
  const auto it = loaded.find(aName);
  const Event *e = Find(aName);
  if (it == loaded.end() || !e) {
    spdlog::warn("[Samples] unknown or not loaded: {}", aName);
    return false;
  }
  Playing p;
  p.event = e;
  p.lengthSeconds = it->second.Seconds();
  p.oneShot = aOneShot;
  p.vehicle = aVehicle;
  if (e->randomPitch > 0.0f) {
    static std::mt19937 generator{std::random_device{}()};
    p.randomOffset = std::uniform_real_distribution<float>(-0.5f * e->randomPitch, 0.5f * e->randomPitch)(generator);
  }
  p.player = std::make_unique<Player>(it->second, float(Out::Rate()), e->loop);
  Drive(p);
  Out::Options options;
  options.bus = e->bus == Bus::Engine ? Buses::Bus::Engine : Buses::Bus::Warning;
  options.name = e->name;
  p.out = Out::Play(Render, p.player.get(), options);
  p.voice = nextVoice++;
  if (!p.out || !Voices::Register(p.voice, aOneShot ? Voices::Kind::PlayerSample : Voices::Kind::Sample, 0, false,
                                  p.out.get(), std::pow(10.0f, e->levelDb / 20.0f))) {
    Release(p);
    return false;
  }
  aOut = std::move(p);
  return true;
}

// the sound's pitch and effects and place for this frame; false when a one-shot is over
bool Step(Playing &p, float aDt) {
  p.time += aDt;
  p.position += aDt * PitchRatio(PitchSemitones(*p.event, p.time, p.randomOffset));
  // a voice the pool has paused is not read: a one-shot is over by its length
  if (p.oneShot && (p.player->Ended() || p.position >= p.lengthSeconds)) {
    return false;
  }
  Drive(p);
  if (auto vehicle = p.vehicle.Lock()) {
    const auto &position = vehicle->worldTransform.Position;
    p.last = {position.x.Bits * (1.0f / 131072.0f), position.y.Bits * (1.0f / 131072.0f),
              position.z.Bits * (1.0f / 131072.0f)};
  }
  Voices::Move(p.voice, p.last, {0, 1, 0}, {0, 0, 1}, 1.0f);
  return true;
}

} // namespace

bool Load(const std::filesystem::path &aDir) {
  StopAll();
  bool ok = true;
  for (const char *name : {"vehicle3_on", "vehicle3_off", "vehicle3_destroyed"}) {
    const Event *e = Find(name);
    Wave wave;
    if (!LoadWave(aDir / e->file, wave)) {
      spdlog::error("[Samples] could not read {}", (aDir / e->file).string());
      ok = false;
      continue;
    }
    loaded[name] = std::move(wave);
  }
  spdlog::info("[Samples] {} sounds loaded from {}", loaded.size(), aDir.string());
  return ok;
}

void PlayOneShot(const std::string &aName, const RED4ext::Handle<RED4ext::vehicle::BaseObject> &aVehicle) {
  Playing p;
  if (Begin(aName, aVehicle, true, p)) {
    oneShots.push_back(std::move(p));
  }
}

void StartLoop(const std::string &aId, const std::string &aName,
               const RED4ext::Handle<RED4ext::vehicle::BaseObject> &aVehicle) {
  StopLoop(aId);
  Playing p;
  if (Begin(aName, aVehicle, false, p)) {
    loops[aId] = std::move(p);
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
}

void Unload() {
  StopAll();
  loaded.clear();
}

} // namespace Samples
