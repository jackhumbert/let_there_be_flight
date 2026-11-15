#pragma once

// The studio events LTBF's own samples came from (vehicle3_on, vehicle3_off, vehicle3_destroyed), as
// plain data for Core channels (docs/design/audio.md, Samples). No FMOD, so tests/ can link it.

#include <array>
#include <string_view>

namespace Samples {

enum class Bus { Engine, Warning };

struct Point {
  float time, value;
};
// linear between the two points, held at the end values outside them
using Curve = std::array<Point, 2>;

struct Event {
  const char *name, *file;
  Bus bus;
  bool loop;
  // the event's own volume, dB
  float levelDb;
  // semitones over the seconds since the start
  Curve pitch;
  // a slow wobble on the pitch
  float lfoHz, lfoDepth;
  // the width of a uniform offset in semitones, drawn once per start
  float randomPitch;
  bool tremolo;
  Curve tremoloHz;
  float duty, square, phase;
  bool distortion;
  Curve distortionLevel;
};

// nullptr when unknown
const Event *Find(std::string_view aName);

float Value(const Curve &aCurve, float aTime);
float PitchSemitones(const Event &aEvent, float aTime, float aRandomOffset);
// 2^(semitones / 12)
float PitchRatio(float aSemitones);

} // namespace Samples
