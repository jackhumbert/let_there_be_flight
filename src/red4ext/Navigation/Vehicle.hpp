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
  float sway = 5.0f;             // m/s2, standardModeSwayFactor
  float lift = 25.0f;            // m/s2 over gravity, standardModeLiftFactor
  float gravityFactor = 1.0f;
  float brakeLinear = 1.0f;      // FlightSettings.brakeFactorLinear
  bool drag = true;              // FlightSettings.physxFlightPhysics: air drag applied
  // the Autopilot's race tuning (FlightModeAutopilot)
  bool drone = true;             // drone style: the body tilts to steer (droneStyle)
  float maxTilt = 55.0f;         // deg, droneMaxTilt
  float driftMax = 20.0f;        // deg the nose turns into a bend, plane style (raceDriftMax)
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
// Where full surge meets the drag (97% of it), at most speedCap.
float TopSpeed(const VehicleSpec &aSpec);
// Sideways acceleration in a level turn: drone style the sway envelope, plane style the lift
// banked into the turn plus the surge's share with the nose drifted in.
float Lateral(const VehicleSpec &aSpec);
// Speeding up at v: surge less drag (at least 0.5).
float Accel(const VehicleSpec &aSpec, float aSpeed);
// Slowing at v: full reverse surge, the brake (grows with v) and drag.
float Decel(const VehicleSpec &aSpec, float aSpeed);
// What a speed plan brakes at: margin of Decel, at most planDecel.
float PlanDecel(const VehicleSpec &aSpec, float aSpeed);
// Pulling the path upwards: the lift over gravity.
float PullUp(const VehicleSpec &aSpec);

// The fastest speed the car can fly along a polyline, sampled every aStep metres: each point's
// limit from its bend (turns at margin x Lateral, crests at pushOver, pull-ups at
// margin x PullUp, over aSpan either side), then speeding up from aStartSpeed (Accel) and braking
// to 0 at the end (PlanDecel). The plan's own profile; the Autopilot's in game
// (FlightModeAutopilot.BuildSpeedProfile) adds the landing approach and corner braking.
struct SpeedProfile {
  enum class Limit : unsigned char { None, Turn, Crest, PullUp };
  std::vector<float> distance; // m along the line
  std::vector<float> speed;    // m/s flown
  std::vector<float> limit;    // m/s the bend alone allows
  std::vector<Limit> why;
  float top = 0.0f;
};
SpeedProfile Profile(const VehicleSpec &aSpec, const std::vector<float> &aX, const std::vector<float> &aY,
                     const std::vector<float> &aZ, float aStartSpeed = 0.0f, float aStep = 2.0f, float aSpan = 10.0f);

} // namespace Navigation
