// The player's settings (Flight/Settings.hpp) registered with Mod Settings from C++: AddVariable is looked up in
// mod_settings.dll at plugin load (LTBF does not link it, Mod Settings is optional), and each variable's callback
// writes Flight::Player() or Flight::AutopilotPlayer(). Mod Settings calls it once with the saved value when it
// registers the variable (when the game's scripts are loaded), and again whenever the player changes it. Mod Settings
// keys a saved value by the class and property names given here ([className] propertyName = value in its user.ini,
// mod_settings ModSettings.cpp WriteToFile), as it does a script field's by its class and field name.
#include "Flight/FlightSettingsLink.hpp"

#include "Flight/Settings.hpp"
#include "stdafx.hpp"

#include <algorithm>
#include <functional>
#include <memory>
#include <string>
#include <ModSettings/ModSettings.hpp>
#include <windows.h>

namespace FlightSettingsLink {

bool Register() {
  auto module = GetModuleHandleW(L"mod_settings");
  auto addVariable =
      module ? reinterpret_cast<void (*)(ModSettings::Variable *)>(GetProcAddress(module, "AddVariable")) : nullptr;
  if (!addVariable) {
    spdlog::info("[Settings] Mod Settings not found: the C++ modes' settings keep their defaults");
    return false;
  }
  // Mod Settings keeps the pointers until the game's scripts are loaded: the variables live as long as the plugin
  static std::vector<ModSettings::Variable> variables;
  variables.clear();
  variables.reserve(Flight::PlayerSettingsTable().size() + Flight::AutopilotSettingsTable().size());
  uint32_t order = 0;
  for (const auto &s : Flight::PlayerSettingsTable()) {
    ModSettings::Variable v{};
    v.modName = Flight::kPlayerSettingsMod;
    v.className = Flight::kPlayerSettingsClass;
    v.categoryName = Flight::kPlayerSettingsCategory;
    v.propertyName = s.name;
    v.type = "Float";
    v.displayName = s.displayName;
    v.description = s.description;
    v.order = order++;
    v.defaultValue.f32 = s.defaultValue;
    v.stepValue.f32 = s.step;
    v.minValue.f32 = s.min;
    v.maxValue.f32 = s.max;
    v.callback = std::make_shared<ModSettings::runtime_class_callback_t>(
        [](RED4ext::CName aCategory, RED4ext::CName aProperty, ModSettings::ModVariableType aValue) {
          if (Flight::SetPlayerSetting(Flight::Player(), aProperty.ToString(), aValue.f32))
            spdlog::info("[Settings] {} = {}", aProperty.ToString(), aValue.f32);
        });
    variables.push_back(v);
  }
  order = 0;
  for (const auto &s : Flight::AutopilotSettingsTable()) {
    ModSettings::Variable v{};
    v.modName = Flight::kPlayerSettingsMod;
    v.className = Flight::kAutopilotSettingsClass;
    v.categoryName = s.category;
    v.propertyName = s.name;
    v.type = s.isBool ? "Bool" : "Float";
    v.displayName = s.displayName;
    v.description = s.description;
    v.order = order++;
    if (s.isBool) {
      v.defaultValue.b = s.defaultValue != 0.0f;
    } else {
      v.defaultValue.f32 = s.defaultValue;
      v.stepValue.f32 = s.step;
      v.minValue.f32 = s.min;
      v.maxValue.f32 = s.max;
    }
    v.callback = std::make_shared<ModSettings::runtime_class_callback_t>(
        [](RED4ext::CName aCategory, RED4ext::CName aProperty, ModSettings::ModVariableType aValue) {
          // Mod Settings keeps runtime callbacks per class and calls each with every variable of the class
          // (ModClass::NotifyListeners), so the type comes from the property handed in, not from this variable. A
          // Bool's value is the union's first byte (the accepted value's bytes as the union).
          const std::string name = aProperty.ToString();
          const auto &table = Flight::AutopilotSettingsTable();
          auto it = std::find_if(table.begin(), table.end(), [&](const auto &aS) { return name == aS.name; });
          if (it == table.end())
            return;
          const float value = it->isBool ? (aValue.b ? 1.0f : 0.0f) : aValue.f32;
          if (Flight::SetAutopilotSetting(Flight::AutopilotPlayer(), aProperty.ToString(), value))
            spdlog::info("[Settings] {}.{} = {}", Flight::kAutopilotSettingsClass, aProperty.ToString(), value);
        });
    variables.push_back(v);
  }
  for (auto &v : variables)
    addVariable(&v);
  spdlog::info("[Settings] {} C++ mode settings registered with Mod Settings", variables.size());
  return true;
}

} // namespace FlightSettingsLink
