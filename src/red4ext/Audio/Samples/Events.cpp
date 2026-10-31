#include "Events.hpp"

#include <cmath>

namespace Samples {

namespace {

// The levels and pitches the old FMOD Studio events played these sounds at (their project is in git
// history, docs/history.md), curves taken as linear. No reverb or EQ: the echoes make the space.
constexpr Event kEvents[] = {
    // 62e81e8b..., group Engine {5c36583b...}
    {"vehicle3_on", "vehicle3_startup.wav", Bus::Engine, false, 0.0f, {{{0.0f, -4.5f}, {0.1f, 0.0f}}}, 0.055f, 1.5f,
     0.0f, false, {{{0.0f, 0.0f}, {0.0f, 0.0f}}}, 0.0f, 0.0f, 0.0f, false, {{{0.0f, 0.0f}, {0.0f, 0.0f}}}},
    // 701a8161..., group Engine
    {"vehicle3_off", "vehicle3_off.wav", Bus::Engine, false, 0.0f, {{{0.0f, 2.0f}, {0.0f, 2.0f}}}, 0.0f, 0.0f, 0.0f,
     false, {{{0.0f, 0.0f}, {0.0f, 0.0f}}}, 0.0f, 0.0f, 0.0f, false, {{{0.0f, 0.0f}, {0.0f, 0.0f}}}},
    // 645b26cd..., group Warning {bb9c119f...}, whose volume is the -4 dB
    {"vehicle3_destroyed", "brake-1.wav", Bus::Warning, true, -4.0f, {{{0.0f, -7.0f}, {1.8f, -5.0f}}}, 1.9f, 0.5f,
     3.0f, true, {{{0.0f, 1.8f}, {20.0f, 20.0f}}}, 0.33f, 0.935f, 0.335f, true, {{{0.0f, 0.42f}, {20.0f, 0.80f}}}},
};

} // namespace

const Event *Find(std::string_view aName) {
  for (const Event &e : kEvents) {
    if (aName == e.name) {
      return &e;
    }
  }
  return nullptr;
}

float Value(const Curve &aCurve, float aTime) {
  const Point &a = aCurve[0], &b = aCurve[1];
  if (aTime <= a.time) {
    return a.value;
  }
  if (aTime >= b.time) {
    return b.value;
  }
  return a.value + (b.value - a.value) * (aTime - a.time) / (b.time - a.time);
}

float PitchSemitones(const Event &aEvent, float aTime, float aRandomOffset) {
  constexpr float kTwoPi = 6.28318530718f;
  return Value(aEvent.pitch, aTime) + aEvent.lfoDepth * std::sin(kTwoPi * aEvent.lfoHz * aTime) + aRandomOffset;
}

float PitchRatio(float aSemitones) {
  return std::pow(2.0f, aSemitones / 12.0f);
}

} // namespace Samples
