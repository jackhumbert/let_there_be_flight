#pragma once

// Hover & Fly and Drone flown offline: the plugin's own mode, input and force code (Flight/Controls, Stats, Modes,
// Force) on a rigid body over a height field, so a mode's feel can be measured and kept from regressing without the
// game.
//
// What is the game's code and what is a model:
// - the input smoothing (StepControls), the stats (UpdateDynamic), the mode (Standard, Drone, ...) and the force tail
//   (ForceTail: ApplyPhysics, the mass and tensor scaling) are the plugin's functions, called as the game calls them;
// - FindGround is modelled: four rays straight down from the wheel points, as FlightComponent.FindGround casts them,
//   averaged with GroundFromHits; the world is a height field;
// - the body is modelled: the linear part integrates the tail's world force as an acceleration (FlightComponent
//   scales it by mass and PhysX divides it out) plus gravity and PhysX's linear damping; the angular part applies the
//   mode torque times a per-axis gain measured in game (TorqueProbe, Archer Quartz hovering, 2026-10-28: about
//   1.25 rad/s2 per unit of mode torque on each axis, answering two frames late at 32 fps);
// - the ground contact is a stop: the body's bottom never goes below the ground, downward speed is removed.
// With groundRay set (the city's collision, Colliders.hpp; docs/design/hover-fly-terrain.md) FindGround's rays and
// the contact go through it instead of the height field: the hit's own normal, overpasses and ledges, and the
// underside's corners as well as its centre.

#include "Colliders.hpp"
#include "Flight/Controls.hpp"
#include "Flight/Force.hpp"
#include "Flight/Modes.hpp"

#include <deque>
#include <functional>
#include <string>
#include <vector>

