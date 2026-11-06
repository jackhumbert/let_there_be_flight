#pragma once

// LTBF's own samples (docs/design/audio-native.md A.3): the startup and off sounds and the destroyed alarm, each
// played by a Player (its WAV, the tremolo and distortion the event had, the pitch driven each frame) on a voice of
// the audio backend (Audio/Out) in its event's bus (Events.hpp). They are at the car through the voice pool
// (Audio/Voices).
//
// Game thread only.

#include <RED4ext/Common.hpp>
#include <RED4ext/Scripting/Natives/Generated/vehicle/BaseObject.hpp>

#include <filesystem>
#include <string>

namespace Samples {

// the sounds' WAVs from aDir; false if one could not be read (logged)
bool Load(const std::filesystem::path &aDir);

// a sound that plays once, from the car (the player's)
void PlayOneShot(const std::string &aName, const RED4ext::Handle<RED4ext::vehicle::BaseObject> &aVehicle);
// a sound that loops until StopLoop, from the car
void StartLoop(const std::string &aId, const std::string &aName,
               const RED4ext::Handle<RED4ext::vehicle::BaseObject> &aVehicle);
void StopLoop(const std::string &aId);

// once a game frame before Voices::Frame: the pitch and DSPs over time, the sounds to the cars, and
// the finished one-shots released
void Frame(float aDt);
// every sound stopped
void StopAll();
// and the WAVs let go
void Unload();

} // namespace Samples
