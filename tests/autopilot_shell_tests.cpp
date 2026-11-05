// The Autopilot's game shell (docs/design/native-ports.md, M6 step 6, tests kind 6): what other tools and players'
// saved files depend on stays as the script mode had it (FlightModeAutopilot.reds at c3e24ab).
//   - the Mod Settings keys: the class, names, types, categories, localization keys, defaults and ranges;
//   - the developer overrides of the tuning by name (autopilot.<name>);
//   - the telemetry CSV's header and row width;
//   - every member of FlightModeAutopilot that script in this repo and in cp2077_tools' LTBF helpers uses is declared,
//     and every native field and function declared is registered by the C++.
#include "check.hpp"

#include "Flight/DeveloperSettings.hpp"
#include "Flight/Settings.hpp"
#include "Navigation/AutopilotParams.hpp"
#include "Navigation/AutopilotTelemetry.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <regex>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>

using namespace Flight;

namespace {

std::string Read(const std::filesystem::path &aPath) {
  std::ifstream in(aPath, std::ios::binary);
  std::stringstream text;
  text << in.rdbuf();
  return text.str();
}

std::set<std::string> Matches(const std::string &aText, const std::regex &aPattern, int aGroup = 1) {
  std::set<std::string> out;
  for (auto it = std::sregex_iterator(aText.begin(), aText.end(), aPattern); it != std::sregex_iterator(); ++it) {
    out.insert((*it)[aGroup].str());
  }
  return out;
}

} // namespace

// Mod Settings saves a value as "[<class>] <property> = <value>" in its user.ini (mod_settings ModSettings.cpp
// WriteToFile: the section is the class's name, the key the variable's name). The script registered its
// @runtimeProperty fields under its own class, so a script field's key is [FlightModeAutopilot] <field>
// (ModSettings::ProcessScriptData: mod->classes[scriptClass->name], the variable named prop->name). The C++ registers
// under kAutopilotSettingsClass with each row's name (FlightSettingsLink: className, propertyName). These are the
// script's declarations at c3e24ab, less the two dropped settings.
TEST(autopilot_shell_mod_settings_keys) {
  struct Expected {
    const char *name, *category, *displayName, *description;
    bool isBool;
    float defaultValue, step, min, max;
  };
  const Expected expected[] = {
      {"enabled", "UI-Settings-Flight-Mode-Settings", "UI-Settings-Autopilot-Enabled",
       "UI-Settings-Autopilot-Enabled-Description", true, 1.0f, 0.0f, 0.0f, 0.0f},
      {"brakeAccel", "UI-Settings-Autopilot", "UI-Settings-Autopilot-Braking",
       "UI-Settings-Autopilot-Braking-Description", false, 6.0f, 0.5f, 2.0f, 15.0f},
      {"landAtDestination", "UI-Settings-Autopilot", "UI-Settings-Autopilot-Land",
       "UI-Settings-Autopilot-Land-Description", true, 1.0f, 0.0f, 0.0f, 0.0f},
      {"padRange", "UI-Settings-Autopilot", "UI-Settings-Autopilot-Pad-Range",
       "UI-Settings-Autopilot-Pad-Range-Description", false, 0.0f, 25.0f, 0.0f, 500.0f},
  };
  CHECK(std::string(kAutopilotSettingsClass) == "FlightModeAutopilot");
  CHECK(std::string(kPlayerSettingsMod) == "Let There Be Flight");
  const auto &table = AutopilotSettingsTable();
  CHECK(table.size() == std::size(expected));
  for (size_t i = 0; i < std::min(table.size(), std::size(expected)); ++i) {
    const auto &t = table[i];
    const auto &e = expected[i];
    CHECK(std::string(t.name) == e.name);
    CHECK(std::string(t.category) == e.category);
    CHECK(std::string(t.displayName) == e.displayName);
    CHECK(std::string(t.description) == e.description);
    CHECK(t.isBool == e.isBool);
    CHECK_NEAR(t.defaultValue, e.defaultValue, 0.0);
    if (!e.isBool) {
      CHECK_NEAR(t.step, e.step, 0.0);
      CHECK_NEAR(t.min, e.min, 0.0);
      CHECK_NEAR(t.max, e.max, 0.0);
    }
  }
  // the same names and defaults as the script's fields (tests/autopilot_script_defaults.txt, read from c3e24ab)
  const std::string script = Read(LTBF_SOURCE_DIR "/tests/autopilot_script_defaults.txt");
  for (const auto &t : table) {
    const std::string decl = std::string("public let ") + t.name + ": " + (t.isBool ? "Bool" : "Float") + " = " +
                             (t.isBool ? (t.defaultValue != 0.0f ? "true" : "false") : "");
    CHECK(script.find(decl) != std::string::npos);
  }
  // the code's defaults are the table's
  const AutopilotSettings defaults;
  CHECK(defaults.enabled && defaults.landAtDestination);
  CHECK_NEAR(defaults.brakeAccel, 6.0, 0.0);
  CHECK_NEAR(defaults.padRange, 0.0, 0.0);
  const Navigation::AutopilotParams params;
  CHECK_NEAR(params.brakeAccel, defaults.brakeAccel, 0.0);
  CHECK(params.landAtDestination == defaults.landAtDestination);
  CHECK_NEAR(params.padRange, defaults.padRange, 0.0);
  // the dropped two are not registered
  for (const auto &t : table) {
    CHECK(std::string(t.name) != "maxSpeed" && std::string(t.name) != "lateralAccel");
  }
}

