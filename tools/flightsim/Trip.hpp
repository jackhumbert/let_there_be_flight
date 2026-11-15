#pragma once

// A whole Autopilot trip flown offline (docs/design/autopilot.md, Trip): Navigation::Autopilot, the mode's
// own C++, flying the flightsim rigid body (Sim.hpp) through the real force tail, from Activate to "landed" (or
// another end), over a MapWorld on a nav map (navmap.bin, or one made in memory for tests). Shared by ltbf_tests
// (autopilot_trip_*) and navplan (NAVPLAN_TRIP, which bench.py and fleet.py run).
//
// What is the game's code and what is a model:
// - the Autopilot (Step, Activate, Teleported), the force tail and the stats are the plugin's;
// - the plans are the plugin's planner on a PlanQueue that is not threaded: each plan is made when asked for and
//   handed over planDelay seconds later (its rest, for a trip planned ahead, planDelay after that);
// - the body is Sim's (rigid, the torque answered through the car's inertia as PhysX does, Response()); the
//   Autopilot is given the world inertia that body has, as the game gives it PhysX's (Flight/Component.hpp);
// - the floor is the nav map's: the surface under a column's gap (gap bottom less the car's half height) is a stop
//   under the body, as Sim's terrain is;
// - what the nav map leaves out is boxes: parked cars (the Vehicle ray, SpotTaken) and statics (the ground ray,
//   SpotSupported), each solid to the body if asked, and each present from the start, from a time on, or once the
//   car is within a distance (streaming). The ground ray also hits the nav map's floor (groundFromMap), as the
//   engine's does once the world has streamed in;
// - a box the body runs into pushes it out and takes its speed into the box; faster than 2 m/s it is a hit
//   (ProcessImpact's recovery timer, which the Autopilot reads as collisionTimer). Contact friction and the spin a
//   hit gives are not modelled; Knock gives a spin.

#include "Sim.hpp"

