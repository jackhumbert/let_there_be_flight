#pragma once

// The game's area reverb on LTBF's voices (Wwise only). The game routes its own sounds into its reverbs by game-defined
// aux sends: values it sets on each of its game objects as they move between acoustic zones (the exterior default,
// the Badlands', a room's, a tunnel's: buses revb_exterior_*, revb_interior_*), through
// AK::SoundEngine::SetGameObjectAuxSendValues. Our voices are game objects of ours, which it never sets. So the
// setter is hooked (SendsHook.cpp) to learn which of its objects get sends (the Book); each frame the one nearest the
// player's car is taken as the car's, its sends read back (Query::GetGameObjectAuxSendValues) and set on our voices
// that take the game's reverb (Out::Options::gameReverb: the scrape voices), for our listener.
//
// No game or Windows types here, so tests/ can link it; the hook and the Wwise calls are in SendsHook.cpp and
// Out/WwiseBackend.cpp.

#include "Types.hpp"

#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace Wwise::Sends {

inline constexpr uint32_t kMax = 8;

// a game object's sends: up to kMax aux buses, each with its level
struct Set {
  AkAuxSendValue values[kMax]{};
  uint32_t count = 0;
  bool operator==(const Set &aOther) const;
};

// The game's objects the setter was called on, with how often; any thread (the game's audio code may call the setter
// from its own). Our own objects (IsOurs) are left out.
class Book {
public:
  void Note(AkGameObjectID aObject, const AkAuxSendValue *aValues, uint32_t aCount);
  // every object noted, oldest first
  std::vector<AkGameObjectID> Objects() const;
  void Forget(AkGameObjectID aObject);
  // calls noted since the last Calls()
  uint64_t TakeCalls();

private:
  mutable std::mutex lock;
  std::unordered_map<AkGameObjectID, uint64_t> objects; // the object, the call it was last noted in
  uint64_t calls = 0, counted = 0;
};
Book &TheBook();

// the index of the position nearest aCar within aRange (m), -1 for none
int Nearest(const std::vector<AkVector64> &aPositions, const AkVector64 &aCar, double aRange);

// the sends as our voices take them: each for aListener (ours; the game's were for its own listener, or for all)
Set ForListener(const Set &aSends, AkGameObjectID aListener);

// a bus's name for the log: the game's reverb buses by name (their IDs are Fnv1 of their names), else its ID
std::string BusName(uint32_t aBus);
// "revb_exterior_default__medium 0.80, ..." or "none"
std::string Describe(const Set &aSends);

} // namespace Wwise::Sends
