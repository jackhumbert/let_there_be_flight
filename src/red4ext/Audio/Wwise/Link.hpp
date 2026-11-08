#pragma once

// The game's Wwise, reached from the plugin: the exe's exported AK::SoundEngine functions, looked up once by their
// mangled names, and the relay installed through SetAudioInputCallbacks (found by Pattern). In game only: the rest of
// Audio/Wwise is game-free and tested in tests/wwise_tests.cpp.

#include "Types.hpp"

#include <string>

namespace Wwise {

struct Link {
  AKRESULT (*registerGameObj)(AkGameObjectID, const char *) = nullptr;
  AKRESULT (*unregisterGameObj)(AkGameObjectID) = nullptr;
  AKRESULT (*setPosition)(AkGameObjectID, const AkWorldTransform &, int) = nullptr;
  AKRESULT (*setListeners)(AkGameObjectID, const AkGameObjectID *, uint32_t) = nullptr;
  // back to the default listeners (SetListeners with none leaves it none)
  AKRESULT (*resetListenersToDefault)(AkGameObjectID) = nullptr;
  AkPlayingID (*postEvent)(AkUniqueID, AkGameObjectID, uint32_t, void *, void *, uint32_t, void *, AkPlayingID) = nullptr;
  void (*stopPlayingID)(AkPlayingID, int32_t, int) = nullptr;
  AkUniqueID (*idFromString)(const char *) = nullptr;
  uint32_t (*sampleRate)() = nullptr;
  AKRESULT (*audioSettings)(AkAudioSettings &) = nullptr;
  AKRESULT (*loadBankMemoryCopy)(const void *, uint32_t, uint32_t &) = nullptr;
  AKRESULT (*listeners)(AkGameObjectID, AkGameObjectID *, uint32_t &) = nullptr;
  AKRESULT (*listenerPosition)(AkGameObjectID, AkWorldTransform &) = nullptr;
  AKRESULT (*position)(AkGameObjectID, AkWorldTransform &) = nullptr;
  // a game object's aux sends: set (the game's reverb on ours), and read back (the car's: Query, count in and out)
  AKRESULT (*setAuxSends)(AkGameObjectID, AkAuxSendValue *, uint32_t) = nullptr;
  AKRESULT (*auxSends)(AkGameObjectID, AkAuxSendValue *, uint32_t &) = nullptr;
  // the exports not found, by name; empty when every one was
  std::string missing;

  static const Link &Get();
};

// the relay's callbacks into the plugin's globals, keeping what was there to pass on; false (and why) when the setter
// is not found exactly once or did not take ours
bool InstallRelay(std::string &aWhy);
// once a frame: ours back in if the game (or a mod) set its own after us; true when it put them back
bool KeepRelayInstalled();
// the callbacks we found (the game's, or none) written back into the globals, so Wwise's audio thread does not call
// into the plugin once it is unloaded; nothing after this reaches Wwise from the relay
void UninstallRelay();

struct RelayInstall {
  bool installed = false;
  int matches = 0;
  uint64_t setter = 0, globals = 0;
  uint32_t reinstalls = 0;
};
RelayInstall RelayInstalled();

} // namespace Wwise
