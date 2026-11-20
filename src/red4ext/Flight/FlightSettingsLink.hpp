#pragma once

// Registers the player's settings (Flight/Settings.hpp) with Mod Settings from C++, at plugin load.

#include <RedLib.hpp>

#include <cstdint>

// Land at Destination's values in RTTI (Flight::kLandingModeType), in Navigation::LandingMode's order: Mod Settings
// lists them and shows each as UI-Settings-Autopilot-Land-<name>.
enum class FlightAutopilotLanding : int32_t { Hover = 0, Wheels = 1, TouchDown = 2 };
RTTI_DEFINE_ENUM(FlightAutopilotLanding);

namespace FlightSettingsLink {

// True when Mod Settings was found and the variables were queued with it.
bool Register();

} // namespace FlightSettingsLink
