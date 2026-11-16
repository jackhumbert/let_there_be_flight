#pragma once

// The player's Mod Settings for the C++ flight modes (docs/design/flight-modes.md, Settings: few, in plain
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
  // Hover & Fly as 0.3.20 flew it (FlightModeHoverFlyClassic) in the cars' mode lists
  bool hoverFlyClassicEnabled = true;
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

// The variables, all under kFlightModesCategory: Standard's turn and climb speed, stored as [LTBF_HoverFly]
// hoverFlyTurnRate and hoverFlyClimbRate (no release before Standard had them, so nothing carries over from one).
const std::vector<PlayerSetting> &PlayerSettingsTable();
constexpr const char *kPlayerSettingsMod = "Let There Be Flight";
constexpr const char *kPlayerSettingsClass = "LTBF_HoverFly";

// The menu's Flight Modes category, shared with the script's mode settings, and each row's place in it (Mod Settings'
// order): Standard 0 to 2, Hover & Fly 3, Drone 4 to 6 and Extra Flight Modes 7 (redscript), the Autopilot 8 to 10.
constexpr const char *kFlightModesCategory = "UI-Settings-Flight-Mode-Settings";

// Writes the value Mod Settings gives for a variable into aSettings, clamped to its range; false for a name not in the
// table.
bool SetPlayerSetting(PlayerSettings &aSettings, const std::string &aName, float aValue);

// Standard's switch (PlayerSettings::standardEnabled), under its own class: a new mode, on for everyone until they
// switch it off.
constexpr const char *kStandardEnabledClass = "FlightModeStandard";
constexpr const char *kStandardEnabledName = "enabled";
constexpr const char *kStandardEnabledDisplayName = "UI-Settings-Standard-Enabled";
constexpr const char *kStandardEnabledDescription = "UI-Settings-Standard-Enabled-Description";

// Hover & Fly's switch (PlayerSettings::hoverFlyClassicEnabled), registered under the class and name the script
// Hover & Fly had it in releases up to 0.3.20, so the value players saved in Mod Settings' user.ini
// ([FlightModeHoverFly] enabled) still switches the mode it was saved for. The mode's own class has a new name
// (FlightModeHoverFlyClassic.hpp).
constexpr const char *kHoverFlyClassicEnabledClass = "FlightModeHoverFly";
constexpr const char *kHoverFlyClassicEnabledName = "enabled";
constexpr const char *kHoverFlyClassicEnabledDisplayName = "UI-Settings-Hover-And-Fly-Enabled";
constexpr const char *kHoverFlyClassicEnabledDescription = "UI-Settings-Hover-And-Fly-Enabled-Description";

// The Autopilot's Mod Settings (FlightModeAutopilot): registered from C++ under the class name the script registered
// them under, with the same names, keys and defaults, so the values players saved in Mod Settings' user.ini
// ([FlightModeAutopilot], name = value) carry over. enabled is the mode's switch (IsEnabled); the others are written
// into the Autopilot's tuning (Navigation::AutopilotParams) when the mode is set up and engaged, and when the player
// changes one. brakeAccel is a developer value (docs/design/flight-modes.md, Developer values): out of the menu, so
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