TEST(autopilot_shell_mod_settings_set) {
  AutopilotSettings s;
  CHECK(SetAutopilotSetting(s, "enabled", 0.0f));
  CHECK(!s.enabled);
  CHECK(SetAutopilotSetting(s, "brakeAccel", 40.0f)); // out of range: clamped
  CHECK_NEAR(s.brakeAccel, 15.0, 0.0);
  CHECK(SetAutopilotSetting(s, "landAtDestination", 0.0f));
  CHECK(!s.landAtDestination);
  CHECK(SetAutopilotSetting(s, "padRange", 100.0f));
  CHECK_NEAR(s.padRange, 100.0, 0.0);
  CHECK(!SetAutopilotSetting(s, "maxSpeed", 30.0f));
  // a change to the player's instance is seen by the modes
  const uint32_t before = AutopilotSettingsGeneration();
  CHECK(SetAutopilotSetting(AutopilotPlayer(), "padRange", 0.0f));
  CHECK(AutopilotSettingsGeneration() != before);
}

TEST(autopilot_shell_developer_overrides) {
  DeveloperSettings d;
  std::string message;
  const uint32_t before = d.autopilotGeneration;
  CHECK(SetDeveloperSetting(d, "autopilot.raceRouteHeight", "55", message) && message.empty());
  CHECK(SetDeveloperSetting(d, "autopilot.droneBrakeTurn", "on", message));
  CHECK(SetDeveloperSetting(d, "autopilot.landingSearch", "7.6", message));
  CHECK(SetDeveloperSetting(d, "autopilot.raceRouteHeight", "60", message)); // keeps its place
  CHECK(d.autopilotGeneration == before + 4);
  CHECK(d.autopilot.size() == 3);
  CHECK(!SetDeveloperSetting(d, "autopilot.maxSpeed", "10", message) && !message.empty());
  CHECK(!SetDeveloperSetting(d, "autopilot.raceRouteHeight", "high", message) && !message.empty());
  CHECK(!SetDeveloperSetting(d, "autopilot.droneBrakeTurn", "maybe", message));
  std::string value;
  CHECK(GetDeveloperSetting(d, "autopilot.raceRouteHeight", value) && value == "60");
  CHECK(GetDeveloperSetting(d, "autopilot.droneBrakeTurn", value) && value == "true");
  CHECK(GetDeveloperSetting(d, "autopilot.landingSearch", value) && value == "8");
  CHECK(GetDeveloperSetting(d, "autopilot.approachHeight", value) && value == "20"); // no override: the default
  // applied as FlightModeAutopilot applies them
  Navigation::AutopilotParams p;
  for (const auto &[name, number] : d.autopilot) {
    CHECK(Navigation::SetAutopilotParam(p, name, number));
  }
  CHECK_NEAR(p.raceRouteHeight, 60.0, 0.0);
  CHECK(p.droneBrakeTurn);
  CHECK(p.landingSearch == 8);
  // developer.ini and the log line
  DeveloperSettings f;
  CHECK(LoadDeveloperSettings(f, "autopilot.approachHeight = 30\nautopilot.nothing = 1\n").size() == 1);
  CHECK(f.autopilot.size() == 1);
  const auto changed = ChangedDeveloperSettings(f);
  CHECK(changed.size() == 1 && changed[0] == "autopilot.approachHeight = 30");
}

