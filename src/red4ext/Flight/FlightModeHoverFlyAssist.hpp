#pragma once

// The redesigned Hover & Fly as a C++ flight mode (docs/design/hover-fly-native.md step 4, the maths
// Flight::HoverFlyAssist designed in tools/flightsim: docs/design/hover-fly-feel.md). Each frame it gathers once what
// the maths reads: the component's stats, controls, thruster tensor and hover height (script fields, by name), the
// ground (FlightGroundNative, the script's rays in C++), the car's torque response (Flight::TorqueResponse) and the
// player's two settings (Flight::Player: turn and climb speed). The rest of HoverFlyAssistParams are the code defaults
// (developer settings when they need changing). Until it replaces Hover & Fly (step 7) it takes Hover & Fly's place in
// the list only with the developer setting assistHoverFly.

#include "Flight/Mode.hpp"
#include "Flight/Modes.hpp"

struct FlightModeHoverFlyAssist : FlightMode {
  Flight::HoverFlyState state;
  // the state's fields as FlightModeHoverFly names them, for the recorder and the bridge (copied after each step)
  float hovering = 1.0f;
  float referenceZ = 0.0f;
  bool holdingAltitude = false;

  void OnActivateNative() override;
  void OnUpdateNative(float aTimeDelta) override;
  RED4ext::CString DescriptionNative() override { return "Hover & Fly (new)"; }

  RTTI_IMPL_TYPEINFO(FlightModeHoverFlyAssist);
  RTTI_IMPL_ALLOCATOR();
};

RTTI_DEFINE_CLASS(FlightModeHoverFlyAssist, {
  RTTI_PARENT(FlightMode);
  RTTI_PROPERTY(hovering);
  RTTI_PROPERTY(referenceZ);
  RTTI_PROPERTY(holdingAltitude);
});
