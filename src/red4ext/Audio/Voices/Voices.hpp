#pragma once

// LTBF's 3D sounds in FMOD through one pool (Pool.hpp, docs/design/audio-native.md A.2): each sound
// is a Core channel that registers here, with an Object Spatializer added on the pool's say-so. Each
// game frame ranks them; the ones that win a Windows Sonic object play, the rest are paused (a paused
// channel's generator is not processed, so a thruster synth that loses costs nothing).
//
// Game thread only. The mixer is not touched.

#include "Pool.hpp"

#include <string>

namespace FMOD {
class Channel;
class System;
} // namespace FMOD

namespace Voices {

// a place and axes in the game's world (z up)
struct Pose {
  Vec3 position, right, forward, up;
  // the listener's velocity, for Doppler
  Vec3 velocity;
};

void Init(FMOD::System *aSystem);

// Adds the Object Spatializer to the channel and starts it paused. aBaseGain is the channel's own
// gain, under the voice's fade. False if FMOD refused (logged).
bool Register(uint64_t aId, Kind aKind, uint64_t aCar, bool aSynth, FMOD::Channel *aChannel, float aBaseGain,
              float aMinDistance = 0, float aMaxDistance = 100);
// each frame: where the sound is and how loud, before rolloff
void Move(uint64_t aId, const Vec3 &aPosition, const Vec3 &aForward, const Vec3 &aUp, float aLoudness,
          const Vec3 &aVelocity = {});
// Removes and releases the spatializer; the owner stops the channel after.
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
