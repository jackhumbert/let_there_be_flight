#pragma once

// The per-frame audio inputs (docs/design/audio-native.md C.7), with no FMOD or game types so tests/ can
// link it. A port of FlightComponent.UpdateAudioParams (both overloads) and the audio half of
// IFlightThruster.Update, rule for rule: the caller fills a Frame from the vehicle, Update turns it into
// Values.
//
// Game thread.

#include "Audio/EngineSynth/Synth.hpp"

#include <algorithm>
#include <cmath>

namespace Inputs {

struct Vec3 {
  float x = 0, y = 0, z = 0;
};

// Who is flying: the player (any mode but the Autopilot), the Autopilot with the player aboard, or nobody
// the player is with (an NPC's car).
enum class Pilot { Player, Autopilot, Npc };

struct Frame {
  Pilot pilot = Pilot::Npc;
  bool drone = false;
  bool destroyed = false;
  // the camera is outside the car: ctlr.isTPP || cinematic
  bool outside = true;
  // false: the timeDelta <= 0 path, UpdateAudioParams(1/60) only
  bool full = true;
  float dt = 0;
  float speed = 0;
  Vec3 forward{0, 1, 0}, right{1, 0, 0}, up{0, 0, 1}, localUp{0, 0, 1}, direction{0, 1, 0}, angularVelocity;
  // the mode's force before the physics helpers, car frame (FlightComponent.smoothForce)
  Vec3 force;
  float healthPercent = 100, collisionTimer = 1e9f, recoveryDelay = 0, recoveryDuration = 1;
  // the player's controls and brakes
  float ctlSurge = 0, ctlYaw = 0, ctlSway = 0, ctlLift = 0, ctlRoll = 0, ctlPitch = 0, linearBrake = 0, angularBrake = 0;
  // the component's own surge, lift, roll, pitch and yaw (FlightComponent)
  float surge = 0, lift = 0, roll = 0, pitch = 0, yaw = 0;
  // the Autopilot's commands
  float apSurge = 0, apSway = 0, gravityFactor = 1, standardModeLiftFactor = 25;
};

// FlightAudioUpdate's fields, in its declaration order
struct Values {
  float speed = 0, surge = 0, yawDiff = 0, lift = 0, yaw = 0, pitchDiff = 0, brake = 0, inside = 0, damage = 0,
        roll = 0, pitch = 0, sway = 0, scrape = 0;
  // the synth's only, not FlightAudioUpdate's (kNames): the scrape's surface (Flight::Surface) and slide (m/s)
  int scrapeSurface = 0;
  float scrapeSlide = 0;
};

// the stems' params.csv header, in the order of Values
inline constexpr const char *kNames[13] = {"speed", "surge", "yawDiff", "lift", "yaw", "pitchDiff", "brake",
                                           "inside", "damage", "roll", "pitch", "sway", "scrape"};
inline constexpr int kValueCount = 13;

// the i-th field of v, as kNames names it
inline float Get(const Values &v, int i) {
  const float all[kValueCount] = {v.speed, v.surge, v.yawDiff, v.lift,   v.yaw,  v.pitchDiff, v.brake,
                                  v.inside, v.damage, v.roll,  v.pitch, v.sway, v.scrape};
  return all[i];
}

// how far the controls have come back after a collision: 0 until delay, then 0 to 1 over duration
float CollisionRatio(float timer, float delay, float duration);

// degrees from a to b around axis, both flattened onto the plane normal to it; right-handed (positive
// from a to b counter-clockwise looking down the axis); 0 when either is along the axis
float AngleAroundDeg(Vec3 a, Vec3 b, Vec3 axis);

// v holds last frame's values; what a rule does not set stays
void Update(const Frame &f, Values &v);

// a front thruster is always at full, the others come in with speed
float ThrusterVolume(bool front, float speed);

struct Axes {
  Vec3 right, forward, up;
};
// the script rotates the cone by the quaternion (-0.707, 0, 0, 0.707) in the mesh's frame: -90 degrees
// about its right axis, pointing the cone down. Exact here (0.707 against sqrt(1/2) is a 3e-4 error).
Axes ConeAxes(const Axes &mesh);

// the synth's inputs: the values as they are, with the thruster's volume, the global water and the cone angle
EngineSynth::Inputs ToSynth(const Values &v, float volume, float water, float cone);

// the pitch of the car's engine for its mass
float CarPitch(float mass);
// the game's volume sliders as the mod hears them; 0 in the menus
float GameVolume(bool inMenu, bool hubMenu, bool worldMap, bool radialMenu, int master, int sfx);

} // namespace Inputs
