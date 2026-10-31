#pragma once

// The engine synth in FMOD (docs/design/engine-synth.md): each thruster a Core channel playing a
// generator DSP (Synth, the same in both channels of a stereo pair as the event's loops), then the
// acoustics DSP (the echoes), the channel's fader and an Object Spatializer (Windows Sonic, as the FMOD
// event vehicle3_TPP), in the Engine bus's channel group so its volume applies. No event pitch: the
// car's pitch is the synth's, so the echoes run at the mixer's rate.
//
// Game thread: Start, Update, Stop, Pause. The mixer thread takes each thruster's newest inputs
// without waiting.

#include "Synth.hpp"

#include <string>

namespace FMOD {
namespace Studio {
class System;
} // namespace Studio
} // namespace FMOD

namespace EngineSynth::Channels {

struct Vec3 {
  float x = 0, y = 0, z = 0;
};
// a place and axes in the game's world (z up)
struct Frame {
  Vec3 position, right, forward, up;
};

// a thruster's channel, by id; aPitch the car's (StartWithPitch's). False if FMOD refused (logged).
bool Start(FMOD::Studio::System *aStudio, const std::string &aId, float aPitch);
// each frame: the thruster's frame (its facing as forward), the listener's, and its inputs
void Update(const std::string &aId, const Frame &aThruster, const Frame &aListener, const Inputs &aInputs);
void Stop(const std::string &aId);
void StopAll();
void Pause(bool aPaused);
bool Playing(const std::string &aId);

// What of the FMOD event's effects every channel keeps (Synth's Character), from now on.
void SetCharacter(const Character &aCharacter);
Character GetCharacter();

// One line for people: how many thrusters the synth plays and its character
std::string Summary();

// JSON for State(): channels, the synth's mean and longest time a block, and its share of real time
std::string State();

} // namespace EngineSynth::Channels
