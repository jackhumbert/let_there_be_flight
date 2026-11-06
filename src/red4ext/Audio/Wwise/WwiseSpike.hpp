#pragma once

// Step 0 of docs/design/wwise-audio.md: one tone through the game's Wwise, to answer the plan's open questions before
// anything is built on it. The game's Audio Input source plugin asks a callback for each playing sound's samples; the
// game sets that callback for REDmod's sounds. This finds the setter by its bytes, keeps the game's callbacks, puts
// ours in front (our playing ID gets a sine, every other ID goes to the game's), registers a game object of ours and
// posts one of mod.bnk's events on it. Everything else is reached through the exe's exported AK:: functions by name.
// For the bridge only (WwiseSpike.Start and the rest); nothing calls it otherwise.

#include <RED4ext/Scripting/IScriptable.hpp>
#include <RedLib.hpp>

struct WwiseSpike : RED4ext::IScriptable {
  // installs the callbacks (once), registers our game object and posts aEvent (a name, e.g. "mod_sfx_2d") on it with a
  // aFrequency Hz tone at aGain; JSON of what happened
  static RED4ext::CString Start(RED4ext::CString aEvent, float aFrequency, float aGain);
  // loads a bank from a file (LoadBankMemoryCopy): for mod.bnk when the game has not loaded it
  static RED4ext::CString LoadBank(RED4ext::CString aPath);
  // our game object at (x, y, z) facing (fx, fy, fz), in Wwise's axes (y up: the game's x, z, y)
  static RED4ext::CString Place(float aX, float aY, float aZ, float aFx, float aFy, float aFz);
  // our object heard by a listener of ours at (x, y, z) facing (fx, fy, fz) in Wwise's axes, or (aOurs false) by the
  // game's default listeners again
  static RED4ext::CString Listen(bool aOurs, float aX, float aY, float aZ, float aFx, float aFy, float aFz);
  // JSON: the callbacks' counts, the buffers seen, the format, the sample rate and frame size, the listener's transform
  // read back (both float and double readings) and ours
  static RED4ext::CString State();
  // stops the tone and unregisters our game object; the callbacks stay chained (they pass everything on)
  static RED4ext::CString Stop();

  RTTI_IMPL_TYPEINFO(WwiseSpike);
  RTTI_IMPL_ALLOCATOR();
};

RTTI_DEFINE_CLASS(WwiseSpike, {
  RTTI_METHOD(Start);
  RTTI_METHOD(LoadBank);
  RTTI_METHOD(Place);
  RTTI_METHOD(Listen);
  RTTI_METHOD(State);
  RTTI_METHOD(Stop);
});
