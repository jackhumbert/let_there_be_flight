#pragma once

// The player's Mod Settings for the C++ flight modes (docs/design/hover-fly-native.md, decision 1: few, in plain
// words), registered from C++ with Mod Settings' AddVariable (FlightSettingsLink.cpp). Without Mod Settings they keep
// the defaults below. Every other tunable is a developer setting (Flight/DeveloperSettings.hpp).

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

} // namespace Flight
