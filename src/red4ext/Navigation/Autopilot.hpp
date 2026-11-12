#pragma once

// The Autopilot as pure C++ (docs/design/native-ports.md, M6 steps 4b to 4e): everything
// FlightModeAutopilot.reds (the script mode, at c3e24ab) did each frame and on its events, over a World
// (free space, the nav map's columns, pads, gates and the landing rays), a PlanQueue (the route plans)
// and an Rng (the free roam picks). The game mode (step 6) and the offline simulation (step 5) both drive
// it: each frame they gather an AutopilotInput, call Step, apply the force and torque, write the input
// fields back into the component and do what the events ask (ExitAutopilot, flight off, the HUD,
// notifications).
//
// Faithful to the script's native paths (nativeTracker, nativeAttitude, nativeAerobatics on, drone
// style): the same decisions, numbers, order and log lines, the fields under the script's names
// (the follower's in tracker, the drone attitude's in attitude, the attitude control's in control,
// as the script keeps them in its helpers). Left out, by the M6 decisions: the manoeuvres, the plane
// attitude, the script paths behind the switches and the fields nothing reads. The telemetry CSV,
// blackboards, facts, notifications and the cinematic camera are the shell's: Step reports what they
// need (AutopilotEvents) and keeps a telemetry row's data in its members.

#include "AutopilotParams.hpp"
#include "Aerobatics.hpp"
#include "DroneAttitude.hpp"
#include "Landing.hpp"
#include "Plans.hpp"
#include "Route.hpp"
#include "Tracker.hpp"
#include "Trajectory.hpp"
#include "Vehicle.hpp"
#include "World.hpp"

#include "Flight/Controls.hpp"
#include "Flight/DeveloperSettings.hpp"
#include "Flight/Force.hpp"
#include "Flight/Modes.hpp"
#include "Flight/Pid.hpp"
#include "Flight/Stats.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace Navigation {

// The free roam picks' random source, seeded (splitmix64), so a test or a simulation replays the same
// legs. RangeF(a, b) is the script's RandRangeF: uniform in [a, b). Range(a, b) is its RandRange as the
// Autopilot uses it (an index into an array of b items): an integer in [a, b), a when b <= a.
struct Rng {
  explicit Rng(uint64_t aSeed = 1) : state(aSeed) {}
  uint64_t Next();
  // uniform in [0, 1)
  float Unit();
  float RangeF(float aMin, float aMax);
  int32_t Range(int32_t aMin, int32_t aMax);
  uint64_t state;
};

// AutopilotSurge's tunables (the script's surgeHold): the drag thrust back, the surge held full where
// the speed is the car's, and the easing of the commanded surge and sway for the engine sound. Its state
// (held, changing) is the follower's (tracker.surge), as FillTracker and TrackerOut pass it.
struct AutopilotSurgeHold {
  bool dragFeedForward = true;
  bool fullSurge = true;
  float slack = 4.0f;       // m/s over the trajectory's speed the surge is still held full
  float dwell = 0.3f;       // s a change of the hold has to last before it is taken
  float audioWindow = 3.0f; // s the demand the command follows
  float audioJump = 0.6f;   // share of full the demand has to fall below the command to follow over audioTime
  float audioTime = 0.25f;  // s
};

// AutopilotPlanAhead: a trip from rest asked for as a first leg flown at once, its rest spliced on at
// its join. Tunables and state under the script's names.
struct AutopilotPlanAhead {
  bool enabled = true;
  float horizon = 500.0f; // m along the planner's doorway route the first leg goes
  float minTrip = 600.0f; // m: a trip shorter than this is planned in one go
  float margin = 2.0f;    // s added to the rest's expected plan time before the join
  bool firstLeg = false;  // the route is a first leg: it ends short of the goal
  bool waiting = false;   // its rest is being planned
  int32_t trip = 0;       // the Trip request the rest comes from
};

// The destination the game tracks (FlightModeAutopilot.GetDestination: FlightNavMap's target override,
// a tracked pad's deck, or vanilla autodrive's GPS destination), resolved by the shell.
struct AutopilotDestination {
  bool has = false;
  Vec3 at{};
};

