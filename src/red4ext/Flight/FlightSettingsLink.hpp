#pragma once

// Registers the player's settings (Flight/Settings.hpp) with Mod Settings from C++, at plugin load.

namespace FlightSettingsLink {

// True when Mod Settings was found and the variables were queued with it.
bool Register();

} // namespace FlightSettingsLink
