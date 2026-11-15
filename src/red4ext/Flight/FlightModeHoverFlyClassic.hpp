#pragma once

// Hover & Fly as releases up to 0.3.20 flew it, as a C++ flight mode over Flight::HoverFlyClassic (Modes.hpp): the old
// standard modes' attitude PIDs and lift (Flight::AttitudeLift) under a hover band. Standard, which took the name's
// place as the mode flight starts in, asks every control for a speed and brings the car to rest; this one keeps the old
// feel, the car pitching and rolling with the input and keeping its momentum.
//
// Each frame it gathers what the maths reads: the component's stats, controls, distance and hover height and its
// pitch, roll and hover-ground PIDs (script fields, by name), the ground (FlightGroundNative, the script's rays in C++),
// the mode's gravityFactor, and the standard factors at the code's defaults (Flight::StandardParams) with the player's
// Handling and Brake Strength. Second in FlightModes.classes (src/tweaks/flight_modes.yaml), after Standard, switched
// on and off by its Mod Setting (Flight::Player().hoverFlyClassicEnabled).
//
// The class name is new: the old script class's name, FlightModeHoverFly, keys Standard's switch in Mod Settings'
// user.ini (Flight::kStandardEnabledClass), so the value players saved carries over to Standard.

#include "Flight/Mode.hpp"
#include "Flight/Modes.hpp"

struct FlightModeHoverFlyClassic : FlightMode {
  Flight::HoverFlyClassicState state;
  // the state's hovering, for the recorder and the bridge (copied after each step)
  float hovering = 1.0f;

  void OnActivateNative() override;
  void OnUpdateNative(float aTimeDelta) override;
  bool IsEnabledNative() override;
  RED4ext::CString DescriptionNative() override { return "Hover & Fly"; }

  RTTI_IMPL_TYPEINFO(FlightModeHoverFlyClassic);
  RTTI_IMPL_ALLOCATOR();
};

RTTI_DEFINE_CLASS(FlightModeHoverFlyClassic, {
  RTTI_PARENT(FlightMode);
  RTTI_PROPERTY(hovering);
});
