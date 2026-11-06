#pragma once

// The wind (docs/design/audio-native.md A.4): two voices of the audio backend (Audio/Out) playing a generator
// (Render, the model's noise) in the Wind bus, each a voice in the pool (Audio/Voices).
// First person puts one at each window so the air comes from the side it does; third person has one
// ahead of the listener along the car's velocity; the cinematic camera has the camera's own wind and
// the car's, which Doppler sweeps as it passes.
//
// Game thread: Init, Update, Tick, Shutdown. The mixer thread takes each channel's newest Params
// without waiting.

#include "Audio/Voices/Voices.hpp"
#include "Model.hpp"

#include <atomic>
#include <string>

namespace Wind {

using Vec3 = Voices::Vec3;

struct Inputs {
  // the car, in the game's world (z up)
  Vec3 carPosition, carRight, carForward, carUp, carVelocity;
  View view = View::Tpp;
  // the car's windows, in the car's own axes (right, forward, up); a zero vector for the default
  Vec3 windowLeft, windowRight;
  Vec3 listenerPosition, listenerVelocity;
  // the last probe's features (Acoustics::LastFeatures), if there is one
  bool haveProbe = false;
  float openSky = 1;
  float floorDistance = std::numeric_limits<float>::infinity();
  float wallDistance = std::numeric_limits<float>::infinity();
};

// the two voices in the Wind bus; false if the backend refused (logged)
bool Init();
void Shutdown();

// each frame the player's car flies
void Update(const Inputs &aInputs);
// once a game frame before Voices::Frame: the wind goes silent when Update has not been called for a
// quarter of a second (the car stopped flying)
void Tick();

extern std::atomic<bool> on;

// JSON: on, view, speed, levelDb, centroidHz, gustDb, buffet, left, right (the first voice's)
std::string State();
// "wind on" or "wind off"
std::string Summary();

} // namespace Wind
