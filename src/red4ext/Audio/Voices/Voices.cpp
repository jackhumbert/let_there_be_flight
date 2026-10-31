#include "Voices.hpp"

#include "Audio/Acoustics/Occlusion.hpp"
#include "Audio/EngineSynth/Channels.hpp"

#include <fmod.hpp>
#include <fmod_errors.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <cmath>
#include <map>
#include <unordered_map>
#include <vector>

namespace Voices {

namespace {

struct Entry {
  Kind kind;
  uint64_t car;
  bool synth;
  FMOD::Channel *channel;
  FMOD::DSP *pan;
  // an NPC car's low-pass for its occlusion (null for the rest)
  FMOD::DSP *lowpass;
  float baseGain, minDistance, maxDistance;
  Vec3 position, forward, up, velocity;
  float loudness = 0;
};

// what State lists, from the last Frame
struct Row {
  uint64_t id;
  Kind kind;
  float levelDb;
  Path path;
  float fade;
  bool synth;
  bool holds;
};

FMOD::System *core = nullptr;
std::unordered_map<uint64_t, Entry> entries;
Pool pool;
bool paused = false;
std::vector<Row> rows;
std::vector<uint64_t> foldedCars;
// per NPC car, how blocked it is: 0 open .. 1 behind static geometry (Acoustics/Occlusion.hpp)
std::map<uint64_t, float> occlusion;

bool Check(FMOD_RESULT aResult, const char *aWhat) {
  if (aResult != FMOD_OK) {
    spdlog::error("[Voices] {}: FMOD error {} - {}", aWhat, int(aResult), FMOD_ErrorString(aResult));
    return false;
  }
  return true;
}

float Dot(const Vec3 &a, const Vec3 &b) {
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

Vec3 Minus(const Vec3 &a, const Vec3 &b) {
  return {a.x - b.x, a.y - b.y, a.z - b.z};
}

// in the listener's own axes, FMOD's right-handed way (x right, y up, z back) as the system is set up
FMOD_VECTOR Relative(const Vec3 &aVector, const Pose &aListener) {
  return {Dot(aVector, aListener.right), Dot(aVector, aListener.up), -Dot(aVector, aListener.forward)};
}

// the game's world (z up) as FlightAudio hands it to FMOD
FMOD_VECTOR World(const Vec3 &v) {
  return {v.x, v.z, -v.y};
}

const char *KindName(Kind aKind) {
  switch (aKind) {
  case Kind::PlayerThruster:
    return "playerThruster";
  case Kind::PlayerSample:
    return "playerSample";
  case Kind::Wind:
    return "wind";
  case Kind::PlayerEcho:
    return "playerEcho";
  case Kind::NpcThruster:
    return "npcThruster";
  default:
    return "sample";
  }
}

const char *PathName(Path aPath) {
  switch (aPath) {
  case Path::Object:
    return "object";
  case Path::Folded:
    return "folded";
  default:
    return "silent";
  }
}

// Publishes the NPC cars that hold a channel to the PhysX caster, nearest kSlots of them, and smooths what
// it found per car. Off, or for a car it has no result for yet, a car is open.
void UpdateOcclusion(const std::vector<Voice> &aVoices, const Pose &aListener, float aDt) {
  struct Sum {
    Vec3 position;
    int n = 0;
  };
  std::map<uint64_t, Sum> cars;
  for (const Voice &v : aVoices) {
    const Entry &e = entries.at(v.id);
    if (e.kind != Kind::NpcThruster || !v.holdsChannel || e.car == 0) {
      continue;
    }
    Sum &s = cars[e.car];
    s.position = {s.position.x + v.position.x, s.position.y + v.position.y, s.position.z + v.position.z};
    ++s.n;
  }
  std::vector<Acoustics::Occlusion::Car> list;
  for (const auto &[car, s] : cars) {
    list.push_back({car, {s.position.x / s.n, s.position.y / s.n, s.position.z / s.n}});
  }
  const Vec3 at = aListener.position;
  auto distance2 = [&](const Acoustics::Occlusion::Car &c) {
    const float dx = c.position.x - at.x, dy = c.position.y - at.y, dz = c.position.z - at.z;
    return dx * dx + dy * dy + dz * dz;
  };
  std::sort(list.begin(), list.end(),
            [&](const auto &a, const auto &b) { return distance2(a) < distance2(b); });
  if (list.size() > size_t(Acoustics::Occlusion::kSlots)) {
    list.resize(Acoustics::Occlusion::kSlots);
  }
  const bool on = Acoustics::Occlusion::enabled.load(std::memory_order_relaxed);
  Acoustics::Occlusion::Publish({at.x, at.y, at.z}, on ? list : std::vector<Acoustics::Occlusion::Car>{});
  std::map<uint64_t, float> next;
  for (const auto &[car, _] : cars) {
    const auto old = occlusion.find(car);
    const bool blocked = on && Acoustics::Occlusion::Blocked(car) == 1;
    next[car] = Acoustics::Occlusion::Smooth(old == occlusion.end() ? 0.0f : old->second, blocked, aDt);
  }
  occlusion = std::move(next);
}

} // namespace

void Init(FMOD::System *aSystem) {
  core = aSystem;
}

bool Register(uint64_t aId, Kind aKind, uint64_t aCar, bool aSynth, FMOD::Channel *aChannel, float aBaseGain,
              float aMinDistance, float aMaxDistance) {
  Unregister(aId);
  FMOD::DSP *pan = nullptr;
  // the Object Spatializer with a linear-squared rolloff over the source's own range
  const bool ok =
      Check(core->createDSPByType(FMOD_DSP_TYPE_OBJECTPAN, &pan), "createDSPByType") &&
      Check(pan->setParameterInt(FMOD_DSP_OBJECTPAN_3D_ROLLOFF, FMOD_DSP_PAN_3D_ROLLOFF_LINEARSQUARED), "rolloff") &&
      Check(pan->setParameterBool(FMOD_DSP_OBJECTPAN_OVERRIDE_RANGE, true), "override range") &&
      Check(pan->setParameterFloat(FMOD_DSP_OBJECTPAN_3D_MIN_DISTANCE, aMinDistance), "min distance") &&
      Check(pan->setParameterFloat(FMOD_DSP_OBJECTPAN_3D_MAX_DISTANCE, aMaxDistance), "max distance") &&
      Check(pan->setParameterInt(FMOD_DSP_OBJECTPAN_3D_EXTENT_MODE, FMOD_DSP_PAN_3D_EXTENT_MODE_OFF), "extent") &&
      Check(aChannel->addDSP(FMOD_CHANNELCONTROL_DSP_HEAD, pan), "addDSP pan") &&
      Check(aChannel->setPaused(true), "setPaused");
  if (!ok) {
    if (pan) {
      aChannel->removeDSP(pan);
      pan->release();
    }
    return false;
  }
  // an NPC car's sound is low-passed while blocked: at the tail, after the generator and before the fader
  // and the object pan
  FMOD::DSP *lowpass = nullptr;
  if (aKind == Kind::NpcThruster) {
    const bool added =
        Check(core->createDSPByType(FMOD_DSP_TYPE_LOWPASS_SIMPLE, &lowpass), "createDSPByType lowpass") &&
        Check(lowpass->setParameterFloat(FMOD_DSP_LOWPASS_SIMPLE_CUTOFF, Acoustics::Occlusion::kOpenHz), "cutoff") &&
        Check(aChannel->addDSP(FMOD_CHANNELCONTROL_DSP_TAIL, lowpass), "addDSP lowpass");
    if (!added) {
      if (lowpass) {
        lowpass->release();
        lowpass = nullptr;
      }
    }
  }
  entries[aId] = {aKind, aCar, aSynth, aChannel, pan, lowpass, aBaseGain, aMinDistance, aMaxDistance, {}, {}, {}, {}, 0.0f};
  return true;
}

void Move(uint64_t aId, const Vec3 &aPosition, const Vec3 &aForward, const Vec3 &aUp, float aLoudness,
          const Vec3 &aVelocity) {
  const auto it = entries.find(aId);
  if (it == entries.end()) {
    return;
  }
  Entry &e = it->second;
  e.position = aPosition;
  e.forward = aForward;
  e.up = aUp;
  e.velocity = aVelocity;
  e.loudness = aLoudness;
}

void Unregister(uint64_t aId) {
  const auto it = entries.find(aId);
  if (it == entries.end()) {
    return;
  }
  // a DSP still on a channel is not released: off first
  it->second.channel->removeDSP(it->second.pan);
  it->second.pan->release();
  if (it->second.lowpass) {
    it->second.channel->removeDSP(it->second.lowpass);
    it->second.lowpass->release();
  }
  entries.erase(it);
}

void Frame(const Pose &aListener, float aDt) {
  std::vector<Source> sources;
  sources.reserve(entries.size());
  for (const auto &[id, e] : entries) {
    sources.push_back({id, e.car, e.kind, e.position, e.loudness, e.synth, e.minDistance, e.maxDistance});
  }
  const Vec3 listener = aListener.position;
  const std::vector<Voice> &voices = pool.Update(sources, listener, aDt);
  UpdateOcclusion(voices, aListener, aDt);
  rows.clear();
  foldedCars.clear();
  for (size_t i = 0; i < voices.size(); ++i) {
    const Voice &v = voices[i];
    const Entry &e = entries.at(v.id);
    const float level = Level(sources[i], listener);
    rows.push_back({v.id, e.kind, level > 0.0f ? 20.0f * std::log10(level) : -200.0f, v.path, v.fade, e.synth,
                    v.holdsChannel});
    if (v.path == Path::Folded) {
      foldedCars.push_back(e.car);
    }
    if (!v.holdsChannel) {
      e.channel->setPaused(true);
      continue;
    }
    float duck = 1.0f;
    if (e.kind == Kind::NpcThruster) {
      const auto o = occlusion.find(e.car);
      const float amount = o == occlusion.end() ? 0.0f : o->second;
      duck = std::pow(10.0f, Acoustics::Occlusion::DuckDb(amount) / 20.0f);
      if (e.lowpass) {
        e.lowpass->setParameterFloat(FMOD_DSP_LOWPASS_SIMPLE_CUTOFF, Acoustics::Occlusion::CutoffHz(amount));
      }
    }
    e.channel->setVolume(e.baseGain * v.fade * v.gain * duck);
    FMOD_DSP_PARAMETER_3DATTRIBUTES_MULTI attributes{};
    attributes.numlisteners = 1;
    attributes.weight[0] = 1.0f;
    attributes.relative[0].position = Relative(Minus(v.position, listener), aListener);
    attributes.relative[0].forward = Relative(e.forward, aListener);
    attributes.relative[0].up = Relative(e.up, aListener);
    attributes.relative[0].velocity = Relative(Minus(e.velocity, aListener.velocity), aListener);
    attributes.absolute.position = World(v.position);
    attributes.absolute.forward = World(e.forward);
    attributes.absolute.up = World(e.up);
    attributes.absolute.velocity = World(e.velocity);
    e.pan->setParameterData(FMOD_DSP_OBJECTPAN_3D_POSITION, &attributes, sizeof(attributes));
    if (!paused) {
      e.channel->setPaused(false);
    }
  }
}

void Pause(bool aPaused) {
  paused = aPaused;
  if (paused) {
    for (auto &[_, e] : entries) {
      e.channel->setPaused(true);
    }
  }
}

void SetBudget(int aMaxObjects, int aMaxSynth) {
  Budget b = pool.GetBudget();
  b.maxObjects = aMaxObjects;
  b.maxSynth = aMaxSynth;
  pool.SetBudget(b);
}

std::string Summary() {
  int objects = 0, silent = 0;
  for (const Row &r : rows) {
    objects += r.path == Path::Object;
    silent += r.path == Path::Silent;
  }
  return fmt::format("voices: {} objects of {}, {} folded cars, {} silent", objects, pool.GetBudget().maxObjects,
                     foldedCars.size(), silent);
}

std::string State() {
  int objects = 0, silent = 0, synthVoices = 0;
  std::string list;
  for (const Row &r : rows) {
    objects += r.path == Path::Object;
    silent += r.path == Path::Silent;
    synthVoices += r.synth && r.holds;
    list += fmt::format("{}{{\"id\":{},\"kind\":\"{}\",\"levelDb\":{:.1f},\"path\":\"{}\",\"fade\":{:.2f}}}",
                        list.empty() ? "" : ",", r.id, KindName(r.kind), r.levelDb, PathName(r.path), r.fade);
  }
  std::string occluded;
  for (const auto &[car, o] : occlusion) {
    occluded += fmt::format("{}{{\"car\":{},\"occlusion\":{:.3f}}}", occluded.empty() ? "" : ",", car, o);
  }
  std::string folded;
  for (uint64_t car : foldedCars) {
    folded += fmt::format("{}{}", folded.empty() ? "" : ",", car);
  }
  const Budget &b = pool.GetBudget();
  return fmt::format("{{\"maxObjects\":{},\"maxSynth\":{},\"sources\":{},\"objects\":{},\"folded\":[{}],\"silent\":{},"
                     "\"synthVoices\":{},\"synthCoreShare\":{:.4f},\"occlusion\":[{}],\"list\":[{}]}}",
                     b.maxObjects, b.maxSynth, rows.size(), objects, folded, silent, synthVoices,
                     synthVoices * EngineSynth::Channels::CoreShare(), occluded, list);
}

} // namespace Voices