// The header as FlightModeAutopilot.WriteTelemetry wrote it (c3e24ab), parsed by name by tools/telemetry,
// tools/navplan/plot.py, tools/acoustics/flyby.py and cp2077_tools' surge_analyse.py and sweep_follower.py.
TEST(autopilot_shell_telemetry_header) {
  const std::string script =
      std::string("t,x,y,z,vx,vy,vz,speed,speed2d,target,rate,cmd,off,following,maneuver,on_line,segment,") +
      "plan_x,plan_y,plan_z,cmd_fwd_x,cmd_fwd_y,cmd_fwd_z,cmd_up_x,cmd_up_y,cmd_up_z,fwd_x,fwd_y,fwd_z,up_x,up_y,up_z," +
      "attitude_error,plan_cmd,cmd_car,up_cmd_car,plan_pitch,cmd_pitch,car_pitch,collision_timer," +
      "drone,cmd_tilt,car_tilt,thrust_right,thrust_fwd,thrust_up,surge_held,drift,audio_surge";
  CHECK(script == Navigation::kAutopilotTelemetryHeader);
  const size_t columns = size_t(std::count(script.begin(), script.end(), ',')) + 1;
  CHECK(columns == 49);

  // a row has a value for each column, in order: a few known ones at their places
  Navigation::NavMap map;
  std::vector<Navigation::Helipad> pads;
  std::vector<Navigation::Gate> gates;
  Navigation::MapWorld world(map, pads, gates);
  Navigation::PlanQueue queue(map, false);
  Navigation::AutopilotParams params;
  Navigation::Rng rng(1);
  Navigation::Autopilot ap(world, queue, params, rng);
  ap.tracker.targetSpeed = 33.0f;
  ap.commandSurge = 0.25f;
  ap.status = "following";
  Flight::StatsState stats;
  stats.d_speed = 12.0f;
  stats.d_up = {0.0f, 0.0f, 1.0f};
  const auto row = Navigation::AutopilotTelemetryRow(ap, 1.5f, {1.0f, 2.0f, 3.0f}, stats, 0.4f);
  CHECK(row.size() == columns);
  if (row.size() == columns) {
    CHECK_NEAR(row[0], 1.5, 0.0);   // t
    CHECK_NEAR(row[3], 3.0, 0.0);   // z
    CHECK_NEAR(row[7], 12.0, 0.0);  // speed
    CHECK_NEAR(row[9], 33.0, 0.0);  // target
    CHECK_NEAR(row[13], 1.0, 0.0);  // following
    CHECK_NEAR(row[14], 0.0, 0.0);  // maneuver: none any more
    CHECK_NEAR(row[39], 0.4, 1e-6); // collision_timer
    CHECK_NEAR(row[40], 1.0, 0.0);  // drone
    CHECK_NEAR(row[42], 0.0, 1e-4); // car_tilt: upright
    CHECK_NEAR(row[48], 0.25, 0.0); // audio_surge
  }
}

namespace {

// the names FlightModeAutopilot.reds and FlightMode.reds declare (fields and functions)
std::set<std::string> Declared() {
  std::set<std::string> out;
  for (const char *file : {"/src/redscript/Flight/FlightModeAutopilot.reds", "/src/redscript/Flight/FlightMode.reds"}) {
    const std::string text = Read(std::string(LTBF_SOURCE_DIR) + file);
    for (const auto &n : Matches(text, std::regex(R"(\blet (\w+)\s*:)"))) {
      out.insert(n);
    }
    for (const auto &n : Matches(text, std::regex(R"(\bfunc (\w+)\s*\()"))) {
      out.insert(n);
    }
  }
  return out;
}

// The members script reaches on a FlightModeAutopilot, each "file: name": within each function, the variables it
// declares with that type (`x: ref<FlightModeAutopilot>`, `let x = ... as FlightModeAutopilot`, `let x =
// <helper>.Autopilot()`, as LtbfBridge and LtbfDraw find the mode) and the statics (FlightModeAutopilot.<name>).
std::set<std::string> Used(const std::filesystem::path &aFile) {
  std::set<std::string> out;
  const std::string text = Read(aFile);
  const std::string file = aFile.filename().string();
  // one piece per function
  const std::regex funcStart(R"(\bfunc \w+\s*\()");
  std::vector<size_t> starts{0};
  for (auto it = std::sregex_iterator(text.begin(), text.end(), funcStart); it != std::sregex_iterator(); ++it) {
    starts.push_back(size_t(it->position()));
  }
  starts.push_back(text.size());
  const std::regex typed(R"((\w+)\s*:\s*ref<FlightModeAutopilot>)");
  const std::regex cast(R"(let (\w+)\s*=[^;]*(as FlightModeAutopilot|\w+\.Autopilot\(\))\s*;)");
  for (size_t i = 0; i + 1 < starts.size(); ++i) {
    const std::string piece = text.substr(starts[i], starts[i + 1] - starts[i]);
    std::set<std::string> vars = Matches(piece, typed);
    for (const auto &v : Matches(piece, cast)) {
      vars.insert(v);
    }
    for (const auto &v : vars) {
      for (const auto &m : Matches(piece, std::regex("\\b" + v + R"(\.(\w+))"))) {
        out.insert(file + ": " + m);
      }
    }
  }
  for (const auto &m : Matches(text, std::regex(R"(\bFlightModeAutopilot\.(\w+))"))) {
    out.insert(file + ": " + m);
  }
  return out;
}

} // namespace

