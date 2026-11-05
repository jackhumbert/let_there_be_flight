// The player's Mod Settings for the C++ modes (Flight/Settings.hpp): the table and the setter.
#include "Flight/Settings.hpp"

#include <algorithm>

namespace Flight {

PlayerSettings &Player() {
  static PlayerSettings settings;
  return settings;
}

const std::vector<PlayerSetting> &PlayerSettingsTable() {
  static const std::vector<PlayerSetting> table = {
      {"hoverFlyTurnRate", "UI-Settings-Hover-And-Fly-Turn-Speed", "UI-Settings-Hover-And-Fly-Turn-Speed-Desc", 90.0f,
       10.0f, 30.0f, 180.0f, &PlayerSettings::hoverFlyTurnRate},
      {"hoverFlyClimbRate", "UI-Settings-Hover-And-Fly-Climb-Speed", "UI-Settings-Hover-And-Fly-Climb-Speed-Desc",
       8.0f, 1.0f, 2.0f, 20.0f, &PlayerSettings::hoverFlyClimbRate},
  };
  return table;
}

bool SetPlayerSetting(PlayerSettings &aSettings, const std::string &aName, float aValue) {
  for (const auto &s : PlayerSettingsTable()) {
    if (aName == s.name) {
      aSettings.*s.field = std::clamp(aValue, s.min, s.max);
      return true;
    }
  }
  return false;
}

} // namespace Flight
