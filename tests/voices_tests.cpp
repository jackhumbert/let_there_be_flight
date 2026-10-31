// The voice pool (src/red4ext/Audio/Voices/Pool): rolloff, ranking, hysteresis, fades, folding an NPC car, the floor
// and the synth cap.
#include "check.hpp"

#include "Audio/Voices/Pool.hpp"

#include <cmath>
#include <vector>

using namespace Voices;

namespace {
const Vec3 kListener{};

float Linear(float aDb) {
  return std::pow(10.0f, aDb / 20.0f);
}

// a source at the listener (rolloff 1), so its loudness is its level
Source At(uint64_t aId, Kind aKind, float aDb, bool aSynth = false, uint64_t aCar = 0) {
  Source s{};
  s.id = aId;
  s.car = aCar;
  s.kind = aKind;
  s.loudness = Linear(aDb);
  s.synth = aSynth;
  return s;
}

int Count(const std::vector<Voice> &aVoices, Path aPath) {
  int n = 0;
  for (const Voice &v : aVoices) {
    n += v.path == aPath;
  }
  return n;
}
} // namespace

TEST(voices_rolloff) {
  CHECK_NEAR(Rolloff(0, 0, 100), 1.0f, 1e-6f);
  CHECK_NEAR(Rolloff(50, 0, 100), 0.25f, 1e-6f);
  CHECK_NEAR(Rolloff(100, 0, 100), 0.0f, 1e-6f);
  CHECK_NEAR(Rolloff(150, 0, 100), 0.0f, 1e-6f);
}

TEST(voices_ranking) {
  Pool pool;
  std::vector<Source> sources;
  for (int i = 0; i < 20; ++i) {
    Source s{};
    s.id = i + 1;
    s.kind = Kind::Sample;
    s.position = {1, 0, 0};
    s.loudness = 0.05f * float(i + 1);
    sources.push_back(s);
  }
  const auto &voices = pool.Update(sources, kListener, 0.02f);
  CHECK(voices.size() == 20);
  CHECK(Count(voices, Path::Object) == 16);
  CHECK(Count(voices, Path::Silent) == 4);
  for (int i = 0; i < 20; ++i) {
    CHECK((voices[i].path == Path::Object) == (i >= 4));
  }
}

TEST(voices_player_first) {
  Pool pool;
  std::vector<Source> sources;
  for (int i = 0; i < 16; ++i) {
    Source s{};
    s.id = i + 1;
    s.kind = Kind::Sample;
    s.position = {1, 0, 0};
    s.loudness = 1;
    sources.push_back(s);
  }
  Source player{};
  player.id = 100;
  player.kind = Kind::PlayerThruster;
  player.position = {90, 0, 0};
  player.loudness = 1;
  player.synth = true;
  sources.push_back(player);
  const auto &voices = pool.Update(sources, kListener, 0.02f);
  CHECK(voices[16].path == Path::Object);
  int silent = 0;
  for (int i = 0; i < 16; ++i) {
    silent += voices[i].path == Path::Silent;
  }
  CHECK(silent == 1);
}

namespace {
// A at -20 dB held for long enough to be at full fade, then joined by B
void Settle(Pool &aPool, std::vector<Source> &aSources) {
  aSources = {At(1, Kind::Sample, -20)};
  for (int i = 0; i < 4; ++i) {
    aPool.Update(aSources, kListener, 0.02f);
  }
}
Budget OneObject() {
  Budget b;
  b.maxObjects = 1;
  b.fadeSeconds = 0.05f;
  return b;
}
} // namespace

TEST(voices_hysteresis) {
  Pool pool;
  pool.SetBudget(OneObject());
  std::vector<Source> sources;
  Settle(pool, sources);
  sources.push_back(At(2, Kind::Sample, -18.5f));
  const auto &joined = pool.Update(sources, kListener, 0.02f);
  CHECK(joined[0].path == Path::Object);
  CHECK(joined[1].path == Path::Silent);
  sources[1] = At(2, Kind::Sample, -16.5f);
  const auto &swapped = pool.Update(sources, kListener, 0.02f);
  CHECK(swapped[0].path != Path::Object);
}

