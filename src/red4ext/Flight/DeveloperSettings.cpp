// Developer settings (Flight/DeveloperSettings.hpp): the table of names, ranges and parsing.
#include "Flight/DeveloperSettings.hpp"

#include "Navigation/AutopilotParams.hpp"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <cmath>
#include <cstdio>

namespace Flight {

namespace {

struct Entry {
  const char *name;
  bool DeveloperSettings::*flag;
  float DeveloperSettings::*number;
  float min, max;
};

const Entry kEntries[] = {
    {"shapeDrag", &DeveloperSettings::shapeDrag, nullptr, 0.0f, 0.0f},
    {"dragK", nullptr, &DeveloperSettings::dragK, 0.1f, 20.0f},
    {"thrustStrength", nullptr, &DeveloperSettings::thrustStrength, 0.0f, 1.0f},
    {"thrustPerThruster", nullptr, &DeveloperSettings::thrustPerThruster, 100.0f, 100000.0f},
    {"nativeModeProbe", &DeveloperSettings::nativeModeProbe, nullptr, 0.0f, 0.0f},
    {"assistHoverFly", &DeveloperSettings::assistHoverFly, nullptr, 0.0f, 0.0f},
    {"acousticsRays", nullptr, &DeveloperSettings::acousticsRays, 0.0f, 4096.0f},
};

const Entry *Find(const std::string &aName) {
  for (const auto &e : kEntries)
    if (aName == e.name)
      return &e;
  return nullptr;
}

std::string Trim(const std::string &aText) {
  size_t a = 0, b = aText.size();
  while (a < b && std::isspace(static_cast<unsigned char>(aText[a])))
    ++a;
  while (b > a && std::isspace(static_cast<unsigned char>(aText[b - 1])))
    --b;
  return aText.substr(a, b - a);
}

std::string Lower(std::string aText) {
  std::transform(aText.begin(), aText.end(), aText.begin(), [](unsigned char c) { return char(std::tolower(c)); });
  return aText;
}

std::string Number(float aValue) {
  char text[32];
  std::snprintf(text, sizeof(text), "%g", aValue);
  return text;
}

bool ParseFlag(const std::string &aValue, bool &aFlag) {
  if (aValue == "true" || aValue == "1" || aValue == "on") {
    aFlag = true;
  } else if (aValue == "false" || aValue == "0" || aValue == "off") {
    aFlag = false;
  } else {
    return false;
  }
  return true;
}

// "autopilot.<name>": the AutopilotParams field's kind, false for a name it does not have
bool AutopilotField(const std::string &aName, const Navigation::AutopilotParamRef *&aRef) {
  const std::string prefix = kAutopilotPrefix;
  if (aName.rfind(prefix, 0) != 0) {
    return false;
  }
  const std::string field = aName.substr(prefix.size());
  for (const auto &r : Navigation::AutopilotParamTable()) {
    if (field == r.name) {
      aRef = &r;
      return true;
    }
  }
  return false;
}

bool SetAutopilotOverride(DeveloperSettings &aSettings, const std::string &aName, const std::string &aValue,
                          std::string &aMessage) {
  const Navigation::AutopilotParamRef *ref = nullptr;
  if (!AutopilotField(aName, ref)) {
    aMessage = "unknown developer setting '" + aName + "'";
    return false;
  }
  float number = 0.0f;
  bool flag = false;
  if (ref->b && ParseFlag(aValue, flag)) {
    number = flag ? 1.0f : 0.0f;
  } else {
    const auto [end, error] = std::from_chars(aValue.data(), aValue.data() + aValue.size(), number);
    if (error != std::errc() || end != aValue.data() + aValue.size() || !std::isfinite(number)) {
      aMessage = aName + ": '" + aValue + "' is not " + (ref->b ? "true or false" : "a number");
      return false;
    }
  }
  if (ref->i) {
    number = float(std::lround(number));
  } else if (ref->b) {
    number = number != 0.0f ? 1.0f : 0.0f;
  }
  const std::string field = aName.substr(std::string(kAutopilotPrefix).size());
  auto it = std::find_if(aSettings.autopilot.begin(), aSettings.autopilot.end(),
                         [&](const auto &aEntry) { return aEntry.first == field; });
  if (it != aSettings.autopilot.end()) {
    it->second = number;
  } else {
    aSettings.autopilot.emplace_back(field, number);
  }
  ++aSettings.autopilotGeneration;
  return true;
}

} // namespace

DeveloperSettings &Developer() {
  static DeveloperSettings settings;
  return settings;
}

bool SetDeveloperSetting(DeveloperSettings &aSettings, const std::string &aName, const std::string &aValue,
                         std::string &aMessage) {
  aMessage.clear();
  if (Trim(aName).rfind(kAutopilotPrefix, 0) == 0) {
    return SetAutopilotOverride(aSettings, Trim(aName), Lower(Trim(aValue)), aMessage);
  }
  const Entry *e = Find(Trim(aName));
  if (!e) {
    aMessage = "unknown developer setting '" + Trim(aName) + "'";
    return false;
  }
  const std::string value = Lower(Trim(aValue));
  if (e->flag) {
    if (!ParseFlag(value, aSettings.*e->flag)) {
      aMessage = std::string(e->name) + ": '" + aValue + "' is not true or false";
      return false;
    }
    return true;
  }
  float number = 0.0f;
  const auto [end, error] = std::from_chars(value.data(), value.data() + value.size(), number);
  if (error != std::errc() || end != value.data() + value.size()) {
    aMessage = std::string(e->name) + ": '" + aValue + "' is not a number";
    return false;
  }
  const float clamped = std::clamp(number, e->min, e->max);
  if (clamped != number)
    aMessage = std::string(e->name) + ": " + Number(number) + " clamped to " + Number(clamped) + " (" + Number(e->min) +
               " to " + Number(e->max) + ")";
  aSettings.*e->number = clamped;
  return true;
}

bool GetDeveloperSetting(const DeveloperSettings &aSettings, const std::string &aName, std::string &aValue) {
  const Navigation::AutopilotParamRef *ref = nullptr;
  if (AutopilotField(Trim(aName), ref)) {
    // the override, or the code default when there is none
    const Navigation::AutopilotParams defaults;
    float value = ref->f ? defaults.*ref->f : ref->i ? float(defaults.*ref->i) : (defaults.*ref->b ? 1.0f : 0.0f);
    for (const auto &[name, number] : aSettings.autopilot) {
      if (name == ref->name) {
        value = number;
      }
    }
    aValue = ref->b ? (value != 0.0f ? "true" : "false") : Number(value);
    return true;
  }
  const Entry *e = Find(Trim(aName));
  if (!e)
    return false;
  aValue = e->flag ? (aSettings.*e->flag ? "true" : "false") : Number(aSettings.*e->number);
  return true;
}

std::vector<std::string> LoadDeveloperSettings(DeveloperSettings &aSettings, const std::string &aText) {
  std::vector<std::string> messages;
  size_t start = 0;
  for (int line = 1; start <= aText.size(); ++line) {
    size_t end = aText.find('\n', start);
    if (end == std::string::npos)
      end = aText.size();
    const std::string text = Trim(aText.substr(start, end - start));
    start = end + 1;
    if (text.empty() || text[0] == ';' || text[0] == '#' || text[0] == '[')
      continue;
    const size_t eq = text.find('=');
    std::string message;
    if (eq == std::string::npos)
      message = "not 'name = value'";
    else
      SetDeveloperSetting(aSettings, text.substr(0, eq), text.substr(eq + 1), message);
    if (!message.empty())
      messages.push_back("line " + std::to_string(line) + ": " + message);
  }
  return messages;
}

std::vector<std::string> ChangedDeveloperSettings(const DeveloperSettings &aSettings) {
  const DeveloperSettings defaults;
  std::vector<std::string> out;
  for (const auto &e : kEntries) {
    std::string value, initial;
    GetDeveloperSetting(aSettings, e.name, value);
    GetDeveloperSetting(defaults, e.name, initial);
    if (value != initial)
      out.push_back(std::string(e.name) + " = " + value);
  }
  for (const auto &[name, number] : aSettings.autopilot) {
    std::string value;
    GetDeveloperSetting(aSettings, kAutopilotPrefix + name, value);
    out.push_back(kAutopilotPrefix + name + " = " + value);
  }
  return out;
}

} // namespace Flight