// What Step reads each frame.
struct AutopilotInput {
  float dt = 0.0f; // s, the frame's timeDelta
  Flight::StatsState stats;    // component.stats' d_ fields
  Flight::StatsStatic statics; // its s_ fields (mass, air resistance, braking friction, inertia, drag area, thrusters)
  // the component's input fields as they are at the top of the frame: the player's (FlightComponent overwrites
  // what the mode wrote last frame), or the drive sim's on an unmounted car. Read for the player's override.
  Flight::Controls controls;
  bool isPlayerMounted = true;
  float collisionTimer = 1000.0f; // FlightComponent.collisionTimer: a drop is a hit
  Flight::Vec3 thrusterTensor{1.0f, 1.0f, 1.0f};
  WorldInertia inertia; // FlightComponent's world inertia rows (GetWorldInertiaRow), valid when PhysX owns flight
  // FlightSettings: physxFlightPhysics, brakeFactorLinear, the angular damping the attitude control gives
  // back (generalDampFactorAngular, generalYawDampFactor, generalDampFactorAngularMax, dampStabilityK,
  // dampMaxPerFrame) and collisionRecoveryDelay
  Flight::ForceParams settings;
  // the mode's Standard factors (standardModeSurgeFactor, standardModeSwayFactor, standardModeLiftFactor),
  // FlightMode's gravityFactor and the rest UpdateWithNormalLift reads when the player takes over
  Flight::StandardParams standard;
  AutopilotDestination tracked;
  // shapeDrag, dragK, thrustStrength, thrustPerThruster as FlightVehicleModel reads them; nullptr: Flight::Developer()
  const Flight::DeveloperSettings *developer = nullptr;
  // the component's pitch and roll PIDs: with both given, a frame the player has taken over is flown here
  // (Flight::AttitudeLift); without, the shell flies it from AutopilotOutput's manual request
  Flight::Pid *pitchPid = nullptr;
  Flight::Pid *rollPid = nullptr;
};

// What the glue around the mode has to do, gathered since the last Step (the methods add to it too).
struct AutopilotEvents {
  bool exitAutopilot = false; // FlightController.ExitAutopilot: the destination untracked, or settled and handed back
  bool flightOff = false;     // FlightController.autopilotRequestsFlightOff: landed with the player in the car
  std::vector<std::string> notifications; // FlightModeAutopilot.Notify, by localization key
  // PublishProgress: vanilla's autodrive panel's remaining length (m) and time (s)
  bool progress = false;
  float remainingLength = 0.0f, remainingTime = 0.0f;
  // PublishFreeRoamUI: the panel's free roam state (player mounted only)
  bool freeRoamUI = false;
  bool freeRoamEnabled = false;
};

struct AutopilotOutput {
  // the mode's force (m/s2) and torque in the car's frame, as FlyAerobatic writes this.force and this.torque
  Flight::Vec3 force{}, torque{};
  // the player has the controls this frame: the Standard mode's UpdateWithNormalLift with this normal and lift
  // force; force and torque are its result when the input gave the PIDs, else zero for the shell to fill
  bool manual = false;
  Flight::Vec3 manualNormal{0.0f, 0.0f, 1.0f};
  float manualLift = 0.0f;
  // the values to write into the component's input fields (surge and sway the eased commands, the rest 0),
  // for the thrusters' look and the engine sound; false on a manual frame (the player's stay)
  bool writeControls = false;
  Flight::Controls controls;
  float commandSurge = 0.0f, commandSway = 0.0f;
  std::string status;
  AutopilotEvents events;
};

class Autopilot {
public:
  // aParams by reference, so a developer override (SetAutopilotParam) reaches the next frame. aWorld, aPlans,
  // aParams and aRng must outlive the Autopilot.
  Autopilot(const World &aWorld, PlanQueue &aPlans, const AutopilotParams &aParams, Rng &aRng);