namespace FlightSim {

using Flight::Vec3;

// The car. The defaults are a mid-size car (Navigation/Vehicle.hpp's VehicleSpec) with a Shion MZ1's torque response,
// fitted to a Hover & Fly recording (hoverfly_check, flightsim --replay, 2026-11-04): pitch 3.0 and yaw 2.57 rad/s2
// per unit of mode torque, 30 ms late. Roll is not fitted yet: 1.2 is the Archer's (TorqueProbe CSV of 2026-10-28).
// A surveyed car (vehicles.tsv) sets inertia and tensor, and its gain is Flight::TorqueResponse's.
struct Car {
  std::string name = "generic";
  std::string record; // vehicles.tsv's first record of the car (empty when not from the table)
  float mass = 1500.0f;
  float airResistance = 2.0f;   // s_airResistanceFactor
  float brakingFriction = 1.4f; // s_brakingFrictionFactor
  // rad/s2 of body angular acceleration per unit of mode torque (body X right, Y forward, Z up)
  Vec3 angularGain{3.0f, 1.2f, 2.57f};
  // s_momentOfInertia and FlightComponent.thrusterTensor; zero inertia means unknown, and CapDamping is then given
  // the inertia that matches angularGain
  Vec3 inertia, tensor{1, 1, 1};
  float torqueDelay = 0.03f; // s the body answers the torque late
  // how far the body's real response is off angularGain, per axis: the controllers are given angularGain, the body
  // answers angularGain x this (for robustness runs: flightsim --set errX=2)
  Vec3 responseError{1, 1, 1};
  // rad/s the body never spins faster than: PhysX's default max angular velocity (a held turn in game levels off at
  // 7.1 rad/s, hoverfly_check on a Shion MZ1, 2026-11-04)
  float maxAngularSpeed = 7.0f;
  // the wheel emitters FindGround casts from, relative to the centre of mass (body frame), and the body's bottom
  float wheelX = 0.8f, wheelY = 1.4f, wheelZ = -0.3f;
  float bottom = -0.6f;
  // the chassis's underside (aero.py, vehicles.tsv's underside): its lowest point in each square of the footprint,
  // body frame from the centre of mass, for the contact with groundRay; empty: the wheel box at the bottom stands in
  std::vector<Vec3> underside;
  // the chassis collision shape's bounds, body frame from the centre of mass (aero.py, vehicles.tsv's boxMin/Max);
  // zero when unknown
  Vec3 boxMin, boxMax;
  // the chassis collision shape's drag area per body axis (m2: X side, Y front, Z top; aero.py, vehicles.tsv), and
  // the fleet-wide factor on it (docs/research/2026-11-04-aero-drag.md); zero: unknown, today's drag stays
  Vec3 cda;
  float dragK = Flight::kShapeDragK;
  float thrusters = 0; // vehicles.tsv; 0: unknown, the thrust stays today's
  // the body's gravity against the 9.81 the modes hold up: an imbalance they have to correct (flightsim --set
  // gravity=0.95; in game the car rose while braking in Hover & Fly, 2026-11-05)
  float gravityScale = 1.0f;
  // 1: the vertical speed the mode reads is a frame of gravity low, as in game with the PhysX-owned flight physics
  // until the fix of 2026-11-06 (g x the frame, 1/60 s at least: the recorded vz against the height's change, 24
  // flights); 0: the body's own, as the game reads it since (flightsim --set velocityLag=1, for older recordings)
  float velocityLag = 0.0f;
};

// Ground height at (x, y); return -infinity for none (a ray that hits nothing).
using Terrain = std::function<float(float x, float y)>;

// The raw inputs as the keys or sticks give them, -1..1, before the input smoothing.
struct Keys {
  float surge = 0, lift = 0, sway = 0, yaw = 0, roll = 0, pitch = 0, linearBrake = 0, angularBrake = 0;
};

// One frame, after the step.
struct Frame {
  float t = 0;
  Vec3 position, velocity;
  float headingDeg = 0, pitchDeg = 0, rollDeg = 0;
  float tiltDeg = 0; // the body's up from the world's: 0 level, 180 upside down
  Vec3 angularVelocity; // body, rad/s
  Flight::Controls controls;
  bool foundGround = false;
  float groundDistance = 0, groundHeight = 0;
  float hovering = 0, hoverHeight = 0, referenceZ = 0;
  bool holdingAltitude = false;
  bool contact = false;
  float impactSpeed = 0; // m/s of downward speed the ground stopped this frame
  // m from the underside's lowest point to the ground under it (kClearanceReach at most; negative: sunk in, before the
  // contact lifted it), and that point's place along the body (body Y, m: + front); with groundRay, rays cast this
  // frame (FindGround's, the underside's and the one under the centre)
  float clearance = 0, clearanceY = 0;
  int rays = 0;
};

// m below the underside the contact's rays look for the ground (Frame::clearance's cap)
constexpr float kClearanceReach = 3.0f;

// The body moved by something else (PhysX, PhysXWorld.hpp) in place of the Sim's own integration and contact: given the
// force (N) and torque (N m) on it this step (world, without gravity), its linear damping (1/s) and its spin cap, it
// moves the body and leaves where it is, how it moves and how many contact points it had.
struct BodyStepIn {
  float dt = 0;
  Vec3 force, torque;
  float linearDamping = 0, maxAngularSpeed = 0;
};
struct BodyStepOut {
  Vec3 position, velocity, angularVelocity; // world
  Flight::Quat orientation;
  int contactPoints = 0;
};
using BodyStep = std::function<void(const BodyStepIn &, BodyStepOut &)>;

// The mode flown.
enum class Mode {
  Standard,         // Flight::Standard (the Hover & Fly redesign), the hover band blended
  Hover,            // Standard, the hover band always on (FlightModeHover)
  Fly,              // Standard, the ground ignored (FlightModeFly)
  Drone,            // Flight::Drone
  AntiGravityDrone, // Drone plus FlightModeDroneAntiGravity's d_localUp x 9.81 x gravityFactor (its script's own line)
  DroneAssist,      // Flight::DroneAssist, Drone with a rate command per axis (a proposal)
};
// The command line's names (flightsim --mode): standard, hover, fly, drone, agdrone, droneassist; assist and hoverfly
// are taken for standard (its older names). ParseMode returns false for a name it does not know.
const char *ModeName(Mode aMode);
bool ParseMode(const std::string &aName, Mode &aMode);
inline bool IsDrone(Mode aMode) { return aMode == Mode::Drone || aMode == Mode::AntiGravityDrone || aMode == Mode::DroneAssist; }

class Sim {
public:
  Sim(Car aCar, Terrain aTerrain);

