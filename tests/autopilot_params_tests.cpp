// The Autopilot's tuning in C++ (src/red4ext/Navigation/AutopilotParams): the same defaults as the script's fields.
#include "check.hpp"

#include "Navigation/AutopilotParams.hpp"

#include <fstream>
#include <regex>
#include <sstream>
#include <string>
#include <unordered_map>

using namespace Navigation;

namespace {
// the script's `let name: Type = value;` defaults, by name
std::unordered_map<std::string, std::string> ScriptDefaults() {
  std::ifstream in(LTBF_SOURCE_DIR "/src/redscript/Flight/FlightModeAutopilot.reds");
  std::stringstream text;
  text << in.rdbuf();
  std::unordered_map<std::string, std::string> out;
  const std::regex let(R"(let (\w+): (?:Float|Bool|Int32) = ([^;]+);)");
  const std::string s = text.str();
  for (auto it = std::sregex_iterator(s.begin(), s.end(), let); it != std::sregex_iterator(); ++it) {
    out.emplace((*it)[1].str(), (*it)[2].str());
  }
  return out;
}
} // namespace

TEST(autopilot_params_defaults) {
  const auto script = ScriptDefaults();
  CHECK(!script.empty());
  const AutopilotParams p;
  for (const auto &r : AutopilotParamTable()) {
    auto it = script.find(r.name);
    if (it == script.end()) {
      std::printf("  %s: not in FlightModeAutopilot.reds\n", r.name);
      CHECK(it != script.end());
      continue;
    }
    if (r.f) {
      CHECK_NEAR(p.*r.f, std::stod(it->second), 1e-5);
    } else if (r.i) {
      CHECK(p.*r.i == std::stoi(it->second));
    } else {
      CHECK((p.*r.b ? "true" : "false") == it->second);
    }
  }
}

TEST(autopilot_params_set) {
  AutopilotParams p;
  CHECK(SetAutopilotParam(p, "raceRouteHeight", 55.0f));
  CHECK_NEAR(p.raceRouteHeight, 55.0, 1e-6);
  CHECK(SetAutopilotParam(p, "landingSearch", 7.6f));
  CHECK(p.landingSearch == 8);
  CHECK(SetAutopilotParam(p, "droneBrakeTurn", 1.0f));
  CHECK(p.droneBrakeTurn);
  CHECK(!SetAutopilotParam(p, "maxSpeed", 10.0f));
  // names unique
  const auto &t = AutopilotParamTable();
  for (size_t i = 0; i < t.size(); ++i) {
    for (size_t j = i + 1; j < t.size(); ++j) {
      CHECK(std::string(t[i].name) != t[j].name);
    }
  }
}