#include "Navigation/Autopilot.hpp"
#include "Navigation/AutopilotParams.hpp"
#include "Navigation/Plans.hpp"
#include "Navigation/World.hpp"

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace FlightSim {

// Something the nav map does not hold, as a box.
struct TripBox {
  enum class Kind {
    Vehicle, // a parked car: the Vehicle collision group's rays see it (SpotTaken)
    Static,  // a prop or a ledge: the ground rays see it (SpotSupported)
  };
  Navigation::Box box;
  Kind kind = Kind::Vehicle;
  bool solid = true; // the body runs into it
  // streamed in: from appearAt seconds after the engage, or once the car is within appearWithin metres of the box
  // (horizontal); both negative: there from the start
  float appearAt = -1.0f;
  float appearWithin = -1.0f;
};

// A hit as Sim::Knock gives it: at a time, or once the route left to fly falls under a distance while following.
struct TripKnock {
  float at = -1.0f;           // s after the engage
  float atRemaining = -1.0f;  // m of route left (tracker.remaining)
  Vec3 bodySpin;              // rad/s, body frame
  Vec3 velocity;              // m/s added, world
  float impact = 1.0f;        // 0..1, the recovery timer's
};

// The game's teleport mid-trip: the car put at rest at `to`, then Autopilot::Teleported.
struct TripTeleport {
  float at = 0.0f; // s after the engage
  Navigation::Vec3 to{};
  float headingDeg = 0.0f;
};

struct Trip;

struct TripSetup {
  Car car;
  Navigation::Vec3 start{}; // the car's centre at rest
  float headingDeg = 0.0f;  // 0 = +Y, clockwise
  // where to: the tracked destination, or (external) a summon's externalDestination written after Activate, as
  // FlightSummonSystem does (with its keep-clear and pad offset)
  Navigation::Vec3 destination{};
  bool tracked = true;
  bool external = false;
  Navigation::Vec3 keepClear{};
  float keepClearRadius = 0.0f;
  Navigation::Vec3 padLandingOffset{};
  bool mounted = true; // isPlayerMounted
  // the component's input fields at the top of every frame: the player's, or an unmounted car's drive sim's
  Flight::Controls controls;
  Navigation::AutopilotParams params;
  Navigation::PlanParams planBase; // FlightNavMap's planParams; doorways off where the map has none
  float planDelay = 0.5f;          // s from a request to its plan
  float dt = 1.0f / 60.0f;
  float maxTime = 300.0f; // s
  float settle = 1.0f;    // s flown on after the trip ends (landed), for the car to come to rest
  uint64_t seed = 1;      // the Autopilot's Rng (free roam)
  bool groundFromMap = true;
  std::vector<TripBox> boxes;
  std::vector<TripKnock> knocks;
  std::vector<TripTeleport> teleports;
  // called after each frame (a test's own events); return false to end the trip
  std::function<bool(Trip &)> onFrame;
};

// One frame, after the Step and the body's step.
struct TripFrame {
  float t = 0.0f;
  Navigation::Vec3 p{}, v{};
  float speed = 0.0f;
  int8_t status = 0;      // TripStatusName
  bool following = false; // following a route with a trajectory
  float s = 0.0f, off = 0.0f, remaining = 0.0f;
  float leadSpeed = -1.0f; // the trajectory's speed where the follower commands (s + speed x lineLead), -1 off it
  float headingDeg = 0.0f, pitchDeg = 0.0f, rollDeg = 0.0f, tiltDeg = 0.0f;
  float attitudeErrorDeg = 0.0f;
  int32_t rays = 0;     // VehicleRay and GroundRay calls in the Step
  float stepMs = 0.0f;  // the Step's wall time (a plan asked for is planned in it: planFrame)
  bool planFrame = false;
  float inside = 0.0f;  // m into a nav map solid (0: free)
  bool contact = false; // on the floor or a box (the body stopped by it this frame)
  bool low = false;     // contact, or the centre under the nav map's floor (a half height over the surface)
  float impactSpeed = 0.0f; // m/s into the floor or a box this frame
  bool hit = false;         // a box run into at more than 2 m/s
  bool nan = false;
};

struct TripStatusChange {
  float t = 0.0f;
  std::string from, to;
};

struct TripResult {
  Navigation::Vec3 start{};
  std::vector<TripFrame> frames;
  std::vector<TripStatusChange> changes;
  std::string status;  // at the end
  bool landed = false; // "landed" reached
  float time = 0.0f;   // s from the engage to the end status (or the last frame)
  float offP50 = 0.0f, offP95 = 0.0f, offP99 = 0.0f, offMax = 0.0f; // m off the route, following
  float speedErrMean = 0.0f, speedErrP95 = 0.0f;                     // |speed - leadSpeed|, following
  Navigation::Vec3 landingSpot{};
  Navigation::Vec3 landingHeading{};
  float landingDistance = -1.0f;   // m (horizontal) from the spot at the end, -1 not landed
  float landingHeadingDeg = -1.0f; // deg between the nose and the landing heading at the end
  float touchdownSpeed = -1.0f;    // m/s down at the first floor contact of the descent, -1 none
  float impactMax = 0.0f;          // m/s, the fastest contact of the trip
  int32_t requests = 0, replans = 0; // plans asked for; those after the first
  int32_t hits = 0;                  // boxes run into (TripFrame::hit)
  int32_t knocks = 0;
  int64_t rays = 0;
  int32_t raysMaxFrame = 0;
  float stepMsMean = 0.0f, stepMsP99 = 0.0f, stepMsMax = 0.0f; // frames that planned nothing
  size_t pendingAtEnd = 0;
  bool nan = false;
  float insideMax = 0.0f;        // m, in flight
  float insideContactMax = 0.0f; // m, low over the floor or on a box (TripFrame::low)
  // m the nav map's floor (a gap's bottom) is above a resting car's centre: the car's half height in the bake less
  // the car's own (its bottom under the centre); a car on the floor is this deep into the map's solid
  float floorDepth = 0.0f;
  bool flightOff = false, exitAutopilot = false;
  std::vector<std::string> notifications;
};

// The status names the frames index (TripFrame::status); -1 for one not in the list.
const char *TripStatusName(int8_t aStatus);
int8_t TripStatusCode(const std::string &aStatus);

// The world the Autopilot asks: the nav map with the boxes streamed in so far, the rays counted.
struct TripWorld : Navigation::MapWorld {
  TripWorld(const Navigation::NavMap &aMap, const std::vector<Navigation::Helipad> &aPads,
            const std::vector<Navigation::Gate> &aGates)
      : MapWorld(aMap, aPads, aGates) {}
  bool VehicleRay(const Navigation::Vec3 &aFrom, const Navigation::Vec3 &aTo) const override;
  bool GroundRay(const Navigation::Vec3 &aFrom, const Navigation::Vec3 &aTo, Navigation::Vec3 &aHit) const override;
  // the highest surface at (x, y) at or below aZ (a gap's bottom less the car's half height), -infinity for none
  float Surface(float aX, float aY, float aZ) const;
  bool groundFromMap = true;
  mutable int64_t rays = 0;
};

// One trip: build it, then Run (or Frame by frame).
struct Trip {
  // aPads and aGates must outlive the trip
  Trip(const Navigation::NavMap &aMap, const std::vector<Navigation::Helipad> &aPads,
       const std::vector<Navigation::Gate> &aGates, TripSetup aSetup);

  // the engage (Activate, then a summon's fields), once
  void Engage();
  // one frame: the boxes streamed in, knocks and teleports due, the Step, the force tail and the body, the contacts
  void Frame();
  // engage and fly until the trip ends (landed, no route, no nav map, handed back, flight off) plus settle, or
  // maxTime; the result
  TripResult Run();
  bool Ended() const { return ended; }
  // the result of the frames so far
  TripResult Result() const;

  TripSetup setup;
  TripWorld world;
  Navigation::PlanQueue queue;
  Navigation::Rng rng;
  Navigation::Autopilot ap;
  float refZ = 0.0f; // the car's centre before the body's step: the floor is the surface at or below it
  Sim sim;
  Navigation::AutopilotInput in;
  Navigation::AutopilotOutput out;
  float time = 0.0f;
  std::vector<TripFrame> frames;
  std::vector<TripStatusChange> changes;
  std::vector<bool> boxIn; // each setup box streamed in
  std::vector<bool> knockDone, teleportDone;
  int32_t requests = 0, knocks = 0;
  bool flightOff = false, exitAutopilot = false;
  std::vector<std::string> notifications;

private:
  void Input();
  void Change(const std::string &aFrom);
  void Stream();
  void Contacts(TripFrame &aFrame);
  bool engaged = false, ended = false;
  float endAt = -1.0f;
  float lastRemaining = 1e9f;
  Flight::Pid pitchPid, rollPid;
};

// What every trip must keep to (docs/design/autopilot.md, Trip checks).
struct TripLimits {
  bool mustLand = true;
  float inside = 0.5f;         // m into a nav map solid allowed in flight
  float insideContact = 0.5f;  // m allowed low over the floor or on a box, over TripResult::floorDepth
  float touchdownMargin = 1.0f; // m/s over touchdownRate
  float landingDistance = 2.0f; // m from the spot
  float landingHeadingDeg = 20.0f;
  int32_t raysPerFrame = 200;   // the old landing search cast up to 18 x 469 x 2 in one frame
  // m (horizontal) round the start where being inside is not checked: a street start beside a wall or under a
  // structure sits in the bake's growth (3 m round every solid) until it lifts off
  float startClear = 0.0f;
};

// The rules broken, one line each; empty when the trip kept to them all. Checks: no NaN, never deeper into a nav
// map solid than the limits, only the status changes the state machine makes, no plan request left at the end,
// and (mustLand) landed, touchdown no faster than touchdownRate plus the margin, within the landing distance and
// heading; the rays per frame.
std::vector<std::string> CheckTrip(const TripResult &aResult, const Navigation::AutopilotParams &aParams,
                                   const TripLimits &aLimits = {});
// The status changes the Autopilot's state machine makes (from, to).
bool LegalStatusChange(const std::string &aFrom, const std::string &aTo);
// Two runs of one setup gave the same flight: every frame's position, velocity and status bit for bit (the wall
// times aside).
bool SameTrip(const TripResult &aA, const TripResult &aB);
// Nearest rank percentile of a sorted array (0 for none).
float TripPercentile(const std::vector<float> &aSorted, double aQ);
// One line of JSON with the result's numbers (navplan's "trip" line).
std::string TripJson(const TripResult &aResult);
// The frames as CSV (t,x,y,z,vx,vy,vz,speed,status,s,off,lead_speed,remaining,heading,pitch,roll,tilt,att_err,rays,
// step_ms,inside,contact,low,impact,hit); false when the file cannot be written.
bool WriteTripCsv(const std::string &aPath, const TripResult &aResult);

} // namespace FlightSim
