#pragma once

// LTBF's own samples on FMOD Core channels (docs/design/audio-native.md A.3): the startup and off
// sounds and the destroyed alarm, each a Sound in its event's bus (Events.hpp), with the tremolo and
// distortion the event had as Core DSPs and the pitch driven each frame. They are objects at the car
// through the voice pool (Audio/Voices).
//
// Game thread only.

#include <RED4ext/Common.hpp>
#include <RED4ext/Scripting/Natives/Generated/vehicle/BaseObject.hpp>

#include <filesystem>
#include <string>

namespace FMOD {
class ChannelGroup;
class System;
} // namespace FMOD

namespace Samples {

// the sounds from aDir, played in the Engine and Warning groups; false if FMOD refused (logged)
bool Load(FMOD::System *aCore, FMOD::ChannelGroup *aEngine, FMOD::ChannelGroup *aWarning,
          const std::filesystem::path &aDir);

// a sound that plays once, from the car (the player's)
void PlayOneShot(const std::string &aName, const RED4ext::Handle<RED4ext::vehicle::BaseObject> &aVehicle);
// a sound that loops until StopLoop, from the car
void StartLoop(const std::string &aId, const std::string &aName,
               const RED4ext::Handle<RED4ext::vehicle::BaseObject> &aVehicle);
void StopLoop(const std::string &aId);

// once a game frame before Voices::Frame: the pitch and DSPs over time, the sounds to the cars, and
// the finished one-shots released
void Frame(float aDt);
// every sound stopped and released
void StopAll();

} // namespace Samples
