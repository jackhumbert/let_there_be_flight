#pragma once

// Registers the player's settings (Flight/Settings.hpp) with Mod Settings from C++, at plugin load.

#include <RedLib.hpp>

#include <cstdint>

// Land at Destination's values in RTTI (Flight::kLandingModeType), in Navigation::LandingMode's order: Mod Settings
// lists them and shows each as UI-Settings-Autopilot-Land-<name>.
enum class FlightAutopilotLanding : int32_t { Hover = 0, Wheels = 1, TouchDown = 2 };
RTTI_DEFINE_ENUM(FlightAutopilotLanding);
// Autopilot Style's values in RTTI (Flight::kAttitudeStyleType), in Navigation::AttitudeStyle's order: shown as
// UI-Settings-Autopilot-Style-<name>.
enum class FlightAutopilotStyle : int32_t { Drone = 0, Spinner = 1 };
RTTI_DEFINE_ENUM(FlightAutopilotStyle);

namespace FlightSettingsLink {

// True when Mod Settings was found and the variables were queued with it.
bool Register();

} // namespace FlightSettingsLink
