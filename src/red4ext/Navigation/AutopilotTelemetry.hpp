#pragma once

// The Autopilot's telemetry: one row a frame to red4ext/logs/ltbf_autopilot_<time>.csv (FlightTelemetry) while its
// `telemetry` is on. Where the car is and how fast, the speed it is held to, and how its orientation follows the plan:
// the route's direction at the car, the nose and up the attitude control asked for last frame (targetForward,
// targetUp) and the car's own, with the angles between them. Taken at the start of the frame, before Step, so a
// command is paired with what the car made of it. The columns, their order and their meaning are the script's
// (FlightModeAutopilot.WriteTelemetry, the last commit with it: c3e24ab): tools/telemetry, tools/navplan/plot.py,
// tools/acoustics/flyby.py and other offline tools read them by name. The manoeuvre
// column is always 0 (the manoeuvres are gone) and drone always 1 (the only style).

#include "Autopilot.hpp"

#include "Flight/Stats.hpp"

#include <vector>

namespace Navigation {

// The header line, 49 columns.
extern const char *const kAutopilotTelemetryHeader;

// One row in the header's order: aClock s since the file was opened, aP the position the frame flies from (the checked
// one, Autopilot::JumpHeldOff), aStats the component's stats, aCollisionTimer FlightComponent.collisionTimer.
std::vector<float> AutopilotTelemetryRow(const Autopilot &aAutopilot, float aClock, const Vec3 &aP,
                                         const Flight::StatsState &aStats, float aCollisionTimer);

} // namespace Navigation