TEST(autopilot_shell_script_uses_declared) {
  const auto declared = Declared();
  CHECK(declared.count("status") && declared.count("FindGroundSpot") && declared.count("component"));
  std::vector<std::filesystem::path> dirs{std::filesystem::path(LTBF_SOURCE_DIR) / "src" / "redscript"};
  const auto tools = std::filesystem::path(LTBF_SOURCE_DIR) / ".." / "cp2077_tools" / "mods" / "let_there_be_flight";
  if (std::filesystem::exists(tools)) {
    dirs.push_back(tools);
  }
  size_t uses = 0;
  for (const auto &dir : dirs) {
    for (const auto &entry : std::filesystem::recursive_directory_iterator(dir)) {
      if (entry.path().extension() != ".reds" || entry.path().filename() == "FlightModeAutopilot.reds") {
        continue;
      }
      for (const auto &use : Used(entry.path())) {
        ++uses;
        const std::string name = use.substr(use.find(": ") + 2);
        if (!declared.count(name)) {
          std::printf("  %s: not declared by FlightModeAutopilot\n", use.c_str());
          CHECK(declared.count(name));
        }
      }
    }
  }
  // the summon's eight fields and its calls are among them
  CHECK(uses >= 15);
}

// Every native field and function FlightModeAutopilot.reds declares is registered by the C++ (a field by RTTI_PROPERTY
// in the header, as a tuning field from AutopilotParamTable or as a state field in DescribeFields), the tuning and state
// fields with their types.
TEST(autopilot_shell_native_fields_registered) {
  const std::string reds = Read(LTBF_SOURCE_DIR "/src/redscript/Flight/FlightModeAutopilot.reds");
  const std::string hpp = Read(LTBF_SOURCE_DIR "/src/red4ext/Flight/FlightModeAutopilot.hpp");
  const std::string cpp = Read(LTBF_SOURCE_DIR "/src/red4ext/Flight/FlightModeAutopilot.cpp");
  std::unordered_map<std::string, std::string> registered; // name -> script type ("" when not known here)
  for (const auto &n : Matches(hpp, std::regex(R"(RTTI_PROPERTY\((\w+)\))"))) {
    registered[n] = "";
  }
  for (const auto &r : Navigation::AutopilotParamTable()) {
    registered[r.name] = r.f ? "Float" : r.i ? "Int32" : "Bool";
  }
  const std::regex field(R"re(LTBF_AP_FIELD\((float|int32_t|bool), "(\w+)")re");
  for (auto it = std::sregex_iterator(cpp.begin(), cpp.end(), field); it != std::sregex_iterator(); ++it) {
    const std::string t = (*it)[1].str();
    registered[(*it)[2].str()] = t == "float" ? "Float" : t == "int32_t" ? "Int32" : "Bool";
  }
  const std::regex let(R"(native let (\w+)\s*:\s*(\w+))");
  size_t fields = 0;
  for (auto it = std::sregex_iterator(reds.begin(), reds.end(), let); it != std::sregex_iterator(); ++it) {
    ++fields;
    const std::string name = (*it)[1].str(), type = (*it)[2].str();
    auto found = registered.find(name);
    if (found == registered.end() || (!found->second.empty() && found->second != type)) {
      std::printf("  native let %s: %s is not registered with that type\n", name.c_str(), type.c_str());
      CHECK(false);
    }
  }
  CHECK(fields >= 111 + 60);
  const auto methods = Matches(hpp, std::regex(R"(RTTI_METHOD\((\w+)\))"));
  for (const auto &n : Matches(reds, std::regex(R"(native func (\w+)\s*\()"))) {
    if (!methods.count(n)) {
      std::printf("  native func %s is not registered\n", n.c_str());
      CHECK(methods.count(n));
    }
  }
}
