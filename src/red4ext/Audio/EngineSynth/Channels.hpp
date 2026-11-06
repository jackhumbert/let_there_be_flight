#pragma once

// The engine synth's voices (docs/design/engine-synth.md): each thruster a voice of the audio backend (Audio/Out)
// playing a generator (Synth) in the Engine bus. Where it is and whether it plays at all are the voice pool's
// (Audio/Voices). The player's thrusters do not
// render: their synths run in the acoustics engine (Acoustics/Engine), which hands each generator its dry
// sound and makes the echoes from their sum; an NPC car's thrusters render their own and have no echoes.
// No event pitch: the car's pitch is the synth's.
//
// Game thread: Start, Update, Stop. The mixer thread takes each thruster's newest inputs
// without waiting.

#include "Audio/Voices/Voices.hpp"
#include "Synth.hpp"

#include <string>

namespace EngineSynth::Channels {

using Vec3 = Voices::Vec3;
// a place and axes in the game's world (z up)
using Frame = Voices::Pose;

// A thruster's voice, by id; aPitch the car's; aCar the entity it belongs to (an NPC car's thrusters fold into one
// voice), aPlayer whether it is the player's. False if the backend refused (logged).
bool Start(const std::string &aId, float aPitch, uint64_t aCar, bool aPlayer);
// each frame: the thruster's frame (its facing as forward) and its inputs
void Update(const std::string &aId, const Frame &aThruster, const Inputs &aInputs);
void Stop(const std::string &aId);
void StopAll();
bool Playing(const std::string &aId);

// What of the FMOD event's effects every channel keeps (Synth's Character), from now on.
void SetCharacter(const Character &aCharacter);
Character GetCharacter();
// What every channel's synth starts from (Sources.hpp), the player's in the acoustics engine too; at once.
void SetSound(Sound aSound);
Sound GetSound();

// One line for people: how many thrusters the synth plays and its character
std::string Summary();

// the share of one core a playing channel takes, measured over every block so far
double CoreShare();

// JSON for State(): channels, the synth's mean and longest time a block, and its share of real time
std::string State();

} // namespace EngineSynth::Channels
