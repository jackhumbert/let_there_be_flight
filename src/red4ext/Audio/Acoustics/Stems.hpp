#pragma once

// Stems of LTBF's own FMOD sound, for test videos (docs/design/acoustics.md): what the acoustics
// DSPs hear, written to WAVs while the game runs.
//
//   <base>_dry.wav   the player's thrusters summed (the acoustics engine's dry sum, before the echo
//                    voices' Object Spatializers): no echoes
//   <base>_wet.wav   the same with the echoes and the reverb (Engine.cpp pans them as Reflections does)
//   <base>_bus.wav   the master group's output: LTBF's other sounds, with their echoes
//   <base>.json      mixer rate and the first sample's wall clock time, to line them up with a video
//   <base>_params.csv  each thruster's parameters as the game sent them (WriteParams)
//
// Stereo 32-bit float. The thrusters are summed unspatialized (Windows Sonic places them, after the
// DSPs): mono sources in both channels. The engine and the master DSP add into a ring buffer by the master's mix clock
// on the mixer thread; a writer thread saves what is half a second old, fixing the WAV headers each time
// (tests end by killing the game).

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace Acoustics::Stems {

enum class Stem { Dry, Wet, Bus };

// Start writing <aBase>_*.wav; false if a file could not be opened (logged). Stops a recording first.
bool Start(const std::string &aBase);
// Write out what is left and close the files.
void Stop();
bool Recording();

// From the master DSP's read callback, every mix block while recording: aLength frames of aChannels at
// mix clock aClock, at aRate, what reached it (aIn) and what it played (aOut). Its blocks are the
// timeline the engine's sound is placed on. aTest: aIn is the test source, so it goes to the dry stem
// and aOut to the wet one (the thrusters are muted), not to the bus stem.
void AddBus(uint64_t aClock, const float *aIn, const float *aOut, unsigned int aLength, int aChannels, int aRate,
            bool aTest, std::vector<float> &aScratch);

// The acoustics engine's place on that timeline. Its bed channel reads more or fewer frames than a mix
// block (in one read or several): what it reads during a mix block is resampled to the block's length
// and placed at it.
struct EventCursor {
  static constexpr size_t kReserve = 1 << 15;
  EventCursor();
  uint64_t mix;
  uint32_t length = 0;
  std::vector<float> dry, wet, scratch; // stereo
};
// From the engine's bed: its dry sum and the wet one with the echoes.
void AddEvent(EventCursor &aCursor, const float *aIn, const float *aOut, unsigned int aLength, int aChannels);

// While recording, a row of <base>_params.csv: wall clock (s), the thruster's id, its start pitch,
// volume, world position (x, y, z) and the FMOD parameters by name (the header from the first row's
// names). Game thread; the file is flushed when the stems stop.
void WriteParams(const std::string &aId, float aPitch, float aVolume, const float *aPosition,
                 const std::vector<std::pair<std::string, float>> &aValues);

// JSON for State(): recording or not, frames written, frames dropped (late or overrun)
std::string State();

} // namespace Acoustics::Stems
