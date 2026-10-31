#pragma once

// Reflections: the flying sound heard in the space round the car (docs/design/acoustics.md).
//
//   two echo pairs   floor and ceiling, left and right wall: one echo off each surface (an image
//                    source), then cross feedback between the two, the second bounce taking the
//                    other side's round trip (the flutter between parallel walls). The floor alone
//                    is the comb that sweeps as the car climbs or dives.
//   scatter          a weak echo back along every other near ray that hit: the rest of the walls
//                    round the car, a cluster of arrivals that smears the pair's echoes into a decay.
//   late reverb      an 8 line Householder feedback delay network, fixed delays, decay per sample.
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
  float wet = 0.15f;               // reverb send at full enclosure
  float maxDelayRate = 0.3f;       // s/s: how fast an echo's delay may change
  float scatter = 0.12f;           // each scattered echo against a main echo off the same surface
  float jumpSeconds = 0.004f;      // a delay this far (beyond the motion) from the tap's: a new reflection
  float fadeSeconds = 0.08f;       // the crossfade to it
  float wetFrom = 0.35f;           // enclosure where the late reverb starts (full at 1)
  bool airAbsorption = false;      // highs lost along each echo's path, and faster in the reverb
  float minDelay = 0.004f;         // s
  float pan = 0.9f, panOpposite = 0.3f; // a wall's echo in its own channel and the other
};

struct Echo {
  float delay = 0; // s, extra path over the direct sound
  float rate = 0;  // s/s, how fast the delay changes with the car moving (its Doppler shift: 1 - rate)
  float gain = 0;
};

struct Params {
  Echo floor, ceiling, left, right;
  // along each near ray (Probe::NearRays order); gain 0 for the rays the main echoes use and misses
  std::array<Echo, kNearRays> scatter{};
  float floorFeedback = 0, wallFeedback = 0;
  float rt60 = 0.2f, wet = 0;
  bool air = false; // Tuning::airAbsorption
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
  void Reset();

  // energy of each part so far, for checking against render.py: floor, ceiling, walls, reverb, dry
  struct Energy {
    double floor = 0, ceiling = 0, walls = 0, reverb = 0, dry = 0, scatter = 0;
  };
  const Energy &Parts() const { return energy; }
  // mean square of the delay lines' recent output: a tail still ringing
  float TailLevel() const { return tail; }

private:
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
    float Air(float x);
    bool started = false;
    // a new target: delay and rate in samples, gain, reached over aInterval samples
    void Set(double aDelay, double aRate, float aGain, int aInterval, const Reflections &aOwner);
    void Step(double aMaxRate, double aMinDelay);
    // the taps' sum from aLine, times the gain or not (the flutter's second bounce)
    float Read(const Line &aLine, uint32_t aWrite, bool aWithGain, bool aSmear) const;
  };

  float sampleRate;
  int rampSamples;
  Tuning tuning;
  Line source, floor, ceiling, left, right;
  std::array<Line, 8> fdn;
  std::array<uint32_t, 8> fdnDelay{};
  uint32_t write = 0;
  Voice floorVoice, ceilingVoice, leftVoice, rightVoice;
  std::array<Voice, kNearRays> scatterVoices;
  // ramped Params
  Ramp floorFeedback, wallFeedback, rt60, wet;
  int fdnGainAge = 0;
  std::array<float, 8> fdnGain{};
  // air absorption in the reverb: each line's loop low-passed (1: none)
  float fdnAir = 1;
  std::array<float, 8> fdnAirState{};
  bool started = false;
  Energy energy;
  float tail = 0;
};

} // namespace Acoustics
