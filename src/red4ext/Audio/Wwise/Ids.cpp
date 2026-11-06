#include "Ids.hpp"

#include <algorithm>

namespace Wwise {

AkUniqueID Fnv1(std::string_view aName) {
  uint32_t hash = 2166136261u;
  for (const char c : aName) {
    hash *= 16777619u;
    hash ^= uint8_t(c >= 'A' && c <= 'Z' ? c - 'A' + 'a' : c);
  }
  return hash;
}

bool RangeFree(std::span<const AkGameObjectID> aActive) {
  return std::none_of(aActive.begin(), aActive.end(), [](AkGameObjectID aId) { return IsOurs(aId); });
}

} // namespace Wwise
