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
//   scrape       a thruster dragged along a surface (ScrapeVoice), by the scrape's intensity, surface and slide
//   master       the cabin view's level, the water low-pass, the event's volume; the Engine return
//                louder when the thruster faces the listener (FMOD's Cone return)
//
// Parameters are smoothed (20 ms) towards the newest Set(); curves are read every 32 samples. No
// game or FMOD types: the plugin's generator DSP and the offline target (tools/acoustics/engine_synth.cpp)
// both drive it. One thread.

#include "Scrape.hpp"
#include "Sources.hpp"

#include <array>
#include <cstdint>

namespace EngineSynth {

// FlightAudioUpdate's fields as FMOD gets them, and the event's volume
struct Inputs {
  float speed = 0, surge = 0, lift = 0, pitch = 0, roll = 0, sway = 0, yaw = 0;
  float brake = 0, damage = 0, inside = 0, scrape = 0, water = 0;
  // the scrape's surface (Flight::Surface as a number; 0: none known, the voice as before) and its slide (m/s)
  int scrapeSurface = 0;
  float scrapeSlide = 0;
  float cone = 90; // FMOD's Event Cone Angle, degrees: the thruster's facing against the way to the listener
  float volume = 1;
};

enum Voice { Surge, Lift, Pitch, Roll, Sway, Air, Brake, Damage, Scrape, kVoices };
inline constexpr const char *kVoiceNames[kVoices] = {"surge", "lift", "pitch", "roll", "sway",
                                                     "air",   "brake", "damage", "scrape"};

// The synth's own constants: each voice's level (dB, on top of its curves), set against the FMOD
// event's output (tools/acoustics/engine_fit.py), and the sound's character.
// Which of the FMOD event's own effects the synth keeps; all on is the event as it was. All off by
// default ("clean"): each leaves its job to the acoustics or drops it.
struct Character {
  bool cone = false;       // the Cone return: up to 7 dB louder when a thruster faces the listener
  bool distortion = false; // FMOD's three distortion stages; off, each stage's level behaviour stays
                           // (a gain following the sound's envelope) without reshaping the wave
  bool darkness = false;   // the Engine return's high shelf (-16 dB over 240 Hz) and 2.7 kHz low-pass;
                           // off, a mild tilt (-4 dB over 800 Hz) and an 8 kHz low-pass
  bool grit = false;       // speed driving the distortion and opening the low-pass; off, held where a
                           // 40 m/s cruise has them (pitch still rises with speed)
  bool operator==(const Character &) const = default;
};
// the character as four bits (cone, distortion, darkness, grit), for a mixer to read without a lock
constexpr uint32_t CharacterBits(const Character &c) {
  return uint32_t(c.cone) | uint32_t(c.distortion) << 1 | uint32_t(c.darkness) << 2 | uint32_t(c.grit) << 3;
}
constexpr Character CharacterFromBits(uint32_t b) {
  return {bool(b & 1), bool(b & 2), bool(b & 4), bool(b & 8)};
}
// "cone=0,distortion=1,...": the names above, 0 or 1; the rest as they are. "fmod" (all on) and
// "clean" (all off) as wholes. False on a name it does not know.
bool ParseCharacter(const char *aText, Character &aCharacter);

struct Tuning {
  Character character;
  // what the axis voices start from (Sources.hpp)
  Sound sound = Sound::Drone;
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
  void SetCharacter(const Character &aCharacter);
  void SetSound(Sound aSound) { tuning.sound = aSound; }
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
    float envelope[2] = {}; // the two distortion stages' input level, when the waves are kept clean
    Source source;          // the other sounds than the drone
  };
  // a distortion stage, or with Character::distortion off its gain at the sound's level
  float Stage(float x, float aLevel, float &aEnvelope) const;
  void EngineFilters();

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
  // scrape: a contact's texture, grit, stick-slip and grains ringing the surface's and the thruster pod's resonances
  ScrapeVoice scrape;
  float scrapeGain = 0;
  // master
  Svf water;
  float masterGain = 0, waterCutoff = 22000;
  float engineDrive = 0, coneGain = 0, engineEnvelope = 0, envelopeAttack = 0, envelopeRelease = 0;
  float insideForFilters = -1;
};

} // namespace EngineSynth
