// The player's settings (Flight/Settings.hpp) registered with Mod Settings from C++: AddVariable is looked up in
// mod_settings.dll at plugin load (LTBF does not link it, Mod Settings is optional), and each variable's callback
// writes Flight::Player(). Mod Settings calls it once with the saved value when it registers the variable, and again
// whenever the player changes it.
#include "Flight/FlightSettingsLink.hpp"

#include "Flight/Settings.hpp"
#include "stdafx.hpp"

#include <functional>
#include <memory>
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
  variables.reserve(Flight::PlayerSettingsTable().size());
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
  for (auto &v : variables)
    addVariable(&v);
  spdlog::info("[Settings] {} C++ mode settings registered with Mod Settings", variables.size());
  return true;
}

} // namespace FlightSettingsLink
