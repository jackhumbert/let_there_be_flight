#pragma once

// The wind as the listener hears it (docs/design/audio-native.md A.4), with no FMOD or game types so
// tests/ can link it.
//
// Field: the city's wind where the listener is. A mean that grows with height above the floor (the
// urban power law) and drops between buildings, plus turbulence: three Ornstein-Uhlenbeck components
// whose time scale is the eddies' size over the speed the listener crosses them at (Taylor's frozen
// turbulence), so flying fast goes through gusts quickly. Passing a building edge (the shelter
// changing) stirs extra turbulence for a moment.
//
// Model: the sound of the air moving past the listener (its velocity minus the wind). Flow noise whose
// sound pressure goes as U^6 (Curle's dipole law) and whose frequencies scale as U (a Strouhal number
// over a length: the listener's head for the rumble, the car's edges for the hiss), an aeolian tone off
// the car's small parts (mirrors, aerials) stronger in crossflow, and in first person the cabin: the
// panels' mass law (6 dB more loss an octave), what leaks round the seals, the cabin's boom, and a
// seal gap's whistle that sounds only where one of its shear layer's modes (Rossiter) crosses the
// gap's resonance, so it comes and goes with speed.
//
// The sound pressure is physical; what the game plays is it compressed (Tuning::compression), since
// the real range (about 100 dB from a breeze to 300 m/s) cannot be played.
//
// Field and Model: game thread. Render: Set and Process on the mixer (Wind.cpp hands the Params over).

#include <cmath>
#include <cstdint>
#include <limits>

namespace Wind {

enum class View { Tpp = 0, Fpp = 1, Cinematic = 2 };

// a direction or velocity in the game's world (z up), m/s
struct Vec {
  float x = 0, y = 0, z = 0;
};

struct FieldTuning {
  // the mean wind at refHeight (m) over the floor in the open, and the urban power law's exponent;
  // heights are capped at maxHeight
  float refWind = 6, refHeight = 10, exponent = 0.3f, maxHeight = 300;
  // the mean left between walls with no open sky (a street canyon), against the open; a side wall
  // within wallShelter (m) halves it again at the wall
  float canyon = 0.35f, wallShelter = 30;
  // turbulence intensity (sigma over the mean, each horizontal component; vertical half of it), the
  // eddies' length scale (m) as a share of the height over the floor, within its bounds
  float intensity = 0.25f, scaleShare = 0.7f, minScale = 10, maxScale = 80;
  // the prevailing direction drifts over drift seconds by up to driftDegrees
  float drift = 120, driftDegrees = 20;
  // a change of shelter stirs stir times its size of extra turbulence, which settles over stirLength
  // (m) of the listener's travel
  float stir = 3, stirLength = 5;
};

struct FieldInput {
  float dt = 0;
  // the listener's speed through the city (m/s): how fast it crosses the eddies
  float crossing = 0;
  // from the probe: the floor's distance, 1 - enclosure, the nearest side wall's distance (m)
  float floorDistance = std::numeric_limits<float>::infinity();
  float openSky = 1;
  float wallDistance = std::numeric_limits<float>::infinity();
};

class Field {
public:
  Field(uint32_t aSeed, const FieldTuning &aTuning = {});

  // the wind's velocity where the listener is
  Vec Update(const FieldInput &aInput);
  // the local mean's speed, the turbulence's sigma and the stir now (for State and tests)
  float Mean() const { return mean; }
  float Sigma() const { return sigma; }
  float Stir() const { return stir; }

private:
  float Draw();

  FieldTuning tuning;
  uint32_t state;
  float heading, drift = 0;
  float gust[3] = {};
  float mean = 0, sigma = 0, stir = 0, lastShelter = -1;
};

struct Tuning {
  // the flow noise's sound pressure level (dB) for an exposed listener at refSpeed (m/s), then
  // 60 dB a decade (U^6)
  float refSpeed = 28, refSpl = 100;
  // what the game plays: (SPL - mixSpl) / compression dB under full scale, at most maxDb
  float mixSpl = 158, compression = 2.5f, maxDb = -3;
  // Strouhal number and the lengths (m) it scales over: the listener's head (the rumble's peak), the
  // car's edges (the hiss, edgeDb under the rumble), its small parts (the aeolian tone, aeolianDb
  // under, slipDb more in full crossflow)
  float strouhal = 0.2f, headScale = 0.2f, edgeScale = 0.02f, edgeDb = -10;
  float strutScale = 0.015f, aeolianDb = -28, slipDb = 8;
  // first person: the panels' surface mass (kg/m2; glass 4 mm), the seals' leak (dB, unfiltered), the
  // cabin's boom (dB under the outside flow, its Hz)
  float panelMass = 10, leakDb = -32, boomDb = -24, boomHz = 45;
  // the seal gap's whistle: its resonance (Hz), the gap's length (m), how close a mode must come (a
  // share of the resonance, one sigma) and its level (dB under the outside flow)
  float whistleHz = 1100, gapLength = 0.03f, lockWidth = 0.08f, whistleDb = -24;
};

// The flow noise's sound pressure level for air at v (m/s) past an exposed listener, dB SPL
float Spl(float v, const Tuning &aTuning = {});
// what the game plays for it, dB under full scale (-inf at rest)
float LevelDb(float v, const Tuning &aTuning = {});
// linear, exactly 0 at rest
float Level(float v, const Tuning &aTuning = {});
// how strongly the seal gap's whistle locks on at v (0..1)
float WhistleLock(float v, const Tuning &aTuning = {});

struct Input {
  View view = View::Tpp;
  // the listener's velocity through the world and the wind's where it is: the air past it is their
  // difference
  Vec velocity, wind;
  // the car's right, for which side the air comes from
  Vec right{1, 0, 0};
  float dt = 0;
};

struct Params {
  // the airspeed (m/s), the outside flow noise's level and Hz, the hiss and the aeolian tone
  float airspeed = 0, level = 0, flowHz = 10, edgeLevel = 0, edgeHz = 100, aeolianLevel = 0, aeolianHz = 500;
  // first person: the panels' low-pass (Hz, 0 = outside), the leak, the boom, the whistle
  float cabinHz = 0, leak = 0, boomLevel = 0, boomHz = 45, whistleLevel = 0, whistleHz = 1100;
  // each side's share (first person)
  float left = 1, right = 1;
};

class Model {
public:
  explicit Model(const Tuning &aTuning = {});

  Params Update(const Input &aInput) const;

private:
  Tuning tuning;
};

class Render {
public:
  Render(float aSampleRate, uint32_t aSeed);

  void Set(const Params &aParams);
  // interleaved stereo, the same sound in both channels
  void Process(float *aStereo, uint32_t aFrames);

private:
  float Noise();

  // a state variable filter; Low and Band are scaled so white noise of unit variance comes out about
  // unit RMS
  struct Svf {
    float ic1 = 0, ic2 = 0, a1 = 0, a2 = 0, a3 = 0, k = 1, lowGain = 1, bandGain = 1;
    void Design(float aHz, float aQ, float aRate);
    void Run(float aX, float &aLow, float &aBand);
  };

  float rate;
  uint32_t state;
  Params target;
  // the smoothed gains: flow, edge, aeolian, boom, whistle, the cabin's share, leak
  float gain[7] = {};
  float smoothing;
  Svf flow, edge, aeolian, boom, whistle;
  float flowHigh = 0, edgeLow = 0, cabin = 0;
};

} // namespace Wind
