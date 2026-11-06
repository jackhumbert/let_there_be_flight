#pragma once

// LTBF's 3D sounds through one pool (Pool.hpp, docs/design/audio-native.md A.2): each sound is a voice of the audio
// backend (Audio/Out) that registers here. Each game frame ranks them; the ones that win a voice play, the rest are
// paused (a paused voice's generator is not run, so a thruster synth that loses costs nothing).
//
// Game thread only. The mixer is not touched.

#include "Audio/Out/Out.hpp"
#include "Pool.hpp"

#include <string>

namespace Voices {

// a place and axes in the game's world (z up), and the listener's velocity
using Pose = Out::Pose;

// The voice (paused, as Out::Play makes it) ranked from now on; its owner keeps it, and Unregisters it before it goes.
// aBaseGain is the voice's own gain, under the pool's fade; aMinDistance and aMaxDistance the rolloff's range, the
// same as the voice's Out::Options. False for a null voice.
bool Register(uint64_t aId, Kind aKind, uint64_t aCar, bool aSynth, Out::Voice *aVoice, float aBaseGain,
              float aMinDistance = 0, float aMaxDistance = 100);
// each frame: where the sound is and how loud, before rolloff
void Move(uint64_t aId, const Vec3 &aPosition, const Vec3 &aForward, const Vec3 &aUp, float aLoudness,
          const Vec3 &aVelocity = {});
void Unregister(uint64_t aId);

// once a game frame, with the listener and the seconds since the last: ranks, then plays and pauses
void Frame(const Pose &aListener, float aDt);
// pauses every voice (the game's pause); Frame leaves them paused until Pause(false)
void Pause(bool aPaused);

void SetBudget(int aMaxObjects, int aMaxSynth);

// "voices: 12 objects of 16, 1 folded cars, 3 silent"
std::string Summary();
// JSON for VoicesState: the budget, who has channels, the folded cars and the synth's cost
std::string State();

} // namespace Voices
