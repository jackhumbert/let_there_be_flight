#pragma once

// Standard, the mode flight starts in (the Hover & Fly redesign, docs/design/settings-audit.md), as a C++ flight
// mode over the maths Flight::Standard (Modes.hpp, designed in tools/flightsim: docs/design/hover-fly-feel.md). Each
// frame it gathers once what the maths reads: the component's stats, controls, thruster tensor and hover height
// (script fields, by name), the ground (FlightGroundNative, the script's rays in C++), the car's torque response
// (Flight::TorqueResponse) and the player's two settings (Flight::Player: turn and climb speed). The rest of
// StandardGains are the code defaults (developer settings when they need changing). It is first in
// FlightModes.classes (src/tweaks/flight_modes.yaml), switched on and off by its Mod Setting
// (Flight::Player().standardEnabled).
//
// Hover and Fly are Standard with the hover band always on and always off (Flight::HoverBand), in the mode list
// behind Extra Flight Modes (FlightSettings.extraModesEnabled). A subclass picks its band with Band().

#include "Flight/Mode.hpp"
#include "Flight/Modes.hpp"
#include "Flight/Settings.hpp"

struct FlightModeStandard : FlightMode {
  Flight::StandardState state;
  // the state's fields, for the recorder and the bridge (copied after each step)
  float hovering = 1.0f;
  float referenceZ = 0.0f;
  bool holdingAltitude = false;

  virtual Flight::HoverBand Band() { return Flight::HoverBand::Blend; }

  void OnActivateNative() override;
  void OnUpdateNative(float aTimeDelta) override;
  bool IsEnabledNative() override { return Flight::Player().standardEnabled; }
  RED4ext::CString DescriptionNative() override { return "Standard"; }

  RTTI_IMPL_TYPEINFO(FlightModeStandard);
  RTTI_IMPL_ALLOCATOR();
};

RTTI_DEFINE_CLASS(FlightModeStandard, {
  RTTI_PARENT(FlightMode);
  RTTI_PROPERTY(hovering);
  RTTI_PROPERTY(referenceZ);
  RTTI_PROPERTY(holdingAltitude);
});

// Whether the Extra Flight Modes switch is on (FlightSettings.extraModesEnabled, a Mod Setting in redscript).
bool ExtraFlightModesEnabled();
// The player's Handling (FlightSettings.handling, Flight::Handling's numbers).
int32_t FlightSettingsHandling();

// Hover: wherever the rays find ground, at any height; lift moves the hover height within the band.
struct FlightModeHover : FlightModeStandard {
  Flight::HoverBand Band() override { return Flight::HoverBand::Always; }
  bool IsEnabledNative() override { return ExtraFlightModesEnabled(); }
  RED4ext::CString DescriptionNative() override { return "Hover"; }

  RTTI_IMPL_TYPEINFO(FlightModeHover);
  RTTI_IMPL_ALLOCATOR();
};

RTTI_DEFINE_CLASS(FlightModeHover, { RTTI_PARENT(FlightModeStandard); });

// Fly: the ground ignored, the height held wherever lift leaves it.
struct FlightModeFly : FlightModeStandard {
  Flight::HoverBand Band() override { return Flight::HoverBand::Never; }
  bool IsEnabledNative() override { return ExtraFlightModesEnabled(); }
  RED4ext::CString DescriptionNative() override { return "Fly"; }

  RTTI_IMPL_TYPEINFO(FlightModeFly);
  RTTI_IMPL_ALLOCATOR();
};

RTTI_DEFINE_CLASS(FlightModeFly, { RTTI_PARENT(FlightModeStandard); });
