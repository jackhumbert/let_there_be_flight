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

namespace {
uint32_t autopilotGeneration = 0;
}

AutopilotSettings &AutopilotPlayer() {
  static AutopilotSettings settings;
  return settings;
}

uint32_t AutopilotSettingsGeneration() { return autopilotGeneration; }

const std::vector<AutopilotSetting> &AutopilotSettingsTable() {
  // FlightModeAutopilot.reds' @runtimeProperty fields (the last commit with it: c3e24ab), less maxSpeed and
  // lateralAccel (dropped, docs/design/native-ports.md, M6 decisions)
  static const std::vector<AutopilotSetting> table = {
      {"enabled", "UI-Settings-Flight-Mode-Settings", "UI-Settings-Autopilot-Enabled",
       "UI-Settings-Autopilot-Enabled-Description", true, 1.0f, 0.0f, 0.0f, 0.0f},
      {"brakeAccel", "UI-Settings-Autopilot", "UI-Settings-Autopilot-Braking",
       "UI-Settings-Autopilot-Braking-Description", false, 6.0f, 0.5f, 2.0f, 15.0f},
      {"landAtDestination", "UI-Settings-Autopilot", "UI-Settings-Autopilot-Land",
       "UI-Settings-Autopilot-Land-Description", true, 1.0f, 0.0f, 0.0f, 0.0f},
      {"padRange", "UI-Settings-Autopilot", "UI-Settings-Autopilot-Pad-Range",
       "UI-Settings-Autopilot-Pad-Range-Description", false, 0.0f, 25.0f, 0.0f, 500.0f},
  };
  return table;
}

bool SetAutopilotSetting(AutopilotSettings &aSettings, const std::string &aName, float aValue) {
  const AutopilotSetting *found = nullptr;
  for (const auto &s : AutopilotSettingsTable()) {
    if (aName == s.name) {
      found = &s;
    }
  }
  if (!found) {
    return false;
  }
  const float value = found->isBool ? aValue : std::clamp(aValue, found->min, found->max);
  if (aName == "enabled") {
    aSettings.enabled = value != 0.0f;
  } else if (aName == "brakeAccel") {
    aSettings.brakeAccel = value;
  } else if (aName == "landAtDestination") {
    aSettings.landAtDestination = value != 0.0f;
  } else {
    aSettings.padRange = value;
  }
  if (&aSettings == &AutopilotPlayer()) {
    ++autopilotGeneration;
  }
  return true;
}

} // namespace Flight
