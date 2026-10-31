#pragma once

// Acoustics in the plugin (docs/design/acoustics.md): the flying sound's echoes off the city.
//
//   game thread     SetCar() each frame the player's car flies (FlightComponent, through
//                   FlightAudio.UpdateAcoustics), SetListener() each frame (FlightSystem's PrePhysics)
//   worker thread   probes the nav map round the car 20 times a second (Probe) and publishes Params
//                   (Targets) for the mixer
//   FMOD mixer      a DSP on each bus given to Attach() renders the echoes (Reflections), taking
//                   the newest Params at the start of each block without waiting on a lock
//
// The echoes fade in and out over 50 ms: out when SetCar() has not been called for a quarter of a
// second (the car stopped flying) or is called with gain 0 (switched off). A DSP keeps running
// while its echoes and reverb ring after the buses' sounds stop.

#include "Probe.hpp"

#include <array>
#include <string>
#include <vector>

namespace FMOD {
class System;
class DSP;
namespace Studio {
class System;
class EventInstance;
} // namespace Studio
} // namespace FMOD

namespace Acoustics {

// A DSP on each named bus's channel group, at its output end after the fader (the bus volume is in
// the sound it gets, so it scales the echoes
// too). Call after the banks load. False if a bus or the DSP could not be set up (logged).
bool Attach(FMOD::Studio::System *aStudio, const std::vector<std::string> &aBuses);
// Before the FMOD system is released: stops the worker and removes the DSPs, the events' too.
void Detach();

// A DSP just before each Object Spatializer in a started event instance (vehicle3_TPP's: those hand
// the sound to the output as a 3D object, past every bus, so a bus DSP never hears it). Returns how
// many; 0 for an event without one. DetachFromEvent before the instance is stopped and released.
int AttachToEvent(FMOD::Studio::System *aStudio, FMOD::Studio::EventInstance *aInstance);
void DetachFromEvent(FMOD::Studio::EventInstance *aInstance);

// A DSP of the same kind for a sound the caller places itself (the engine synth's channels): just
// before the sound's spatializer. The caller removes and releases it.
FMOD::DSP *CreateDsp(FMOD::System *aCore);

// Where the near rays' live hits come from.
enum class RaySource : int32_t { None = 0, Scripts = 1, PhysX = 2 };

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
void SetLiveHits(const Vec3 &aOrigin, const std::array<float, kNearRays> &aDistance,
                 const std::array<std::string, kNearRays> &aMaterial, RaySource aSource);
// how long the PhysX caster took for its last set of rays (State)
void SetPhysXCastTime(float aMilliseconds);
// A test sound in place of LTBF's FMOD sound, to hear the echoes on something plain: the master bus
// DSP plays it (at the mixer's rate, not spatialized) instead of what reaches the bus, and the event
// DSPs mute the thrusters. Off restores both.
enum class TestSource : int32_t { Off = 0, Clicks = 1, PinkNoise = 2, Sine = 3 };
void SetTestSource(TestSource aSource);

// A CSV row a probe to aPath (empty: stop): the nav map's and the live distances side by side,
// with the materials hit.
void SetLog(const std::string &aPath);

// JSON: on or off, the last features and Params, probe and DSP counts (for tests and the log)
std::string State();

} // namespace Acoustics