  // The tuning, at the mod's defaults; change before Place.
  Flight::StandardParams standard;
  Flight::ForceParams force;
  Mode mode = Mode::Standard;
  // shape drag (Flight::ShapeDragAccel from the car's cda) in place of today's airResistanceFactor drag
  bool shapeDrag = false;
  // surge and sway scaled by each car's thrust (Flight::ThrustScale): 0 today's, 1 by its thrusters alone
  float thrustStrength = 0.0f, thrustPerThruster = 6000.0f;
  Flight::StandardGains standardGains;
  Flight::DroneParams drone;
  Flight::DroneAssistParams droneAssist;
  Flight::PidGains hoverPid{1.0f, 0.005f, 0.5f}, pitchPid{1.0f, 0.2f, 0.05f}, rollPid{1.0f, 0.5f, 0.0f};
  // the ground as rays (RayOn a Colliders) in place of the terrain; empty: the terrain
  GroundRay groundRay;
  // Drone's skid as FlightModeControl.cpp's Skid adds it: the clearance it holds the car's lowest point at (m; 0 off),
  // and with skidPerThruster 1 at each wheel emitter from its own ray (Flight::DroneSkidPoints) instead of once through
  // the centre of mass from the rays' mean (Flight::DroneSkid)
  // the player's Handling (Flight::Handling: 0 Loose, 1 Default, 2 Tight), put over force and standardGains as the
  // plugin does
  int handling = 1;
  float skid = 0.0f, skidPerThruster = 0.0f;
  float skidTurn = Flight::DroneSkidParams{}.turn; // DroneSkidParams::turn
  // 1: the ground contact at points, each an impulse where it touches so a corner down turns the body, with friction,
  // in place of the stop under the lowest point: the thruster spheres' bottoms at the four wheel emitters (unless
  // sphereContacts is 0, as the developer setting thrusterContacts off) and the chassis's underside
  float pointContact = 0.0f, sphereContacts = 1.0f;
  float contactFriction = 0.5f; // the points' Coulomb friction
  // set: the body is moved by it (PhysX), not by the Sim; the rays still read the ground for the frame's clearance
  BodyStep bodyStep;
  // the thruster spheres' radius (FlightConfiguration::AddColliders, the developer setting thrusterRadius): their
  // centres stay put, so their bottom is the table's (taken at 0.4 m, the game's) raised by 0.4 less this
  float sphereRadius = 0.4f;
  // over 0: the thruster colliders are boxes over the pods' sides and top, open this far up from the pods' bottom
  // (the developer setting thrusterOpenBottom): the skid holds the pods' bottom as drawn (0.245 m under the spheres'
  // centres) over the ground, and the boxes' bottom corners are what touches
  float openBottom = 0.0f;

  // Puts the car at rest, level, at position with the nose to headingDeg (0 = +Y, clockwise) and engages the mode
  // (FlightModeStandard.OnActivateNative). velocity starts it moving.
  void Place(Vec3 aPosition, float aHeadingDeg, Vec3 aVelocity = {});
  Frame Step(const Keys &aKeys, float aDt);

  // A recorded state to fly on from (flightsim --replay): the body, and the mode's fields as the game had them.
  struct State {
    Vec3 position, velocity, forward, right, angularVelocity; // angular velocity in the body frame
    float hovering = 0, hoverHeight = 0, referenceZ = 0, groundDistance = 0;
    bool holdingAltitude = false;
  };
  void SetState(const State &aState);
  // A collision as the mode sees one: the body's spin (body frame) and velocity change at once, the spin counted as
  // the world's (FlightComponent.contactSpin, which ApplyPhysics leaves undamped, decaying over
  // kContactSpinDecay), and FlightComponent.ProcessImpact's recovery timer for an impact of 0..1 (the grid's
  // biggest desiredChange). The contact itself (shapes, friction, more hits) is not modelled.
  void Knock(Vec3 aBodySpin, Vec3 aVelocity, float aImpact);
  // One frame on controls already smoothed, as the game's FlightComponent holds them, over the ground given (the
  // recorded FindGround) or, without one, the terrain's.
  Frame Advance(const Flight::Controls &aControls, const Flight::Ground *aGround, float aDt);

