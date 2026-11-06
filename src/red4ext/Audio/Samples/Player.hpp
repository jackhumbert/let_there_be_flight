#pragma once

// A sample played by LTBF's own code (Samples.hpp), for both audio backends: a WAV decoded to mono floats once, read
// at a pitch ratio with linear interpolation, through the tremolo and the distortion the old studio event had (in
// place of FMOD's DSPs of those names: the same parameters, LTBF's own curves). Game-free so tests/ can link it.
//
// Game thread: Load, Set. Audio thread: Render.

#include <atomic>
#include <cstdint>
#include <filesystem>
#include <vector>

namespace Samples {

struct Wave {
  float rate = 0;
  // the channels averaged
  std::vector<float> mono;
  float Seconds() const { return rate > 0 ? float(mono.size()) / rate : 0.0f; }
};

// 16-bit PCM or 32-bit float WAV; false if it is neither or could not be read
bool LoadWave(const std::filesystem::path &aPath, Wave &aOut);
bool ParseWave(const std::vector<uint8_t> &aBytes, Wave &aOut);

// the tremolo's gain aCycles into its LFO: a triangle on for aDuty of each cycle (from aPhase on), flattened toward a
// square by aSquare, at full depth (FMOD's tremolo as the destroyed alarm set it)
float Tremolo(double aCycles, float aDuty, float aSquare, float aPhase);
// a soft clip, harder as aLevel goes 0 to 1 (FMOD's distortion's level)
float Distort(float aSample, float aLevel);

class Player {
public:
  Player(const Wave &aWave, float aOutputRate, bool aLoop);

  // each game frame: the pitch ratio, and the tremolo and distortion (off when their flags are off)
  void Set(float aRatio, bool aTremolo, float aTremoloHz, float aDuty, float aSquare, float aPhase, bool aDistortion,
           float aDistortionLevel);
  // a one-shot read to its end
  bool Ended() const { return ended.load(std::memory_order_relaxed); }

  void Render(float *aOut, uint32_t aFrames);

private:
  const Wave &wave;
  float outputRate;
  bool loop;
  std::atomic<float> ratio{1}, tremoloHz{0}, duty{1}, square{0}, phase{0}, distortionLevel{0};
  std::atomic<bool> tremolo{false}, distortion{false}, ended{false};
  // the audio thread's
  // in the wave's samples, and the tremolo's cycles (its rate changes as it goes)
  double position = 0, cycles = 0;
};

} // namespace Samples
