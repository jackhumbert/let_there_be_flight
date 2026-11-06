#pragma once

// Reflections: the flying sound heard in the space round the car (docs/design/acoustics.md).
//
//   two echo pairs   floor and ceiling, left and right wall: one echo off each surface (an image
//                    source), then cross feedback between the two, the second bounce taking the
//                    other side's round trip (the flutter between parallel walls). The floor alone
//                    is the comb that sweeps as the car climbs or dives.
//   scatter          a weak echo back along every other near ray that hit: the rest of the walls
//                    round the car, a cluster of arrivals that smears the pair's echoes into a decay.
//   distant echoes   one a quarter of the horizon, off its facade 20 to 200 m out (Probe::Quarters): the slap back
//                    off far buildings.
//   late reverb      two 8 line Householder feedback delay networks, small and large fixed delays, crossfaded by
//                    the space's size; each line's loop decays the highs faster than the lows (the two RT60s), and
//                    the tail leaves by quarter of the horizon, as enclosed as each is.
//
// Targets() turns the probe's features and where the car and listener are into Params; Process()
// runs per sample and ramps to new Params over one probe interval. Each echo is a voice of two
// taps: its delay moves at the rate the car's velocity gives it (its Doppler shift, worked out from
// the geometry, not from probe to probe), steered onto the next probe's distance; a distance that
// jumps further than the motion explains (the nearest ray now on another building) is a new
// reflection: the other tap fades in at it while the old one fades out.
//
// The port of tools/acoustics/render.py, which stays the reference. No game or FMOD types: the FMOD
// DSP and the offline target (tools/acoustics/acoustics.cpp) both drive it. Process() and SetTargets()
// belong to one thread (the mixer's); the caller hands Params over.

#include "Probe.hpp"

#include <array>
#include <cstdint>
#include <vector>

namespace Acoustics {

// The sound's constants, in one place.
struct Tuning {
  float wallReflectivity = 0.8f;
  float groundReflectivity = 0.7f;
  float gain = 1.0f;               // every echo and the reverb
  float nearDistance = 3.0f;       // m: the dry sound's distance, for the echoes' spreading
  float range = 249.0f;            // m: no echo off a surface further than this (the probe's cap)
  float flutterFeedback = 0.5f;    // times the square root of the two surfaces' reflectivity
  float flutterNear = 60.0f;       // m apart (the two sides summed): full flutter up to here
  float flutterFar = 120.0f;       // and none from here
  float wallAbsorption = 0.5f;     // reverb: the city's surfaces; open sky absorbs everything
  float rt60Min = 0.2f, rt60Max = 2.0f;
  float highAir = 0.01f;           // 1/m: the air's absorption of the highs (about 5 kHz) in the reverb's Eyring
  float smallFreePath = 8.0f;      // m: the small reverb alone up to this mean free path,
  float largeFreePath = 25.0f;     // the large alone from this
  float distant = 1.0f;                // a distant echo against a main echo off the same distance, times its facade's share
  float wet = 0.15f;               // reverb send at full enclosure
  float maxDelayRate = 0.3f;       // s/s: how fast an echo's delay may change
  float scatter = 0.12f;           // each scattered echo against a main echo off the same surface
  float jumpSeconds = 0.004f;      // a delay this far (beyond the motion) from the tap's: a new reflection
  float fadeSeconds = 0.08f;       // the crossfade to it
  float wetFrom = 0.35f;           // enclosure where the late reverb starts (full at 1)
  bool airAbsorption = true;       // highs lost along each echo's path, and faster in the reverb
  float minDelay = 0.0005f;        // s: short enough for the floor's comb with the car low under a third person camera
  float pan = 0.9f, panOpposite = 0.3f; // a wall's echo in its own channel and the other
};

struct Echo {
  float delay = 0; // s, extra path over the direct sound
  float rate = 0;  // s/s, how fast the delay changes with the car moving (its Doppler shift: 1 - rate)
  float gain = 0;
  // the surface's reflectivity per band against the mid band's (low/mid, 1, high/mid; 1: unknown)
  float tilt[3] = {1, 1, 1};
};

// An echo's tone from its surface: the sound split at 250 Hz and 2.5 kHz by one-pole low-passes into
// low, mid and high, each scaled by the tilt. With every tilt 1 the input passes untouched.
struct BandSplit {
  // implicit default: Voice is reset with `v = {}`, which cannot use an explicit constructor
  BandSplit() : BandSplit(48000.0f) {}
  explicit BandSplit(float aRate);
  void Set(const float aTilt[3]);
  float Run(float x);

private:
  float lowCoefficient, highCoefficient;
  float tilt[3] = {1, 1, 1};
  bool flat = true;
  float lowState = 0, highState = 0;
};

struct Params {
  Echo floor, ceiling, left, right;
  // along each near ray (Probe::NearRays order); gain 0 for the rays the main echoes use and misses
  std::array<Echo, kNearRays> scatter{};
  float floorFeedback = 0, wallFeedback = 0;
  // the reverb: decay of the lows and the mids, and of the highs (no longer); the space's size (0 the small network,
  // 1 the large); each quarter's share of the tail, as amplitude (their squares add up to 4: the old stereo bed's energy)
  float rt60 = 0.2f, rt60High = 0.2f, wet = 0, size = 1;
  std::array<float, kQuarters> tail{1, 1, 1, 1};
  // each quarter's distant echo and where it and its tail are heard from (its image, or a point that way)
  std::array<Echo, kQuarters> distant{};
  std::array<Vec3, kQuarters> quarterImage{};
  bool air = false; // Tuning::airAbsorption
  // where each echo is heard from (world): its image source, the scatter echoes' by side weighted by
  // gain (the source itself when a side has none)
  Vec3 floorImage, ceilingImage, leftImage, rightImage, scatterRightImage, scatterLeftImage;
};

// The echoes of the car's sound at aSource heard at aListener, from the features probed at aSource;
// aVelocity (m/s, the car's, the listener moving with it) gives each its rate.
Params Targets(const Features &aFeatures, const Vec3 &aSource, const Vec3 &aListener, const Tuning &aTuning,
               const Vec3 &aVelocity = {});

class Reflections {
public:
  // aRampSamples: how long a new set of Params takes to reach (one probe interval)
  Reflections(float aSampleRate, int aRampSamples, const Tuning &aTuning = {});

