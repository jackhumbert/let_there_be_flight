#include "FlightDeveloperSettings.hpp"

#include "Flight/DeveloperSettings.hpp"
#include "stdafx.hpp"

#include <fstream>
#include <sstream>

bool FlightDeveloperSettings::Set(RED4ext::CString aName, RED4ext::CString aValue) {
  std::string message;
  const bool ok = Flight::SetDeveloperSetting(Flight::Developer(), aName.c_str(), aValue.c_str(), message);
  if (!message.empty())
    spdlog::warn("[DeveloperSettings] {}", message);
  if (ok) {
    std::string value;
    Flight::GetDeveloperSetting(Flight::Developer(), aName.c_str(), value);
    spdlog::info("[DeveloperSettings] {} = {}", aName.c_str(), value);
  }
  return ok;
}

RED4ext::CString FlightDeveloperSettings::Get(RED4ext::CString aName) {
  std::string value;
  return Flight::GetDeveloperSetting(Flight::Developer(), aName.c_str(), value) ? value.c_str() : "";
}

RED4ext::CString FlightDeveloperSettings::Describe() {
  std::string out;
  for (const auto &line : Flight::ChangedDeveloperSettings(Flight::Developer()))
    out += (out.empty() ? "" : "\n") + line;
  return out.c_str();
}

void FlightDeveloperSettings::LoadFile(const std::filesystem::path &aFile) {
  if (!std::filesystem::exists(aFile))
    return;
  std::ifstream file(aFile);
  std::stringstream text;
  text << file.rdbuf();
  for (const auto &message : Flight::LoadDeveloperSettings(Flight::Developer(), text.str()))
    spdlog::warn("[DeveloperSettings] {}: {}", aFile.filename().string(), message);
  for (const auto &line : Flight::ChangedDeveloperSettings(Flight::Developer()))
    spdlog::info("[DeveloperSettings] {}", line);
}
