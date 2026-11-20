#pragma once

// Where a car rests on its wheels, for the Autopilot's set-down (docs/design/autopilot.md, Descent): the ground under
// the four wheel emitters, and how high the car's origin sits over it when it stands on its wheels.
//
// The origin's height is measured when flight comes on (FlightConfiguration's OnActivationCore, before anything has
// moved the car): a car switched to flight standing still and level on its wheels has its origin that far over the
// ground under them. It is kept per vehicle record, so a summoned car of the same model has it too. A record never
// measured is estimated from the chassis collision shape's lowest point (IFlightConfiguration::chassisBottom, under
// the origin): the origin rests 2.2 times that depth over the ground (the Shion, Hellhound, Archer Quartz and Kusanagi
// within 1.5 cm of what was measured), or 0.4 m before the chassis is measured.

#include "Flight/Modes.hpp"

#include <RED4ext/Scripting/IScriptable.hpp>

class FlightComponent;

namespace WheelRest {

// The ground under aComponent's four wheel emitters (the static world, as FlightGroundNative::Find casts): valid when
// every ray hits, the mean of the hits' heights and their spread (highest less lowest), world metres.
struct Ground {
  bool valid = false;
  float z = 0.0f;
  float spread = 0.0f;
};
Ground Under(RED4ext::IScriptable *aComponent);

// Measures the origin's height over the ground under the wheels when the car looks to be standing on them (level, the
// four hits within 8 cm of each other, the origin within 0.6 m of them) and keeps it for the car's record; logged.
void Measure(FlightComponent *aComponent);

// The origin's height over the ground resting on the wheels (m): measured for this car's record, else the estimate.
// aMeasured is set to whether it was measured.
float OriginHeight(RED4ext::IScriptable *aComponent, bool *aMeasured = nullptr);

// How high a point at world height aZ on the car (its centre, stats.d_position) is over the ground when the car rests
// on its wheels: OriginHeight plus the point's height over the origin now. 0 when the car is not found.
float RestHeight(RED4ext::IScriptable *aComponent, float aZ);

} // namespace WheelRest
