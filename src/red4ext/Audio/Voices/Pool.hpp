#pragma once

// The ranking behind LTBF's 3D sounds (docs/design/audio-native.md, A.2), with no FMOD or game types
// so tests/ can link it. Every source says where it is and how loud; each Update ranks them by their
// level at the listener and decides who gets a Windows Sonic object, who is folded into one voice
// per car, and who plays nothing.
//
// Game thread only; allocates.

#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace Voices {

struct Vec3 {
  float x = 0, y = 0, z = 0;
};

enum class Kind : uint8_t { PlayerThruster, PlayerSample, Wind, PlayerEcho, NpcThruster, Sample };

// under Windows Sonic's dynamic object limit
inline constexpr int kMaxObjects = 16;
// thruster synth voices that may play at once, about 1% of a core each (docs/history.md, 'Engine synth')
inline constexpr int kMaxSynth = 16;

struct Budget {
  int maxObjects = kMaxObjects;
  int maxSynth = kMaxSynth;
  // below this level (dB, linear amplitude after rolloff) a source plays nothing
  float floorDb = -60.0f;
  // a source that held an object last time ranks this much louder than it is
  float hysteresisDb = 3.0f;
  // a voice fades in and out over this long
  float fadeSeconds = 0.05f;
};

struct Source {
  uint64_t id;
  // the car a thruster belongs to (0: none), for folding an NPC car's thrusters into one voice
  uint64_t car = 0;
  Kind kind;
  Vec3 position;
  // linear amplitude before rolloff
  float loudness;
  // a thruster synth voice, limited by Budget::maxSynth
  bool synth;
  float minDistance = 0, maxDistance = 100;
};

// FMOD's linear-squared rolloff
float Rolloff(float aDistance, float aMin, float aMax);
// the source's loudness at the listener
float Level(const Source &aSource, const Vec3 &aListener);

enum class Path : uint8_t { Object, Folded, Silent };

struct Voice {
  uint64_t id;
  Path path;
  // the voice has an FMOD channel playing (an object, a folded car's lead, or fading out)
  bool holdsChannel;
  float fade;
  Vec3 position;
  // 1, except a folded car's lead: the car's level over its own
  float gain;
};

class Pool {
public:
  void SetBudget(const Budget &aBudget) { budget = aBudget; }
  const Budget &GetBudget() const { return budget; }

  // One Voice per source, in the sources' order. A source's Voice carries over from the last call
  // by id; sources not passed are forgotten.
  const std::vector<Voice> &Update(const std::vector<Source> &aSources, const Vec3 &aListener, float aDt);

private:
  struct Last {
    float fade = 0;
    bool holds = false;
  };

  Budget budget;
  std::vector<Voice> voices;
  std::unordered_map<uint64_t, Last> last;
  // who won last time: sources by id, folded cars by car id
  std::unordered_set<uint64_t> wonSources, wonCars;
};

} // namespace Voices
