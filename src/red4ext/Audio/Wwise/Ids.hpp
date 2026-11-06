#pragma once

// Wwise's names and LTBF's game objects: the hash Wwise turns a name into an ID with, and the range of game-object IDs
// LTBF registers its voices and listener in.

#include "Types.hpp"

#include <span>
#include <string_view>

namespace Wwise {

// AK::SoundEngine::GetIDFromString: 32-bit FNV-1 of the lower-cased name ("mod_sfx_2d" is 72893892)
AkUniqueID Fnv1(std::string_view aName);

// LTBF's game objects: "LTBF" in the high 32 bits, a number of ours in the low (the spike's 0x4C54424600000001 was
// free; the game's own IDs are entity IDs and small numbers)
inline constexpr AkGameObjectID kObjectBase = 0x4C54424600000000ull;
constexpr AkGameObjectID ObjectId(uint32_t aNumber) {
  return kObjectBase | aNumber;
}
constexpr bool IsOurs(AkGameObjectID aId) {
  return (aId >> 32) == (kObjectBase >> 32);
}
// whether none of the active objects (Query::GetActiveGameObjects, before ours are registered) is in our range
bool RangeFree(std::span<const AkGameObjectID> aActive);

} // namespace Wwise
