#pragma once

// The acoustics engine for the player's car (docs/design/audio-native.md D.9): one place where the
// car's echoes are made, from the dry sum of its thrusters, and played by voices at the echoes'
// image sources.
//
//   bed             a generator DSP in the Engine group, never paused while the player flies. It
//                   renders the player's thruster synths (the slots), writes each to its dry ring and
//                   adds it to the dry sum, runs Reflections::ProcessStreams on the sum, and writes
//                   each echo stream to an echo ring. Its own output is silent: the late reverb leaves
//                   by quarter of the horizon with the distant echoes.
//   thrusters       a slot's generator DSP (EngineSynth/Channels) reads its dry ring and is placed by
//                   the voice pool as before.
//   echo voices     ten generator DSPs reading the echo rings, registered with the voice pool at the
//                   image sources (Params::*Image) with no rolloff: the spreading is in the echo's
//                   gain, the Doppler in its delay rate. The last four are the quarters of the horizon:
//                   each its distant echo and its share of the late reverb (Params::quarterImage).
//
// The rings run one mixer block behind (their prefill), the dry and the echoes alike, so the echoes'
// delays against the thrusters stay exact. NPC cars have no echoes.
//
// Game thread: Start, Acquire, Update, Release, Frame, Stop. Mixer thread: the bed and ReadDry; they
// never wait on a lock or allocate.

#include "Audio/EngineSynth/Synth.hpp"
#include "Audio/Acoustics/Probe.hpp"
#include "Audio/Voices/Voices.hpp"

#include <cstdint>
#include <string>

namespace FMOD {
class ChannelGroup;
class System;
} // namespace FMOD

namespace Acoustics::Engine {

// player thruster slots
inline constexpr int kSlots = 8;
// the echo voices: floor, ceiling, left, right, scatter right, scatter left, and the four quarters
inline constexpr int kEchoes = 6 + kQuarters;
// the channels' own gain, the same as the thrusters' (Channels), so the echoes sit against the dry
// sound as Reflections sets them (docs/history.md)
inline constexpr float kChannelGainDb = 4.7f;

// Creates the bed and the echo voices in aGroup (the Engine bus's); once, later calls do nothing.
// False if FMOD refused (logged).
bool Start(FMOD::System *aCore, FMOD::ChannelGroup *aGroup);
// Stops them and frees the slots.
void Stop();

// A slot for a player thruster's synth (allocated here), or -1 when the engine is not running or all
// are taken. The slot renders from the next mixer block.
int Acquire(float aPitch, uint32_t aSeed);
void Update(int aSlot, const EngineSynth::Inputs &aInputs);
// The slot's thruster is gone; the mixer lets go of it, then Frame or Acquire frees its synth.
void Release(int aSlot);
// Mixer thread, the thruster's generator DSP: aFrames of the slot's dry sound (zeros if it has none yet).
void ReadDry(int aSlot, float *aOut, uint32_t aFrames);

// once a game frame, before Voices::Frame: frees the synths of slots the mixer let go, and moves
// the echo voices to their image sources (the newest Params) for a listener at aListener
void Frame(const Voices::Vec3 &aListener);

void SetCharacter(const EngineSynth::Character &aCharacter);
// what the player's thrusters' synths start from
void SetSound(EngineSynth::Sound aSound);

// the synth's cost over every block it rendered here, as Channels counts its own
struct Cost {
  uint64_t blocks = 0, frames = 0, nanos = 0, maxNanos = 0;
};
Cost GetCost();

// JSON for Channels::State: slots in use, the rings' underruns and overruns, the engine's time
std::string State();

} // namespace Acoustics::Engine
