#include "Pool.hpp"

#include <algorithm>
#include <cmath>

namespace Voices {

float Rolloff(float aDistance, float aMin, float aMax) {
  if (aDistance <= aMin) {
    return 1.0f;
  }
  if (aDistance >= aMax) {
    return 0.0f;
  }
  const float t = 1.0f - (aDistance - aMin) / (aMax - aMin);
  return t * t;
}

float Level(const Source &aSource, const Vec3 &aListener) {
  const float dx = aSource.position.x - aListener.x, dy = aSource.position.y - aListener.y,
              dz = aSource.position.z - aListener.z;
  return aSource.loudness * Rolloff(std::sqrt(dx * dx + dy * dy + dz * dz), aSource.minDistance, aSource.maxDistance);
}

namespace {

// something to rank: a source, or a car's folded thrusters
struct Candidate {
  uint64_t id;
  bool folded;
  // the source, or the fold's index
  size_t index;
  int tier;
  float key;
  bool synth;
};

bool Before(const Candidate &a, const Candidate &b) {
  if (a.tier != b.tier) {
    return a.tier < b.tier;
  }
  if (a.key != b.key) {
    return a.key > b.key;
  }
  return a.id < b.id;
}

int Tier(Kind aKind) {
  switch (aKind) {
  case Kind::PlayerThruster:
  case Kind::PlayerSample:
  case Kind::Wind:
    return 0;
  case Kind::PlayerEcho:
    return 1;
  default:
    return 2;
  }
}

struct Fold {
  uint64_t car = 0;
  std::vector<size_t> members;
  float level = 0;
  Vec3 centre;
  // the member with the lowest id, which carries the car's voice
  size_t lead = 0;
};

} // namespace

const std::vector<Voice> &Pool::Update(const std::vector<Source> &aSources, const Vec3 &aListener, float aDt) {
  const size_t n = aSources.size();
  const float step = aDt / std::max(budget.fadeSeconds, 1e-6f);
  const float bonus = std::pow(10.0f, budget.hysteresisDb / 20.0f);

  // (a) every voice fades out unless it wins below
  voices.assign(n, {});
  std::vector<float> level(n);
  std::vector<char> candidate(n, 0), heldBefore(n, 0);
  int fadingOut = 0;
  for (size_t i = 0; i < n; ++i) {
    const Source &s = aSources[i];
    const auto it = last.find(s.id);
    const Last before = it != last.end() ? it->second : Last{};
    const float fade = std::max(before.fade - step, 0.0f);
    voices[i] = {s.id, Path::Silent, before.holds && fade > 0.0f, fade, s.position, 1.0f};
    heldBefore[i] = before.holds;
    // (c) the level, and whether it is audible at all
    level[i] = Level(s, aListener);
    candidate[i] = level[i] > 0.0f && 20.0f * std::log10(level[i]) >= budget.floorDb;
    // (b) a fading voice that cannot win again keeps its channel until it is out
    if (voices[i].holdsChannel && !candidate[i]) {
      ++fadingOut;
    }
  }
  const int available = std::max(budget.maxObjects - fadingOut, 0);

  // (d) rank
  std::vector<Candidate> ranked;
  for (size_t i = 0; i < n; ++i) {
    if (candidate[i]) {
      const Source &s = aSources[i];
      ranked.push_back({s.id, false, i, Tier(s.kind), level[i] * (wonSources.contains(s.id) ? bonus : 1.0f), s.synth});
    }
  }
  std::sort(ranked.begin(), ranked.end(), Before);

  // (e) objects down the list while channels and synth slots last
  std::vector<char> object(n, 0);
  std::vector<Candidate> winners;
  int used = 0, synthUsed = 0;
  for (const Candidate &c : ranked) {
    if (used >= available) {
      break;
    }
    if (c.synth && synthUsed >= budget.maxSynth) {
      continue;
    }
    object[c.index] = 1;
    winners.push_back(c);
    ++used;
    synthUsed += c.synth;
  }

  // (f) an NPC car with a thruster out of the set folds into one voice at its centre
  std::unordered_map<uint64_t, Fold> cars;
  std::vector<uint64_t> carOrder;
  for (size_t i = 0; i < n; ++i) {
    const Source &s = aSources[i];
    if (s.kind != Kind::NpcThruster || s.car == 0 || !candidate[i]) {
      continue;
    }
    auto [it, added] = cars.try_emplace(s.car);
    if (added) {
      it->second.car = s.car;
      carOrder.push_back(s.car);
    }
    it->second.members.push_back(i);
  }
  std::vector<Fold> folds;
  for (uint64_t car : carOrder) {
    Fold &f = cars[car];
    if (std::all_of(f.members.begin(), f.members.end(), [&](size_t i) { return object[i]; })) {
      continue;
    }
    float sum = 0;
    Vec3 mean;
    f.lead = f.members[0];
    for (size_t i : f.members) {
      sum += level[i] * level[i];
      mean.x += aSources[i].position.x;
      mean.y += aSources[i].position.y;
      mean.z += aSources[i].position.z;
      if (aSources[i].id < aSources[f.lead].id) {
        f.lead = i;
      }
      object[i] = 0;
    }
    const float m = float(f.members.size());
    f.centre = {mean.x / m, mean.y / m, mean.z / m};
    f.level = std::sqrt(sum);
    folds.push_back(f);
  }
  std::vector<char> foldWins(folds.size(), 0);
  if (!folds.empty()) {
    // the folded cars' channels and synth slots go back, then the folded cars and the remaining
    // tier-2 sources take them in order
    winners.erase(std::remove_if(winners.begin(), winners.end(), [&](const Candidate &c) { return !object[c.index]; }),
                  winners.end());
    used = int(winners.size());
    synthUsed = 0;
    for (const Candidate &c : winners) {
      synthUsed += c.synth;
    }
    std::vector<Candidate> pool;
    for (size_t k = 0; k < folds.size(); ++k) {
      pool.push_back({folds[k].car, true, k, 2, folds[k].level * (wonCars.contains(folds[k].car) ? bonus : 1.0f), true});
    }
    for (const Candidate &c : ranked) {
      if (c.tier == 2 && !object[c.index] && aSources[c.index].kind != Kind::NpcThruster) {
        pool.push_back(c);
      }
    }
    std::sort(pool.begin(), pool.end(), Before);
    for (const Candidate &c : pool) {
      if (used >= available) {
        break;
      }
      if (c.synth && synthUsed >= budget.maxSynth) {
        continue;
      }
      if (c.folded) {
        foldWins[c.index] = 1;
      } else {
        object[c.index] = 1;
      }
      winners.push_back(c);
      ++used;
      synthUsed += c.synth;
    }
  }

  // A newcomer waits for a fading voice to give its channel up rather than going over the budget.
  int stillFading = 0;
  for (size_t i = 0; i < n; ++i) {
    bool wins = object[i] != 0;
    for (size_t k = 0; k < folds.size(); ++k) {
      wins = wins || (foldWins[k] && folds[k].lead == i);
    }
    if (voices[i].holdsChannel && candidate[i] && !wins) {
      ++stillFading;
    }
  }
  while (int(winners.size()) + stillFading > available) {
    auto newcomer = std::find_if(winners.rbegin(), winners.rend(), [&](const Candidate &c) {
      return !heldBefore[c.folded ? folds[c.index].lead : c.index];
    });
    if (newcomer == winners.rend()) {
      break;
    }
    if (newcomer->folded) {
      foldWins[newcomer->index] = 0;
    } else {
      object[newcomer->index] = 0;
    }
    winners.erase(std::next(newcomer).base());
  }

  // (g) the winners fade in
  wonSources.clear();
  wonCars.clear();
  for (size_t i = 0; i < n; ++i) {
    if (object[i]) {
      const auto it = last.find(aSources[i].id);
      voices[i].path = Path::Object;
      voices[i].holdsChannel = true;
      voices[i].fade = std::min((it != last.end() ? it->second.fade : 0.0f) + step, 1.0f);
      wonSources.insert(aSources[i].id);
    }
  }
  for (size_t k = 0; k < folds.size(); ++k) {
    const Fold &f = folds[k];
    for (size_t i : f.members) {
      voices[i].path = Path::Silent;
      voices[i].holdsChannel = false;
      voices[i].fade = 0.0f;
    }
    if (!foldWins[k]) {
      continue;
    }
    Voice &lead = voices[f.lead];
    const auto it = last.find(aSources[f.lead].id);
    lead.path = Path::Folded;
    lead.holdsChannel = true;
    lead.fade = std::min((it != last.end() ? it->second.fade : 0.0f) + step, 1.0f);
    lead.position = f.centre;
    lead.gain = std::min(f.level / level[f.lead], 4.0f);
    wonCars.insert(f.car);
  }

  last.clear();
  for (const Voice &v : voices) {
    last[v.id] = {v.fade, v.holdsChannel};
  }
  return voices;
}

} // namespace Voices
