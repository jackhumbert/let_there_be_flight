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
  // lists (an enum type's name as the type) from ModSettings::ApiVersion 1; before it the export is missing
  auto apiVersion = reinterpret_cast<uint32_t (*)()>(GetProcAddress(module, "ApiVersion"));
  const bool lists = apiVersion && apiVersion() >= 1;
  if (!lists) {
    spdlog::info("[Settings] this Mod Settings has no lists: Land at Destination and Autopilot Style are switches (on: "
                 "set down on the wheels, spinner style)");
  }
  // Mod Settings keeps the pointers until the game's scripts are loaded: the variables live as long as the plugin
  static std::vector<ModSettings::Variable> variables;
  variables.clear();
  variables.reserve(Flight::PlayerSettingsTable().size() + 2 + Flight::AutopilotSettingsTable().size());
  for (const auto &s : Flight::PlayerSettingsTable()) {
    ModSettings::Variable v{};
    v.modName = Flight::kPlayerSettingsMod;
    v.className = Flight::kPlayerSettingsClass;
    v.categoryName = Flight::kFlightModesCategory;
    v.propertyName = s.name;
    v.type = "Float";
    v.displayName = s.displayName;
    v.description = s.description;
    v.order = s.order;
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
  {
    // Standard's switch, alone in its class (Mod Settings calls a class's callbacks with each of its variables)
    ModSettings::Variable v{};
    v.modName = Flight::kPlayerSettingsMod;
    v.className = Flight::kStandardEnabledClass;
    v.categoryName = Flight::kFlightModesCategory;
    v.propertyName = Flight::kStandardEnabledName;
    v.type = "Bool";
    v.displayName = Flight::kStandardEnabledDisplayName;
    v.description = Flight::kStandardEnabledDescription;
    v.order = 0;
    v.defaultValue.b = true;
    v.callback = std::make_shared<ModSettings::runtime_class_callback_t>(
        [](RED4ext::CName aCategory, RED4ext::CName aProperty, ModSettings::ModVariableType aValue) {
          if (aProperty.ToString() != std::string(Flight::kStandardEnabledName))
            return;
          Flight::Player().standardEnabled = aValue.b;
          spdlog::info("[Settings] {}.{} = {}", Flight::kStandardEnabledClass, Flight::kStandardEnabledName, aValue.b);
        });
    variables.push_back(v);
  }
  {
    // Hover & Fly's switch, alone in its class too
    ModSettings::Variable v{};
    v.modName = Flight::kPlayerSettingsMod;
    v.className = Flight::kHoverFlyClassicEnabledClass;
    v.categoryName = Flight::kFlightModesCategory;
    v.propertyName = Flight::kHoverFlyClassicEnabledName;
    v.type = "Bool";
    v.displayName = Flight::kHoverFlyClassicEnabledDisplayName;
    v.description = Flight::kHoverFlyClassicEnabledDescription;
    v.order = 3;
    v.defaultValue.b = true;
    v.callback = std::make_shared<ModSettings::runtime_class_callback_t>(
        [](RED4ext::CName aCategory, RED4ext::CName aProperty, ModSettings::ModVariableType aValue) {
          if (aProperty.ToString() != std::string(Flight::kHoverFlyClassicEnabledName))
            return;
          Flight::Player().hoverFlyClassicEnabled = aValue.b;
          spdlog::info("[Settings] {}.{} = {}", Flight::kHoverFlyClassicEnabledClass, Flight::kHoverFlyClassicEnabledName, aValue.b);
        });
    variables.push_back(v);
  }
  for (const auto &s : Flight::AutopilotSettingsTable()) {
    ModSettings::Variable v{};
    v.modName = Flight::kPlayerSettingsMod;
    v.className = Flight::kAutopilotSettingsClass;
    v.categoryName = Flight::kFlightModesCategory;
    v.propertyName = s.name;
    // a list on a Mod Settings without them: the old Bool under the same name (its 0 and 1 are the list's first two)
    const auto kind = s.kind == Flight::AutopilotSetting::Kind::List && !lists ? Flight::AutopilotSetting::Kind::Bool
                                                                                : s.kind;
    v.type = kind == Flight::AutopilotSetting::Kind::Bool    ? RED4ext::CName("Bool")
             : kind == Flight::AutopilotSetting::Kind::List ? RED4ext::CName(s.listType)
                                                             : RED4ext::CName("Float");
    v.displayName = s.displayName;
    v.description = s.description;
    v.order = s.order;
    if (kind == Flight::AutopilotSetting::Kind::Bool) {
      v.defaultValue.b = s.defaultValue != 0.0f;
    } else if (kind == Flight::AutopilotSetting::Kind::List) {
      v.defaultValue.u32 = uint32_t(s.defaultValue);
    } else {
      v.defaultValue.f32 = s.defaultValue;
      v.stepValue.f32 = s.step;
      v.minValue.f32 = s.min;
      v.maxValue.f32 = s.max;
    }
    v.callback = std::make_shared<ModSettings::runtime_class_callback_t>(
        [lists](RED4ext::CName aCategory, RED4ext::CName aProperty, ModSettings::ModVariableType aValue) {
          // Mod Settings keeps runtime callbacks per class and calls each with every variable of the class
          // (ModClass::NotifyListeners), so the type comes from the property handed in, not from this variable. A
          // Bool's value is the union's first byte (the accepted value's bytes as the union), a list's its index.
          const std::string name = aProperty.ToString();
          const auto &table = Flight::AutopilotSettingsTable();
          auto it = std::find_if(table.begin(), table.end(), [&](const auto &aS) { return name == aS.name; });
          if (it == table.end())
            return;
          float value = aValue.f32;
          if (it->kind == Flight::AutopilotSetting::Kind::Bool || (it->kind == Flight::AutopilotSetting::Kind::List && !lists))
            value = aValue.b ? 1.0f : 0.0f;
          else if (it->kind == Flight::AutopilotSetting::Kind::List)
            value = float(aValue.u32);
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
