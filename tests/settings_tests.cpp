// The player's Mod Settings for the C++ modes (src/red4ext/Flight/Settings): the variables Mod Settings is given, and
// the callback's writes; and Handling's sets (Flight/Handling.hpp).
#include "check.hpp"

#include "Flight/Handling.hpp"
#include "Flight/Settings.hpp"

#include <set>
#include <string>

using namespace Flight;

TEST(settings_table) {
  // the player's few settings (docs/design/flight-modes.md, Settings): the turn and the climb, each default inside its range on a step,
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
  CHECK_NEAR(defaults.standardTurnRate, 90.0f, 0.0f);
  CHECK_NEAR(defaults.standardClimbRate, 8.0f, 0.0f);
}

TEST(settings_callback_writes) {
  PlayerSettings s;
  CHECK(SetPlayerSetting(s, "hoverFlyTurnRate", 120.0f));
  CHECK_NEAR(s.standardTurnRate, 120.0f, 0.0f);
  CHECK(SetPlayerSetting(s, "hoverFlyClimbRate", 50.0f)); // out of range: clamped
  CHECK_NEAR(s.standardClimbRate, 20.0f, 0.0f);
  CHECK(!SetPlayerSetting(s, "noSuchSetting", 1.0f));
}

TEST(settings_standard_switch_key) {
  // Hover & Fly's switch is registered where the script Hover & Fly had it up to 0.3.20 (FlightModeHoverFly.reds:
  // `enabled`), so a value saved in Mod Settings' user.ini ([FlightModeHoverFly] enabled) still switches Hover & Fly:
  // a player who turned it off keeps it off. Standard, a new mode, has a key of its own and so starts on. Standard's
  // switch and its two speeds are in the Flight Modes category, first.
  CHECK(std::string(kHoverFlyClassicEnabledClass) == "FlightModeHoverFly");
  CHECK(std::string(kHoverFlyClassicEnabledName) == "enabled");
  CHECK(std::string(kStandardEnabledClass) == "FlightModeStandard");
  CHECK(std::string(kStandardEnabledName) == "enabled");
  CHECK(std::string(kFlightModesCategory) == "UI-Settings-Flight-Mode-Settings");
  CHECK(std::string(kStandardEnabledDisplayName) == "UI-Settings-Standard-Enabled");
  CHECK(std::string(kPlayerSettingsClass) == "LTBF_HoverFly");
  CHECK(PlayerSettings{}.standardEnabled);
  const auto &table = PlayerSettingsTable();
  CHECK(table.size() == 2 && std::string(table[0].name) == "hoverFlyTurnRate" && table[0].order == 1 &&
        std::string(table[1].name) == "hoverFlyClimbRate" && table[1].order == 2);
}

TEST(settings_handling_default_keeps_the_developer_values) {
  // Default (and a number that is no choice) leaves the force tail's values as the developer set them
  ForceParams p;
  p.generalDampFactorAngular = 7.0f;
  p.generalYawDirectionalityFactor = 9.0f;
  p.brakeFactorLinear = 2.0f;
  for (int32_t h : {static_cast<int32_t>(Handling::Default), 5, -1}) {
    const ForceParams q = WithHandling(p, h);
    CHECK_NEAR(q.generalDampFactorAngular, 7.0f, 0.0f);
    CHECK_NEAR(q.generalYawDirectionalityFactor, 9.0f, 0.0f);
    CHECK_NEAR(q.brakeFactorLinear, 2.0f, 0.0f);
  }
  // the code's defaults are FlightSettings' (HandlingFactors and ForceParams agree)
  const HandlingFactors d;
  const ForceParams f;
  CHECK_NEAR(d.generalDampFactorAngular, f.generalDampFactorAngular, 0.0f);
  CHECK_NEAR(d.generalDampFactorAngularMax, f.generalDampFactorAngularMax, 0.0f);
  CHECK_NEAR(d.generalPitchDirectionalityFactor, f.generalPitchDirectionalityFactor, 0.0f);
  CHECK_NEAR(d.generalYawDirectionalityFactor, f.generalYawDirectionalityFactor, 0.0f);
  CHECK_NEAR(d.generalYawDampFactor, f.generalYawDampFactor, 0.0f);
}

TEST(settings_handling_loose_and_tight) {
  // Loose damps less and turns the velocity less than Default; Tight more; neither touches the brakes, the linear
  // damping (the top speed) or the rest
  ForceParams p;
  p.brakeFactorAngular = 3.0f;
  const ForceParams loose = WithHandling(p, static_cast<int32_t>(Handling::Loose));
  const ForceParams tight = WithHandling(p, static_cast<int32_t>(Handling::Tight));
  CHECK(loose.generalDampFactorAngular < p.generalDampFactorAngular &&
        p.generalDampFactorAngular < tight.generalDampFactorAngular);
  CHECK(loose.generalYawDirectionalityFactor < p.generalYawDirectionalityFactor &&
        p.generalYawDirectionalityFactor < tight.generalYawDirectionalityFactor);
  CHECK(loose.generalDampFactorAngularMax < tight.generalDampFactorAngularMax);
  CHECK_NEAR(tight.generalDampFactorLinear, p.generalDampFactorLinear, 0.0f);
  CHECK_NEAR(loose.generalDampFactorLinear, p.generalDampFactorLinear, 0.0f);
  CHECK_NEAR(loose.brakeFactorAngular, 3.0f, 0.0f);
  CHECK_NEAR(tight.brakeFactorAngular, 3.0f, 0.0f);
  CHECK_NEAR(tight.collisionRecoveryDelay, p.collisionRecoveryDelay, 0.0f);
  // Standard: Loose brakes and grips less, Tight more; Default leaves the gains; the turn and climb are not handling
  const StandardGains g;
  const StandardGains gl = WithHandling(g, static_cast<int32_t>(Handling::Loose));
  const StandardGains gt = WithHandling(g, static_cast<int32_t>(Handling::Tight));
  const StandardGains gd = WithHandling(g, static_cast<int32_t>(Handling::Default));
  CHECK(gl.brake < g.brake && g.brake < gt.brake && gl.sideBrake < g.sideBrake && g.sideBrake < gt.sideBrake);
  CHECK_NEAR(gd.brake, g.brake, 0.0f);
  CHECK_NEAR(gd.sideBrake, g.sideBrake, 0.0f);
  CHECK_NEAR(gt.maxYawRate, g.maxYawRate, 0.0f);
  CHECK_NEAR(gt.maxClimbRate, g.maxClimbRate, 0.0f);
}
