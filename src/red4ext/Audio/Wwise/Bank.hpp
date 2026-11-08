#pragma once

// LTBF's sound bank, written in code (no Wwise Authoring, nothing shipped) and loaded with LoadBankMemoryCopy. Each of
// our sounds is an Audio Input Sound started by an event of its own, the way the game's mod.bnk does it (bank version
// 150, Wwise 2023.1; its layout read from mod.bnk):
//   FxCustom (the Audio Input plugin's parameters) <- Sound (its source) <- Action Play <- Event
// Why not mod.bnk's own events (the spike played through them): mod_sfx_street, the positioned one, has an attenuation
// that fades it out by 30 m and sends to the game's reverb. Ours are positioned with no attenuation and no sends: the
// gain, the rolloff and the reverb stay ours, in the samples (the plan: "Wwise only positions").

#include "Ids.hpp"

#include <span>
#include <string_view>
#include <vector>

namespace Wwise::Bank {

inline constexpr uint32_t kVersion = 150;
// Audio Input: company 0, plugin 200, a source
inline constexpr uint32_t kAudioInput = 0x00C80002;
// the game's Wwise project (mod.bnk's header)
inline constexpr uint32_t kProject = 476;
// the "sfx" bus: mod_sfx_street's output, under the game's SFX volume (the spike: the SFX volume at 0 silenced it)
inline constexpr AkUniqueID kSfxBus = 393239870;

enum Type : uint8_t { kSound = 2, kAction = 3, kEvent = 4, kAttenuation = 14, kFxCustom = 17 };

// a point of an attenuation curve: the distance (m) and the value (spread: percent), linear to the next
struct CurvePoint {
  float distance, value;
};

// An attenuation that only spreads: no volume, filter or focus curve, so the loudness stays ours; the spread (how
// much of the speakers the sound covers, 100% all of them) over distance. Wwise pans a point source with no spread
// hard to one side (step 1: 75 dB between the channels at 6 m): mod_sfx_street's own spread is the one taken.
struct Spread {
  std::string_view name; // its ID is Fnv1(name)
  std::span<const CurvePoint> points;
};

// a property of a node (AkPropBundle): its ID and its value's 32 bits (a float's, or an ID)
struct Prop {
  uint8_t id;
  uint32_t value;
};

struct Sound {
  std::string_view event;
  // 3D: Wwise pans it from its game object's position and facing; otherwise 2D, as rendered
  bool positioned = true;
  AkUniqueID bus = kSfxBus;
  // what mod_sfx_street has besides, for comparing ours with it in game: a parent node, properties, its attenuation on,
  // a user aux send
  AkUniqueID parent = 0;
  std::span<const Prop> props = {};
  bool attenuated = false;
  AkUniqueID aux = 0;
  // a Spread's ID (Fnv1 of its name), written as the attenuation property with the attenuation on; 0: none
  AkUniqueID spread = 0;
  // takes the game-defined aux sends its game object has (the game's area reverb, Sends.hpp); off, none
  bool gameSends = false;
};

// mod_sfx_street's spread: all the speakers within 1.7 m, a quarter at 6.8 m, a point from 30 m
inline constexpr CurvePoint kStreetSpread[] = {{0.0f, 100.0f}, {1.745f, 100.0f}, {6.766f, 25.46f}, {30.0f, 0.0f}};

// the IDs of a sound's objects, from its event's name (the event's is what PostEvent takes, as GetIDFromString gives)
struct Ids {
  AkUniqueID event, action, sound, input;
};
Ids IdsFor(std::string_view aEvent);

std::vector<uint8_t> Build(std::string_view aBank, std::span<const Sound> aSounds, std::span<const Spread> aSpreads = {});

// a bank read back: its header and its objects, ok when every size added up
struct Object {
  uint8_t type;
  AkUniqueID id;
  std::vector<uint8_t> body; // after the ID
};
struct Parsed {
  bool ok = false;
  uint32_t version = 0, bank = 0, language = 0, project = 0;
  std::vector<Object> objects;
  const Object *Find(AkUniqueID aId) const;
};
Parsed Parse(std::span<const uint8_t> aBytes);

} // namespace Wwise::Bank
