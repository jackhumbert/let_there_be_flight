#pragma once

// The car's limits, worked out in one place from its stats and the flight tuning: the planner
// (Planner.cpp), the Autopilot in game (FlightVehicleModel, FlightModeAutopilot.reds) and the
// offline tools (navplan, tools/navplan/vehicles.json) all read them from here
// (docs/design/vehicle-limits.md).
//
// No game types: navplan builds this without the game.

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace Navigation {

struct VehicleSpec {
  // the car (FlightStats from its vehicle record)
  float mass = 1500.0f;          // kg
  float airResistance = 2.0f;    // s_airResistanceFactor
  float brakingFriction = 1.4f;  // s_brakingFrictionFactor
  // the flight tuning (FlightMode_Standard, FlightMode, FlightSettings)
  float surge = 15.0f;           // m/s2, standardModeSurgeFactor
  // share of the surge the car gets speeding up in level flight (telemetry fit, docs/history.md
  // 2026-10-30: 0.88 for the Shion, drag as modelled); top speed is where that meets the drag
  float surgeShare = 0.88f;
  float sway = 5.0f;             // m/s2, standardModeSwayFactor
  float lift = 25.0f;            // m/s2 over gravity, standardModeLiftFactor
  float gravityFactor = 1.0f;
  float brakeLinear = 1.0f;      // FlightSettings.brakeFactorLinear
  bool drag = true;              // FlightSettings.physxFlightPhysics: air drag applied
  // the Autopilot's race tuning (FlightModeAutopilot)
  bool drone = true;             // drone style: the body tilts to steer (droneStyle)
  float maxTilt = 55.0f;         // deg, droneMaxTilt
  float brakeShare = 0.25f;      // share of the thrust's tilt the body takes slowing (droneBrakeShare): the nose
                                 // pulls up less than it leans speeding up, reverse surge does the rest
  float driftMax = 20.0f;        // deg the nose turns into a bend (raceDriftMax)
  float speedCap = 120.0f;       // m/s, raceSpeedCap (freeRoamTopSpeed in free roam)
  float margin = 0.85f;          // share of the envelope planned for (raceMargin)
  float planDecel = 18.0f;       // m/s2 braking planned at most (racePlanDecel)
  float pushOver = 21.0f;        // m/s2 bending the path downwards, as planned (margin already in: dronePushOver;
                                 // racePushOver 15 plane style)

  // "key=value,..." with the field names above (bools 0/1); unknown keys are errors. For navplan
  // (NAVPLAN_VEHICLE) and tools/navplan/vehicles.json.
  bool Parse(const std::string &aText, std::string &aError);
  std::string Describe() const;
};

// Air drag at speed v, m/s2 (FlightMode.ApplyPhysics).
float Drag(const VehicleSpec &aSpec, float aSpeed);
// Drone style: the most horizontal acceleration level flight leaves along one body axis with
// `thrusters` m/s2 across it, the body tilted at most maxTilt and the lift holding the car up.
float DroneAccel(const VehicleSpec &aSpec, float aThrusters);
// Where the surge the car gets (surgeShare) meets the drag (97% of it), at most speedCap.
float TopSpeed(const VehicleSpec &aSpec);
// Sideways acceleration in a level turn: drone style the sway envelope, plane style the lift
// banked into the turn; both plus the surge's share with the nose drifted in.
float Lateral(const VehicleSpec &aSpec);
// Speeding up at v: the surge the car gets (surgeShare) less drag (at least 0.5).
float Accel(const VehicleSpec &aSpec, float aSpeed);
// Slowing at v: full reverse surge, the brake (grows with v) and drag.
float Decel(const VehicleSpec &aSpec, float aSpeed);
// What a speed plan brakes at: margin of Decel, at most planDecel.
float PlanDecel(const VehicleSpec &aSpec, float aSpeed);
// Pulling the path upwards: the lift over gravity.
float PullUp(const VehicleSpec &aSpec);

} // namespace Navigation