  // Engaged: the state reset, free roam chosen when nothing is tracked. The glue (the combat facts, the
  // autodrive panel) stays the shell's, with the player mounted.
  void Activate(const AutopilotInput &aIn);
  // Disengaged: the route cleared and its plan dropped.
  void Deactivate();
  // A frame: the route, hits, override, the state machine, the follower and the drone attitude.
  AutopilotOutput Step(const AutopilotInput &aIn);
  // The car moved somewhere else at rest: hold there, and the route planned again from there if there was one.
  void Teleported(const AutopilotInput &aIn);
  // A tap of the autodrive key while engaged: free roam to the tracked destination, or back.
  void ToggleFreeRoam(const AutopilotInput &aIn);
  // A ground spot near aTarget with room to land, kept aKeepRadius (2D) from aKeepClear (the summon's V);
  // the Autopilot's own keep-clear is not touched.
  bool FindGroundSpot(const Vec3 &aTarget, const Vec3 &aKeepClear, float aKeepRadius, Vec3 &aSpot) const;
  // The route's trajectory built again for this car (the script's public BuildTrajectory, for
  // test helpers, after the tuning changes).
  void BuildTrajectory(const AutopilotInput &aIn);
  // The car's limits filled again from aIn (the script's public RefreshVehicle, for test readers of them).
  void RefreshVehicle(const AutopilotInput &aIn);
  // The events gathered since the last Step, cleared (for a method called between frames).
  AutopilotEvents TakeEvents();
  // Whether Step would hold off aPosition this frame as a jump further than the car could have flown (CheckedPosition):
  // the position the frame flies from is then lastPosition. For the telemetry row the shell writes before Step.
  bool JumpHeldOff(const Vec3 &aPosition, float aSpeed, float aDt) const;

  int32_t RouteSize() const { return route.Size(); }
  // a trajectory to follow (every route gets one)
  bool FollowsTrajectory() const { return trajectory.samples.size() >= 2; }

  const World &world;
  PlanQueue &plans;
  const AutopilotParams &params;
  Rng &rng;
  // the planner's base params the plans start from (FlightNavMap's planParams)
  PlanParams planBase;
  AutopilotSurgeHold surgeHold;
  AutopilotPlanAhead ahead;

  // --- state, under the script's names ---
  std::string status = "idle";
  Route route;
  Trajectory trajectory;
  int32_t request = 0;
  Vec3 destination{};
  bool hasDestination = false;
  // a summon's goal for this flight (FlightSummonSystem), read before the tracked one; cleared by Activate
  Vec3 externalDestination{};
  bool hasExternalDestination = false;
  Vec3 padLandingOffset{};
  Vec3 landingKeepClear{};
  float landingKeepClearRadius = 0.0f;
  Vec3 holdPosition{};
  float overrideTimer = 0.0f;
  float replanTimer = 0.0f;
  float logTimer = 0.0f;
  float stuckTimer = 0.0f;
  Vec3 lastPosition{};
  float positionJumpTimer = 0.0f;
  float settleTimer = 0.0f;
  Vec3 routeGoal{};
  bool landing = false;
  Vec3 landingSpot{};
  int32_t landingPad = -1;
  float touchdownTimer = 0.0f;
  float landingStartZ = 0.0f;
  Vec3 landingHeading0{};
  Vec3 landingHeading{};
  float touchdownLow = 0.0f;
  float spotCheckTimer = 0.0f;
  bool requestRace = false;
  bool requestRejoin = false;
  bool routeLine = false;
  bool onLine = false;
  bool freeRoam = false;
  bool freeRoamChosen = false;
  Vec3 freeRoamCentre{};
  bool requestJoin = false;
  float joinS = 0.0f;
  Vec3 joinPoint{};
  std::vector<Vec3> gateTail;
  std::vector<int32_t> recentGates;
  float gateEndS = 0.0f;
  Vec3 gateEntry{};
  std::vector<Vec3> routeGateTail;
  Vec3 routeGateEntry{};
  bool spliceGate = false;
  std::vector<AvoidSpot> hits; // the script's hits: x, y, z and the radius kept away from
  std::vector<float> hitTimes;
  float clock = 0.0f;
  float lastCollisionTimer = 1000.0f;
  float recovering = 0.0f;
  bool landSlewOn = false;
  float landSlewP = 0.0f;
  float landSlewDescent = 0.0f;
  float landSlewStartYaw = 0.0f;
  float landSlewDelta = 0.0f;
  Vec3 landSlewHeading{};
  bool droneEaseLive = false;
  Flight::Vec3 thrustCmd{};
  float attitudeError = 0.0f;
  float commandSurge = 0.0f, commandSway = 0.0f;
  // the follower's fields (segment, closestS, offRouteNow, remaining, closestNow, aheadSmooth, curvatureAhead,
  // pathCurvature, blockedTimer, blockedAhead, emergencyBrake, targetSpeed, speedCmd, speedRate, accelCmd) and the
  // surge hold's held and changing
  TrackerState tracker;
  // the drone attitude's (droneYaw, droneYawRate, dronePitch, dronePitchRate, droneRoll, droneRollRate, attAccel,
  // attRef, headingRef)
  AttitudeState attitude;
  // the attitude control's (targetRight, targetForward, targetUp, omegaRef, bankAngle, bankRateNow)
  AttitudeControlState control;
  // the car's limits as RefreshVehicle last filled them, and surge and sway as this car flies them
  VehicleSpec vehicle;
  float surgeMax = 15.0f;
  float swayMax = 5.0f;
  // the last plan taken: FlightNavMap's GetLastPlanInfo, GetLastPlanPartial and GetLastPlanJoin, per Autopilot
  std::string lastPlanInfo;
  bool lastPlanPartial = false;
  float lastPlanJoin = 0.0f;
  // with telemetry on, the [AutopilotA] line every frame (the CSV is the shell's)
  bool telemetry = false;
  float telemetryTime = 0.0f;

private:
  // the frame (or the method call) being run: the stats, settings and destination the helpers read
  const AutopilotInput *in = nullptr;
  AutopilotEvents pending;

