#pragma once

// SetAudioInputCallbacks is not exported: it is found by its bytes in the exe's code. The function stores its three
// arguments in three consecutive globals:
//   test rcx,rcx / je / test rdx,rdx / je / mov [rip+a],rcx / mov [rip+a+8],rdx / mov [rip+a+16],r8 / ret
// A match whose three stores do not land 8 bytes apart is not taken.

#include <cstdint>
#include <span>
#include <vector>

namespace Wwise {

struct SetterMatch {
  size_t offset;    // in the code searched
  uint64_t globals; // the execute callback's global; format and gain follow 8 and 16 bytes on
};

// every match in aCode, which sits at aCodeAddress (the rip-relative stores resolve against it)
std::vector<SetterMatch> FindSetters(std::span<const uint8_t> aCode, uint64_t aCodeAddress);

} // namespace Wwise
