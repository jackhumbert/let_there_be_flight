#pragma once

// The Autopilot's tuning (docs/design/autopilot.md, Tuning): every field of FlightModeAutopilot.reds
// the native Autopilot reads, with the script's defaults and comments, by the script's names. A test
// (autopilot_params_defaults) checks each default against the script while both exist. Not here: the Mod
// Settings that did nothing (maxSpeed, lateralAccel), the manoeuvres and the plane attitude (deleted),
// the native switches and the runtime state.

#include <cstdint>
#include <string_view>
#include <vector>

namespace Navigation {

// How the Autopilot ends a flight to a destination on the ground (AutopilotParams::landingMode, the Mod Setting Land at
// Destination's index): hover over it and hand back, set the car down on its wheels and switch flight off, or sink
// until it touches down and switch flight off.
enum LandingMode : int32_t { kLandHover = 0, kLandWheels = 1, kLandTouchDown = 2 };

struct AutopilotParams {
  float brakeAccel = 6.0f; // m/s2 planned deceleration
  int32_t landingMode = kLandWheels; // LandingMode
  float padRange = 0.0f; // m around a destination on the ground to look for a landing pad (0 = off)
  // drone style: the nose turns into the landing heading over the last seconds before the descent
  // (LandSlewYaw; not in Mod Settings, set by test helpers; a Mod Setting is written back from its stored value when the mode
  // is engaged)
  bool droneBrakeTurn = false;
  // drone style: the body tilts droneBodyShare of the way to the thrust direction the commanded
  // acceleration and gravity need, and the surge and sway thrusters point the rest (half: 22.5
  // degrees of body for travelling at 45), so the thrusters keep moving
  float droneBodyShare = 0.5f;
  // slowing, the share of the tilt the body takes: less than speeding up (braking is the harder push), so
  // the nose pulls up less than it leans forward speeding up and reverse surge does the rest
  float droneBrakeShare = 0.25f;
  // x g: the vertical the body's tilt is worked out against at least (crests: see DroneAttitude)
  float droneTiltFloor = 1.0f;
  // taking off: the nose leans at least droneTakeoffTilt forward while the car climbs out and speeds
  // up at a route's start, in by droneTakeoffIn m/s, whole to droneTakeoffFull and gone by
  // droneTakeoffOut (the climb's own thrust is near vertical and left it level; the car is at 30 m/s
  // a second after lifting off)
  float droneTakeoffTilt = 10.0f; // deg
  float droneTakeoffIn = 5.0f; // m/s
  float droneTakeoffFull = 25.0f; // m/s
  float droneTakeoffOut = 50.0f; // m/s
  float droneTakeoffLength = 250.0f; // m along the route it applies over
  float droneMaxTilt = 55.0f; // deg the body tilts at most, either axis (sets the speed envelope)
  float droneTiltTime = 0.2f; // s, smoothing of the acceleration the tilt follows
  // m/s2 bending the path downwards, in place of racePushOver: the body stays upright (DroneAttitude
  // keeps the thrust from pointing down) and the lift pushes down: gravity plus
  // (standardModeLiftFactor - g) of lift downwards, 25 m/s2, planned at raceMargin
  float dronePushOver = 21.0f;
  float trajectoryAttitudeLead = 0.0f; // s ahead the planned tilt is read (the plan's tilts already start early)
  // leaving the line on a trajectory (docs/design/trajectory.md, Leaving the plan): replanned from the car's
  // state, never steered back to a point behind it
  float replanOffLine = 8.0f; // m off the line before replanning (replanDistance without a trajectory)
  float trajectoryCorrectionMax = 25.0f; // m/s2 the follower adds to the trajectory's acceleration at most (swept)
  float rejoinAhead = 3.0f; // s of the car's speed along the line it rejoins it (rejoinMin m at least)
  float rejoinMin = 150.0f;
  float hitAvoidRadius = 10.0f; // m round a hit the next plans keep away from
  float hitAvoidTime = 60.0f; // s a hit is kept away from
  float droneYawLead = 0.3f; // s along the line the nose aims (DroneAttitude)
  float droneYawFrequency = 3.0f; // rad/s, heading easing
  float droneYawRateMax = 1.5f; // rad/s
  float droneYawAccelMax = 5.0f; // rad/s2
  float droneLandSlewYaw = 120.0f; // deg the landing turn comes round by, nearest (LandSlewYaw)
  float droneLandSlewBefore = 2.0f; // s before the descent the landing turn starts
  float droneLandSlewDuring = 2.0f; // s into the descent it ends; spread evenly over both
  // takeoff and landing
  float approachHeight = 20.0f; // m above the landing spot the route ends at
  float descentRate = 8.0f; // m/s at most during the landing descent (a descent of up to 32 m is all speeding up and braking)
  float touchdownRate = 0.8f; // m/s the touch down ends on (kLandTouchDown), and the landed car settles at
  // the descent's profile: the sink speeds up at descentAccel and brakes at it, so a long descent is half each (the
  // height two parabola halves), capped at descentRate; the car follows the profile's height, speed and acceleration
  // with a critically damped spring of descentFrequency
  float descentAccel = 2.0f; // m/s2
  float descentFrequency = 4.0f; // rad/s
  // kLandWheels: the car is held wheelClearance over the height it rests at on its wheels (the ground from the wheel
  // emitters' rays), and flight goes off once it has stayed within wheelTolerance of it, slower than wheelStill and
  // upright, for wheelSettle. Ground under the wheels further apart in height than wheelGroundSpread (a kerb under one
  // side), or no ground found under them, and it touches down instead.
  float wheelClearance = 0.1f; // m
  float wheelTolerance = 0.05f; // m
  float wheelStill = 0.3f; // m/s
  float wheelSettle = 0.4f; // s
  float wheelGroundSpread = 0.3f; // m
  int32_t landingSearch = 12; // rings of 2 m columns searched for a landing spot
  float landingShaft = 4.0f; // m around a landing spot that must be open above it too (ShaftClear)
  // m around a landing spot whose floor must be level on the nav map (IsFlat): the bake grows solids
  // by 3 m for the car, so a roof's edge on the map is 3 m out from the real one, then half the car
  // and a couple of metres to spare (the map is 2 m columns)
  float landingFlat = 8.0f;
  float landingFootprint = 3.0f; // m out from a landing spot the game's collision must be under the car too (SpotSupported)
  // m from the spot across within which the descent sinks: the clear shaft (ShaftClear), so the
  // car slides the last metres over the spot while coming down instead of stopping above it first
  float landingOverSpot = 4.0f;
  // the descent takes over from the route this far from its end (m) and below this speed (m/s); it
  // keeps closing on the spot and sinks once within landingOverSpot (taken over right above the
  // spot, the car hung still before sinking)
  float landingEntry = 15.0f;
  float landingEntrySpeed = 10.0f;
  float landingRecheck = 300.0f; // m before the route's end the landing spot is checked again (vehicles, the game's collision under the car)
  float landingShiftSpan = 150.0f; // m of the route's end moved over to a new spot
  float padSnapDistance = 25.0f; // m: a destination this close to a landing pad (and at its level) is on it
  float padSnapLevel = 6.0f; // m between the destination's height and the pad's deck
  // easing
  float brakeMax = 9.0f; // m/s2 at most when slowing (planned at brakeAccel)
  float speedGain = 1.0f; // 1/s, half the rate the gap to the speed blocked space allows is braked at
  float accelJerk = 20.0f; // m/s3, rate limit of the acceleration command
  float yawSmoothing = 0.25f; // s, low-pass of the yaw input
  // controller tuning, not in Mod Settings
  float velocityGain = 1.2f; // 1/s, velocity error -> acceleration
  float maxAccel = 12.0f; // m/s2
  float overrideHold = 1.5f; // s the autopilot stays off after player input
  float replanDistance = 30.0f; // m off the route before replanning
  float arriveDistance = 6.0f;
  float cornerOvershoot = 10.0f; // m the lookahead may reach past the next corner
  float cornerSpan = 15.0f; // m of route before / after a corner that set its radius
  float collisionHorizon = 1.5f; // s of current velocity checked against the map
  // aerobatics tuning, not in Mod Settings
  float attitudeFrequency = 5.0f; // rad/s, natural frequency of the attitude control
  float attitudeDamping = 0.8f;
  float maxAngularAccel = 20.0f; // rad/s2
  // rad/s2 of angular acceleration per unit of mode torque x mass x thruster tensor / inertia
  // (FlightComponent scales the torque by those, PhysX divides by its own inertia), per axis,
  // measured with TorqueProbe (tools/ingame) on an Archer Quartz; all respond two frames late
  float torqueGainPitch = 2.0f;
  float torqueGainRoll = 0.5f;
  float torqueGainYaw = 0.9f;
  float attitudeLead = 0.25f; // s of the commanded acceleration added to the velocity for the nose
  float maxHeadingRate = 0.9f; // rad/s the heading may turn at hover speeds
  // race: the same thrust the player has (surge, lift, brake), with the lift banked into turns
  float raceMargin = 0.85f; // share of the envelope the trajectory plans for
  float raceJerk = 60.0f; // m/s3, rate limit of the acceleration command
  float raceSpeedCap = 120.0f; // m/s, whatever the drag would allow
  // race: corners rounded much wider (the bend's radius sets its speed); the cut steps down by
  // raceRoundStep until the arc is clear, to at least raceRoundMin
  float raceCornerRound = 150.0f; // m
  float raceRoundShare = 0.5f; // of each leg at most (two corners can meet midway)
  float raceRoundStep = 0.7f;
  float raceRoundMin = 8.0f; // m
  float raceOffLine = 4.0f; // m off a line before the car is treated as off it
  // race route character (Navigation::RacePreset holds the same defaults; tools/navplan/bench.py
  // compares settings): between the buildings rather than over the rooftops
  float raceRouteHeight = 40.0f; // m above the local floor the route would like to fly
  // climbing is cheap: dearer, every rise of the floor-following height made routes go round the
  // city over the water
  float raceClimbCost = 0.5f; // per metre of climbing, against 1 per metre flown
  // 12: narrower slots between decks and skybridges kink the line at speed. A start with no 12 m
  // way out gets the retry without it. The doorways in navmap.bin are baked with this value
  // (tools/navmap_bake/doorways.py): rebake if it changes.
  float raceMinPassage = 12.0f; // m of free height the route may squeeze through at least (0 = any)
  float raceSearchWeight = 1.2f; // A* heuristic weight, 1 = optimal (slower to plan)
  // per 45 degrees the route turns over a few cells (as metres flown): straighter, more open
  // corridors, whose bends the line takes wide
  float raceTurnCost = 6.559f;
  // drift: in a bend the nose turns into it, up to raceDriftMax at raceDriftFull sideways (none
  // below raceDriftStart), so the surge thrust pushes round the corner along with the banked lift
  float raceDriftMax = 20.0f; // deg
  // m/s2 the trajectory plans slowing with at most: the brake grows with speed, but the flown
  // speed eases into it, and planned at full the car arrived too fast
  float racePlanDecel = 18.0f; // what the car reached following a speed profile (19 m/s2)
  // m/s2 at most bending the path downwards: the car stays upright and pushes with negative lift
  // (FlyAerobatic keeps the top from turning over past raceTopDown); at half a g crests were most
  // of the slowdowns
  float racePushOver = 15.0f;
  // line tracking: the line's direction at the flown speed, its curvature fed forward
  // lineLead ahead (the bank takes a moment to build), a damped spring onto the line across it.
  // (Aiming at a lookahead point cuts inside every bend at speed.)
  float lineLead = 0.25f; // s
  float lineSpring = 1.0f; // 1/s, distance off the line -> speed back towards it
  float lineVelocityGain = 2.5f; // 1/s, velocity error -> acceleration
  float lineSpan = 10.0f; // m either side a line point's direction and bend are taken over
  // bank easing: the bank follows its target through a critically damped second order filter
  // with limited roll rate and roll acceleration (the target flicks as route points pass)
  float raceRollFrequency = 2.5f; // rad/s
  // Race rolls are the gentlest movements (faster ones swung the bank to 90 degrees within a
  // second); the ribbon looks further ahead (ribbonLead) so the slower roll still starts in time
  float raceRollRateMax = 1.0f; // rad/s
  float raceRollAccelMax = 3.0f; // rad/s2
  // pitch in Race, the same way as the bank (off the line too: the take-off and the arrival); on a
  // line from the line's own slope ribbonLead ahead
  // (not the velocity plus the commanded acceleration, which carries every height correction and
  // bobbed the nose), eased through the same filter
  float racePitchFrequency = 2.5f; // rad/s
  float racePitchRateMax = 1.0f; // rad/s
  float racePitchAccelMax = 3.0f; // rad/s2
  // free roam (engaged with no destination tracked, like vanilla autodrive's free roam): legs to
  // random points ahead, chained into one route without stopping, around where it began
  float freeRoamRadius = 2500.0f; // m from where free roam began the legs stay within
  float freeRoamLegMin = 900.0f; // m
  float freeRoamLegMax = 1600.0f; // m
  float freeRoamSpread = 60.0f; // deg either side of the heading a leg may turn (widened when nothing fits)
  float freeRoamMaxDepth = 2.0f; // m below 0 the ground under a leg's end may be at most (open sea reads -13.5 on the map)
  float freeRoamMaxTurn = 100.0f; // deg either side of the heading a leg may turn at most, unless nothing fits
  float freeRoamMaxPitch = 45.0f; // deg, steepest climb or descent a leg may have
  float freeRoamGateMin = 300.0f; // m, nearest gate a leg goes for (PickFreeRoamGate)
  float gateLead = 60.0f; // m of straight run before and after a gate (gates.py checks 80)
  float gateStep = 5.0f; // m between the run's points (a line's spacing)
  int32_t gateMemory = 8; // gates flown lately, not picked again
  float freeRoamChainAt = 700.0f; // m left on the route when the next leg is planned
  float freeRoamJoinAhead = 4.0f; // s of the flown speed ahead the next leg starts, at least freeRoamJoinMin
  float freeRoamJoinMin = 150.0f; // m
  float freeRoamSmoothSpan = 60.0f; // m either side of a join smoothed out
  // m/s at most: faster, the world cannot stream in new areas in time and the game stops on its
  // loading overlay
  float freeRoamTopSpeed = 60.0f;
  // m a leg's end may differ in height from where it starts (from a tower top to a street the line
  // crested and dived, and the car rolled over pushing into the dive)
  float freeRoamMaxRise = 100.0f;
  // free roam flies lower than the Race and follows the buildings more closely: the route's preferred
  // height over the floor, and the line's lineHeightFree and lineHeightWavelength (Planner.hpp). On
  // three legs: median 23 to 27 m over the floor instead of 40 to 59, at about the same speed.
  float freeRoamRouteHeight = 15.0f;
  float freeRoamHeightFree = 8.0f;
  float freeRoamHeightWavelength = 300.0f;
  // per metre flown over a building (PlanParams.overCost): legs go round buildings rather than over
  // them (on two Dogtown legs 17 to 21% over buildings instead of 37%, the legs 3 to 14 s longer)
  float freeRoamOverCost = 2.0f;
  // per metre flown off the game's roads (Roads, roads.tsv; PlanParams.guideCost): legs keep near some
  // road, 15 to 40 m over it, and may leave one for another; 0: off. freeRoamGuideLine: the line kept
  // to that band too, not only the route search
  float freeRoamGuideCost = 0.0f;
  bool freeRoamGuideLine = false;
  float positionJumpHold = 0.5f; // s a jump is held off before it counts as a real move
};

// Each field by its script name, for the default check and for developer overrides by name.
struct AutopilotParamRef {
  const char *name;
  float AutopilotParams::*f = nullptr;
  int32_t AutopilotParams::*i = nullptr;
  bool AutopilotParams::*b = nullptr;
};
const std::vector<AutopilotParamRef> &AutopilotParamTable();

// Sets the field named aName (a Bool from 0 or 1, an Int32 rounded); false for an unknown name.
bool SetAutopilotParam(AutopilotParams &aParams, std::string_view aName, float aValue);

} // namespace Navigation
