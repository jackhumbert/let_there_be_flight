// The player's Mod Settings for the C++ modes (src/red4ext/Flight/Settings): the variables Mod Settings is given, and
// the callback's writes.
#include "check.hpp"

#include "Flight/Settings.hpp"

#include <set>
#include <string>

using namespace Flight;

TEST(settings_table) {
  // decision 1 (docs/design/hover-fly-native.md): the turn and the climb, each default inside its range on a step,
  // every name and localization key distinct
  const auto &table = PlayerSettingsTable();
  CHECK(table.size() == 2);
  std::set<std::string> names, keys;
  const PlayerSettings defaults;
  for (const auto &s : table) {
    CHECK(names.insert(s.name).second);
    CHECK(keys.insert(s.displayName).second && keys.insert(s.description).second);
    CHECK(std::string(s.displayName).rfind("UI-Settings-", 0) == 0);
    CHECK(s.min < s.max && s.step > 0.0f);
    CHECK(s.defaultValue >= s.min && s.defaultValue <= s.max);
    CHECK(defaults.*s.field == s.defaultValue);
  }
  CHECK_NEAR(defaults.hoverFlyTurnRate, 90.0f, 0.0f);
  CHECK_NEAR(defaults.hoverFlyClimbRate, 8.0f, 0.0f);
}

TEST(settings_callback_writes) {
  PlayerSettings s;
  CHECK(SetPlayerSetting(s, "hoverFlyTurnRate", 120.0f));
  CHECK_NEAR(s.hoverFlyTurnRate, 120.0f, 0.0f);
  CHECK(SetPlayerSetting(s, "hoverFlyClimbRate", 50.0f)); // out of range: clamped
  CHECK_NEAR(s.hoverFlyClimbRate, 20.0f, 0.0f);
  CHECK(!SetPlayerSetting(s, "noSuchSetting", 1.0f));
}
