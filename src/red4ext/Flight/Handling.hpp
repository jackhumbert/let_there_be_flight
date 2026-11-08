#pragma once

// Handling: Loose / Default / Tight, the player's one lever on how flying feels (docs/design/settings-audit.md), in
// place of the ten handling values that were Mod Settings and are developer values now (FlightSettings' fields of the
// same names). Default is those fields and Standard's gains as they are; Loose and Tight replace them with the sets
// below. Applied where the values are read (the force tail, FlightForce; Standard, Hover and Fly; the Autopilot),
// never written into the fields, so the developer values stay what they were set to.
//
// Two parts, because the modes feel through different values (flown in tools/flightsim over the surveyed fleet):
// - The force tail's damping and directionality: the Drone modes' feel (how long a spin or a slide lasts). On Standard
//   they change little: its own controller sets the turn rate and the slide. Tight's are the settings guide's
//   "Gamepad, less floaty" preset (docs/nexus/settings-guide.md); the guide's keyboard preset is not used, as its
//   linear damping held Standard's top speed to 15 m/s from 97.
// - Standard's braking and side grip (Flight::StandardGains brake, sideBrake, maxBrake): how far it coasts and how much
//   it slides through a turn. At full speed Loose coasts about 134 m with up to 12 deg of slip in a turn, Default 128 m
//   and 9 deg, Tight 90 m and 6 deg; the top speed is the same. All three keep Standard's spec (tests/feel_tests.cpp,
//   feel_standard_fleet and feel_handling_*). Not yet flown in game.

#include "Flight/Force.hpp"
#include "Flight/Modes.hpp"

#include <cstdint>

namespace Flight {

// FlightSettings.handling (the redscript enum FlightHandling has the same numbers).
enum class Handling : int32_t { Loose = 0, Default = 1, Tight = 2 };

// The ten values, with FlightSettings' names.
struct HandlingFactors {
  float generalDampFactorLinear = 0.0f;
  float generalDampFactorAngular = 2.0f;
  float generalDampFactorAngularMax = 3.5f;
  float generalPitchAeroFactor = 0.0f;
  float generalYawAeroFactor = 0.0f;
  float generalPitchDirectionalityFactor = 15.0f;
  float generalYawDirectionalityFactor = 5.0f;
  float generalYawDampFactor = 1.0f;
  float generalSwayWithYaw = 0.0f;
  float generalRollWithYaw = 0.15f;
};

// The set for a choice: aDefault (the developer values) for Default and for a number that is no choice, else Loose's
// or Tight's.
HandlingFactors HandlingSet(Handling aHandling, const HandlingFactors &aDefault);
HandlingFactors HandlingSet(int32_t aHandling, const HandlingFactors &aDefault);

// The force tail's params with a choice's set in place of their handling values (rollWithYaw is not one of them: the
// Autopilot's AttitudeLift reads it).
ForceParams WithHandling(ForceParams aParams, int32_t aHandling);

// Standard's gains with a choice's braking and side grip; Default leaves them.
StandardGains WithHandling(StandardGains aGains, int32_t aHandling);

} // namespace Flight
