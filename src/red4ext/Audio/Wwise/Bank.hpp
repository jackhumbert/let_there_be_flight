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

enum Type : uint8_t { kSound = 2, kAction = 3, kEvent = 4, kFxCustom = 17 };

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
};

// the IDs of a sound's objects, from its event's name (the event's is what PostEvent takes, as GetIDFromString gives)
struct Ids {
  AkUniqueID event, action, sound, input;
};
Ids IdsFor(std::string_view aEvent);

std::vector<uint8_t> Build(std::string_view aBank, std::span<const Sound> aSounds);

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
