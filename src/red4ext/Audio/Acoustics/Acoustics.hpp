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

#include <string>
#include <vector>

namespace FMOD {
class System;
namespace Studio {
class System;
}
} // namespace FMOD

namespace Acoustics {

// A DSP on each named bus's channel group, before its fader (so the bus volume scales the echoes
// too). Call after the banks load. False if a bus or the DSP could not be set up (logged).
bool Attach(FMOD::Studio::System *aStudio, const std::vector<std::string> &aBuses);
// Before the FMOD system is released: stops the worker and removes the DSPs.
void Detach();

// The player's flying car (world, z up), the echoes' level (Tuning::gain; 0 = off) and how far the
// view is inside the cabin (0 outside .. 1 first person: the echoes drop about 10 dB).
void SetCar(const Vec3 &aPosition, const Vec3 &aForward, const Vec3 &aUp, float aGain, float aInside);
// where the sound is heard from (world): the player's soundListener or the cinematic camera
void SetListener(const Vec3 &aPosition);

// JSON: on or off, the last features and Params, probe and DSP counts (for tests and the log)
std::string State();

} // namespace Acoustics
