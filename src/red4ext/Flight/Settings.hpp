#pragma once

// The player's Mod Settings for the C++ flight modes (docs/design/hover-fly-native.md, decision 1: few, in plain
// words), registered from C++ with Mod Settings' AddVariable (FlightSettingsLink.cpp). Without Mod Settings they keep
// the defaults below. Every other tunable is a developer setting (Flight/DeveloperSettings.hpp).

#include <cstdint>
#include <string>
#include <vector>

namespace Flight {

struct PlayerSettings {
  // Standard (and Hover and Fly, built on it): how fast a held turn turns, deg/s, and a held climb or descent climbs,
  // m/s
  float standardTurnRate = 90.0f;
  float standardClimbRate = 8.0f;
  // Standard in the cars' mode lists (IsEnabled)
  bool standardEnabled = true;
};

// The instance the modes read, written by Mod Settings' callbacks.
PlayerSettings &Player();

// One Mod Settings variable: the field's name (its key in Mod Settings' user.ini), the localization keys of its
// name and description, the default, step and range, and its row in the Flight Modes category.
struct PlayerSetting {
  const char *name;
  const char *displayName;
  const char *description;
  float defaultValue, step, min, max;
  float PlayerSettings::*field;
  uint32_t order;
};

// The variables, all under kFlightModesCategory. Their class and names are the ones Hover & Fly had, so the values
// players saved ([LTBF_HoverFly] hoverFlyTurnRate = ...) carry over to Standard.
const std::vector<PlayerSetting> &PlayerSettingsTable();
constexpr const char *kPlayerSettingsMod = "Let There Be Flight";
constexpr const char *kPlayerSettingsClass = "LTBF_HoverFly";

// The menu's Flight Modes category, shared with the script's mode settings, and each row's place in it (Mod Settings'
// order): Standard 0 to 2, Drone 3 to 5 and Extra Flight Modes 6 (redscript), the Autopilot 7 to 9.
constexpr const char *kFlightModesCategory = "UI-Settings-Flight-Mode-Settings";

// Writes the value Mod Settings gives for a variable into aSettings, clamped to its range; false for a name not in the
// table.
bool SetPlayerSetting(PlayerSettings &aSettings, const std::string &aName, float aValue);

// Standard's switch (PlayerSettings::standardEnabled), registered under the class and name the script Hover & Fly had
// it, so the value players saved ([FlightModeHoverFly] enabled) carries over.
constexpr const char *kStandardEnabledClass = "FlightModeHoverFly";
constexpr const char *kStandardEnabledName = "enabled";
constexpr const char *kStandardEnabledDisplayName = "UI-Settings-Standard-Enabled";
constexpr const char *kStandardEnabledDescription = "UI-Settings-Standard-Enabled-Description";

// The Autopilot's Mod Settings (FlightModeAutopilot): registered from C++ under the class name the script registered
// them under, with the same names, keys and defaults, so the values players saved in Mod Settings' user.ini
// ([FlightModeAutopilot], name = value) carry over. enabled is the mode's switch (IsEnabled); the others are written
// into the Autopilot's tuning (Navigation::AutopilotParams) when the mode is set up and engaged, and when the player
// changes one. brakeAccel is a developer value (docs/design/settings-audit.md): out of the menu, so
// a saved value is no longer read and it stays at its default (the developer setting autopilot.brakeAccel overrides
// the tuning).
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
  const char *displayName;
  const char *description;
  bool isBool;
  float defaultValue, step, min, max;
  uint32_t order; // its row in kFlightModesCategory
};

// The variables in the menu, all under kFlightModesCategory.
const std::vector<AutopilotSetting> &AutopilotSettingsTable();
constexpr const char *kAutopilotSettingsClass = "FlightModeAutopilot";

// Writes the value Mod Settings gives (a Bool as 0 or 1) into aSettings, a Float clamped to its range; false for a name
// not in the table (brakeAccel included). On AutopilotPlayer() it bumps AutopilotSettingsGeneration.
bool SetAutopilotSetting(AutopilotSettings &aSettings, const std::string &aName, float aValue);

} // namespace Flight
