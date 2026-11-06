#include "Voices.hpp"

#include "Audio/Acoustics/Occlusion.hpp"
#include "Audio/EngineSynth/Channels.hpp"

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
  Out::Voice *voice;
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

std::unordered_map<uint64_t, Entry> entries;
Pool pool;
bool paused = false;
std::vector<Row> rows;
std::vector<uint64_t> foldedCars;
// per NPC car, how blocked it is: 0 open .. 1 behind static geometry (Acoustics/Occlusion.hpp)
std::map<uint64_t, float> occlusion;

float Dot(const Vec3 &a, const Vec3 &b) {
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

Vec3 Minus(const Vec3 &a, const Vec3 &b) {
  return {a.x - b.x, a.y - b.y, a.z - b.z};
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

bool Register(uint64_t aId, Kind aKind, uint64_t aCar, bool aSynth, Out::Voice *aVoice, float aBaseGain,
              float aMinDistance, float aMaxDistance) {
  Unregister(aId);
  if (!aVoice) {
    return false;
  }
  aVoice->SetPaused(true);
  entries[aId] = {aKind, aCar, aSynth, aVoice, aBaseGain, aMinDistance, aMaxDistance, {}, {}, {}, {}, 0.0f};
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
  entries.erase(aId);
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
      e.voice->SetPaused(true);
      continue;
    }
    float duck = 1.0f;
    if (e.kind == Kind::NpcThruster) {
      const auto o = occlusion.find(e.car);
      const float amount = o == occlusion.end() ? 0.0f : o->second;
      duck = std::pow(10.0f, Acoustics::Occlusion::DuckDb(amount) / 20.0f);
      e.voice->SetLowpass(Acoustics::Occlusion::CutoffHz(amount));
    }
    e.voice->SetVolume(e.baseGain * v.fade * v.gain * duck);
    const Vec3 d = Minus(v.position, listener);
    const float distance = std::sqrt(Dot(d, d));
    e.voice->Place({v.position, e.forward, e.up, e.velocity, aListener, Rolloff(distance, e.minDistance, e.maxDistance)});
    if (!paused) {
      e.voice->SetPaused(false);
    }
  }
}

void Pause(bool aPaused) {
  paused = aPaused;
  if (paused) {
    for (auto &[_, e] : entries) {
      e.voice->SetPaused(true);
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
