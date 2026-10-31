#pragma once

// One thruster's sound made in code (docs/design/engine-synth.md): a voice for each track of the FMOD
// event vehicle3_TPP, answering the same parameters through its curves (Curves.hpp), with the tone
// (oscillators, saturation, filters) and none of the space (no reverb, chorus or flanger: the
// acoustics add that).
//
//   axis voices  surge, lift, pitch, roll, sway: two detuned band-limited saws and a sine at the
//                track's fundamental times the car's pitch, moved by speed, damage and the axis,
//                then the track's chain as FMOD has it (EQ, distortion by speed and axis, gain by
//                axis, a second distortion, a low-pass following speed); yaw and lift lift two
//                bands of the surge voice. They sum into the event's Engine return (EQ,
//                distortion, low-pass and shelves: most of the sound's darkness).
//   air          pink noise band-passed, opening with thrust and speed
//   brake        two bands of hiss, the high one following brake, tremolo faster with speed
//   damage       a whirl that rises with speed, and the voices' pitch up with damage
//   scrape       crackle (sparse filtered clicks) with the wheel's spring force above 10 m/s
//   master       the cabin view's level, the water low-pass, the event's volume; the Engine return
//                louder when the thruster faces the listener (FMOD's Cone return)
//
// Parameters are smoothed (20 ms) towards the newest Set(); curves are read every 32 samples. No
// game or FMOD types: the plugin's generator DSP and the offline target (tools/acoustics/engine_synth.cpp)
// both drive it. One thread.

#include <array>
#include <cstdint>

namespace EngineSynth {

// FlightAudioUpdate's fields as FMOD gets them, and the event's volume
struct Inputs {
  float speed = 0, surge = 0, lift = 0, pitch = 0, roll = 0, sway = 0, yaw = 0;
  float brake = 0, damage = 0, inside = 0, scrape = 0, water = 0;
  float cone = 90; // FMOD's Event Cone Angle, degrees: the thruster's facing against the way to the listener
  float volume = 1;
};

enum Voice { Surge, Lift, Pitch, Roll, Sway, Air, Brake, Damage, Scrape, kVoices };
inline constexpr const char *kVoiceNames[kVoices] = {"surge", "lift", "pitch", "roll", "sway",
                                                     "air",   "brake", "damage", "scrape"};

// The synth's own constants: each voice's level (dB, on top of its curves), set against the FMOD
// event's output (tools/acoustics/engine_fit.py), and the sound's character.
struct Tuning {
  // surge, lift, pitch, roll, sway, air, brake, damage, scrape
  std::array<float, kVoices> level{1, -1, 3, -3.5f, -1, -30, 10, 0, 0};
  float master = -7.2f;     // dB
  float source = -6.0f;     // dB: the oscillators' level, as the loops' into the chain
  float detune = 0.07f;     // st between the two saws
  float sine = 0.5f;        // the sine at the fundamental against the saws
  float spread = 0.5f;      // st: each thruster's fixed random detune, up to this either way
};

class Synth {
public:
  // aPitch: the car's pitch (StartWithPitch, from its mass); aSeed: per thruster, for its detune and noise
  Synth(float aSampleRate, float aPitch, uint32_t aSeed, const Tuning &aTuning = {});

  void Set(const Inputs &aInputs);
  // aFrames mono samples to aOut; aVoices (optional): each voice's part, aFrames x kVoices interleaved
  void Render(float *aOut, uint32_t aFrames, float *aVoices = nullptr);

private:
  struct Svf {
    float ic1 = 0, ic2 = 0, a1 = 0, a2 = 0, a3 = 0, k = 1;
    void Set(float aFrequency, float aQ, float aSampleRate);
    // low-pass, band-pass
    void Run(float aIn, float &aLow, float &aBand);
  };
  // RBJ biquad, transposed direct form II
  struct Biquad {
    float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
    void Peak(float aFrequency, float aGain, float aQ, float aSampleRate);
    void LowPass(float aFrequency, float aQ, float aSampleRate);
    void Shelf(bool aHigh, float aFrequency, float aGain, float aQ, float aSampleRate);
    float Run(float x);
  };
  struct Axis {
    double phase[3] = {0, 0.33, 0.71}; // two saws and the sine
    float frequency = 0, preGain = 1, drive = 0, gain = 0, send = 0;
    Biquad eq[3];
    int bands = 0;
    Biquad lowPass;
    bool hasLowPass = false;
  };

  void Control();
  float Noise();

  float sampleRate;
  float carPitch;
  Tuning tuning;
  float detune;   // this thruster's, in st
  uint32_t rng;
  Inputs target, now;
  float smooth;   // per control block
  int controlLeft = 0;

  std::array<Axis, 5> axes;
  Svf yawBand, liftBand;
  float yawGain = 0, liftGain = 0;
  // the Engine return
  Biquad engineEq[3], engineLowPass, engineHighShelf, engineLowShelf;
  // air
  float pink[7] = {};
  Svf air;
  float airGain = 0;
  // brake
  Svf brakeHigh, brakeLow;
  float brakeGain = 0, tremoloPhase = 0, tremoloRate = 2;
  // damage
  double whirlPhase = 0;
  float whirlFrequency = 0, whirlGain = 0;
  // scrape
  Svf scrapeBand;
  float scrapeGain = 0, scrapeRate = 0, click = 0;
  // master
  Svf water;
  float masterGain = 0, waterCutoff = 22000;
  float engineDrive = 0, coneGain = 0;
};

} // namespace EngineSynth
