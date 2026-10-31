#pragma once

// Acoustics in the plugin (docs/design/acoustics.md): the flying sound's echoes off the city.
//
//   game thread     SetCar() each frame the player's car flies (Inputs::Gather, from
//                   FlightComponent's update), SetListener() each frame (FlightSystem's PrePhysics)
//   worker thread   probes the nav map round the car 20 times a second (Probe) and publishes Params
//                   (Targets) for the mixer
//   FMOD mixer      a DSP on the master group given to Attach() renders the echoes (Reflections), taking
//                   the newest Params at the start of each block without waiting on a lock
//
// The echoes fade in and out over 50 ms: out when SetCar() has not been called for a quarter of a
// second (the car stopped flying) or is called with gain 0 (switched off). A DSP keeps running
// while its echoes and reverb ring after the buses' sounds stop.

#include "Probe.hpp"
#include "Reflections.hpp"

#include <array>
#include <string>
#include <vector>

namespace FMOD {
class ChannelGroup;
class System;
} // namespace FMOD

namespace Acoustics {

// probes per second: the worker's rate, and so the ramp a new Params takes to reach
inline constexpr float kProbeHz = 20.0f;

// A DSP on the master group, at its output end (the buses' volumes are in the sound it gets, so they scale
// the echoes too). False if the DSP could not be set up (logged).
bool Attach(FMOD::System *aCore, FMOD::ChannelGroup *aMaster);
// Before the FMOD system is released: stops the worker and removes the DSP.
void Detach();

// Where the near rays' live hits come from.
enum class RaySource : int32_t { None = 0, PhysX = 2 };

// The player's flying car (world, z up), the echoes' level (Tuning::gain; 0 = off), how far the
// view is inside the cabin (0 outside .. 1 first person: the echoes drop about 10 dB) and who casts
// the near rays through the game's collision.
void SetCar(const Vec3 &aPosition, const Vec3 &aForward, const Vec3 &aUp, float aGain, float aInside,
            RaySource aRays);
// For the PhysX caster (LiveRays.cpp), from any thread, with the position of a car it could cast
// from: true and the reported car's pose when it should cast now (rays from PhysX, the car fresh and
// within a few metres of aCandidate, the last cast at least a probe interval ago).
bool PhysXRaysDue(const Vec3 &aCandidate, Vec3 &aPosition, Vec3 &aForward, Vec3 &aUp);
// where the sound is heard from (world): the player's soundListener or the cinematic camera
void SetListener(const Vec3 &aPosition);

// The game's own collision along the near rays (Probe::NearRays from the car's axes), cast from
// aOrigin: distance (< 0: nothing within the cast) and the hit's physics material. Used for a
// probe within 200 ms and 10 m of them, in place of the nav map wherever a ray hit.
// Also each hit's position (world) and surface normal, for the planes (Planes.hpp).
void SetLiveHits(const Vec3 &aOrigin, const std::array<float, kNearRays> &aDistance,
                 const std::array<std::string, kNearRays> &aMaterial, const std::array<Vec3, kNearRays> &aPosition,
                 const std::array<Vec3, kNearRays> &aNormal);
// how long the PhysX caster took for its last set of rays (State)
void SetPhysXCastTime(float aMilliseconds);
// Echoes off planes fitted to the live hits of the last second (on by default); off, the nav map's and
// each cast's nearest hits seen square on.
void SetPlanes(bool aOn);
// Air absorption on the echoes and reverb (Tuning::airAbsorption), from the next probe.
void SetAirAbsorption(bool aOn);
// NPC cars heard through buildings (Occlusion.hpp; on by default). Off, no rays are cast for them and
// the cars fade back to open.
void SetOcclusion(bool aOn);

// A test sound in place of LTBF's sound, to hear the echoes on something plain: the master DSP plays it
// (at the mixer's rate, not spatialized) instead of what reaches it. Off restores it.
enum class TestSource : int32_t { Off = 0, Clicks = 1, PinkNoise = 2, Sine = 3 };
void SetTestSource(TestSource aSource);

// A CSV row a probe to aPath (empty: stop): the nav map's and the live distances side by side,
// with the materials hit.
void SetLog(const std::string &aPath);

// One line for people: echoes on or off and their level, air absorption, the test sound, where the
// near rays come from, stems recording or not (as the plugin has them now, not as asked)
std::string Summary();

// The last probe's features (copied under a lock); false when there is none or it is older than half a second.
bool LastFeatures(Features &aFeatures);

// The last Params the worker published (copied under a lock); false when there are none yet.
bool LastParams(Params &aParams);
// For the engine's mixer thread (Engine.hpp): the newest Params if there are newer than aSeq (0: none
// yet), without waiting on the worker; aSeq becomes theirs. False: nothing new, or the worker held the lock.
bool NewestParams(uint32_t &aSeq, Params &aParams);
// the echoes' level the mixer fades to (Tuning::gain, less inside the cabin; 0: off)
float Level();

// JSON: on or off, the last features and Params, probe and DSP counts (for tests and the log)
std::string State();

} // namespace Acoustics