TEST(voices_fade) {
  Pool pool;
  pool.SetBudget(OneObject());
  std::vector<Source> sources;
  Settle(pool, sources);
  sources.push_back(At(2, Kind::Sample, -18.5f));
  pool.Update(sources, kListener, 0.02f);
  sources[1] = At(2, Kind::Sample, -16.5f);
  const float expected[3] = {0.6f, 0.2f, 0.0f};
  for (int step = 0; step < 3; ++step) {
    const auto &voices = pool.Update(sources, kListener, 0.02f);
    CHECK_NEAR(voices[0].fade, expected[step], 1e-5f);
    CHECK(voices[0].path == Path::Silent);
    // B waits for A's channel
    CHECK((voices[1].path == Path::Object) == (step == 2));
    CHECK(voices[0].holdsChannel == (step < 2));
    if (step == 2) {
      CHECK_NEAR(voices[1].fade, 0.4f, 1e-5f);
    }
  }
}

TEST(voices_fold_npc_car) {
  Pool pool;
  Budget b;
  b.maxObjects = 4;
  pool.SetBudget(b);
  std::vector<Source> sources;
  sources.push_back(At(1, Kind::PlayerThruster, -20, true));
  for (int i = 0; i < 4; ++i) {
    Source s = At(10 + i, Kind::NpcThruster, -30, true, 7);
    s.position = {float(i), 2.0f * float(i), 3.0f};
    // At() puts a source at the listener; moved, the default rolloff (min 0, max 100) would give the four
    // different levels. A min distance past all four keeps the rolloff at 1, so they stay equal.
    s.minDistance = 50.0f;
    sources.push_back(s);
  }
  sources.push_back(At(20, Kind::Sample, -50));
  sources.push_back(At(21, Kind::Sample, -51));
  const auto &voices = pool.Update(sources, kListener, 0.02f);
  CHECK(voices[0].path == Path::Object);
  CHECK(voices[1].path == Path::Folded);
  CHECK(voices[1].holdsChannel);
  CHECK_NEAR(voices[1].gain, 2.0f, 1e-5f);
  CHECK_NEAR(voices[1].position.x, 1.5f, 1e-4f);
  CHECK_NEAR(voices[1].position.y, 3.0f, 1e-4f);
  CHECK_NEAR(voices[1].position.z, 3.0f, 1e-4f);
  for (int i = 2; i < 5; ++i) {
    CHECK(voices[i].path == Path::Silent);
    CHECK(!voices[i].holdsChannel);
  }
  CHECK(voices[5].path == Path::Object);
  CHECK(voices[6].path == Path::Object);
}

TEST(voices_floor) {
  Pool pool;
  const auto &quiet = pool.Update({At(1, Kind::Sample, -61)}, kListener, 0.02f);
  CHECK(quiet[0].path == Path::Silent);
  CHECK(!quiet[0].holdsChannel);
  const auto &audible = pool.Update({At(2, Kind::Sample, -59)}, kListener, 0.02f);
  CHECK(audible[0].path == Path::Object);
}

TEST(voices_synth_cap) {
  Pool pool;
  Budget b;
  b.maxSynth = 2;
  pool.SetBudget(b);
  const auto &voices = pool.Update({At(1, Kind::Sample, -10, true), At(2, Kind::Sample, -11, true),
                                    At(3, Kind::Sample, -12, true), At(4, Kind::Sample, -13, false)},
                                   kListener, 0.02f);
  CHECK(voices[0].path == Path::Object);
  CHECK(voices[1].path == Path::Object);
  CHECK(voices[2].path == Path::Silent);
  CHECK(voices[3].path == Path::Object);
}
