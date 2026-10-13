#pragma once
// LTBF R41 (macOS): copied from the jackhumbert fork (deps/red4ext.sdk, branch new-types); macOS changes, if any, are marked LTBF R41.

#include <cstdint>

#include <RED4ext/Common.hpp>

namespace RED4ext
{
namespace game::data
{
#pragma pack(push, 1)
struct TweakValue
{
  uint32_t hash;          // 00
  uint8_t length;         // 04
  uint8_t tdbOffsetBE[3]; // 05
  uint8_t spinLock;       // 08
  uint8_t unk09;
  uint8_t unk0A;
  uint8_t unk0B;
};
#pragma pack(pop)

RED4EXT_ASSERT_SIZE(TweakValue, 0xC);
} // namespace game::data
} // namespace RED4ext
