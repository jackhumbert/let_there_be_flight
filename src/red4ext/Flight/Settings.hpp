#pragma once

// The player's Mod Settings for the C++ flight modes (docs/design/hover-fly-native.md, decision 1: few, in plain
// words), registered from C++ with Mod Settings' AddVariable (FlightSettingsLink.cpp). Without Mod Settings they keep
// the defaults below. Every other tunable is a developer setting (Flight/DeveloperSettings.hpp).

#include <cstdint>
#include <string>
#include <vector>

namespace Flight {

struct PlayerSettings {
  // Hover & Fly (the C++ mode): how fast a held turn turns, deg/s, and a held climb or descent climbs, m/s
  float hoverFlyTurnRate = 90.0f;
  float hoverFlyClimbRate = 8.0f;
};

// The instance the modes read, written by Mod Settings' callbacks.
PlayerSettings &Player();

// One Mod Settings variable: the field's name (its key in Mod Settings' user.ini), the localization keys of its
// name and description, the default, step and range.
struct PlayerSetting {
  const char *name;
  const char *displayName;
  const char *description;
  float defaultValue, step, min, max;
  float PlayerSettings::*field;
};

// The variables, in the order Mod Settings shows them, all under kPlayerSettingsCategory.
const std::vector<PlayerSetting> &PlayerSettingsTable();
constexpr const char *kPlayerSettingsMod = "Let There Be Flight";
constexpr const char *kPlayerSettingsClass = "LTBF_HoverFly";
constexpr const char *kPlayerSettingsCategory = "UI-Settings-Hover-And-Fly";

// Writes the value Mod Settings gives for a variable into aSettings, clamped to its range; false for a name not in the
// table.
bool SetPlayerSetting(PlayerSettings &aSettings, const std::string &aName, float aValue);

// The Autopilot's Mod Settings (FlightModeAutopilot): registered from C++ under the class name the script registered
// them under, with the same names, categories, keys, defaults and ranges, so the values players saved in Mod Settings'
// user.ini ([FlightModeAutopilot], name = value) carry over. enabled is the mode's switch (IsEnabled); the other three
// are written into the Autopilot's tuning (Navigation::AutopilotParams) when the mode is set up and engaged, and when
// the player changes one.
struct AutopilotSettings {
  bool enabled = true;
  float brakeAccel = 6.0f; // m/s2 planned deceleration
  bool landAtDestination = true;
  float padRange = 0.0f; // m around a destination on the ground to look for a landing pad (0 = off)
};

// The instance the Autopilot reads, written by Mod Settings' callbacks.
AutopilotSettings &AutopilotPlayer();
// Bumped by every SetAutopilotSetting on AutopilotPlayer(): a mode applies the settings again when it changes.
uint32_t AutopilotSettingsGeneration();

// One Mod Settings variable of the Autopilot: a Bool (step, min and max unused) or a Float.
struct AutopilotSetting {
  const char *name;
  const char *category;
  const char *displayName;
  const char *description;
  bool isBool;
  float defaultValue, step, min, max;
};

// The variables in the order the script declared them.
const std::vector<AutopilotSetting> &AutopilotSettingsTable();
constexpr const char *kAutopilotSettingsClass = "FlightModeAutopilot";

// Writes the value Mod Settings gives (a Bool as 0 or 1) into aSettings, a Float clamped to its range; false for a name
// not in the table. On AutopilotPlayer() it bumps AutopilotSettingsGeneration.
bool SetAutopilotSetting(AutopilotSettings &aSettings, const std::string &aName, float aValue);

} // namespace Flight
