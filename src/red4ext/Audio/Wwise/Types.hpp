#pragma once

// Wwise 2023.1's types as the game's exe has them (docs/research/2026-11-05-wwise-integration.md, and the spike's
// readings: docs/design/wwise-audio.md, "Where it stands"), only as far as LTBF needs them. No game or Windows
// headers, so tests/ can link everything under Audio/Wwise but Link.

#include <cstddef>
#include <cstdint>

namespace Wwise {

using AkGameObjectID = uint64_t;
using AkPlayingID = uint32_t;
using AkUniqueID = uint32_t;
using AKRESULT = int32_t;
inline constexpr AKRESULT AK_Success = 1;
// an AkAudioBuffer's state after a source filled it
inline constexpr int32_t AK_DataReady = 45;

struct AkAudioBuffer {
  float *pData; // non-interleaved: channel c at pData + c * uMaxFrames
  uint32_t channelConfig;
  int32_t eState;
  uint16_t uMaxFrames;
  uint16_t uValidFrames;
};
static_assert(offsetof(AkAudioBuffer, eState) == 0xC && offsetof(AkAudioBuffer, uMaxFrames) == 0x10 &&
              offsetof(AkAudioBuffer, uValidFrames) == 0x12);

// what the format callback fills: the rate, the channel config and the sample's bit fields
struct AkAudioFormat {
  uint32_t sampleRate;
  uint32_t channelConfig;
  uint32_t bits;
};
static_assert(sizeof(AkAudioFormat) == 12);

// an AkChannelConfig: channels (8 bits), the config type (4 bits, 1: standard) and the speaker mask (20 bits): mono
// is the centre speaker, stereo front left and right
constexpr uint32_t ChannelConfig(uint32_t aChannels) {
  return aChannels | (1u << 8) | ((aChannels == 1 ? 0x4u : 0x3u) << 12);
}
// 32-bit float samples (bits 32, block align 4, type 1 float, non-interleaved): what the game's format callback gives
inline constexpr uint32_t kFloatBits = 32u | (4u << 6) | (1u << 16) | (1u << 18);

struct AkVector {
  float x, y, z;
};
struct AkVector64 {
  double x, y, z;
};
// SetPosition's and GetListenerPosition's 48 bytes: the facing and the up (unit vectors at right angles, or
// SetPosition refuses them), then the position. Wwise is y up: the game's (x, y, z) is Wwise's (x, z, y).
struct AkWorldTransform {
  AkVector front, top;
  AkVector64 position;
};
static_assert(sizeof(AkWorldTransform) == 48);

struct AkAudioSettings {
  uint32_t samplesPerFrame, samplesPerSecond;
};

// the Audio Input plugin's three callbacks
using ExecuteFn = void (*)(AkPlayingID, AkAudioBuffer *);
using FormatFn = void (*)(AkPlayingID, AkAudioFormat &);
using GainFn = float (*)(AkPlayingID);

} // namespace Wwise
