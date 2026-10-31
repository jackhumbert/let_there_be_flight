#pragma once

// Reflections: the flying sound heard in the space round the car (docs/design/acoustics.md).
//
//   two echo pairs   floor and ceiling, left and right wall: one echo off each surface (an image
//                    source), then cross feedback between the two, the second bounce taking the
//                    other side's round trip (the flutter between parallel walls). The floor alone
//                    is the comb that sweeps as the car climbs or dives.
//   late reverb      an 8 line Householder feedback delay network, fixed delays, decay per sample.
//
// Targets() turns the probe's features and where the car and listener are into Params; Process()
// runs per sample and ramps to new Params over one probe interval. Delays follow the geometry
// sample by sample (fractional reads, a three tap smear) so a reflection carries its own Doppler
// shift; their rate is capped so a ray sliding off a building edge glides.
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
  float minDelay = 0.004f;         // s
  float pan = 0.9f, panOpposite = 0.3f; // a wall's echo in its own channel and the other
};

struct Echo {
  float delay = 0; // s, extra path over the direct sound
  float gain = 0;
};

struct Params {
  Echo floor, ceiling, left, right;
  float floorFeedback = 0, wallFeedback = 0;
  float rt60 = 0.2f, wet = 0;
};

// The echoes of the car's sound at aSource heard at aListener, from the features probed at aSource.
Params Targets(const Features &aFeatures, const Vec3 &aSource, const Vec3 &aListener, const Tuning &aTuning);

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
    double floor = 0, ceiling = 0, walls = 0, reverb = 0, dry = 0;
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
  };

  double Slew(double &aState, double aTarget) const;

  float sampleRate;
  int rampSamples;
  Tuning tuning;
  Line source, floor, ceiling, left, right;
  std::array<Line, 8> fdn;
  std::array<uint32_t, 8> fdnDelay{};
  uint32_t write = 0;
  // ramped Params
  Ramp floorDelay, floorGain, ceilingDelay, ceilingGain, leftDelay, leftGain, rightDelay, rightGain;
  Ramp floorFeedback, wallFeedback, rt60, wet;
  // the delays as slewed, in samples (negative: not started)
  double floorSlew = -1, ceilingSlew = -1, leftSlew = -1, rightSlew = -1;
  int fdnGainAge = 0;
  std::array<float, 8> fdnGain{};
  bool started = false;
  Energy energy;
  float tail = 0;
};

} // namespace Acoustics
