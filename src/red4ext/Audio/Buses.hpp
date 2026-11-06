#pragma once

// The three buses of the plugin's own FMOD Core system (docs/design/audio-native.md B.5, B.6), with no FMOD
// types so tests/ can link it: what each bus's volume is, and the limiters on the Engine group and the
// master.

namespace Buses {

enum class Bus { Engine, Wind, Warning };

// the Mod Settings sliders (each bus) and the game's own volume (Inputs::GameVolume)
struct Volumes {
  float game = 1, engine = 1, wind = 0.6f, warning = 0.5f;
};

// the Engine bus raised over its setting (Jack, 2026-11-05: the thrusters 3 dB louder); the limiters after the
// fader still hold its peaks at their ceiling
inline constexpr float kEngineGainDb = 3.0f;

// the bus's setting times the game's (FlightAudio::UpdateVolume's formula), the Engine bus's raised by kEngineGainDb
float BusVolume(Bus aBus, const Volumes &aVolumes);

struct Limiter {
  float ceilingDb;
  float releaseMs;
};

// The master's (Master.xml: ceiling -12 dB, release 7.8 ms) or the Engine group's (the Metadata Group
// 5c36583b: ceiling -12 dB, FMOD's default release, 10 ms)
Limiter LimiterFor(bool aMaster);

} // namespace Buses
