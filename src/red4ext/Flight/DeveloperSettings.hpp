#pragma once

// Settings only Jack changes, never shown in Mod Settings (docs/design/hover-fly-native.md, decision 2): the code
// defaults below, overridden at plugin load by an optional developer.ini in the plugin folder (releases never ship
// one) and live through FlightDeveloperSettings.Set (bridge.py call FlightDeveloperSettings.Set <name> <value>).

#include "Flight/Force.hpp"

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace Flight {

struct DeveloperSettings {
  // The car's character (docs/research/2026-11-04-aero-drag.md). Off by default: every car flies as today.
  // shapeDrag: the air drag from the chassis collision shape on each body axis (Flight::ShapeDragAccel, the car's
  // dragArea) in place of airResistanceFactor's along the velocity; dragK is the fleet-wide factor on it.
  bool shapeDrag = false;
  float dragK = kShapeDragK;
  // thrustStrength: surge and sway scaled by the car's thrusters over its mass (Flight::ThrustScale); 0 every car the
  // same, 1 by its thrusters alone. thrustPerThruster: the force of one thruster, N.
  float thrustStrength = 0.0f;
  float thrustPerThruster = 6000.0f;
  // the native mode spike (FlightModeNativeProbe) appended to the mode list of cars attached after it is set
  bool nativeModeProbe = false;
  // PhysX rays the acoustics cast each time (Audio/Acoustics/LiveRays.cpp, 20 a second): the first 32 are the near
  // rays the echoes use; the rest the live sphere, bunched where the car is going, for the reverb's enclosure and mean
  // free path (Probe::SphereRays; docs/design/acoustics.md, "How many rays"). 32: the nav map's sphere alone.
  float acousticsRays = 1024.0f;
  // The Autopilot's tuning (Navigation::AutopilotParams) by field name, set as "autopilot.<name>" (a Bool as
  // true/false/1/0/on/off, an Int32 rounded), in the order set, a name set again keeping its place. Each
  // FlightModeAutopilot writes them over its tuning when it is set up, when it is engaged and when
  // autopilotGeneration changes.
  std::vector<std::pair<std::string, float>> autopilot;
  uint32_t autopilotGeneration = 0;
};

// The instance the plugin reads: the code defaults until LoadDeveloperSettings or SetDeveloperSetting change it.
DeveloperSettings &Developer();

// The prefix of an Autopilot tuning override's name ("autopilot.raceRouteHeight").
constexpr const char *kAutopilotPrefix = "autopilot.";

// One setting by its field name, from text: a bool takes true/false/1/0/on/off, a float a number. An unknown name or a
// value that does not parse leaves the setting and returns false; a number out of the setting's range is clamped. In
// both cases aMessage says what happened (empty when the value was taken as given).
bool SetDeveloperSetting(DeveloperSettings &aSettings, const std::string &aName, const std::string &aValue,
                         std::string &aMessage);
// The setting's value as text; false for an unknown name.
bool GetDeveloperSetting(const DeveloperSettings &aSettings, const std::string &aName, std::string &aValue);
// developer.ini's text: "name = value" lines; blank lines, ';' and '#' comments and [section] headings are skipped.
// Returns a message per line that was refused or clamped, with its line number.
std::vector<std::string> LoadDeveloperSettings(DeveloperSettings &aSettings, const std::string &aText);
// "name = value" for each setting that differs from the code default, for the log.
std::vector<std::string> ChangedDeveloperSettings(const DeveloperSettings &aSettings);

} // namespace Flight
