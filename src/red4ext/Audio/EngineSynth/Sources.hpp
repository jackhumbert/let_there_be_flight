#pragma once

// What each axis voice of the synth starts from before its EQs and gains (the old event's loops were a
// tuned drone; docs/research/thruster-sound-sources.md): the drone stays in Synth, the others are here.
//
// Bullroarer: a slat spinning on a cord, from measured recordings: a harmonic series twice the slat's
// spin, swelling and wobbling (Doppler) at the orbit rate, gliding down as the cord twists until the slat
// stops and restarts the other way. Each thruster's slat is a different width, so a car's four beat.
// Fan: a ducted fan: blade-pass harmonics and a shaft hum with slow random wobble, two turbine partials,
// broadband rush.
//
// No FMOD or game types. Control once a control block, Next once a sample; neither allocates.

#include <cstdint>

namespace EngineSynth {

enum class Sound : uint32_t { Drone = 0, Bullroarer = 1, Fan = 2 };
inline constexpr int kSounds = 3;
const char *SoundName(Sound aSound);
// "drone", "bullroarer" or "fan"; false on anything else
bool ParseSound(const char *aText, Sound &aSound);

// what drives a voice's source
struct SourceControl {
  float frequency = 46; // Hz: the voice's pitch from its curves (the drone's fundamental)
  float push = 0;       // 0..1: how hard the voice's axis is held
  float brake = 0;      // 0..1
  float damage = 0;     // 0..0.25
};

class Source {
public:
  // aThruster: the same for a thruster's five voices (its slat), aVoice: which of them
  void Init(float aSampleRate, uint32_t aThruster, uint32_t aVoice);
  // once a control block of aSeconds
  void Control(Sound aSound, const SourceControl &aControl, float aSeconds);
  // one sample, about the drone's RMS while sounding
  float Next(Sound aSound);

  // for tests: the bullroarer's pitch now (Hz) and its reversal envelope (0..1)
  float BullroarerHz() const { return roarHz; }
  float BullroarerStop() const { return stop; }
  float Slat() const { return slat; }

private:
  float Uniform();
  float NoiseStep(float &aState, float aSeconds, float aTime);

  float rate = 48000;
  uint32_t rng = 1;
  // bullroarer
  float slat = 1;                  // this thruster's slat against the middle one (pitch ratio)
  float twist = 0, period = 7;     // seconds into this spin, and how long it lasts
  float stop = 1;                  // the reversal's envelope: 1 spinning, down to 0 at the stop
  float stopTime = -1;             // seconds into the stop, -1 while spinning
  float orbit = 0, orbitRate = 2;  // turns of the orbit, and its rate (Hz)
  float spinHz = 0, roarHz = 0;    // the slat's pitch eased (spin-up lag), and with the orbit's Doppler
  float roarGain = 0, roarNorm = 1, roarAmp[8] = {};
  double phase = 0;
  // fan
  float bpf = 0, wobblePitch = 0, wobbleLevel[6] = {}, turbinePitch[2] = {};
  double turbinePhase[2] = {0.3, 0.7}, shaftPhase = 0;
  float harmonicGain[6] = {}, rushGain = 0, fanNorm = 1;
  float bpfNow = 0, turbineHz[2] = {}; // with this block's wobble
  // rush and the bullroarer's air: a band of noise
  float ic1 = 0, ic2 = 0, a1 = 0, a2 = 0, a3 = 0, k = 1, bandGain = 1, noiseGain = 0;
};

} // namespace EngineSynth