  void SetTargets(const Params &aParams);
  // interleaved, aChannels in and out (1 or more; channels 0 and 1 take the panning, the rest the
  // echoes' sum); the input is the direct sound, kept as it is, and the echoes are made from its mean
  void Process(const float *aIn, float *aOut, uint32_t aFrames, int aChannels);

  // Process() with each part in a stream of its own, unpanned (aFrames mono samples each): the
  // floor, ceiling, left and right echoes with their flutter, the scattered echoes by side, and the
  // reverb's two channels at the current wet level. The input is the mono dry sound.
  struct Streams {
    float *floor, *ceiling, *left, *right, *scatterRight, *scatterLeft, *reverbLeft, *reverbRight;
    // each quarter's distant echo and tail (Params::quarterImage); the reverb's two channels stay for the stems
    float *quarter[kQuarters];
  };
  void ProcessStreams(const float *aDryMono, const Streams &aStreams, uint32_t aFrames);
  void Reset();

  // energy of each part so far, for checking against render.py: floor, ceiling, walls, reverb, dry
  struct Energy {
    double floor = 0, ceiling = 0, walls = 0, reverb = 0, dry = 0, scatter = 0, distant = 0;
  };
  const Energy &Parts() const { return energy; }
  // mean square of the delay lines' recent output: a tail still ringing
  float TailLevel() const { return tail; }
  // times a sample came out not a number and everything started again from silence (Reset)
  uint64_t Resets() const { return resets; }
  const char *ResetPart() const { return resetPart; }
  // Params refused for a value not a number, and the last one's field
  uint64_t BadParams() const { return badParams; }
  const char *BadParam() const { return badParam; }

private:
  // one sample of every part (the engine under Process and ProcessStreams)
  struct Sample {
    float floor = 0, ceiling = 0, left = 0, right = 0, scatterLeft = 0, scatterRight = 0, reverbLeft = 0, reverbRight = 0;
    std::array<float, kQuarters> distant{}, tail{};
  };
  Sample Tick(float aMono);
  // double: a delay of tens of thousands of samples moving a thousandth of a sample a sample would
  // stand still in float
  struct Ramp {
    double value = 0, step = 0;
    int left = 0;
    void To(double aTarget, int aSamples);
    double Next();
  };
  struct Line {
    std::vector<float> buffer; // power of two
    uint32_t mask = 0;
    void Resize(uint32_t aMinSize);
    float Read(uint32_t aWrite, double aDelay) const; // aDelay samples before aWrite, smeared
    float ReadLinear(uint32_t aWrite, double aDelay) const;
  };
  // one echo: two taps, one playing and one fading out after a jump
  struct Tap {
    double delay = 0, rate = 0; // samples, samples a sample
    float fade = 0, fadeStep = 0;
  };
  struct Voice {
    Tap taps[2];
    int active = 0;
    Ramp gain;
    float level = 0; // this sample's gain
    // air absorption: a one-pole low-pass set by the echo's path (1: none), and its state
    float air = 1, airState = 0;
    // then the surface's tone
    BandSplit bands;
    // air absorption and the surface's bands on one echo sample
    float Filter(float x);
    bool started = false;
    // a new target: delay and rate in samples, gain, reached over aInterval samples
    void Set(double aDelay, double aRate, float aGain, int aInterval, const Reflections &aOwner);
    void Step(double aMaxRate, double aMinDelay);
    // the taps' sum from aLine, times the gain or not (the flutter's second bounce)
    float Read(const Line &aLine, uint32_t aWrite, bool aWithGain, bool aSmear) const;
  };

  // a feedback delay network: its lines, and each line's loop filter (a one-pole with the lows' gain at DC and the
  // highs' at Nyquist: low (1 - pole) / (1 - pole z^-1)) and state
  struct Fdn {
    std::array<Line, 8> lines;
    std::array<uint32_t, 8> delay{};
    std::array<float, 8> low{}, pole{}, state{};
  };
  float sampleRate;
  int rampSamples;
  Tuning tuning;
  Line source, floor, ceiling, left, right;
  Fdn small, large;
  uint32_t write = 0;
  Voice floorVoice, ceilingVoice, leftVoice, rightVoice;
  std::array<Voice, kNearRays> scatterVoices;
  std::array<Voice, kQuarters> farVoices;
  // ramped Params
  Ramp floorFeedback, wallFeedback, rt60, rt60High, wet, size;
  std::array<Ramp, kQuarters> tailRamps;
  int fdnGainAge = 0;
  bool started = false;
  Energy energy;
  float tail = 0;
  uint64_t resets = 0, badParams = 0;
  const char *resetPart = "", *badParam = "";
};

} // namespace Acoustics