  // A mode computed outside the Sim (the Autopilot, Trip.hpp) flying the body. Observe updates the stats from the
  // body as it is now and returns them, for the mode to read; AdvanceMode then takes the mode's force and torque
  // (car frame, as ModeOutput) and the input fields it left for the force tail, through the tail and the body as
  // Advance does. No ground is found: the mode casts its own rays.
  const Flight::StatsState &Observe();
  Frame AdvanceMode(const Flight::ModeOutput &aMode, const Flight::Controls &aControls, float aDt);
  // A contact with something solid (a box the terrain does not hold): the body moved by aPush (world), its velocity
  // into the surface (aNormal, world, unit, out of the solid) taken away, and with aImpact > 0 (0..1) a hit:
  // ProcessImpact's recovery timer, as Knock sets it.
  void Collide(Vec3 aPush, Vec3 aNormal, float aImpact);
  // The car's statics as the force tail reads them (mass, drag, inertia, thrusters), filled by Place.
  const Flight::StatsStatic &Statics() const { return statics; }
  const Car &Vehicle() const { return car; }
  Vec3 Position() const { return position; }
  Vec3 Velocity() const { return velocity; }
  Vec3 AngularVelocity() const { return angularVelocity; } // world, rad/s
  Flight::Quat Orientation() const { return orientation; }
  // FlightComponent.collisionTimer: it drops on a hit (Knock, Collide) and counts back up
  float CollisionTimer() const { return collisionTimer; }
  // rad/s2 per unit of mode torque on each body axis, as the body answers (angularGain x responseError)
  Vec3 Response() const;

  const Frame &Last() const { return last; }

private:
  Flight::Ground FindGround();
  // the skid's force and torque (body frame, as ModeOutput) from FindGround's last rays
  Flight::ModeOutput Skid(const Flight::Controls &aControls, const Flight::Ground &aGround) const;
  // pointContact's impulses after the body moved; fills the frame's contact, clearance and impact speed
  void ContactPoints(Frame &aFrame, float aDt);
  // the frame's time, pose, angles and the mode's state, after the body has moved; aGround the ground under the centre
  // (minus infinity: cast for it)
  Frame Finish(Frame &aFrame, const Flight::Controls &aControls, const Flight::Ground &aGround, float aDt,
               float aGroundUnder = -std::numeric_limits<float>::infinity());
  // with groundRay: the clearance under the underside's points (the thruster spheres as the tyres at the four wheel
  // emitters at the bottom, and the chassis's underside, or the centre at the bottom without one), each cast from the
  // centre's height down to kClearanceReach under it: the lowest, negative when the ground is above it, where it is and
  // the rays cast
  struct Clearance {
    float clearance = 0, bodyY = 0;
    int rays = 0;
  };
  Clearance Underside() const;
  // the force tail and the body for the mode's output, from the stats already updated
  Frame Tail(const Flight::ModeOutput &aMode, const Flight::Controls &aControls, const Flight::Ground &aGround, float aDt);
  void FillInertia(float aDt);

  Car car;
  Terrain terrain;
  Flight::ControlAxes axes;
  Flight::StatsState stats;
  Flight::StatsStatic statics;
  Flight::StandardState standardState;
  Flight::Pid hover, pitch, roll;
  float lastDistance = 0;
  int rays = 0; // FindGround's this frame
  Flight::Hit lastHits[4]; // FindGround's rays in its order: left back, left front, right back, right front
  float collisionTimer = 10.0f;
  Vec3 contactSpin; // world, rad/s
  float t = 0;

  Vec3 position, velocity, angularVelocity; // world; position is the centre of mass
  Flight::Quat orientation{0, 0, 0, 1};
  std::deque<Vec3> torqueQueue; // body torque waiting for the delay
  Frame last;
};

// A flat world at height h.
Terrain Flat(float aHeight = 0.0f);

// The cars of a survey table (tab-separated, a header line, one car a line; columns: record, name, type, mass,
// airResistance, brakingFriction, inertiaX, inertiaY, inertiaZ, tensorX, tensorY, tensorZ, wheelX, wheelY, wheelZ,
// bottom), each with its angular gain worked out (Flight::TorqueResponse). Empty when the file cannot be read; bad lines are skipped.
std::vector<Car> LoadCars(const std::string &aPath);

// vehicles.tsv's underside column ("cell,x0,y0,nx,ny:z,z,...", aero.underside()) as body points, each square's centre
// at its lowest z; empty when it cannot be read.
std::vector<Vec3> ParseUnderside(const std::string &aText);

} // namespace FlightSim
