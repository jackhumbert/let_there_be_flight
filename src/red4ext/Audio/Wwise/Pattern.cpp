#include "Pattern.hpp"

#include <cstring>

namespace Wwise {

namespace {

// the bytes, -1 where any byte goes (the jumps' offsets and the stores' displacements)
constexpr int kPattern[] = {0x48, 0x85, 0xC9, 0x74, -1,   0x48, 0x85, 0xD2, 0x74, -1,   0x48, 0x89, 0x0D, -1, -1, -1,
                            -1,   0x48, 0x89, 0x15, -1,   -1,   -1,   -1,   0x4C, 0x89, 0x05, -1,   -1,   -1, -1, 0xC3};
constexpr size_t kLength = sizeof(kPattern) / sizeof(kPattern[0]);

} // namespace

std::vector<SetterMatch> FindSetters(std::span<const uint8_t> aCode, uint64_t aCodeAddress) {
  std::vector<SetterMatch> out;
  for (size_t i = 0; i + kLength <= aCode.size(); ++i) {
    const uint8_t *q = aCode.data() + i;
    bool same = true;
    for (size_t k = 0; k < kLength && same; ++k) {
      same = kPattern[k] < 0 || q[k] == kPattern[k];
    }
    if (!same) {
      continue;
    }
    // each store is 7 bytes; its target is the next instruction plus the displacement
    auto target = [&](size_t aAt) {
      int32_t d;
      std::memcpy(&d, q + aAt + 3, 4);
      return aCodeAddress + i + aAt + 7 + uint64_t(int64_t(d));
    };
    const uint64_t a = target(10), b = target(17), c = target(24);
    if (b == a + 8 && c == a + 16) {
      out.push_back({i, a});
    }
  }
  return out;
}

} // namespace Wwise
