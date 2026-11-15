#pragma once

// The car's limits, worked out in one place from its stats and the flight tuning: the planner
// (Planner.cpp), the Autopilot in game (FlightModeAutopilot, FlightVehicleModel for script) and the
// offline tools (navplan, tools/navplan/vehicles.json) all read them from here
// (docs/design/vehicle-limits.md).
//
// No game types: navplan builds this without the game.

#include "Flight/Force.hpp"

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
  // share of the surge the car gets speeding up in level flight near top speed (telemetry fit,
  // tools/telemetry/fit_surge.py: 1.0 for the Shion, drag as modelled); top speed is where that meets
  // the drag
  float surgeShare = 1.0f;
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
  float tiltFloor = 1.0f;        // x g: the vertical the body's tilt is worked out against at least (droneTiltFloor);
                                 // bending down, the lift pushes less or down and the thrusters take the rest
  float driftMax = 20.0f;        // deg the nose turns into a bend (raceDriftMax)
  float speedCap = 120.0f;       // m/s, raceSpeedCap (freeRoamTopSpeed in free roam)
  float margin = 0.85f;          // share of the envelope planned for (raceMargin)
  float planDecel = 18.0f;       // m/s2 braking planned at most (racePlanDecel)
  float pushOver = 21.0f;        // m/s2 bending the path downwards, as planned (margin already in: dronePushOver;
                                 // racePushOver 15 plane style)
  // the car's shape and thrust (docs/design/autopilot.md, "Character"; passed only with the developer setting
  // shapeDrag on, off by default, and the defaults keep the airResistance model): the chassis shape's frontal drag area (m2, tools/flightsim/vehicles.tsv cdaY;
  // 0: airResistance's drag) with Flight::kShapeDragK, and the thrusters that scale surge and sway in Effective
  float cdaFront = 0.0f;
  // the roof's drag area (m2, vehicles.tsv cdaZ; 0: none): drone style the body stays near level, so a climb or a
  // dive meets the air with it (DragAt)
  float cdaTop = 0.0f;
  float dragK = Flight::kShapeDragK;
  float thrusters = 0.0f;        // 0: unknown, surge and sway stay as set
  float thrustStrength = 0.0f;   // Flight::ThrustScale's strength: 0 today's thrust, 1 by the thrusters alone
  float thrustPerThruster = 6000.0f; // N

  // "key=value,..." with the field names above (bools 0/1); unknown keys are errors. For navplan
  // (NAVPLAN_VEHICLE) and tools/navplan/vehicles.json.
  bool Parse(const std::string &aText, std::string &aError);
  std::string Describe() const;
};

// Air drag at speed v, m/s2 (FlightMode.ApplyPhysics).
float Drag(const VehicleSpec &aSpec, float aSpeed);
// The same along a path climbing or diving at aSlope (its direction's vertical part, -1 to 1): drone style with a
// roof area the air meets the near-level body's front and roof in the path's proportions (cdaFront cos2 + cdaTop
// sin2), so a 45 deg climb costs the Shion 8 times the drag of level flight; otherwise Drag.
float DragAt(const VehicleSpec &aSpec, float aSpeed, float aSlope);
// Accel and TopSpeed on such a path (TopAt: where the surge meets DragAt, 97% of it, at most TopSpeed).
float AccelAt(const VehicleSpec &aSpec, float aSpeed, float aSlope);
float TopAt(const VehicleSpec &aSpec, float aSlope);
// The spec with surge and sway scaled by the car's thrust (Flight::ThrustScale over thrusters, mass and
// thrustStrength), and thrustStrength 0 so it is not scaled twice. Build a car's spec, then this, once.
VehicleSpec Effective(const VehicleSpec &aSpec);
// Drone style: the most horizontal acceleration level flight leaves along one body axis with
// `thrusters` m/s2 across it, the body tilted at most maxTilt and the lift holding the car up.
float DroneAccel(const VehicleSpec &aSpec, float aThrusters);
// Where the surge the car gets (surgeShare) meets the drag (97% of it), at most speedCap.
float TopSpeed(const VehicleSpec &aSpec);
// Sideways acceleration in a level turn: drone style the sway envelope, plane style the lift
// banked into the turn; both plus the surge's share with the nose drifted in.
float Lateral(const VehicleSpec &aSpec);
// The same where the path also needs `vertical` m/s2 upwards from the thrust (gravity plus the bend's
// own: less over a crest). Drone style, the body's tilt turns the car only through the lift, so its
// share is cut to what the lift has left (none pushing over a crest, where only sway and the drifted
// surge turn the car); plane style as Lateral.
float LateralAt(const VehicleSpec &aSpec, float aVertical);
// The same at a speed: the drifted nose's share counts only the surge the drag leaves at aSpeed (near top speed
// the surge all goes into holding it; counting it anyway put the heavy, low-thrust cars off the line in turns).
// The speed-free forms above are their upper bound.
float Lateral(const VehicleSpec &aSpec, float aSpeed);
float LateralAt(const VehicleSpec &aSpec, float aVertical, float aSpeed);
// The speed full surge holds (the surge hold flies at it, past TopSpeed's 97%), at most speedCap.
float FlownTop(const VehicleSpec &aSpec);
// Speeding up at v: the surge the car gets (surgeShare) less drag (at least 0.5).
float Accel(const VehicleSpec &aSpec, float aSpeed);
// Slowing at v: full reverse surge, the brake (grows with v) and drag.
float Decel(const VehicleSpec &aSpec, float aSpeed);
// What a speed plan brakes at: margin of Decel, at most planDecel.
float PlanDecel(const VehicleSpec &aSpec, float aSpeed);
// Pulling the path upwards: the lift over gravity.
float PullUp(const VehicleSpec &aSpec);
// Drone style: the body's tilt on one axis for a thrust direction aAngle (rad) from vertical: aShare of it, or
// more where the thrusters (aThrusters m/s2 across the body at most, margin of it planned) cannot point the rest
// of aThrust m/s2.
float DroneBodyAngle(const VehicleSpec &aSpec, float aAngle, float aThrust, float aThrusters, float aShare);

} // namespace Navigation