  const Flight::DeveloperSettings &Dev() const;
  bool CurrentDestination(Vec3 &aTarget) const;
  bool PlayerInput() const;
  Vec3 CheckedPosition(const Vec3 &aPosition, float aDt);

  // the vehicle (AutopilotVehicle.cpp in Autopilot.cpp)
  void RefreshVehicle();
  Vec3 SurgeDrag(const Vec3 &aVelocity) const;
  KeepClear Keep() const;

  // the route (AutopilotRoute.cpp)
  void ClearRoute();
  void DropRequest();
  void ChooseRouteGoal();
  void RequestRoute(const Vec3 &aFrom);
  bool AheadRequest(const Vec3 &aFrom, const Vec3 &aVelocity);
  void AheadReset();
  void AheadTaken();
  bool AheadTakeRest(std::vector<Vec3> &aPoints);
  void UpdateRoute(float aDt);
  void SpliceRoute(const std::vector<Vec3> &aPoints);
  std::vector<Vec3> RoundCorners(const std::vector<Vec3> &aPoints) const;
  bool TakePlan(int32_t aId, std::vector<Vec3> &aPoints);
  void WatchHits(float aDt, const Vec3 &aP, const Vec3 &aV);
  void Replan(const Vec3 &aP, const Vec3 &aV);
  Vec3 NearestFree(const Vec3 &aP) const;
  void BuildTrajectory();
  void RecheckLandingSpot(float aDt);

  // free roam (AutopilotFreeRoam.cpp)
  void BeginFreeRoam();
  bool PickFreeRoamGoal(const Vec3 &aFrom, const Vec3 &aHeading, float aMaxTurn, Vec3 &aGoal);
  bool PickFreeRoamGate(const Vec3 &aFrom, const Vec3 &aHeading, Vec3 &aEntry);
  bool NextFreeRoamLeg(const Vec3 &aFrom, const Vec3 &aHeading, bool aJoin);
  void ChainFreeRoam();
  void PublishFreeRoamUI();

  // hold, descent and yaw (AutopilotDescent.cpp)
  Vec3 HoldVelocity(const Vec3 &aP, const Vec3 &aToTarget);
  float TimeToDescent() const;
  float LandSlewYaw(float aTimeLeft, float aDt, float aYaw);
  float ChooseYaw(float aDt, bool aLive, float aWantedYaw);
  Vec3 Heading(const Vec3 &aP, const Vec3 &aDesired) const;

  // the frame (Autopilot.cpp)
  void Arrive(const Vec3 &aP);
  void Descend(const Vec3 &aP, float aDt);
  void FlyAerobatic(float aTimeDelta, const Vec3 &aAccel, const Vec3 &aHeading, AutopilotOutput &aOut);
};

// Steepest climb or descent along the points, degrees, each taken over at least aSpan metres of horizontal
// travel (FlightModeAutopilot.SteepestPitch).
float SteepestPitch(const std::vector<Vec3> &aPoints, float aSpan);
// Horizontal unit heading a fraction f of the way from a to b, turning the short way round
// (FlightModeAutopilot.BlendHeading).
Vec3 BlendHeading(const Vec3 &aA, const Vec3 &aB, float aF);

} // namespace Navigation
