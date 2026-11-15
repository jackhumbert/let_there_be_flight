#pragma once

// The beginner's situations Hover & Fly is measured on, and Drone's (its spin per axis, lift, surge and angular brake,
// and its skid over the ground: drone_skid_*), each a scripted input over a small world, and the numbers each one is
// judged by (docs/design/flightsim.md; the feel spec: docs/design/flight-modes.md). The feel tests (tests/feel_tests.cpp) and the flightsim tool
// (flightsim.cpp) both run these.

#include "Sim.hpp"

#include <string>
#include <vector>

namespace FlightSim {

struct Metric {
  std::string name;
  float value;
  std::string unit;
};

struct Run {
  std::string name;
  std::vector<Frame> frames;
  std::vector<Metric> metrics;
  float Get(const std::string &aName) const;
};

// The tuning a run is flown with: the mod's defaults unless changed.
struct Setup {
  Car car;
  Flight::StandardParams standard;
  Flight::ForceParams force;
  Mode mode = Mode::Standard;
  bool shapeDrag = false;
  float thrustStrength = 0.0f, thrustPerThruster = 6000.0f;
  Flight::StandardGains standardGains;
  Flight::DroneParams drone;
  Flight::DroneAssistParams droneAssist;
  // Drone's skid and the ground contact at points (Sim's fields of these names; 0 or 1 for the switches)
  float handling = 1.0f; // Sim::handling
  float skid = 0.0f, skidPerThruster = 0.0f, pointContact = 0.0f, sphereContacts = 1.0f;
  float skidTurn = Flight::DroneSkidParams{}.turn, sphereRadius = 0.4f, openBottom = 0.0f;
  float dt = 1.0f / 60.0f;
};

// A Sim given the setup's tuning (everything but the car, which the Sim is made with).
void Configure(Sim &aSim, const Setup &aSetup);

// Every scenario's name, or the ones for one kind of mode: Drone's (drone_*) for the drone modes, the rest for Hover &
// Fly's. Any scenario can be flown in any mode.
std::vector<std::string> ScenarioNames();
std::vector<std::string> ScenarioNames(Mode aMode);
Run RunScenario(const std::string &aName, const Setup &aSetup = {});

} // namespace FlightSim
