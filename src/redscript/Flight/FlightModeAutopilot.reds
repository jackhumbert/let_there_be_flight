// Autopilot: flies the planned route to the GPS destination (or FlightNavMap's target override).
//
// Route: FlightNavMap.PlanRaceAsync (C++, src/red4ext/Navigation/Planner.cpp) gives a line
// over the baked nav map (or, when it cannot make one, the plain route with its corners rounded
// here), and FlightTrajectory (docs/design/trajectory.md) the speed, its rate and the body tilt
// along it. Each frame:
//   - follow: on a line the line's direction at the trajectory's speed, its bend and the
//     speed's rate fed forward, a spring back onto it; on a plain route a lookahead point along it;
//   - speed: the trajectory's at the car's place on the route, lowered only to stop short of
//     blocked space the map does not know about (PredictBlocked);
//   - attitude (DroneAttitude): the trajectory's tilt plus what the follower's correction adds.
// The outputs go into the same inputs the player drives (surge, yaw, sway, brake), so thrusters,
// audio and the standard attitude control react as usual. Any player input wins for that frame
// and holds the autopilot off briefly; off the route it is replanned from the car's state.
public class FlightModeAutopilot extends FlightModeStandard {
  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-Flight-Mode-Settings")
  @runtimeProperty("ModSettings.displayName", "UI-Settings-Autopilot-Enabled")
  @runtimeProperty("ModSettings.description", "UI-Settings-Autopilot-Enabled-Description")
  public let enabled: Bool = true;

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-Autopilot")
  @runtimeProperty("ModSettings.displayName", "UI-Settings-Autopilot-Max-Speed")
  @runtimeProperty("ModSettings.description", "UI-Settings-Autopilot-Max-Speed-Description")
  @runtimeProperty("ModSettings.step", "1.0")
  @runtimeProperty("ModSettings.min", "10.0")
  @runtimeProperty("ModSettings.max", "100.0")
  public let maxSpeed: Float = 45.0;          // m/s

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-Autopilot")
  @runtimeProperty("ModSettings.displayName", "UI-Settings-Autopilot-Cornering")
  @runtimeProperty("ModSettings.description", "UI-Settings-Autopilot-Cornering-Description")
  @runtimeProperty("ModSettings.step", "0.5")
  @runtimeProperty("ModSettings.min", "1.0")
  @runtimeProperty("ModSettings.max", "15.0")
  public let lateralAccel: Float = 5.0;       // m/s2 allowed in corners

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-Autopilot")
  @runtimeProperty("ModSettings.displayName", "UI-Settings-Autopilot-Braking")
  @runtimeProperty("ModSettings.description", "UI-Settings-Autopilot-Braking-Description")
  @runtimeProperty("ModSettings.step", "0.5")
  @runtimeProperty("ModSettings.min", "2.0")
  @runtimeProperty("ModSettings.max", "15.0")
  public let brakeAccel: Float = 6.0;         // m/s2 planned deceleration

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-Autopilot")
  @runtimeProperty("ModSettings.displayName", "UI-Settings-Autopilot-Land")
  @runtimeProperty("ModSettings.description", "UI-Settings-Autopilot-Land-Description")
  public let landAtDestination: Bool = true;

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-Autopilot")
  @runtimeProperty("ModSettings.displayName", "UI-Settings-Autopilot-Pad-Range")
  @runtimeProperty("ModSettings.description", "UI-Settings-Autopilot-Pad-Range-Description")
  @runtimeProperty("ModSettings.step", "25.0")
  @runtimeProperty("ModSettings.min", "0.0")
  @runtimeProperty("ModSettings.max", "500.0")
  public let padRange: Float = 0.0;           // m around a destination on the ground to look for a landing pad (0 = off)

  // Manoeuvres (FlightAerobatics.reds: rolls, loop, Immelmann, split-S) are disconnected: nothing
  // starts one, and these are not Mod Settings. The code is kept (docs/history.md).
  public let aerobatics: Bool = false;        // full 3D attitude (Race implies it)
  public let stuntInterval: Float = 0.0;      // s between stunts on long straights (unused while disconnected)

  // The Race flies like a quadcopter (DroneAttitude), the only style; the plane attitude in
  // FlyAerobatic is kept but not flown. Not a Mod Setting.
  public let droneStyle: Bool = true;
  // drone style: the nose turns into the landing heading over the last seconds before the descent
  // (LandSlewYaw; not in Mod Settings: LtbfBridge.DroneBrakeTurn; a Mod Setting is written back from its stored value when the mode
  // is engaged)
  public let droneBrakeTurn: Bool = false;

  // drone style: the body tilts droneBodyShare of the way to the thrust direction the commanded
  // acceleration and gravity need, and the surge and sway thrusters point the rest (half: 22.5
  // degrees of body for travelling at 45), so the thrusters keep moving
  public let droneBodyShare: Float = 0.5;
  // slowing, the share of the tilt the body takes: less than speeding up (braking is the harder push), so
  // the nose pulls up less than it leans forward speeding up and reverse surge does the rest
  public let droneBrakeShare: Float = 0.25;
  // x g: the vertical the body's tilt is worked out against at least (crests: see DroneAttitude)
  public let droneTiltFloor: Float = 1.0;
  // taking off: the nose leans at least droneTakeoffTilt forward while the car climbs out and speeds
  // up at a route's start, in by droneTakeoffIn m/s, whole to droneTakeoffFull and gone by
  // droneTakeoffOut (the climb's own thrust is near vertical and left it level; the car is at 30 m/s
  // a second after lifting off)
  public let droneTakeoffTilt: Float = 10.0;  // deg
  public let droneTakeoffIn: Float = 5.0;     // m/s
  public let droneTakeoffFull: Float = 25.0;  // m/s
  public let droneTakeoffOut: Float = 50.0;   // m/s
  public let droneTakeoffLength: Float = 250.0; // m along the route it applies over
  public let droneMaxTilt: Float = 55.0;      // deg the body tilts at most, either axis (sets the speed envelope)
  public let droneTiltTime: Float = 0.2;      // s, smoothing of the acceleration the tilt follows
  // m/s2 bending the path downwards, in place of racePushOver: the body stays upright (DroneAttitude
  // keeps the thrust from pointing down) and the lift pushes down: gravity plus
  // (standardModeLiftFactor - g) of lift downwards, 25 m/s2, planned at raceMargin
  public let dronePushOver: Float = 21.0;
  private let vehicleModel: ref<FlightVehicleModel>;
  // the route's trajectory (FlightTrajectory, docs/design/trajectory.md): the speeds, the speed's
  // rate and the body tilt the car flies
  public let trajectory: ref<FlightTrajectory>;
  public let trajectoryAttitudeLead: Float = 0.0;  // s ahead the planned tilt is read (the plan's tilts already start early)
  private let attRef: Vector4;                     // the trajectory's acceleration, smoothed as attAccel is
  public let nativeAttitude: Bool = true;  // drone attitude in C++ (FlightDroneAttitude); false: DroneAttitude's script below (docs/design/native-ports.md)
  private let attitude: ref<FlightDroneAttitude>;
  public let nativeTracker: Bool = true;  // the follower in C++ (FlightTracker); false: the script in Update (docs/design/native-ports.md)
  public let nativeAerobatics: Bool = false;  // manoeuvre geometry and the attitude torque in C++ (FlightManeuverShape, FlightAttitudeControl); false: the script in FlightAerobatics.reds and FlyAerobatic (docs/design/native-ports.md)
  public let attitudeControl: ref<FlightAttitudeControl>;
  private let tracker: ref<FlightTracker>;
  // leaving the line on a trajectory (docs/design/trajectory.md, step 5): replanned from the car's
  // state, never steered back to a point behind it
  public let replanOffLine: Float = 8.0;    // m off the line before replanning (replanDistance without a trajectory)
  public let trajectoryCorrectionMax: Float = 25.0; // m/s2 the follower adds to the trajectory's acceleration at most (swept)
  public let rejoinAhead: Float = 3.0;      // s of the car's speed along the line it rejoins it (rejoinMin m at least)
  public let rejoinMin: Float = 150.0;
  private let requestRejoin: Bool;
  public let hitAvoidRadius: Float = 10.0;  // m round a hit the next plans keep away from
  public let hitAvoidTime: Float = 60.0;    // s a hit is kept away from
  private let lastCollisionTimer: Float = 1000.0;
  private let recovering: Float;            // s left of the controls' damping after a hit (FlightComponent.ProcessImpact)
  public let hits: array<Vector4>;         // x, y, z and the radius kept away from
  private let hitTimes: array<Float>;
  private let clock: Float;
  public let droneYawLead: Float = 0.3;       // s along the line the nose aims (DroneAttitude)
  public let droneYawFrequency: Float = 3.0;  // rad/s, heading easing
  public let droneYawRateMax: Float = 1.5;    // rad/s
  public let droneYawAccelMax: Float = 5.0;   // rad/s2
  public let droneLandSlewYaw: Float = 120.0; // deg the landing turn comes round by, nearest (LandSlewYaw)
  public let droneLandSlewBefore: Float = 2.0; // s before the descent the landing turn starts
  public let droneLandSlewDuring: Float = 2.0; // s into the descent it ends; spread evenly over both

  // The Autopilot always flies the Race (the line at the car's full thrust). Nothing reads
  // this any more; it is kept for LtbfBridge.Aerobatics's race argument (docs/history.md).
  public let raceMode: Bool = true;

  // takeoff and landing
  public let approachHeight: Float = 20.0;    // m above the landing spot the route ends at
  public let descentRate: Float = 4.0;        // m/s at most during the landing descent
  public let touchdownRate: Float = 0.8;      // m/s in the last metres
  public let landingSearch: Int32 = 12;       // rings of 2 m columns searched for a landing spot
  public let landingShaft: Float = 4.0;       // m around a landing spot that must be open above it too (ShaftClear)
  // m around a landing spot whose floor must be level on the nav map (IsFlat): the bake grows solids
  // by 3 m for the car, so a roof's edge on the map is 3 m out from the real one, then half the car
  // and a couple of metres to spare (the map is 2 m columns)
  public let landingFlat: Float = 8.0;
  public let landingFootprint: Float = 3.0;   // m out from a landing spot the game's collision must be under the car too (SpotSupported)
  // m from the spot across within which the descent sinks: the clear shaft (ShaftClear), so the
  // car slides the last metres over the spot while coming down instead of stopping above it first
  public let landingOverSpot: Float = 4.0;
  // the descent takes over from the route this far from its end (m) and below this speed (m/s); it
  // keeps closing on the spot and sinks once within landingOverSpot (taken over right above the
  // spot, the car hung still before sinking)
  public let landingEntry: Float = 15.0;
  public let landingEntrySpeed: Float = 10.0;
  public let landingRecheck: Float = 300.0;   // m before the route's end the landing spot is checked again (vehicles, the game's collision under the car)
  public let landingShiftSpan: Float = 150.0; // m of the route's end moved over to a new spot
  public let spotCheckTimer: Float;
  public let padSnapDistance: Float = 25.0;   // m: a destination this close to a landing pad (and at its level) is on it
  public let padSnapLevel: Float = 6.0;       // m between the destination's height and the pad's deck

  // easing
  public let brakeMax: Float = 9.0;           // m/s2 at most when slowing (planned at brakeAccel)
  public let speedGain: Float = 1.0;          // 1/s, half the rate the gap to the speed blocked space allows is braked at
  public let accelJerk: Float = 20.0;         // m/s3, rate limit of the acceleration command
  public let yawSmoothing: Float = 0.25;      // s, low-pass of the yaw input

  // controller tuning, not in Mod Settings
  public let velocityGain: Float = 1.2;       // 1/s, velocity error -> acceleration
  public let maxAccel: Float = 12.0;          // m/s2
  public let overrideHold: Float = 1.5;       // s the autopilot stays off after player input
  public let replanDistance: Float = 30.0;    // m off the route before replanning
  public let arriveDistance: Float = 6.0;
  public let cornerOvershoot: Float = 10.0;   // m the lookahead may reach past the next corner
  public let cornerSpan: Float = 15.0;        // m of route before / after a corner that set its radius
  public let collisionHorizon: Float = 1.5;   // s of current velocity checked against the map

  // aerobatics tuning, not in Mod Settings
  public let attitudeFrequency: Float = 5.0;  // rad/s, natural frequency of the attitude control
  public let attitudeDamping: Float = 0.8;
  public let maxAngularAccel: Float = 20.0;   // rad/s2
  // rad/s2 of angular acceleration per unit of mode torque x mass x thruster tensor / inertia
  // (FlightComponent scales the torque by those, PhysX divides by its own inertia), per axis,
  // measured with TorqueProbe (tools/ingame) on an Archer Quartz; all respond two frames late
  public let torqueGainPitch: Float = 2.0;
  public let torqueGainRoll: Float = 0.5;
  public let torqueGainYaw: Float = 0.9;
  public let attitudeLead: Float = 0.25;      // s of the commanded acceleration added to the velocity for the nose
  public let attitudeDelay: Float = 0.1;      // s ahead a manoeuvre's attitude is taken (the car's response lag)
  public let settledError: Float = 10.0;      // deg of attitude error under which a manoeuvre may start
  public let maxHeadingRate: Float = 0.9;     // rad/s the heading may turn at hover speeds
  public let maneuverAccel: Float = 10.0;     // m/s2 a pitch manoeuvre's circle is sized for
  public let maneuverMaxAccel: Float = 25.0;  // m/s2 the tracking of a manoeuvre may command
  public let maneuverJerk: Float = 80.0;      // m/s3 rate limit of the acceleration during a manoeuvre
  public let maneuverPosGain: Float = 1.0;    // 1/s2, position error -> acceleration
  public let maneuverVelGain: Float = 2.0;    // 1/s, velocity error -> acceleration
  public let maneuverRollTime: Float = 1.5;   // s of a half roll
  public let maneuverMinSpeed: Float = 12.0;  // m/s
  public let maneuverMaxSpeed: Float = 25.0;  // m/s, faster and the circles do not fit in the city
  public let maneuverAbortDistance: Float = 15.0; // m off the shape before giving up on it
  public let pitchMargin: Float = 1.0;        // m of room above and below a pitched car (see FlightAerobatics.reds)
  // race: the same thrust the player has (surge, lift, brake), with the lift banked into turns
  public let raceMargin: Float = 0.85;        // share of the envelope the trajectory plans for
  public let raceJerk: Float = 60.0;          // m/s3, rate limit of the acceleration command
  public let raceSpeedCap: Float = 120.0;     // m/s, whatever the drag would allow
  // race: corners rounded much wider (the bend's radius sets its speed); the cut steps down by
  // raceRoundStep until the arc is clear, to at least raceRoundMin
  public let raceCornerRound: Float = 150.0;  // m
  public let raceRoundShare: Float = 0.5;     // of each leg at most (two corners can meet midway)
  public let raceRoundStep: Float = 0.7;
  public let raceRoundMin: Float = 8.0;       // m
  public let raceBankTime: Float = 0.12;      // s, smoothing of the acceleration the attitude follows
  public let raceOffLine: Float = 4.0;        // m off a line before the car is treated as off it
  // race route character (Navigation::RacePreset holds the same defaults; tools/navplan/bench.py
  // compares settings): between the buildings rather than over the rooftops
  public let raceRouteHeight: Float = 40.0;   // m above the local floor the route would like to fly
  // climbing is cheap: dearer, every rise of the floor-following height made routes go round the
  // city over the water
  public let raceClimbCost: Float = 0.5;      // per metre of climbing, against 1 per metre flown
  // 12: narrower slots between decks and skybridges kink the line at speed. A start with no 12 m
  // way out gets the retry without it. The doorways in navmap.bin are baked with this value
  // (tools/navmap_bake/doorways.py): rebake if it changes.
  public let raceMinPassage: Float = 12.0;    // m of free height the route may squeeze through at least (0 = any)
  public let raceSearchWeight: Float = 1.2;   // A* heuristic weight, 1 = optimal (slower to plan)
  // per 45 degrees the route turns over a few cells (as metres flown): straighter, more open
  // corridors, whose bends the line takes wide
  public let raceTurnCost: Float = 6.559;
  // drift: in a bend the nose turns into it, up to raceDriftMax at raceDriftFull sideways (none
  // below raceDriftStart), so the surge thrust pushes round the corner along with the banked lift
  public let raceDriftMax: Float = 20.0;      // deg
  public let raceDriftStart: Float = 8.0;     // m/s2
  public let raceDriftFull: Float = 28.0;     // m/s2
  // m/s2 the trajectory plans slowing with at most: the brake grows with speed, but the flown
  // speed eases into it, and planned at full the car arrived too fast
  public let racePlanDecel: Float = 18.0;     // what the car reached following a speed profile (19 m/s2)
  // m/s2 at most bending the path downwards: the car stays upright and pushes with negative lift
  // (FlyAerobatic keeps the top from turning over past raceTopDown); at half a g crests were most
  // of the slowdowns
  public let racePushOver: Float = 15.0;
  public let raceTopDown: Float = 0.8;        // g of downward pull the attitude follows; the thrust gives the rest
  // line tracking: the line's direction at the flown speed, its curvature fed forward
  // lineLead ahead (the bank takes a moment to build), a damped spring onto the line across it.
  // (Aiming at a lookahead point cuts inside every bend at speed.)
  public let lineLead: Float = 0.25;          // s
  public let lineSpring: Float = 1.0;         // 1/s, distance off the line -> speed back towards it
  public let lineVelocityGain: Float = 2.5;   // 1/s, velocity error -> acceleration
  public let lineSpan: Float = 10.0;          // m either side a line point's direction and bend are taken over
  // ribbon: on a line the bank comes from the line's own bend (over ribbonSpan either side,
  // ribbonLead seconds ahead) at the speed flown, not from the commanded acceleration, which
  // carries every small correction back onto the line and rolled the car for each of them
  public let ribbonSpan: Float = 50.0;        // m
  public let ribbonLead: Float = 0.8;         // s (0.3 with the quicker roll)
  // sideways acceleration the ribbon ignores: at 80 m/s a 2 km bend in the line already asks for
  // 3 m/s2 (an 18 degree bank), which read as rolling for nothing; only what is above it banks
  public let ribbonDeadband: Float = 3.0;     // m/s2
  public let ribbonSlow: Float = 10.0;        // m/s under which the ribbon banks nothing (full from 25 m/s)
  // bank easing: the bank follows its target through a critically damped second order filter
  // with limited roll rate and roll acceleration (the target flicks as route points pass)
  public let raceRollFrequency: Float = 2.5;  // rad/s
  // Race rolls are the gentlest movements (faster ones swung the bank to 90 degrees within a
  // second); the ribbon looks further ahead (ribbonLead) so the slower roll still starts in time
  public let raceRollRateMax: Float = 1.0;    // rad/s
  public let raceRollAccelMax: Float = 3.0;   // rad/s2
  // pitch in Race, the same way as the bank (off the line too: the take-off and the arrival); on a
  // line from the line's own slope ribbonLead ahead
  // (not the velocity plus the commanded acceleration, which carries every height correction and
  // bobbed the nose), eased through the same filter
  public let racePitchFrequency: Float = 2.5; // rad/s
  public let racePitchRateMax: Float = 1.0;   // rad/s
  public let racePitchAccelMax: Float = 3.0;  // rad/s2
  // free roam (engaged with no destination tracked, like vanilla autodrive's free roam): legs to
  // random points ahead, chained into one route without stopping, around where it began
  public let freeRoamRadius: Float = 2500.0;  // m from where free roam began the legs stay within
  public let freeRoamLegMin: Float = 900.0;   // m
  public let freeRoamLegMax: Float = 1600.0;  // m
  public let freeRoamSpread: Float = 60.0;    // deg either side of the heading a leg may turn (widened when nothing fits)
  public let freeRoamMaxDepth: Float = 2.0;   // m below 0 the ground under a leg's end may be at most (open sea reads -13.5 on the map)
  public let freeRoamMaxTurn: Float = 100.0;  // deg either side of the heading a leg may turn at most, unless nothing fits
  public let freeRoamMaxPitch: Float = 45.0;  // deg, steepest climb or descent a leg may have
  public let freeRoamGateMin: Float = 300.0;  // m, nearest gate a leg goes for (PickFreeRoamGate)
  public let gateLead: Float = 60.0;          // m of straight run before and after a gate (gates.py checks 80)
  public let gateStep: Float = 5.0;           // m between the run's points (a line's spacing)
  public let gateMemory: Int32 = 8;           // gates flown lately, not picked again
  public let gateTail: array<Vector4>;        // the run through the gate of the leg being planned
  public let recentGates: array<Int32>;
  public let gateEndS: Float;                 // route distance where the last gate's straight run ends (0: none)
  public let gateEntry: Vector4;              // where the leg being planned meets its gate's run
  public let routeGateTail: array<Vector4>;   // the gate run of the leg in use, for a replan towards it
  public let routeGateEntry: Vector4;
  public let spliceGate: Bool;                // the plan being taken is a gate leg
  public let freeRoamChainAt: Float = 700.0;  // m left on the route when the next leg is planned
  public let freeRoamJoinAhead: Float = 4.0;  // s of the flown speed ahead the next leg starts, at least freeRoamJoinMin
  public let freeRoamJoinMin: Float = 150.0;  // m
  public let freeRoamSmoothSpan: Float = 60.0;  // m either side of a join smoothed out
  // m/s at most: faster, the world cannot stream in new areas in time and the game stops on its
  // loading overlay
  public let freeRoamTopSpeed: Float = 60.0;
  // m a leg's end may differ in height from where it starts (from a tower top to a street the line
  // crested and dived, and the car rolled over pushing into the dive)
  public let freeRoamMaxRise: Float = 100.0;
  // free roam flies lower than the Race and follows the buildings more closely: the route's preferred
  // height over the floor, and the line's lineHeightFree and lineHeightWavelength (Planner.hpp). On
  // three legs: median 23 to 27 m over the floor instead of 40 to 59, at about the same speed.
  public let freeRoamRouteHeight: Float = 15.0;
  public let freeRoamHeightFree: Float = 8.0;
  public let freeRoamHeightWavelength: Float = 300.0;
  // per metre flown over a building (PlanParams.overCost): legs go round buildings rather than over
  // them (on two Dogtown legs 17 to 21% over buildings instead of 37%, the legs 3 to 14 s longer)
  public let freeRoamOverCost: Float = 2.0;

  // state, public for tests (tools/ingame/ltbf_bridge.reds)
  public let status: String = "idle";
  public let route: ref<FlightRoute>;         // the points and their distances from the start
  public let aheadSmooth: Float;              // m, lookahead distance (grows back smoothly)
  public let segment: Int32;
  public let request: Int32;
  public let destination: Vector4;
  public let hasDestination: Bool;
  public let holdPosition: Vector4;
  public let lookahead: Vector4;
  public let targetSpeed: Float;
  public let remaining: Float;
  public let overrideTimer: Float;
  public let replanTimer: Float;
  public let logTimer: Float;
  public let curvatureAhead: Float;           // 1/m, signed (positive = turning right)
  public let stuckTimer: Float;
  public let lastPosition: Vector4;           // last position CheckedPosition passed
  public let positionJumpTimer: Float;        // s a jump in the position has been held off
  public let positionJumpHold: Float = 0.5;   // s a jump is held off before it counts as a real move
  public let offRouteNow: Float;
  public let blockedAhead: Bool;
  public let emergencyBrake: Bool;            // blocked space too close to ease into: brake hard
  public let blockedTimer: Float;
  public let settleTimer: Float;
  public let routeGoal: Vector4;              // where the route ends: the destination, or above the landing spot
  public let landing: Bool;                   // the destination is on the ground: descend and touch down there
  public let landingSpot: Vector4;            // car-centre resting height (the bake's floor)
  public let landingPad: Int32;               // FlightNavMap helipad index landed on, -1: a spot found on the map (set in Activate)
  public let touchdownTimer: Float;
  public let landingStartZ: Float;
  public let landingHeading0: Vector4;
  public let landingHeading: Vector4;
  public let touchdownLow: Float;
  public let speedCmd: Float;                 // speed flown along the route (the trajectory's, or less for blocked space)
  public let speedRate: Float;
  public let accelCmd: Vector4;
  public let yawCmd: Float;
  public let lastRoll: Float;                 // rad, measured bank last frame
  public let rollRate: Float;                 // rad/s, filtered
  public let telemetry: Bool;
  public let telemetryTime: Float;
  public let telemetryFile: Int32;            // the open FlightTelemetry file (WriteTelemetry), 0: none
  public let telemetryClock: Float;           // s since it was opened
  // aerobatics
  public let maneuver: ref<FlightManeuver>;
  public let maneuverCooldown: Float;
  public let stuntTimer: Float;
  public let closestNow: Vector4;             // closest point on the route this frame
  public let overlayVelocity: Vector4;        // a barrel roll's offset rate, added to the desired velocity
  public let headingRef: Vector4;             // horizontal heading flown at hover speeds
  public let attAccel: Vector4;               // commanded acceleration, smoothed, for the attitude
  public let targetRight: Vector4;            // attitude target, world
  public let targetForward: Vector4;
  public let targetUp: Vector4;
  public let omegaRef: Vector4;               // rad/s, world: rate of the attitude target
  public let attitudeError: Float;            // rad
  public let pathCurvature: Vector4;          // 1/m, towards the centre of the route's bend ahead (race feed-forward)
  public let bankAngle: Float;                // rad, eased bank of the attitude target about the nose
  public let requestRace: Bool;               // the pending plan is the planner's line
  public let routeLine: Bool;                 // the route is a line (evenly spaced, smooth)
  public let closestS: Float;                 // route distance of the closest point
  public let onLine: Bool;                    // following a line this frame (the ribbon applies)
  public let ribbonNeed: Vector4;             // m/s2, the line's bend ahead at the speed flown (no deadband)
  public let bankRateNow: Float;
  public let pitchAngle: Float;               // rad, eased nose pitch on a line
  public let pitchRateNow: Float;             // rad/s
  public let commandSurge: Float;             // -1..1, the thrust it commands eased (AutopilotSurge.audioTime), for the engine sound
  public let commandSway: Float;
  public let dronePitch: Float;               // rad, drone style's eased body tilt forward (DroneAttitude)
  public let droneRoll: Float;                // rad, and to the right
  public let dronePitchRate: Float;
  public let droneRollRate: Float;
  public let droneEaseLive: Bool;             // drone style's attitude was flown last frame
  public let droneYaw: Float;                 // rad, drone style's eased heading
  public let droneYawRate: Float;
  public let landSlewOn: Bool;                // a landing turn is under way (LandSlewYaw)
  public let landSlewP: Float;                // 0..1 through it, before easing
  public let landSlewDescent: Float;          // s since the descent began, the turn still under way
  public let landSlewStartYaw: Float;         // rad, the nose when it began
  public let landSlewDelta: Float;            // rad, signed, the whole turn to the landing heading
  public let landSlewHeading: Vector4;        // the heading it ends on, landed on by the descent
  public let thrustCmd: Vector4;             // m/s2 in the car's frame (sway, surge, lift) FlyAerobatic set (force is redone by ApplyPhysics)
  public let freeRoam: Bool;                  // flying free roam legs (no destination tracked)
  // free roam chosen, as vanilla autodrive does (AutoDriveSystem.OnAutodriveToggled): on when engaged
  // with no destination tracked, and kept when one is tracked later (a gig the game tracks on a
  // call); a tap of the autodrive key switches to a tracked destination and back (ToggleFreeRoam)
  public let freeRoamChosen: Bool;
  public let freeRoamCentre: Vector4;
  public let requestJoin: Bool;               // the pending plan is the next free roam leg, from joinPoint
  public let joinS: Float;                    // route distance of joinPoint
  public let joinPoint: Vector4;
  public let ahead: ref<AutopilotPlanAhead>;  // plan ahead: a first leg at once, the rest while flying it
  public let surgeHold: ref<AutopilotSurge>;  // the surge held as a player holds it: drag thrust back, full where the speed is the car's
  private let cinematicCallback: ref<CallbackHandle>;
  private let cinematicContext: Bool;         // UIGameContext.CinematicCamera pushed              // rad/s

  public static func Create(component: ref<FlightComponent>) -> ref<FlightModeAutopilot> {
    let self = new FlightModeAutopilot();
    self.Initialize(component);
    self.ahead = new AutopilotPlanAhead();
    self.surgeHold = new AutopilotSurge();
    self.attitude = new FlightDroneAttitude();
    self.tracker = new FlightTracker();
    self.ClearRoute();
    return self;
  }

  public func GetDescription() -> String = "Autopilot";

  public func RouteSize() -> Int32 {
    return IsDefined(this.route) ? this.route.Size() : 0;
  }

  private func ClearRoute() -> Void {
    let none: array<Vector4>;
    this.route = FlightRoute.Create(none);
  }

  public func Activate() -> Void {
    this.ahead.Reset();
    let target: Vector4;
    this.freeRoamChosen = !this.CurrentDestination(target);
    // as vanilla autodrive: no combat events while it drives (AutoDriveSystem.OnAutodriveToggled)
    if this.component.isPlayerMounted {
      let game = this.component.GetVehicle().GetGame();
      SetFactValue(game, n"disable_combat_events", 1);
      SetFactValue(game, n"player_in_autodrive", 1);
    }
    this.holdPosition = this.component.stats.d_position;
    this.ClearRoute();
    this.request = 0;
    this.hasDestination = false;
    this.freeRoam = false;
    this.requestJoin = false;
    this.status = "idle";
    this.replanTimer = 0.0;
    this.remaining = 0.0;
    this.landing = false;
    this.landingPad = -1;
    this.speedCmd = this.component.stats.d_speed;
    this.speedRate = 0.0;
    this.accelCmd = new Vector4(0.0, 0.0, 0.0, 0.0);
    this.yawCmd = 0.0;
    this.lastPosition = this.component.stats.d_position;
    this.positionJumpTimer = 0.0;
    this.gateEndS = 0.0;
    ArrayClear(this.gateTail);
    this.lastRoll = -AsinF(ClampF(Vector4.Dot(this.component.stats.d_right, FlightUtils.Up()), -1.0, 1.0));
    this.rollRate = 0.0;
    // the take-off is part of the plan: the route starts at the car, and the planner finds the way
    // out from under a deck or between buildings itself (PlanParams.startRadius)
    this.aheadSmooth = 15.0;
    this.maneuver = null;
    this.maneuverCooldown = 0.0;
    this.stuntTimer = 0.0;
    this.overlayVelocity = new Vector4(0.0, 0.0, 0.0, 0.0);
    this.headingRef = this.component.stats.d_forward2D;
    this.attAccel = new Vector4(0.0, 0.0, 0.0, 0.0);
    this.targetRight = this.component.stats.d_right;
    this.targetForward = this.component.stats.d_forward;
    this.targetUp = this.component.stats.d_up;
    this.omegaRef = new Vector4(0.0, 0.0, 0.0, 0.0);
    this.bankAngle = 0.0;
    this.bankRateNow = 0.0;
    this.pathCurvature = new Vector4(0.0, 0.0, 0.0, 0.0);
    if this.component.isPlayerMounted {
      this.PublishAutodriveUI(true);
    }
  }

  // --- takeoff and landing -----------------------------------------------------------------

  // The free gap a car at p is in, or the one just above it (a car resting on the ground sits a
  // little under the bake's floor).
  private static func GapAt(p: Vector4, out gap: Vector2) -> Bool {
    for g in FlightNavMap.GetGaps(p) {
      if p.Z < g.Y {
        gap = g;
        return true;
      }
    }
    return false;
  }

  // A destination on the ground (a GPS pin on a street): the nearest column within reach that is
  // flat, at the same level, and open far enough above to come straight down into. The spot is
  // the bake's floor there, which is where a resting car's position sits.
  private func FindLandingSpot(target: Vector4, out spot: Vector4, out top: Float) -> Bool {
    // first a spot with room to come straight down from the approach height (a spot with little
    // room over it can be under a canopy)
    return this.FindLandingSpotWithRoom(target, this.approachHeight + 4.0, spot, top)
      || this.FindLandingSpotWithRoom(target, 6.0, spot, top);
  }

  private func FindLandingSpotWithRoom(target: Vector4, room: Float, out spot: Vector4, out top: Float) -> Bool {
    let gap: Vector2;
    if !FlightModeAutopilot.GapAt(target, gap) || target.Z - gap.X > 4.0 {
      return false; // a point in the air: hover there
    }
    let level = gap.X;
    let r = 0;
    while r <= this.landingSearch {
      let count = r == 0 ? 1 : 6 * r;
      let best = 1000000.0;
      let i = 0;
      while i < count {
        let angle = 6.2831853 * Cast<Float>(i) / Cast<Float>(count);
        let c = new Vector4(target.X + 2.0 * Cast<Float>(r) * CosF(angle), target.Y + 2.0 * Cast<Float>(r) * SinF(angle), level + 1.0, 1.0);
        let g: Vector2;
        if FlightModeAutopilot.GapAt(c, g) && AbsF(g.X - level) < 3.0 && g.Y - g.X > room && this.IsFlat(c, g.X)
          && this.ShaftClear(c, g.X, room) && !this.SpotTaken(new Vector4(c.X, c.Y, g.X, 1.0))
          && this.SpotSupported(new Vector4(c.X, c.Y, g.X, 1.0)) {
          let d = Vector4.DistanceSquared2D(c, target);
          if d < best {
            best = d;
            spot = new Vector4(c.X, c.Y, g.X, 1.0);
            top = g.Y;
          }
        }
        i += 1;
      }
      if best < 1000000.0 {
        return true;
      }
      r += 1;
    }
    return false;
  }

  // The columns around c out to landingShaft metres are open at least room above floor too, so
  // the car comes down a shaft, not a hole: the bake grows solids by 3 m for the car, and a car
  // longer than that, turning on the way down, catches a walkway over the neighbouring columns.
  private func ShaftClear(c: Vector4, floor: Float, room: Float) -> Bool {
    let ring = 2.0;
    while ring <= this.landingShaft {
      let i = 0;
      while i < 8 {
        let angle = 0.7853982 * Cast<Float>(i);
        let g: Vector2;
        let p = new Vector4(c.X + ring * CosF(angle), c.Y + ring * SinF(angle), floor + 1.0, 1.0);
        if !FlightModeAutopilot.GapAt(p, g) || g.Y < floor + room {
          return false;
        }
        i += 1;
      }
      ring += 2.0;
    }
    return true;
  }

  // A vehicle in the spot (floor height, car centre at rest): vertical rays in the Vehicle
  // collision group (it sees cars, not the ground) from 6 m above down through the spot, at its
  // centre and on rings of 2 and 4 m. Parked cars are not on the nav map, and only exist where the
  // world has streamed in, so the spot is checked again on the approach and on arrival.
  private func SpotTaken(spot: Vector4) -> Bool {
    let queries = GameInstance.GetSpatialQueriesSystem(this.component.GetVehicle().GetGame());
    let ring = 0.0;
    while ring <= 4.0 {
      let count = ring > 0.0 ? 8 : 1;
      let i = 0;
      while i < count {
        let angle = 0.7853982 * Cast<Float>(i);
        let x = spot.X + ring * CosF(angle);
        let y = spot.Y + ring * SinF(angle);
        let hit: TraceResult;
        if queries.SyncRaycastByCollisionGroup(new Vector4(x, y, spot.Z + 6.0, 1.0), new Vector4(x, y, spot.Z - 2.0, 1.0), n"Vehicle", hit, false, false) {
          return true;
        }
        i += 1;
      }
      ring += 2.0;
    }
    return false;
  }

  // the columns out to landingFlat around c (rings 2 m apart, eight directions) have their floor
  // within half a metre of this one: room for the whole car, not just its centre (on a roof 2 m in
  // from the map's edge the car's centre hung over the real one and it fell off)
  private func IsFlat(c: Vector4, floor: Float) -> Bool {
    let ring = 2.0;
    while ring <= this.landingFlat + 0.01 {
      let i = 0;
      while i < 8 {
        let angle = 0.7853982 * Cast<Float>(i);
        let g: Vector2;
        let p = new Vector4(c.X + ring * CosF(angle), c.Y + ring * SinF(angle), floor + 1.0, 1.0);
        if !FlightModeAutopilot.GapAt(p, g) || AbsF(g.X - floor) > 0.5 {
          return false;
        }
        i += 1;
      }
      ring = ring + 2.0 > this.landingFlat && ring < this.landingFlat ? this.landingFlat : ring + 2.0;
    }
    return true;
  }

  // The game's own collision under the car at the spot, where it has streamed in: rays down at the
  // centre and landingFootprint out in eight directions. All hitting at about one height: room to
  // land. Some missing (an edge) or at another height (a ledge): not. None hitting: the world there
  // has not streamed in yet, nothing to say (the nav map's IsFlat stands; it is checked again on
  // the approach, RecheckLandingSpot).
  private func SpotSupported(spot: Vector4) -> Bool {
    let filter: QueryFilter;
    QueryFilter.AddGroup(filter, n"Static");
    QueryFilter.AddGroup(filter, n"Terrain");
    QueryFilter.AddGroup(filter, n"VehicleBlocker");
    let queries = GameInstance.GetSpatialQueriesSystem(this.component.GetVehicle().GetGame());
    let hits = 0;
    let lo = 1000000.0;
    let hi = -1000000.0;
    let i = -1;
    while i < 8 {
      let angle = 0.7853982 * Cast<Float>(i);
      let r = i < 0 ? 0.0 : this.landingFootprint;
      let x = spot.X + r * CosF(angle);
      let y = spot.Y + r * SinF(angle);
      let result: TraceResult;
      queries.SyncRaycastByQueryFilter(new Vector4(x, y, spot.Z + 2.0, 1.0), new Vector4(x, y, spot.Z - 6.0, 1.0), filter, result, false, false);
      if TraceResult.IsValid(result) {
        hits += 1;
        lo = MinF(lo, result.position.Z);
        hi = MaxF(hi, result.position.Z);
      }
      i += 1;
    }
    return hits == 0 || (hits == 9 && hi - lo < 0.6);
  }

  // A landing pad (FlightNavMap helipads, docs/archive/helipads.md) to land on for this destination: the
  // one it is on (within padSnapDistance and at the pad's level), else with padRange set and the
  // destination on the ground, the nearest within padRange. -1: none.
  private func FindLandingPad(target: Vector4) -> Int32 {
    let pad = FlightNavMap.FindNearestHelipad(target, this.padSnapDistance);
    if pad >= 0 {
      let deck = FlightNavMap.GetHelipadPosition(pad);
      if AbsF(target.Z - deck.Z) < this.padSnapLevel {
        return pad;
      }
    }
    let gap: Vector2;
    if this.padRange > 0.0 && FlightModeAutopilot.GapAt(target, gap) && target.Z - gap.X <= 4.0 {
      return FlightNavMap.FindNearestHelipad(target, this.padRange);
    }
    return -1;
  }

  // A pad's heading to land on: its yaw (the prefab's forward, +Y turned by yaw), of its two ways
  // the one nearer the arrival heading.
  private static func PadHeading(pad: Int32, arrival: Vector4) -> Vector4 {
    let yaw = Deg2Rad(FlightNavMap.GetHelipadYaw(pad));
    let d = new Vector4(-SinF(yaw), CosF(yaw), 0.0, 0.0);
    return Vector4.Dot(d, arrival) >= 0.0 ? d : d * -1.0;
  }

  // Where the route should end for this destination: above the landing spot, or the point itself.
  private func ChooseRouteGoal() -> Void {
    let top: Float;
    this.landing = false;
    this.landingPad = -1;
    if this.landAtDestination {
      this.landingPad = this.FindLandingPad(this.destination);
      if this.landingPad >= 0 {
        // the pad's deck is checked against the map when the pads are found; the spot is where a
        // resting car's centre sits, as for a spot found on the map
        let deck = FlightNavMap.GetHelipadPosition(this.landingPad);
        this.landingSpot = new Vector4(deck.X, deck.Y, deck.Z + FlightNavMap.GetCarHalfHeight(), 1.0);
        top = deck.Z + FlightNavMap.GetHelipadClearance(this.landingPad);
        this.landing = true;
      } else {
        this.landing = this.FindLandingSpot(this.destination, this.landingSpot, top);
      }
    }
    if this.landing {
      this.routeGoal = new Vector4(this.landingSpot.X, this.landingSpot.Y, MinF(this.landingSpot.Z + this.approachHeight, top - 2.0), 1.0);
      FlightLog.Info("[Autopilot] landing " + (this.landingPad >= 0 ? "pad " + FlightNavMap.GetHelipadId(this.landingPad) : "spot") + " at "
        + FloatToStringPrec(this.landingSpot.X, 1) + ", " + FloatToStringPrec(this.landingSpot.Y, 1) + ", " + FloatToStringPrec(this.landingSpot.Z, 1)
        + " (" + FloatToStringPrec(Vector4.Distance2D(this.landingSpot, this.destination), 1) + " m from the destination)");
    } else {
      this.routeGoal = this.destination;
    }
  }

  public func Deactivate() -> Void {
    if this.component.isPlayerMounted {
      let game = this.component.GetVehicle().GetGame();
      SetFactValue(game, n"disable_combat_events", 0);
      SetFactValue(game, n"player_in_autodrive", 0);
    }
    this.ClearRoute();
    this.request = 0;
    this.freeRoam = false;
    this.requestJoin = false;
    this.status = "idle";
    if this.component.isPlayerMounted {
      this.PublishAutodriveUI(false);
    }
  }

  // --- destination and route ---------------------------------------------------------------

  // The player is flying and the Autopilot mode is the one flying the car.
  public static func IsEngaged() -> Bool {
    let ctlr = FlightController.GetInstance();
    let pc = FlightSystem.GetInstance().playerComponent;
    return IsDefined(ctlr) && ctlr.IsActive() && IsDefined(pc) && IsDefined(pc.GetFlightMode() as FlightModeAutopilot);
  }

  // Engaged and flying free roam (no destination tracked).
  public static func IsFreeRoaming() -> Bool {
    if !FlightModeAutopilot.IsEngaged() {
      return false;
    }
    let mode = FlightSystem.GetInstance().playerComponent.GetFlightMode() as FlightModeAutopilot;
    return mode.freeRoam;
  }

  // Vanilla autodrive's toast (AutoDriveSystem.SendNotification): same style, its own strings.
  public static func Notify(game: GameInstance, locKey: String) -> Void {
    let msg: SimpleScreenMessage;
    msg.isShown = true;
    msg.duration = 1.5;
    msg.message = locKey;
    msg.type = SimpleMessageType.Autodrive;
    GameInstance.GetBlackboardSystem(game).Get(GetAllBlackboardDefs().UI_Notifications).SetVariant(GetAllBlackboardDefs().UI_Notifications.WarningMessage, ToVariant(msg), true);
  }

  // Drive vanilla's autodrive HUD panel (autodrive.script reads UI_AutodriveData): shown while
  // engaged, with the route's remaining length and time. Native ground autodrive stays off.
  private func PublishAutodriveUI(enabled: Bool) -> Void {
    let game = this.component.GetVehicle().GetGame();
    let bb = GameInstance.GetBlackboardSystem(game).Get(GetAllBlackboardDefs().UI_AutodriveData);
    bb.SetBool(GetAllBlackboardDefs().UI_AutodriveData.FreeRoamEnabled, false);
    bb.SetBool(GetAllBlackboardDefs().UI_AutodriveData.AutoDriveEnabled, enabled, true);
    if enabled {
      if !IsDefined(this.cinematicCallback) {
        this.cinematicCallback = bb.RegisterListenerBool(GetAllBlackboardDefs().UI_AutodriveData.CinematicCameraActive, this, n"OnCinematicCameraChanged");
      }
      this.OnCinematicCameraChanged(bb.GetBool(GetAllBlackboardDefs().UI_AutodriveData.CinematicCameraActive));
      return;
    }
    if IsDefined(this.cinematicCallback) {
      bb.UnregisterListenerBool(GetAllBlackboardDefs().UI_AutodriveData.CinematicCameraActive, this.cinematicCallback);
      this.cinematicCallback = null;
      FlightAudio.Get().SetCinematic(false);
    }
    this.OnCinematicCameraChanged(false);
    if bb.GetBool(GetAllBlackboardDefs().UI_AutodriveData.CinematicCameraActive) {
      // leave the cinematic camera with the autopilot, as vanilla does when autodrive ends
      GetPlayer(game).QueueEvent(new vehicleCinematicCameraToggleEvent());
    }
  }

  // Vanilla autodrive's cinematic camera hides the whole HUD (minimap, quest, hints, the panel):
  // the engine pushes UIGameContext.CinematicCamera for its own autodrive only, so with the
  // Autopilot flying the HUD stayed up. Pushed here while the cinematic camera is on.
  protected cb func OnCinematicCameraChanged(active: Bool) -> Bool {
    if Equals(active, this.cinematicContext) {
      return true;
    }
    this.cinematicContext = active;
    FlightAudio.Get().SetCinematic(active);
    let ui = GameInstance.GetUISystem(this.component.GetVehicle().GetGame());
    if active {
      ui.PushGameContext(UIGameContext.CinematicCamera);
    } else {
      ui.PopGameContext(UIGameContext.CinematicCamera);
    }
    return true;
  }

  // vanilla's panel in its free roam state (no counters)
  private func PublishFreeRoamUI() -> Void {
    if !this.component.isPlayerMounted {
      return;
    }
    let bb = GameInstance.GetBlackboardSystem(this.component.GetVehicle().GetGame()).Get(GetAllBlackboardDefs().UI_AutodriveData);
    bb.SetBool(GetAllBlackboardDefs().UI_AutodriveData.FreeRoamEnabled, this.freeRoam, true);
  }

  private func PublishProgress() -> Void {
    if this.freeRoam {
      return;
    }
    let bb = GameInstance.GetBlackboardSystem(this.component.GetVehicle().GetGame()).Get(GetAllBlackboardDefs().UI_AutodriveData);
    let speed = MaxF(MaxF(this.component.stats.d_speed, this.targetSpeed), 10.0);
    bb.SetFloat(GetAllBlackboardDefs().UI_AutodriveData.DestinationRemainingLength, this.remaining);
    bb.SetFloat(GetAllBlackboardDefs().UI_AutodriveData.DestinationRemainingTime, this.remaining / speed);
  }

  // The GPS destination vanilla autodrive would use, or FlightNavMap's target override.
  public static func GetDestination(game: GameInstance, out target: Vector4) -> Bool {
    if FlightNavMap.HasTargetOverride() {
      target = FlightNavMap.GetTargetOverride();
      return true;
    }
    // a landing pad's pin tracked on the map: its deck (vanilla's destination is snapped to a road)
    let helipads = FlightHelipadSystem.GetInstance(game);
    let pad = IsDefined(helipads) ? helipads.GetTrackedPad() : -1;
    if pad >= 0 {
      target = FlightNavMap.GetHelipadPosition(pad);
      return true;
    }
    let autodrive = GameInstance.GetScriptableSystemsContainer(game).Get(n"AutoDriveSystem") as AutoDriveSystem;
    if IsDefined(autodrive) && NotEquals(autodrive.GetAutodriveDestinationType(), gameAutodriveDestinationType.None) {
      target = Vector4.Vector3To4(autodrive.GetAutodriveDestination());
      return true;
    }
    return false;
  }

  private func CurrentDestination(out target: Vector4) -> Bool {
    return FlightModeAutopilot.GetDestination(this.component.GetVehicle().GetGame(), target);
  }

  private func RequestRoute(from: Vector4) -> Void {
    this.RefreshVehicle();
    if !FlightNavMap.IsLoaded() {
      this.status = "no nav map";
      return;
    }
    // Race: the planner spreads the route into a line of wide bends
    this.requestRace = true;
    this.requestJoin = false;
    // a replan towards a gate still ahead (off the route on the way there): through it again, or the
    // leg ends at the gate's entry and the next one is joined before it
    if ArraySize(this.gateTail) == 0 && ArraySize(this.routeGateTail) > 0 && this.gateEndS > this.closestS
      && Vector4.Distance(this.routeGoal, this.routeGateEntry) < 1.0 {
      this.gateTail = this.routeGateTail;
    }
    // from rest: a first leg at once, the rest planned while the car flies it (AutopilotPlanAhead)
    if this.freeRoam {
      this.request = FlightNavMap.PlanFreeRoamAsync(from, this.routeGoal, this.freeRoamRouteHeight, this.raceClimbCost, this.raceMinPassage,
        this.raceSearchWeight, this.raceTurnCost, this.Vehicle(), this.hits, this.component.stats.d_velocity, this.freeRoamHeightFree,
        this.freeRoamHeightWavelength, this.freeRoamOverCost);
    } else if !this.ahead.Request(this, from, this.component.stats.d_velocity) {
      this.request = FlightNavMap.PlanRaceAsync(from, this.routeGoal, this.raceRouteHeight, this.raceClimbCost, this.raceMinPassage,
        this.raceSearchWeight, this.raceTurnCost, this.Vehicle(), true, this.hits, this.component.stats.d_velocity);
    }
    this.replanTimer = 3.0;
    if this.RouteSize() < 2 {
      // nothing to follow meanwhile: hold here (a replan keeps following the old route)
      this.status = "planning";
      this.holdPosition = from;
    }
  }

  private func UpdateRoute(timeDelta: Float) -> Void {
    this.replanTimer -= timeDelta;
    // the rest of a trip begun on a first leg: on from its join, or (late) the trip again from here
    let rest: array<Vector4>;
    if this.ahead.TakeRest(this, rest) {
      if this.ahead.firstLeg {
        this.ahead.Reset();
        this.RequestRoute(this.component.stats.d_position);
      } else {
        this.SpliceRoute(rest);
      }
    }
    let target: Vector4;
    let has = this.CurrentDestination(target);
    if !has && !this.freeRoam && (Equals(this.status, "descending") || Equals(this.status, "landed") || Equals(this.status, "arrived")) {
      // the game untracks a map pin once the player reaches it, mid-descent: finish the landing
      // (or the hand-back), not free roam from there
      return;
    }
    if this.freeRoamChosen {
      // free roam until the player switches (ToggleFreeRoam), whatever gets tracked meanwhile
      if !this.freeRoam {
        this.BeginFreeRoam();
      }
      this.ChainFreeRoam();
    } else if has {
      if !this.hasDestination || Vector4.Distance(target, this.destination) > 20.0 {
        this.destination = target;
        this.hasDestination = true;
        this.ClearRoute();
        this.ChooseRouteGoal();
        this.RequestRoute(this.component.stats.d_position);
      }
    } else {
      // the destination untracked on the way: as vanilla autodrive
      // (AutoDriveSystem.OnUpdateAutodriveOnDestinationChangeRequest), the Autopilot hands back
      FlightLog.Info("[Autopilot] destination untracked on the way: handing back");
      FlightController.GetInstance().ExitAutopilot();
      return;
    }
    if this.request > 0 && FlightNavMap.IsPlanReady(this.request) {
      let points = FlightNavMap.TakePlan(this.request);
      this.request = 0;
      if this.requestRejoin {
        this.requestRejoin = false;
        FlightLog.Info("[Autopilot] " + FlightNavMap.GetLastPlanInfo());
        if ArraySize(points) < 2 {
          this.RequestRoute(this.component.stats.d_position);
          return;
        }
      }
      // a free roam leg to a gate: planned to just before it, then straight through
      let gateLeg = ArraySize(points) >= 2 && ArraySize(this.gateTail) > 0;
      if gateLeg {
        for q in this.gateTail {
          ArrayPush(points, q);
        }
        this.routeGateTail = this.gateTail;
        this.routeGateEntry = this.gateEntry;
      }
      ArrayClear(this.gateTail);
      this.spliceGate = gateLeg;
      // free roam: no leg steeper than freeRoamMaxPitch anywhere. A leg's ends are fixed, so the
      // planner's 45 degree limit does not reach them (legs into gates among tall buildings came
      // over the roofs and dropped to the gate). Another leg instead.
      // Chained legs only: a first leg climbs out from the street, a replan starts wherever the car is.
      if this.requestJoin && ArraySize(points) >= 2 && FlightModeAutopilot.SteepestPitch(points, 10.0) > this.freeRoamMaxPitch {
        FlightLog.Info("[Autopilot] free roam: leg steeper than " + FloatToStringPrec(this.freeRoamMaxPitch, 0) + " degrees ("
          + FloatToStringPrec(FlightModeAutopilot.SteepestPitch(points, 10.0), 0) + "), another one");
        this.requestJoin = false;
        this.replanTimer = 0.2;
        return;
      }
      if this.requestJoin {
        this.requestJoin = false;
        // the next free roam leg, if the car has not passed where it starts
        if ArraySize(points) >= 2 && this.joinS > this.closestS + 20.0 {
          this.SpliceRoute(points);
        } else {
          this.replanTimer = 0.5; // ChainFreeRoam tries another
          FlightLog.Warn("[Autopilot] free roam: next leg " + (ArraySize(points) >= 2 ? "came too late" : "found no route"));
        }
        FlightLog.Info("[Autopilot] " + FlightNavMap.GetLastPlanInfo());
        return;
      }
      if ArraySize(points) >= 2 {
        // the planner's line has its points lineSpacing (5 m) apart; a two-point hop has corners
        // to round
        this.routeLine = this.requestRace && ArraySize(points) > 2
          && Vector4.Distance(points[0], points[ArraySize(points) - 1]) / Cast<Float>(ArraySize(points) - 1) < 10.0;
        // a line is already smooth: no corners to round
        this.route = FlightRoute.Create(this.routeLine ? points : this.RoundCorners(points));
        let total = this.route.Length();
        this.segment = 0;
        this.closestS = 0.0;
        this.landSlewOn = false;
        this.gateEndS = gateLeg ? total : 0.0;
        this.status = "following";
        this.BuildTrajectory();
        this.ahead.Taken(this);
      } else {
        this.status = "no route";
        this.holdPosition = this.component.stats.d_position;
      }
      FlightLog.Info("[Autopilot] " + FlightNavMap.GetLastPlanInfo());
    }
  }

  // --- free roam ------------------------------------------------------------------------------

  // A tap of the autodrive key while engaged (FlightController), as vanilla's ToggleFreeRoamRequest:
  // free roam to the tracked destination, or the destination to free roam. In free roam with
  // nothing tracked it stays.
  public func ToggleFreeRoam() -> Void {
    let target: Vector4;
    let has = this.CurrentDestination(target);
    if this.freeRoamChosen {
      if !has {
        return;
      }
      this.freeRoamChosen = false;
      this.freeRoam = false;
      this.hasDestination = false;
      this.request = 0;
      this.requestJoin = false;
      this.ahead.Reset();
      this.PublishFreeRoamUI();
      FlightLog.Info("[Autopilot] free roam off: to the tracked destination");
    } else {
      this.freeRoamChosen = true;
      this.hasDestination = false;
      this.landing = false;
      this.request = 0;
      this.ahead.Reset();
      FlightLog.Info("[Autopilot] free roam on");
    }
  }

  private func BeginFreeRoam() -> Void {
    let stats = this.component.stats;
    this.freeRoam = true;
    this.hasDestination = false;
    this.landing = false;
    this.landingPad = -1;
    this.request = 0;
    this.requestJoin = false;
    this.freeRoamCentre = stats.d_position;
    this.ClearRoute();
    this.holdPosition = stats.d_position;
    this.replanTimer = 0.0;
    this.status = "free roam";
    this.PublishFreeRoamUI();
    FlightLog.Info("[Autopilot] free roam around " + FloatToStringPrec(stats.d_position.X, 1) + ", " + FloatToStringPrec(stats.d_position.Y, 1));
  }

  // A point for the next leg to end at: freeRoamLegMin to freeRoamLegMax away from `from`, within
  // freeRoamSpread of `heading` (wider for each candidate that does not fit, at the edge of the
  // area or of the map, up to maxTurn degrees), inside freeRoamRadius of where free roam began,
  // freeRoamRouteHeight above the ground under open sky there.
  private func PickFreeRoamGoal(from: Vector4, heading: Vector4, maxTurn: Float, out goal: Vector4) -> Bool {
    let base = AtanF(heading.Y, heading.X);
    let tries = 0;
    while tries < 24 {
      let spread = MinF(Deg2Rad(this.freeRoamSpread) * (1.0 + Cast<Float>(tries) / 6.0), Deg2Rad(maxTurn));
      let angle = base + RandRangeF(-1.0, 1.0) * spread;
      let distance = RandRangeF(this.freeRoamLegMin, this.freeRoamLegMax);
      let c = new Vector4(from.X + distance * CosF(angle), from.Y + distance * SinF(angle), from.Z, 1.0);
      if Vector4.Distance2D(c, this.freeRoamCentre) <= this.freeRoamRadius {
        let gaps = FlightNavMap.GetGaps(c);
        let n = ArraySize(gaps);
        if n > 0 {
          let sky = gaps[n - 1];
          let z = MinF(sky.X + this.freeRoamRouteHeight, sky.Y - 5.0);
          // near the height of the leg before, until nothing like that fits; over land: past the
          // playable area's edge offshore the game puts the car back where flight was switched on
          if z > sky.X + 5.0 && sky.X >= -this.freeRoamMaxDepth && (AbsF(z - from.Z) <= this.freeRoamMaxRise || tries >= 16) {
            goal = new Vector4(c.X, c.Y, z, 1.0);
            return true;
          }
        }
      }
      tries += 1;
    }
    return false;
  }

  // A gate for the next leg (gates.tsv: openings between buildings, mined from the nav map's
  // doorways by tools/navmap_bake/gates.py): freeRoamGateMin to freeRoamLegMax away, within
  // freeRoamSpread of `heading`, inside freeRoamRadius, not one of the last few flown. `entry` is
  // gateLead before it, where the planned route ends; gateTail is the straight run from there
  // through the gate to gateLead past it, gateStep apart, for the route to carry on with.
  private func PickFreeRoamGate(from: Vector4, heading: Vector4, out entry: Vector4) -> Bool {
    let candidates: array<Int32>;
    for i in FlightNavMap.FindGatesInRange(from, this.freeRoamLegMax) {
      let gate = FlightNavMap.GetGatePosition(i);
      let toGate = gate - from;
      if !ArrayContains(this.recentGates, i) && Vector4.Length2D(toGate) >= this.freeRoamGateMin
        && Vector4.Distance2D(gate, this.freeRoamCentre) <= this.freeRoamRadius
        && Vector4.Dot(FlightModeAutopilot.Flat(toGate), FlightModeAutopilot.Flat(heading)) >= CosF(Deg2Rad(this.freeRoamSpread)) {
        ArrayPush(candidates, i);
      }
    }
    while ArraySize(candidates) > 0 {
      let k = RandRange(0, ArraySize(candidates));
      let i = candidates[k];
      ArrayErase(candidates, k);
      let gate = FlightNavMap.GetGatePosition(i);
      let yaw = Deg2Rad(FlightNavMap.GetGateYaw(i));
      let way = new Vector4(CosF(yaw), SinF(yaw), 0.0, 0.0);
      if Vector4.Dot(way, gate - from) < 0.0 {
        way = -way;
      }
      let start = gate - way * this.gateLead;
      let end = gate + way * this.gateLead;
      if FlightNavMap.SegmentClear(start, end) >= 1.0 {
        entry = start;
        this.gateEntry = start;
        ArrayClear(this.gateTail);
        let d = this.gateStep;
        while d <= 2.0 * this.gateLead + 0.01 {
          ArrayPush(this.gateTail, start + way * d);
          d += this.gateStep;
        }
        ArrayPush(this.recentGates, i);
        if ArraySize(this.recentGates) > this.gateMemory {
          ArrayErase(this.recentGates, 0);
        }
        FlightLog.Info("[Autopilot] free roam: through gate " + ToString(i) + " at " + FloatToStringPrec(gate.X, 1) + ", "
          + FloatToStringPrec(gate.Y, 1) + ", " + FloatToStringPrec(gate.Z, 1));
        return true;
      }
    }
    return false;
  }

  // Plans a leg from `from` towards a new goal; join: it continues the route from route point
  // joinIndex (spliced on by SpliceRoute once planned).
  private func NextFreeRoamLeg(from: Vector4, heading: Vector4, join: Bool) -> Bool {
    let goal: Vector4;
    ArrayClear(this.gateTail);
    // a gate ahead, else a point ahead; at the edge of the area, the area moved to here and the
    // same again; turning further only when nothing else fits (a coast with the sea ahead): a goal
    // behind the car turned it round on the spot
    let found = this.PickFreeRoamGate(from, heading, goal) || this.PickFreeRoamGoal(from, heading, this.freeRoamMaxTurn, goal);
    if !found {
      this.freeRoamCentre = from;
      FlightLog.Info("[Autopilot] free roam: nothing ahead, area moved to " + FloatToStringPrec(from.X, 1) + ", " + FloatToStringPrec(from.Y, 1));
      found = this.PickFreeRoamGate(from, heading, goal) || this.PickFreeRoamGoal(from, heading, this.freeRoamMaxTurn, goal)
        || this.PickFreeRoamGoal(from, heading, 180.0, goal);
    }
    if !found {
      this.replanTimer = 1.0;
      FlightLog.Warn("[Autopilot] free roam: no point found for the next leg");
      return false;
    }
    this.destination = ArraySize(this.gateTail) > 0 ? this.gateTail[ArraySize(this.gateTail) - 1] : goal;
    this.routeGoal = goal;
    this.RequestRoute(from);
    this.requestJoin = join;
    FlightLog.Info("[Autopilot] free roam: next leg to " + FloatToStringPrec(goal.X, 1) + ", " + FloatToStringPrec(goal.Y, 1) + ", " + FloatToStringPrec(goal.Z, 1)
      + " (" + FloatToStringPrec(Vector4.Distance2D(from, goal), 0) + " m)");
    return true;
  }

  // With no route, a first leg from here; on a route that is running out, the next leg from a
  // point ahead on it, so the car flies on into it without slowing.
  private func ChainFreeRoam() -> Void {
    if this.request > 0 || this.replanTimer > 0.0 {
      return;
    }
    let stats = this.component.stats;
    let n = this.RouteSize();
    if n < 2 {
      let heading = Vector4.Length2D(stats.d_velocity) > 3.0 ? FlightModeAutopilot.Flat(stats.d_velocity) : FlightModeAutopilot.Flat(stats.d_forward);
      this.NextFreeRoamLeg(stats.d_position, heading, false);
      return;
    }
    if !Equals(this.status, "following") || this.route.Length() - this.closestS > this.freeRoamChainAt {
      return;
    }
    // the point at that distance, not the next route point after it (a plain route's legs can be
    // a kilometre long)
    // and not before the end of a gate's straight run still ahead: the next leg would cut it off
    let s = MinF(MaxF(this.closestS + MaxF(this.freeRoamJoinMin, stats.d_speed * this.freeRoamJoinAhead), this.gateEndS), this.route.Length());
    let point = this.PointAt(s);
    let back = point - this.PointAt(s - 10.0);
    let heading = Vector4.Length2D(back) > 0.1 ? FlightModeAutopilot.Flat(back) : FlightModeAutopilot.Flat(stats.d_forward);
    if this.NextFreeRoamLeg(point, heading, true) {
      this.joinS = s;
      this.joinPoint = point;
    }
  }

  // The route kept from just behind the car up to joinPoint, then the new leg (which starts
  // there), the bend where they meet smoothed out over freeRoamSmoothSpan.
  private func SpliceRoute(points: array<Vector4>) -> Void {
    let line = this.requestRace && ArraySize(points) > 2
      && Vector4.Distance(points[0], points[ArraySize(points) - 1]) / Cast<Float>(ArraySize(points) - 1) < 10.0;
    let tail = line ? points : this.RoundCorners(points);
    let first = Max(this.segment - 2, 0);
    let trimmed = this.route.Distance(first);
    let route: array<Vector4>;
    let i = first;
    while i < this.RouteSize() && this.route.Distance(i) < this.joinS - 0.5 {
      ArrayPush(route, this.route.Point(i));
      i += 1;
    }
    ArrayPush(route, this.joinPoint);
    let join = ArraySize(route) - 1;
    i = 1;
    while i < ArraySize(tail) {
      ArrayPush(route, tail[i]);
      i += 1;
    }
    // the legs meet at an angle: ease the points round the join towards their neighbours where
    // the map allows (a point only moves if both its segments stay clear), leaving the stretch
    // the car is on alone. The line's points are about 5 m apart.
    let span = Cast<Int32>(this.freeRoamSmoothSpan / 5.0);
    let lo = Max(join - span, this.segment - first + 2);
    if this.gateEndS >= this.joinS - 1.0 {
      lo = Max(lo, join); // joined at the end of a gate's run: the run stays straight
    }
    let hi = Min(join + span, ArraySize(route) - 2);
    this.route = FlightRoute.Create(FlightRoute.SmoothJoin(route, join, lo, hi, this.freeRoamSmoothSpan, 20));
    this.routeLine = this.routeLine && line;
    this.segment -= first;
    this.closestS -= trimmed;
    let total = this.route.Length();
    this.gateEndS = this.spliceGate ? total : MaxF(this.gateEndS - trimmed, 0.0);
    this.BuildTrajectory();
    FlightLog.Info("[Autopilot] next leg joined, " + FloatToStringPrec(total - this.closestS, 0) + " m ahead");
  }

  // The planner's route is straight legs meeting at corners. Replace each corner with a short
  // quadratic Bezier arc (cut back up to cornerRound metres, and at most 30% of each leg) so the
  // car starts turning before the corner instead of at it. An arc is kept only where it is free on
  // the nav map; otherwise the corner stays as it was. The cut steps down (full, half, quarter)
  // until the arc is clear.
  private func RoundCorners(points: array<Vector4>) -> array<Vector4> {
    // full cut, then smaller; below raceRoundMin the corner stays sharp: tiny arcs made for very
    // slow corners (8 m/s), where the lookahead cutting a sharp corner flew it faster and fine
    return FlightRoute.RoundCorners(points, this.raceCornerRound, this.raceRoundShare, this.raceRoundMin, this.raceRoundStep);
  }

  // Horizontal unit heading a fraction f of the way from a to b, turning the short way round.
  private static func BlendHeading(a: Vector4, b: Vector4, f: Float) -> Vector4 {
    let a0 = AtanF(a.Y, a.X);
    let delta = AtanF(b.Y, b.X) - a0;
    while delta > 3.1415927 {
      delta -= 6.2831853;
    }
    while delta < -3.1415927 {
      delta += 6.2831853;
    }
    let angle = a0 + delta * f;
    return new Vector4(CosF(angle), SinF(angle), 0.0, 0.0);
  }

  // Heading to land on: along the most open line through the spot (a street, a plaza's long
  // side), of its two ways the one nearer the arrival heading; the arrival heading itself when
  // nothing is open.
  private func LandingHeading(spot: Vector4, arrival: Vector4) -> Vector4 {
    let up = new Vector4(spot.X, spot.Y, spot.Z + 1.0, 1.0);
    let arrive = Vector4.Normalize(new Vector4(arrival.X, arrival.Y, 0.0, 0.0));
    let best = arrive;
    let bestScore = 0.5;
    let i = 0;
    while i < 8 {
      let angle = 3.1415927 * Cast<Float>(i) / 8.0;
      let d = new Vector4(CosF(angle), SinF(angle), 0.0, 0.0);
      let open = FlightNavMap.SegmentClear(up, up + d * 25.0) + FlightNavMap.SegmentClear(up, up - d * 25.0);
      let score = open + 0.15 * AbsF(Vector4.Dot(d, arrive));
      if score > bestScore {
        bestScore = score;
        best = Vector4.Dot(d, arrive) >= 0.0 ? d : d * -1.0;
      }
      i += 1;
    }
    return best;
  }

  // The horizontal unit vector of v (zero if v is vertical). Vector4.Normalize2D keeps v's Z: a dot
  // product of two of those adds the Zs' product.
  public static func Flat(v: Vector4) -> Vector4 {
    let length = SqrtF(v.X * v.X + v.Y * v.Y);
    return length > 0.0001 ? new Vector4(v.X / length, v.Y / length, 0.0, 0.0) : new Vector4(0.0, 0.0, 0.0, 0.0);
  }

  // Route distance from the car to the next bend worth slowing the lookahead for: a point where
  // the route's direction cornerSpan before and after it differs by more than 25 degrees (a
  // rounded corner's arc points each turn only a little, so they are judged as one bend).
  private func DistanceToSharpBend(closest: Vector4) -> Float {
    return this.route.SharpBend(this.segment, closest, this.cornerSpan);
  }

  // The predicted path p + v t + accelCmd t^2 / 2 for t up to collisionHorizon is blocked on the
  // nav map; travelled is the length flown before the first blocked sample, hit that sample.
  private func PredictBlocked(from: Vector4, v: Vector4, out travelled: Float, out hit: Vector4) -> Bool {
    let prev = from;
    travelled = 0.0;
    let k = 1;
    while k <= 3 {
      let t = this.collisionHorizon * Cast<Float>(k) / 3.0;
      let q = from + v * t + this.accelCmd * (0.5 * t * t);
      let length = Vector4.Distance(prev, q);
      let clear = FlightNavMap.SegmentClear(prev, q);
      if clear < 1.0 {
        travelled += clear * length;
        hit = prev + (q - prev) * clear;
        return true;
      }
      travelled += length;
      prev = q;
      k += 1;
    }
    return false;
  }

  // Distance from q to the route, over the stretch from the current segment to collisionHorizon
  // of travel plus 50 m past it (by distance: rounded corners make many short segments, and a
  // count of segments fell short, so a point on the route ahead read as off it).
  private func DistanceToRoute(q: Vector4) -> Float {
    return this.route.DistanceTo(q, this.segment, this.component.stats.d_speed * this.collisionHorizon + 50.0);
  }

  // Closest point on the route from the current segment on; advances the segment index.
  private func ClosestOnRoute(p: Vector4, out closest: Vector4, out distance: Float) -> Void {
    this.segment = this.route.Nearest(p, this.segment, 12);
    closest = this.route.Project(p, this.segment);
    distance = Vector4.Distance(p, closest);
  }

  // Point `along` metres further along the route from `from` (which lies on segment this.segment).
  private func AlongRoute(from: Vector4, along: Float) -> Vector4 {
    return this.route.Ahead(this.segment, from, along);
  }

  // The route from `from` (on segment this.segment) is free on the nav map for `along` metres.
  private func RouteClear(from: Vector4, along: Float) -> Bool {
    return this.route.ClearAhead(this.segment, from, along);
  }

  // Point at route distance s from the start (clamped to the ends).
  private func PointAt(s: Float) -> Vector4 {
    return this.route.At(s);
  }

  // Signed horizontal curvature of the route around the lookahead: the heading change between
  // the stretch just ahead and the stretch beyond the lookahead point, over the distance between.
  private func CurvatureAhead(closest: Vector4, ahead: Float) -> Float {
    return this.route.Turn(this.segment, closest, ahead);
  }

  // A line's direction at route distance s and its bend there (towards the centre, 1 / radius
  // long), both taken over lineSpan either side.
  private func LineFrame(s: Float, out tangent: Vector4, out bend: Vector4) -> Void {
    this.LineFrameOver(s, this.lineSpan, tangent, bend);
  }

  private func LineFrameOver(s: Float, span: Float, out tangent: Vector4, out bend: Vector4) -> Void {
    tangent = this.route.Tangent(s, span);
    if Vector4.Length(tangent) < 0.5 {
      tangent = this.component.stats.d_forward;
    }
    bend = this.route.Bend(s, span);
  }

  // Curvature of the route over the first part of the lookahead, 3D: towards the bend's centre,
  // 1 / radius long.
  private func CurvatureVector(closest: Vector4, ahead: Float) -> Vector4 {
    return this.route.Curvature(this.segment, closest, ahead);
  }

  // --- control -----------------------------------------------------------------------------

  private func PlayerInput() -> Bool {
    return AbsF(this.component.surge) > 0.15 || AbsF(this.component.yaw) > 0.15 || AbsF(this.component.lift) > 0.15
      || AbsF(this.component.sway) > 0.15 || this.component.linearBrake > 0.15;
  }

  // A teleport: the car is somewhere else at rest. Hold there, and plan the route again from there
  // if there was one (the old one starts where the car was).
  public func Teleported() -> Void {
    let p = this.component.stats.d_position;
    this.holdPosition = p;
    this.lastPosition = p;
    this.positionJumpTimer = 0.0;
    this.speedCmd = 0.0;
    this.speedRate = 0.0;
    if this.RouteSize() > 0 || this.request != 0 {
      this.ClearRoute();
      this.request = 0;
      this.RequestRoute(p);
    }
  }

  // The car's position, with a jump further than it could have flown held off for up to
  // positionJumpHold (then taken as a real move, a teleport). The game can freeze the car and
  // report a stale position for a moment (seen at the Badlands edge); followed, the Autopilot
  // replanned from there and braked.
  private func CheckedPosition(position: Vector4, timeDelta: Float) -> Vector4 {
    let reach = MaxF(this.component.stats.d_speed, 20.0) * MaxF(timeDelta, 0.02) * 3.0 + 10.0;
    if Vector4.Distance(position, this.lastPosition) > reach && this.positionJumpTimer < this.positionJumpHold {
      if this.positionJumpTimer == 0.0 {
        FlightLog.Warn("[Autopilot] position jumped " + FloatToStringPrec(Vector4.Distance(position, this.lastPosition), 0) + " m to "
          + FloatToStringPrec(position.X, 1) + ", " + FloatToStringPrec(position.Y, 1) + ", " + FloatToStringPrec(position.Z, 1) + ": held off");
      }
      this.positionJumpTimer += timeDelta;
      return this.lastPosition;
    }
    this.positionJumpTimer = 0.0;
    this.lastPosition = position;
    return position;
  }

  // Angle between two directions, degrees (-1 if either is zero).
  private static func AngleBetween(a: Vector4, b: Vector4) -> Float {
    let la = SqrtF(a.X * a.X + a.Y * a.Y + a.Z * a.Z);
    let lb = SqrtF(b.X * b.X + b.Y * b.Y + b.Z * b.Z);
    if la < 0.0001 || lb < 0.0001 {
      return -1.0;
    }
    return Rad2Deg(AcosF(ClampF((a.X * b.X + a.Y * b.Y + a.Z * b.Z) / (la * lb), -1.0, 1.0)));
  }

  // With telemetry on, one row a frame to red4ext/logs/ltbf_autopilot_<time>.csv (FlightTelemetry):
  // where the car is and how fast, the speed it is held to, and how its orientation follows the
  // plan: the route's direction at the car, the nose and up the controller asked for last frame
  // (targetForward / targetUp) and the car's own, with the angles between them (plan_cmd: the
  // controller's choices such as drift into bends; cmd_car and up_cmd_car: the car's response).
  // Taken at the start of the frame, so a command is paired with what the car made of it.
  // tools/telemetry/autopilot.py summarises a file.
  private func WriteTelemetry(p: Vector4, v: Vector4, timeDelta: Float) -> Void {
    if !this.telemetry {
      if this.telemetryFile != 0 {
        FlightTelemetry.End(this.telemetryFile);
        this.telemetryFile = 0;
      }
      return;
    }
    if this.telemetryFile == 0 {
      this.telemetryFile = FlightTelemetry.Begin("autopilot", "t,x,y,z,vx,vy,vz,speed,speed2d,target,rate,cmd,off,following,maneuver,on_line,segment,"
        + "plan_x,plan_y,plan_z,cmd_fwd_x,cmd_fwd_y,cmd_fwd_z,cmd_up_x,cmd_up_y,cmd_up_z,fwd_x,fwd_y,fwd_z,up_x,up_y,up_z,"
        + "attitude_error,plan_cmd,cmd_car,up_cmd_car,plan_pitch,cmd_pitch,car_pitch,collision_timer,"
        + "drone,cmd_tilt,car_tilt,thrust_right,thrust_fwd,thrust_up,surge_held,drift,audio_surge");
      this.telemetryClock = 0.0;
      if this.telemetryFile == 0 {
        return;
      }
    }
    this.telemetryClock += timeDelta;
    let stats = this.component.stats;
    let n = this.RouteSize();
    let plan = new Vector4(0.0, 0.0, 0.0, 0.0);
    if this.segment + 1 < n {
      plan = this.route.Point(this.segment + 1) - this.route.Point(this.segment);
    }
    let fwd = this.targetForward;
    let up = this.targetUp;
    let row: array<Float>;
    ArrayPush(row, this.telemetryClock);
    ArrayPush(row, p.X);
    ArrayPush(row, p.Y);
    ArrayPush(row, p.Z);
    ArrayPush(row, v.X);
    ArrayPush(row, v.Y);
    ArrayPush(row, v.Z);
    ArrayPush(row, stats.d_speed);
    ArrayPush(row, stats.d_speed2D);
    ArrayPush(row, this.targetSpeed);
    ArrayPush(row, this.speedRate);
    ArrayPush(row, this.speedCmd);
    ArrayPush(row, this.offRouteNow);
    ArrayPush(row, Equals(this.status, "following") ? 1.0 : 0.0);
    ArrayPush(row, IsDefined(this.maneuver) ? 1.0 : 0.0);
    ArrayPush(row, this.onLine ? 1.0 : 0.0);
    ArrayPush(row, Cast<Float>(this.segment));
    ArrayPush(row, plan.X);
    ArrayPush(row, plan.Y);
    ArrayPush(row, plan.Z);
    ArrayPush(row, fwd.X);
    ArrayPush(row, fwd.Y);
    ArrayPush(row, fwd.Z);
    ArrayPush(row, up.X);
    ArrayPush(row, up.Y);
    ArrayPush(row, up.Z);
    ArrayPush(row, stats.d_forward.X);
    ArrayPush(row, stats.d_forward.Y);
    ArrayPush(row, stats.d_forward.Z);
    ArrayPush(row, stats.d_up.X);
    ArrayPush(row, stats.d_up.Y);
    ArrayPush(row, stats.d_up.Z);
    ArrayPush(row, Rad2Deg(this.attitudeError));
    ArrayPush(row, FlightModeAutopilot.AngleBetween(plan, fwd));
    ArrayPush(row, FlightModeAutopilot.AngleBetween(fwd, stats.d_forward));
    ArrayPush(row, FlightModeAutopilot.AngleBetween(up, stats.d_up));
    ArrayPush(row, FlightModeAutopilot.PitchOf(plan));
    ArrayPush(row, FlightModeAutopilot.PitchOf(fwd));
    ArrayPush(row, FlightModeAutopilot.PitchOf(stats.d_forward));
    ArrayPush(row, this.component.collisionTimer);
    // body tilt from vertical, and the thrust in the car's frame (m/s2: sway, surge, lift)
    ArrayPush(row, this.droneStyle ? 1.0 : 0.0);
    ArrayPush(row, FlightModeAutopilot.AngleBetween(up, FlightUtils.Up()));
    ArrayPush(row, FlightModeAutopilot.AngleBetween(stats.d_up, FlightUtils.Up()));
    ArrayPush(row, this.thrustCmd.X);
    ArrayPush(row, this.thrustCmd.Y);
    ArrayPush(row, this.thrustCmd.Z);
    ArrayPush(row, this.surgeHold.held ? 1.0 : 0.0);
    ArrayPush(row, this.FollowsTrajectory() ? Rad2Deg(this.trajectory.Drift(this.closestS)) : 0.0);
    ArrayPush(row, this.commandSurge);
    FlightTelemetry.Row(this.telemetryFile, row);
  }

  // Steepest climb or descent along the points, degrees, each taken over at least `span` metres of
  // horizontal travel (single short steps are noise).
  private static func SteepestPitch(points: array<Vector4>, span: Float) -> Float {
    let steepest = 0.0;
    let n = ArraySize(points);
    let i = 0;
    let j = 1;
    while j < n {
      let flat = Vector4.Distance2D(points[i], points[j]);
      if flat >= span {
        steepest = MaxF(steepest, Rad2Deg(AtanF(AbsF(points[j].Z - points[i].Z), flat)));
        i += 1;
      } else {
        // a vertical stretch shorter than span across: steep as can be
        if AbsF(points[j].Z - points[i].Z) > span {
          steepest = MaxF(steepest, Rad2Deg(AtanF(AbsF(points[j].Z - points[i].Z), MaxF(flat, 0.01))));
        }
        j += 1;
      }
    }
    return steepest;
  }

  // Pitch of a direction above the horizontal, degrees.
  private static func PitchOf(d: Vector4) -> Float {
    let l = SqrtF(d.X * d.X + d.Y * d.Y + d.Z * d.Z);
    return l < 0.0001 ? 0.0 : Rad2Deg(AsinF(ClampF(d.Z / l, -1.0, 1.0)));
  }

  public func Update(timeDelta: Float) -> Void {
    this.RefreshVehicle();
    let stats = this.component.stats;
    let p = this.CheckedPosition(stats.d_position, timeDelta);
    let v = stats.d_velocity;
    this.WriteTelemetry(p, v, timeDelta);

    this.UpdateRoute(timeDelta);
    this.WatchHits(timeDelta, p, v);

    if this.PlayerInput() {
      this.overrideTimer = this.overrideHold;
    }
    if this.overrideTimer > 0.0 {
      // manual: player inputs as given, altitude held by damping vertical speed
      this.overrideTimer -= timeDelta;
      this.holdPosition = p;
      this.speedCmd = stats.d_speed;
      this.speedRate = 0.0;
      this.accelCmd = new Vector4(0.0, 0.0, 0.0, 0.0);
      this.yawCmd = 0.0;
      if IsDefined(this.maneuver) {
        FlightLog.Info("[Autopilot] " + this.maneuver.GetName() + " cut short by player input");
        this.maneuver = null;
      }
      this.headingRef = stats.d_forward2D;
      this.UpdateWithNormalLift(timeDelta, FlightUtils.Up(), 9.81 * this.gravityFactor + this.component.lift * this.standardModeLiftFactor - v.Z);
      return;
    }

    // where to go
    let target = this.holdPosition;
    this.targetSpeed = 0.0;
    this.emergencyBrake = false;
    let shaped = IsDefined(this.maneuver) && !this.maneuver.IsOverlay();
    this.overlayVelocity = new Vector4(0.0, 0.0, 0.0, 0.0);
    let following = Equals(this.status, "following") && this.RouteSize() >= 2 && !shaped;
    if following {
      if this.nativeTracker {
        // the follower in C++ (FlightTracker): the same fields written, the replans and the route request left to the script
        let offset = new Vector4(0.0, 0.0, 0.0, 0.0);
        // barrel roll: aim at the route point moved round the roll's circle
        if IsDefined(this.maneuver) && Equals(this.maneuver.kind, FlightManeuverKind.BarrelRoll) {
          let barrelUp: Vector4;
          this.maneuver.BarrelOffset(this.maneuver.time, offset, this.overlayVelocity, barrelUp);
        }
        this.FillTracker(timeDelta, p, v);
        this.tracker.aimOffset = offset;
        this.tracker.Track(this.route, this.trajectory, this.Vehicle());
        this.TrackerOut();
        target = this.tracker.target;
        if this.tracker.replanLine {
          FlightLog.Info("[Autopilot] " + FloatToStringPrec(this.offRouteNow, 1) + " m off the line at " + FloatToStringPrec(stats.d_speed, 1) + " m/s: replanning from here");
          this.Replan(p, v);
        }
        if this.tracker.requestRoute {
          this.RequestRoute(p);
        }
      } else {
        let closest: Vector4;
        let offRoute: Float;
        this.ClosestOnRoute(p, closest, offRoute);
        this.offRouteNow = offRoute;
        this.closestNow = closest;
        this.closestS = this.route.Distance(this.segment) + Vector4.Distance(this.route.Point(this.segment), closest);
        this.remaining = this.route.Length() - this.closestS;
        if this.routeLine {
          // off the line: a new trajectory from where the car is going, at most once a second
          if offRoute > this.replanOffLine && this.recovering <= 0.0 && this.replanTimer <= 2.0 && this.request == 0 {
            FlightLog.Info("[Autopilot] " + FloatToStringPrec(offRoute, 1) + " m off the line at " + FloatToStringPrec(stats.d_speed, 1) + " m/s: replanning from here");
            this.Replan(p, v);
          }
        } else if offRoute > this.replanDistance && this.replanTimer <= 0.0 && this.request == 0 {
          this.RequestRoute(p);
        }
        let ahead = ClampF(0.7 * stats.d_speed + 12.0, 15.0, 90.0);
        // don't aim far past the next sharp bend: a long lookahead cuts inside it, and route legs
        // graze obstacles there
        if this.segment + 2 < this.RouteSize() {
          ahead = MinF(ahead, this.DistanceToSharpBend(closest) + this.cornerOvershoot);
        }
        target = this.AlongRoute(closest, ahead);
        // the straight line to the aim point must be free on the nav map, or the car cuts through
        // whatever the route bends around (a corner, the edge of a deck it passes under); pull the
        // aim point back along the route until it is. Check from the car, or from the route when
        // the car sits inside the map's margin (resting on or brushing a surface).
        let from = FlightNavMap.IsFree(p) ? p : closest;
        let tries = 0;
        while tries < 6 && ahead > 4.0 && FlightNavMap.SegmentClear(from, target) < 1.0 {
          ahead *= 0.6;
          target = this.AlongRoute(closest, ahead);
          tries += 1;
        }
        // pulled back at once (that is for clearance), grown back smoothly: a lookahead that jumps
        // out again swings the aim point and the heading with it
        if ahead < this.aheadSmooth {
          this.aheadSmooth = ahead;
        } else {
          this.aheadSmooth += (ahead - this.aheadSmooth) * ClampF(timeDelta / 0.6, 0.0, 1.0);
          ahead = this.aheadSmooth;
          target = this.AlongRoute(closest, ahead);
        }
        this.curvatureAhead = this.CurvatureAhead(closest, ahead);
        // smoothed: it steps as route points pass
        let curvature = this.CurvatureVector(closest, ahead);
        this.pathCurvature += (curvature - this.pathCurvature) * ClampF(timeDelta / 0.25, 0.0, 1.0);
        // barrel roll: aim at the route point moved round the roll's circle
        if IsDefined(this.maneuver) && Equals(this.maneuver.kind, FlightManeuverKind.BarrelRoll) {
          let offset: Vector4;
          let barrelUp: Vector4;
          this.maneuver.BarrelOffset(this.maneuver.time, offset, this.overlayVelocity, barrelUp);
          target += offset;
        }
        // the trajectory's speed and its rate at the car's place on the line, a moment ahead for the
        // time the thrust takes
        let sRef = this.closestS + stats.d_speed * this.lineLead;
        let planned = this.FollowsTrajectory();
        this.targetSpeed = planned ? this.trajectory.Speed(sRef) : 0.0;
        // where the current velocity is taking the car must be free on the map; brake to stop short
        // of the first blocked point otherwise (things the map lacks: cars, streamed-in props). Only
        // when the car is leaving a route that is clear over the same distance: where the route
        // itself grazes the map's margin (passing a ledge or under a deck) the velocity test fails
        // too, and braking there stalls the car on the line
        this.blockedAhead = false;
        if stats.d_speed > 3.0 {
          // the path the car is flying: velocity plus the acceleration it is commanding (turning
          // into a bend curves the prediction with it), in three pieces over collisionHorizon
          let travelled: Float;
          let hit: Vector4;
          let blockedAt = this.PredictBlocked(FlightNavMap.IsFree(p) ? p : closest, v, travelled, hit);
          let reach = v * this.collisionHorizon;
          // ...and only when it is actually taking the car off the route: where the route threads a
          // tight passage or skims over an edge, a path a metre beside it clips the map's margin
          // (which already holds the car's size), and trusting the planner's clear route there
          // avoids a full-brake jolt for nothing
          // (blocked space within 10 m cannot be braked for at speed, only steered away from, and
          // came from the map's margin around the car itself on the route)
          // (on a line, which the planner checked free all the way, only once the car has
          // left it: at speed the straight-line prediction runs into the walls past every bend)
          let lineHeld = this.routeLine && offRoute < this.raceOffLine;
          if blockedAt && !lineHeld && travelled > 10.0 && this.DistanceToRoute(hit) > 2.0 && this.RouteClear(closest, Vector4.Length(reach)) {
            let room = MaxF(travelled - 5.0, 0.0);
            this.targetSpeed = MinF(this.targetSpeed, SqrtF(2.0 * this.raceMargin * this.RaceDecel(0.0) * room));
            this.blockedAhead = true;
          }
        }
        this.speedCmd = this.targetSpeed;
        this.speedRate = planned ? this.trajectory.Along(sRef) : -this.RaceDecel(stats.d_speed);
        if this.blockedAhead {
          // braking for blocked space: the gap from the car's speed to the blocked one closed at
          // 2 speedGain, at most the race decel
          let brake = MinF(MaxF(stats.d_speed - this.targetSpeed, 0.0) * 2.0 * this.speedGain, this.RaceDecel(stats.d_speed));
          this.speedRate = MinF(this.speedRate, -brake);
        }
        // blocked space: braked for like any other limit while there is room to stop at brakeMax,
        // braked hard (emergencyBrake) only when there is not and it has been blocked for a moment.
        // At speed the 1.5 s velocity ray often pokes into geometry the route is about to bend
        // over or around for a frame or two; braking at 40 m/s2 for those was the harshest jolt.
        this.emergencyBrake = false;
        // once clear again, carry on from the speed the car has
        if !this.blockedAhead && this.blockedTimer > 0.2 {
          this.speedCmd = MaxF(this.speedCmd, stats.d_speed);
          this.speedRate = MaxF(this.speedRate, 0.0);
        }
        this.blockedTimer = this.blockedAhead ? this.blockedTimer + timeDelta : 0.0;
        if this.blockedAhead && this.blockedTimer > 0.2 && stats.d_speed > this.targetSpeed {
          let room = MaxF(this.targetSpeed * this.targetSpeed / (2.0 * this.brakeAccel), 0.5);
          if stats.d_speed * stats.d_speed / (2.0 * room) > this.brakeMax {
            this.speedCmd = this.targetSpeed;
            this.speedRate = MinF(this.speedRate, 0.0);
            this.emergencyBrake = true;
          }
        }
      }
      // barely moving while it should fly: wedged against something the map doesn't know about
      // (or thrown somewhere enclosed by a collision); out to the nearest free space first, and a
      // new route from there
      if stats.d_speed < 1.5 && this.targetSpeed > 5.0 {
        this.stuckTimer += timeDelta;
      } else {
        this.stuckTimer = 0.0;
      }
      if this.stuckTimer > 4.0 {
        let free = this.NearestFree(p);
        FlightLog.Warn("[Autopilot] wedged at " + FloatToStringPrec(p.X, 1) + ", " + FloatToStringPrec(p.Y, 1) + ", " + FloatToStringPrec(p.Z, 1)
          + ": out to " + FloatToStringPrec(free.X, 1) + ", " + FloatToStringPrec(free.Y, 1) + ", " + FloatToStringPrec(free.Z, 1) + " and replanning");
        this.stuckTimer = 0.0;
        this.ClearRoute();
        this.RequestRoute(free);
      }
      if this.component.isPlayerMounted {
        this.PublishProgress();
      }
      if this.RouteSize() >= 2 {
        this.RecheckLandingSpot(timeDelta);
      }
      if this.RouteSize() < 2 {
        // the route was just cleared (wedged, replanning): nothing to arrive at this frame
      } else if this.freeRoam && this.remaining < this.arriveDistance && stats.d_speed < 3.0 {
        // the end of the route before the next leg joined: on from here (ChainFreeRoam)
        this.holdPosition = this.route.Point(this.route.Size() - 1);
        this.ClearRoute();
        this.status = "free roam";
        FlightLog.Info("[Autopilot] free roam: end of the route, planning on from here");
      } else if this.ahead.firstLeg {
        // the end of a first leg, not the trip's: the rest is on its way (or asked for again)
      } else if this.landing ? this.remaining < this.landingEntry && stats.d_speed < this.landingEntrySpeed : this.remaining < this.arriveDistance && stats.d_speed < 3.0 {
        // the spot again now the cars around it have streamed in: taken, the nearest free one
        // with room (from up here a few metres across is fine; the shaft above it is open)
        if this.landing && this.landingPad < 0 && this.SpotTaken(this.landingSpot) {
          let top: Float;
          let spot: Vector4;
          if this.FindLandingSpot(this.destination, spot, top) {
            FlightLog.Info("[Autopilot] landing spot taken by a vehicle, moving to " + FloatToStringPrec(spot.X, 1) + ", " + FloatToStringPrec(spot.Y, 1)
              + " (" + FloatToStringPrec(Vector4.Distance2D(spot, this.landingSpot), 1) + " m over)");
            this.landingSpot = spot;
          } else {
            FlightLog.Info("[Autopilot] landing spot taken by a vehicle and no free one near: hovering");
            this.landing = false;
          }
        }
        this.status = this.landing ? "descending" : "arrived";
        this.holdPosition = this.landing ? this.landingSpot : this.route.Point(this.route.Size() - 1);
        if this.landing {
          this.landingStartZ = p.Z;
          this.landingHeading0 = stats.d_forward2D;
          this.landingHeading = this.landingPad >= 0
            ? FlightModeAutopilot.PadHeading(this.landingPad, stats.d_forward2D)
            : this.LandingHeading(this.landingSpot, stats.d_forward2D);
          // a landing turn under way (LandSlewYaw) ends on its own heading, droneLandSlewDuring into the descent
          if this.landSlewOn {
            this.landingHeading = this.landSlewHeading;
            this.landSlewDescent = 0.0;
          }
        } else {
          this.landSlewOn = false;
        }
        this.ClearRoute();
        this.settleTimer = 0.0;
        this.touchdownTimer = 0.0;
        FlightLog.Info("[Autopilot] arrived" + (this.landing ? ", descending" : ""));
      }
    } else if Equals(this.status, "descending") {
      // down the column at a capped rate, slowing in the last metres and sinking on at the
      // touchdown rate past the bake's floor (the ground can be lower); touched down once the car
      // stops sinking close to it, resting on its wheels
      let height = p.Z - this.landingSpot.Z;
      this.settleTimer += timeDelta; // time since the descent began
      if Vector4.Distance2D(p, this.landingSpot) > this.landingOverSpot {
        this.settleTimer = 0.0; // still moving over the spot: not sinking yet
      }
      // stopped sinking: no new low point for a while (the reported vertical speed stays around
      // -0.5 m/s on the wheels while the thrusters press down, so it is no use here)
      if this.settleTimer < 2.0 || p.Z < this.touchdownLow - 0.1 {
        this.touchdownLow = p.Z;
        this.touchdownTimer = 0.0;
      } else {
        this.touchdownTimer += timeDelta;
      }
      if this.touchdownTimer > 0.7 && height < 3.0 {
        this.status = "landed";
        FlightLog.Info("[Autopilot] touched down " + FloatToStringPrec(height, 2) + " m above the floor, " + FloatToStringPrec(Vector4.Distance2D(p, this.landingSpot), 1) + " m from the spot");
        if this.component.isPlayerMounted {
          FlightModeAutopilot.Notify(this.component.GetVehicle().GetGame(), "LocKey#96672");
          FlightController.GetInstance().autopilotRequestsFlightOff = true;
        }
      } else if this.touchdownTimer > 1.5 {
        // held up well above the floor (something the map doesn't have): stay in the air
        this.status = "arrived";
        this.holdPosition = p;
        this.settleTimer = 0.0;
        FlightLog.Warn("[Autopilot] descent stopped " + FloatToStringPrec(height, 1) + " m above the floor, hovering");
      }
    } else {
      if Equals(this.status, "arrived") && this.component.isPlayerMounted {
        // hold on the destination until the car is still, then hand the controls back like
        // vanilla autodrive ("destination reached"). The previous mode has no horizontal
        // damping, so speed left over at the hand-back carries the car on for tens of metres.
        this.settleTimer += timeDelta;
        if (stats.d_speed < 0.5 && Vector4.Distance(p, this.holdPosition) < 2.0) || this.settleTimer > 5.0 {
          this.status = "idle";
          FlightLog.Info("[Autopilot] settled after " + FloatToStringPrec(this.settleTimer, 1) + " s, handing back");
          FlightModeAutopilot.Notify(this.component.GetVehicle().GetGame(), "LocKey#96672");
          FlightController.GetInstance().ExitAutopilot();
        }
      }
    }
    this.lookahead = target;

    // desired velocity and acceleration (world)
    let toTarget = target - p;
    let desiredVelocity: Vector4;
    if following {
      desiredVelocity = Vector4.Normalize(toTarget) * this.speedCmd + this.overlayVelocity;
    } else {
      // start the next stretch of route following from the speed the car has
      this.speedCmd = stats.d_speed;
      this.speedRate = 0.0;
      this.aheadSmooth = 15.0;
      // hold: approach the hold point, slowing down near it
      desiredVelocity = toTarget * 0.8;
      if Vector4.Length(desiredVelocity) > 10.0 {
        desiredVelocity = Vector4.Normalize(desiredVelocity) * 10.0;
      }
      if Equals(this.status, "descending") {
        // over the spot first, at the height the descent began (sliding across while sinking, the car
        // can hit what is over the neighbouring columns)
        if Vector4.Distance2D(p, this.landingSpot) > this.landingOverSpot {
          desiredVelocity.Z = ClampF(0.5 * (this.landingStartZ - p.Z), -2.0, 2.0);
        } else {
          desiredVelocity.Z = -ClampF(0.5 * (p.Z - this.landingSpot.Z), this.touchdownRate, this.descentRate);
        }
      } else if Equals(this.status, "landed") {
        // flight is being switched off: keep settling onto the wheels
        desiredVelocity = new Vector4(0.0, 0.0, -this.touchdownRate, 0.0);
      }
    }
    let accel = (desiredVelocity - v) * this.velocityGain;
    let jerk = this.accelJerk;
    this.onLine = following && this.routeLine;
    if this.nativeTracker && following {
      // the script's line and aim-point acceleration is skipped: FlightTracker.Command works it out below
    } else if this.onLine {
      let tangent: Vector4;
      let bend: Vector4;
      this.LineFrame(this.closestS, tangent, bend);
      let leadTangent: Vector4;
      this.LineFrame(this.closestS + stats.d_speed * this.lineLead, leadTangent, bend);
      let across = this.closestNow - p;
      across -= tangent * Vector4.Dot(across, tangent);
      let back = across * this.lineSpring;
      if Vector4.Length(back) > 15.0 {
        back = Vector4.Normalize(back) * 15.0;
      }
      desiredVelocity = tangent * this.speedCmd + back + this.overlayVelocity;
      // plus the flown speed's own rate along the line: on velocity error alone the car trails a
      // braking curve by rate / lineVelocityGain
      // the bend at the lower of the speed flown and the trajectory's: planned at its own speed, a car
      // a little slower turned in too hard and ran inside the line; at the flown speed, a car still
      // faster than a new trajectory (just rejoined) was asked to take its bend at 85 m/s against 42
      // planned, and lost all its speed to it
      let bendSpeed = this.FollowsTrajectory() ? MinF(stats.d_speed, this.speedCmd) : this.speedCmd;
      let correct = (desiredVelocity - v) * this.lineVelocityGain;
      // on a trajectory its own acceleration goes in whole and the correction is capped: flying a
      // little across a new line (just replanned), the whole velocity error braked the car from 79
      // to 18 m/s against 57 planned
      if this.FollowsTrajectory() && Vector4.Length(correct) > this.trajectoryCorrectionMax {
        correct = Vector4.Normalize(correct) * this.trajectoryCorrectionMax;
      }
      accel = correct + bend * (bendSpeed * bendSpeed) + tangent * this.speedRate;
      target = this.PointAt(this.closestS + 30.0);
      this.lookahead = target;
      jerk = this.raceJerk;
    } else if following {
      // the bend's own acceleration ahead of time (velocity error alone turns too late at speed);
      // the thrust limits are applied in the car's frame (FlyAerobatic); the trajectory's own rate
      // along the route fed forward too
      accel += this.pathCurvature * (this.speedCmd * this.speedCmd);
      if Vector4.Length(toTarget) > 0.1 {
        accel += Vector4.Normalize(toTarget) * this.speedRate;
      }
      jerk = this.raceJerk;
    } else {
      let horizontal = new Vector4(accel.X, accel.Y, 0.0, 0.0);
      if Vector4.Length(horizontal) > this.maxAccel {
        horizontal = Vector4.Normalize(horizontal) * this.maxAccel;
      }
      accel.X = horizontal.X;
      accel.Y = horizontal.Y;
      accel.Z = ClampF(accel.Z, -this.maxAccel, this.maxAccel);
    }
    if shaped {
      // track the manoeuvre's shape: its own acceleration plus corrections towards it
      let refPos: Vector4;
      let refVel: Vector4;
      let refAcc: Vector4;
      let refFwd: Vector4;
      let refUp: Vector4;
      this.maneuver.Sample(this.maneuver.time, refPos, refVel, refAcc, refFwd, refUp);
      target = refPos;
      this.lookahead = refPos;
      desiredVelocity = refVel;
      accel = refAcc + (refVel - v) * this.maneuverVelGain + (refPos - p) * this.maneuverPosGain;
      if Vector4.Length(accel) > this.maneuverMaxAccel {
        accel = Vector4.Normalize(accel) * this.maneuverMaxAccel;
      }
      jerk = this.maneuverJerk;
      if Vector4.Distance(refPos, p) > this.maneuverAbortDistance {
        FlightLog.Warn("[Autopilot] " + this.maneuver.GetName() + " abandoned " + FloatToStringPrec(Vector4.Distance(refPos, p), 1) + " m off its shape at "
          + FloatToStringPrec(this.maneuver.time, 2) + " s: car " + FloatToStringPrec(p.X, 1) + ", " + FloatToStringPrec(p.Y, 1) + ", " + FloatToStringPrec(p.Z, 1)
          + " shape " + FloatToStringPrec(refPos.X, 1) + ", " + FloatToStringPrec(refPos.Y, 1) + ", " + FloatToStringPrec(refPos.Z, 1)
          + " from " + FloatToStringPrec(this.maneuver.origin.X, 1) + ", " + FloatToStringPrec(this.maneuver.origin.Y, 1) + ", " + FloatToStringPrec(this.maneuver.origin.Z, 1));
        this.EndManeuver();
      }
    }
    // after a hit, while the controls are damped: hold the velocity the car has (pulled back to the
    // line it reversed towards a point it had passed), level, until the replan
    if this.recovering > 0.0 && following && !this.nativeTracker {
      accel = new Vector4(0.0, 0.0, 0.0, 0.0);
      desiredVelocity = v;
    }
    if this.nativeTracker {
      // the follower's acceleration (or, not following, the one worked out above) rate limited, and the
      // surge hold updated with the inputs FlyAerobatic gave it
      this.FillTracker(timeDelta, p, v);
      this.tracker.following = following;
      this.tracker.target = target;
      this.tracker.overlayVelocity = this.overlayVelocity;
      this.tracker.rawAccel = accel;
      this.tracker.rawJerk = jerk;
      this.tracker.rawDesiredVelocity = desiredVelocity;
      this.tracker.surgeFree = this.recovering <= 0.0 && stats.d_speed > 1.0 && this.FollowsTrajectory() && Equals(this.status, "following");
      this.tracker.Command(this.route, this.trajectory);
      this.TrackerOut();
      accel = this.accelCmd;
      if following {
        desiredVelocity = this.tracker.desiredVelocity;
        this.lookahead = this.tracker.lookahead;
      }
    } else {
      // ease changes of the acceleration itself (it is what is felt): rate-limit the command,
      // except in an emergency stop for blocked space
      let change = accel - this.accelCmd;
      let maxChange = jerk * timeDelta;
      if !this.emergencyBrake && Vector4.Length(change) > maxChange {
        change = Vector4.Normalize(change) * maxChange;
      }
      this.accelCmd += change;
      accel = this.accelCmd;
    }

    // fly like a drone when slow, like a plane when fast
    let blend = ClampF((stats.d_speed - 8.0) / 17.0, 0.0, 1.0);
    blend = blend * blend * (3.0 - 2.0 * blend);

    // heading: along the desired velocity, or keep the current one when hovering; during the
    // takeoff climb, turn to the route's first leg
    // takeoff and landing turn with the height: the heading goes from the one the climb started
    // with to the route's first leg in step with the height gained, and from the arrival heading to
    // the landing heading in step with the height lost, so the turn and the climb or descent
    // happen as one motion and end together
    let heading = new Vector4(desiredVelocity.X, desiredVelocity.Y, 0.0, 0.0);
    if Equals(this.status, "descending") || Equals(this.status, "landed") {
      let f = ClampF((this.landingStartZ - p.Z) / MaxF(this.landingStartZ - this.landingSpot.Z, 0.5), 0.0, 1.0);
      heading = FlightModeAutopilot.BlendHeading(this.landingHeading0, this.landingHeading, f * f * (3.0 - 2.0 * f)) * 10.0;
    }
    this.FlyAerobatic(timeDelta, accel, heading);
  }

  // --- race ---------------------------------------------------------------------------------

  // The car's limits (FlightVehicleModel, worked out in C++ in Navigation/Vehicle.hpp) filled from
  // the car's stats and this mode's tuning by RefreshVehicle, once at the top of Update and at the
  // start of every call that plans (so a tuning field set between frames, e.g. by LtbfBridge,
  // reaches the plan); Vehicle() returns the model as last filled, so per-frame readers between
  // refreshes see that frame's values.
  public func RefreshVehicle() -> Void {
    if !IsDefined(this.vehicleModel) {
      this.vehicleModel = new FlightVehicleModel();
    }
    let m = this.vehicleModel;
    let stats = this.component.stats;
    let settings = FlightSettings.GetInstance();
    m.mass = stats.s_mass;
    m.airResistance = stats.s_airResistanceFactor;
    m.brakingFriction = stats.s_brakingFrictionFactor;
    m.surge = this.standardModeSurgeFactor;
    m.sway = this.standardModeSwayFactor;
    m.lift = this.standardModeLiftFactor;
    m.gravityFactor = this.gravityFactor;
    m.brakeLinear = settings.brakeFactorLinear;
    m.drag = settings.physxFlightPhysics;
    m.drone = this.droneStyle;
    m.maxTilt = this.droneMaxTilt;
    m.brakeShare = this.droneBrakeShare;
    m.tiltFloor = this.droneTiltFloor;
    m.driftMax = this.raceDriftMax;
    m.speedCap = this.freeRoam ? this.freeRoamTopSpeed : this.raceSpeedCap;
    m.margin = this.raceMargin;
    m.planDecel = this.racePlanDecel;
    m.pushOver = this.droneStyle ? this.dronePushOver : this.racePushOver;
  }

  public func Vehicle() -> ref<FlightVehicleModel> {
    if !IsDefined(this.vehicleModel) {
      this.RefreshVehicle();
    }
    return this.vehicleModel;
  }

  // Air drag at speed v (FlightMode.ApplyPhysics), m/s2.
  private func DragAccel(v: Float) -> Float {
    return this.Vehicle().Drag(v);
  }

  // Where full surge equals the drag.
  private func RaceTopSpeed() -> Float {
    return this.Vehicle().TopSpeed();
  }

  // Slowing at speed v: full reverse surge, the brake (velocity damping, FlightMode_Standard) and drag.
  private func RaceDecel(v: Float) -> Float {
    return this.Vehicle().Decel(v);
  }

  // Sideways acceleration in a level turn: drone style the sway envelope, plane style the full lift
  // banked into it (the rest holding the car up) plus the surge's share with the nose drifted in.
  private func RaceLateral() -> Float {
    return this.Vehicle().Lateral();
  }

  // A hit (FlightComponent resets collisionTimer): remembered for the next plans for hitAvoidTime,
  // and once the controls' damping is over (collisionRecoveryDelay) a replan from the car's state.
  private func WatchHits(dt: Float, p: Vector4, v: Vector4) -> Void {
    this.clock += dt;
    let timer = this.component.collisionTimer;
    if timer < this.lastCollisionTimer - 0.05 && Equals(this.status, "following") {
      ArrayPush(this.hits, new Vector4(p.X, p.Y, p.Z, this.hitAvoidRadius));
      ArrayPush(this.hitTimes, this.clock);
      this.recovering = FlightSettings.GetFloatN(n"collisionRecoveryDelay");
      FlightLog.Info("[Autopilot] hit at " + FloatToStringPrec(p.X, 1) + ", " + FloatToStringPrec(p.Y, 1) + ", " + FloatToStringPrec(p.Z, 1)
        + " at " + FloatToStringPrec(Vector4.Length(v), 1) + " m/s: holding, then replanning round it");
    }
    this.lastCollisionTimer = timer;
    while ArraySize(this.hitTimes) > 0 && this.clock - this.hitTimes[0] > this.hitAvoidTime {
      ArrayErase(this.hits, 0);
      ArrayErase(this.hitTimes, 0);
    }
    if this.recovering > 0.0 {
      this.recovering -= dt;
      if this.recovering <= 0.0 && this.request == 0 && Equals(this.status, "following") {
        this.Replan(p, v);
      }
    }
  }

  // Back onto the line from where the car is: a connector from it (a lead-in along its velocity) to the
  // line a few seconds on, then the rest of the line as it was (FlightNavMap.PlanRejoinAsync; a full
  // replan when there is no line left to rejoin, or no way back onto it).
  private func Replan(p: Vector4, v: Vector4) -> Void {
    this.RefreshVehicle();
    let n = this.RouteSize();
    if !this.routeLine || n - this.segment < 4 {
      this.RequestRoute(p);
      return;
    }
    let line: array<Vector4>;
    let i = this.segment;
    while i < n {
      ArrayPush(line, this.route.Point(i));
      i += 1;
    }
    this.requestRace = true;
    this.requestJoin = false;
    this.requestRejoin = true;
    this.request = FlightNavMap.PlanRejoinAsync(p, v, line, MaxF(this.rejoinMin, Vector4.Length(v) * this.rejoinAhead), this.Vehicle(), this.hits);
    this.replanTimer = 3.0;
  }

  // The nearest free point to p on the nav map, up, sideways or down, 1 to 8 m out (p if none).
  private func NearestFree(p: Vector4) -> Vector4 {
    if FlightNavMap.IsFree(p) {
      return p;
    }
    let r = 1.0;
    while r <= 8.0 {
      let dirs = [new Vector4(0.0, 0.0, 1.0, 0.0), new Vector4(1.0, 0.0, 0.0, 0.0), new Vector4(-1.0, 0.0, 0.0, 0.0),
        new Vector4(0.0, 1.0, 0.0, 0.0), new Vector4(0.0, -1.0, 0.0, 0.0), new Vector4(0.0, 0.0, -1.0, 0.0)];
      for d in dirs {
        let q = p + d * r;
        if FlightNavMap.IsFree(q) {
          return q;
        }
      }
      r += 1.0;
    }
    return p;
  }

  // A trajectory to follow (every route gets one, BuildTrajectory).
  private func FollowsTrajectory() -> Bool {
    return IsDefined(this.trajectory) && this.trajectory.IsValid();
  }

  // The route's trajectory for this car, built whenever the route changes: from the car's speed
  // when the route starts at the car, free to start at any speed when the route is rebuilt around
  // a car already on it (a spliced leg, a moved landing spot). Public for LtbfBridge.DroneTuning.
  public func BuildTrajectory() -> Void {
    this.RefreshVehicle();
    if this.RouteSize() < 2 {
      return;
    }
    let start = this.closestS < 1.0 ? this.component.stats.d_speed : 1000.0;
    this.trajectory = FlightTrajectory.Build(this.route.Points(), this.Vehicle(), start, true);
    let bad = this.trajectory.Violations();
    if StrLen(bad) > 0 {
      FlightLog.Warn("[Autopilot] trajectory outside the car's limits: " + bad);
    }
    let top = 0.0;
    let length = this.trajectory.Length();
    let s = 0.0;
    while s <= length {
      top = MaxF(top, this.trajectory.Speed(s));
      s += 10.0;
    }
    let stats = this.component.stats;
    FlightLog.Info("[Autopilot] race" + (this.droneStyle ? " (drone)" : "") + ": " + FloatToStringPrec(length, 0) + " m in " + FloatToStringPrec(this.trajectory.Duration(), 1)
      + " s, top " + FloatToStringPrec(top, 1) + " m/s (car " + FloatToStringPrec(this.RaceTopSpeed(), 1) + "), lateral " + FloatToStringPrec(this.RaceLateral(), 1)
      + " m/s2, decel " + FloatToStringPrec(this.RaceDecel(0.0), 1) + " + " + FloatToStringPrec(this.RaceDecel(30.0) - this.RaceDecel(0.0), 1) + " at 30 m/s"
      + " (mass " + FloatToStringPrec(stats.s_mass, 0) + ", air " + FloatToStringPrec(stats.s_airResistanceFactor, 3) + ", brake friction " + FloatToStringPrec(stats.s_brakingFrictionFactor, 2) + ")");
  }

  // The route's end moved over to a landing spot `to` (the end keeping its height above the floor):
  // the last landingShiftSpan metres shifted by the same offset, eased in, each moved point free
  // on the map; false (and nothing changed) where one is not. The trajectory is rebuilt.
  private func ShiftRouteEnd(to: Vector4) -> Bool {
    let n = this.RouteSize();
    if n < 3 {
      return false;
    }
    let total = this.route.Length();
    let end = this.route.Point(n - 1);
    let offset = new Vector4(to.X - end.X, to.Y - end.Y, to.Z - this.landingSpot.Z, 0.0);
    let moved: array<Vector4>;
    let i = 0;
    while i < n {
      let q = this.route.Point(i);
      let left = total - this.route.Distance(i);
      if i > this.segment + 1 && left < this.landingShiftSpan {
        let f = 1.0 - left / this.landingShiftSpan;
        f = f * f * (3.0 - 2.0 * f);
        q += offset * f;
        if !FlightNavMap.IsFree(q) {
          return false;
        }
      }
      ArrayPush(moved, q);
      i += 1;
    }
    this.route = FlightRoute.Create(moved);
    this.routeGoal = this.route.Point(n - 1);
    this.BuildTrajectory();
    return true;
  }

  // The landing spot again on the approach, once the cars around it have streamed in: taken, the
  // route's end is moved to the nearest free one (ShiftRouteEnd), so the car flies into it in one
  // go rather than stopping over the taken spot and moving over.
  private func RecheckLandingSpot(timeDelta: Float) -> Void {
    if !this.landing || this.ahead.firstLeg || this.landingPad >= 0 || this.remaining > this.landingRecheck || this.remaining < 15.0 {
      return;
    }
    this.spotCheckTimer -= timeDelta;
    if this.spotCheckTimer > 0.0 {
      return;
    }
    this.spotCheckTimer = 0.5;
    let taken = this.SpotTaken(this.landingSpot);
    if !taken && this.SpotSupported(this.landingSpot) {
      return;
    }
    let top: Float;
    let spot: Vector4;
    if this.FindLandingSpot(this.destination, spot, top) && Vector4.Distance2D(spot, this.landingSpot) > 2.0 {
      let over = Vector4.Distance2D(spot, this.landingSpot);
      if this.ShiftRouteEnd(spot) {
        this.landingSpot = spot;
        FlightLog.Info("[Autopilot] landing spot " + (taken ? "taken by a vehicle" : "not under the whole car (an edge or a ledge)") + ", " + FloatToStringPrec(this.remaining, 0) + " m out: route end moved "
          + FloatToStringPrec(over, 1) + " m to " + FloatToStringPrec(spot.X, 1) + ", " + FloatToStringPrec(spot.Y, 1));
      }
    }
  }

  // Seconds the trajectory takes from here to where the descent takes over (landingEntry before
  // the end); far off (1000) without one, so the landing turn does not start early.
  private func TimeToDescent() -> Float {
    if !this.FollowsTrajectory() {
      return 1000.0;
    }
    let total = this.trajectory.Length();
    return MaxF(this.trajectory.Time(total - this.landingEntry) - this.trajectory.Time(this.closestS), 0.0);
  }

  // Into the landing the nose makes one turn, from where it was when the turn began to the heading
  // it lands on, the descent finishing it: one motion. The landing heading is the spot's most open
  // line (LandingHeading) or the pad's, whichever of its two ways, turned either way round, makes a
  // turn nearest droneLandSlewYaw. Returns the nose yaw wanted.
  // Once started it runs on its own to where the descent takes over (a moved spot stretches what
  // is left of it instead of starting again).
  private func LandSlewYaw(timeLeft: Float, dt: Float, yaw: Float) -> Float {
    if !this.landSlewOn {
      this.landSlewOn = true;
      this.landSlewP = 0.0;
      this.landSlewStartYaw = yaw;
      let start = this.landSlewStartYaw;
      let startHeading = new Vector4(CosF(start), SinF(start), 0.0, 0.0);
      let base = this.landingPad >= 0 ? FlightModeAutopilot.PadHeading(this.landingPad, startHeading) : this.LandingHeading(this.landingSpot, startHeading);
      // either way round (direction 1: the yaw rising, the nose turning left)
      let want = Deg2Rad(this.droneLandSlewYaw);
      let best = -1.0;
      let bestDirection = 1.0;
      let d = 0;
      while d < 2 {
        let direction = d == 0 ? 1.0 : -1.0;
        let i = 0;
        while i < (this.landingPad >= 0 ? 1 : 2) {
          let heading = i == 0 ? base : base * -1.0;
          let turn = FlightModeAutopilot.WrapAngle(AtanF(heading.Y, heading.X) - start) * direction;
          if turn < 0.0 {
            turn += 6.2831853;
          }
          if best < 0.0 || AbsF(turn - want) < AbsF(best - want) {
            best = turn;
            bestDirection = direction;
            this.landSlewHeading = heading;
          }
          i += 1;
        }
        d += 1;
      }
      this.landSlewDelta = best * bestDirection;
      FlightLog.Info("[Autopilot]   landing turn " + FloatToStringPrec(Rad2Deg(this.landSlewDelta), 0) + " deg");
    }
    // progress carried on over the time left, so a moved end stretches what is left of the turn
    // instead of setting it back. Over time, not distance: over distance most of the turn is done
    // early in a long brake, still at speed.
    if dt > 0.0 {
      this.landSlewP = ClampF(this.landSlewP + (1.0 - this.landSlewP) * dt / MaxF(timeLeft, dt), 0.0, 1.0);
    }
    let p = this.landSlewP;
    return this.landSlewStartYaw + this.landSlewDelta * p * p * (3.0 - 2.0 * p);
  }

  // --- aerobatics --------------------------------------------------------------------------

  // While following the route: turn round over the top or underneath when the route doubles back
  // on the car, and with stuntInterval set, a stunt on a long straight now and then. Only what
  // fits on the nav map is flown.
  private func ChooseManeuver(p: Vector4, closest: Vector4, timeDelta: Float) -> Void {
    this.maneuverCooldown = MaxF(this.maneuverCooldown - timeDelta, 0.0);
    let stats = this.component.stats;
    let v = stats.d_velocity;
    if IsDefined(this.maneuver) || this.maneuverCooldown > 0.0 || this.blockedAhead || this.emergencyBrake
      || stats.d_speed2D < this.maneuverMinSpeed || AbsF(v.Z) > 3.0 {
      return;
    }
    let side = RandRangeF(0.0, 1.0) < 0.5 ? -1.0 : 1.0;
    // the route goes back the way the car came (overshot, or a new destination behind): an
    // Immelmann, or a split-S where the route carries on lower
    let ahead = this.AlongRoute(closest, 25.0) - p;
    if stats.d_speed2D <= this.maneuverMaxSpeed && Vector4.Length2D(ahead) > 5.0
      && Vector4.Dot(FlightModeAutopilot.Flat(ahead), FlightModeAutopilot.Flat(v)) < -0.5 {
      let down = ahead.Z < -10.0;
      if !this.TryManeuver(down ? FlightManeuverKind.SplitS : FlightManeuverKind.Immelmann, side, p, closest)
        && !this.TryManeuver(down ? FlightManeuverKind.Immelmann : FlightManeuverKind.SplitS, side, p, closest) {
        this.maneuverCooldown = 1.0; // neither fits here: look again in a moment
      }
      return;
    }
    if this.stuntInterval <= 0.0 {
      return;
    }
    this.stuntTimer += timeDelta;
    if this.stuntTimer < this.stuntInterval {
      return;
    }
    let kinds: array<FlightManeuverKind> = [FlightManeuverKind.AileronRoll, FlightManeuverKind.BarrelRoll, FlightManeuverKind.Loop];
    let kind = kinds[Min(Cast<Int32>(RandRangeF(0.0, 3.0)), 2)];
    if Equals(kind, FlightManeuverKind.Loop) && stats.d_speed2D > this.maneuverMaxSpeed {
      kind = FlightManeuverKind.AileronRoll;
    }
    let reach = stats.d_speed * FlightManeuver.Create(kind, side, p, v, this.maneuverAccel, this.maneuverRollTime, this.nativeAerobatics).duration;
    if this.DistanceToSharpBend(closest) > reach + 30.0 && this.remaining > reach + 100.0 && AbsF(this.curvatureAhead) < 0.004
      && this.TryManeuver(kind, side, p, closest) {
      this.stuntTimer = 0.0;
    } else {
      this.stuntTimer = this.stuntInterval - 1.0; // try again in a second
    }
  }

  // Starts the manoeuvre if its whole shape is free on the map.
  private func TryManeuver(kind: FlightManeuverKind, side: Float, p: Vector4, closest: Vector4) -> Bool {
    let stats = this.component.stats;
    // only from settled flight with the nose along the path: a shape started off it begins with a
    // jump of the attitude target (up to 75 degrees after a previous manoeuvre), which the car
    // overshot and fell out of
    if Rad2Deg(this.attitudeError) > this.settledError || Vector4.Length(stats.d_angularVelocity) > 1.0
      || Vector4.Dot(stats.d_forward2D, Vector4.Normalize2D(stats.d_velocity)) < 0.95 {
      return false;
    }
    let m = FlightManeuver.Create(kind, side, p, stats.d_velocity, this.maneuverAccel, this.maneuverRollTime, this.nativeAerobatics);
    let fits: Bool;
    if Equals(kind, FlightManeuverKind.AileronRoll) {
      fits = this.RouteClear(closest, stats.d_speed * m.duration + 10.0);
    } else if Equals(kind, FlightManeuverKind.BarrelRoll) {
      fits = this.BarrelFits(m, closest);
    } else {
      fits = m.Fits(this.pitchMargin);
    }
    if !fits {
      return false;
    }
    this.maneuver = m;
    FlightLog.Info("[Autopilot] " + m.GetName() + " at " + FloatToStringPrec(stats.d_speed, 1) + " m/s"
      + (m.IsOverlay() ? "" : ", radius " + FloatToStringPrec(m.radius, 1) + " m") + ", " + FloatToStringPrec(m.duration, 1) + " s");
    return true;
  }

  // The barrel roll's circle around the route ahead is free on the map.
  private func BarrelFits(m: ref<FlightManeuver>, closest: Vector4) -> Bool {
    let speed = this.component.stats.d_speed;
    let prev = closest;
    let fits = true;
    let i = 1;
    while fits && i <= 16 {
      m.time = m.duration * Cast<Float>(i) / 16.0;
      let offset: Vector4;
      let offsetVelocity: Vector4;
      let up: Vector4;
      m.BarrelOffset(m.time, offset, offsetVelocity, up);
      let q = this.AlongRoute(closest, speed * m.time) + offset;
      fits = FlightNavMap.IsFree(q) && FlightNavMap.SegmentClear(prev, q) >= 1.0;
      prev = q;
      i += 1;
    }
    m.time = 0.0;
    return fits;
  }

  private func EndManeuver() -> Void {
    if IsDefined(this.maneuver) {
      FlightLog.Info("[Autopilot] " + this.maneuver.GetName() + " done");
      if !this.maneuver.IsOverlay() {
        // pick the route up again from where the shape left the car
        this.speedCmd = this.component.stats.d_speed;
        this.speedRate = 0.0;
        this.aheadSmooth = 15.0;
        this.replanTimer = 0.0;
        this.holdPosition = this.component.stats.d_position;
      }
    }
    this.maneuver = null;
    this.maneuverCooldown = 4.0;
    this.stuntTimer = 0.0;
  }

  // For tests (LtbfBridge.Aerobatics): kind as FlightManeuverKind (1 aileron roll .. 5 split-S), side +-1.
  public func StartManeuver(kind: Int32, side: Float) -> String {
    if this.droneStyle {
      return "manoeuvres are disconnected (drone style)";
    }
    if IsDefined(this.maneuver) {
      return "busy with " + this.maneuver.GetName();
    }
    let k = IntEnum<FlightManeuverKind>(kind);
    let following = Equals(this.status, "following") && this.RouteSize() >= 2;
    if (Equals(k, FlightManeuverKind.AileronRoll) || Equals(k, FlightManeuverKind.BarrelRoll)) && !following {
      return "rolls need a route to follow";
    }
    if this.component.stats.d_speed2D < 8.0 {
      return "too slow";
    }
    if this.component.stats.d_speed2D > this.maneuverMaxSpeed && !(Equals(k, FlightManeuverKind.AileronRoll) || Equals(k, FlightManeuverKind.BarrelRoll)) {
      return "too fast for a pitch manoeuvre";
    }
    if Rad2Deg(this.attitudeError) > this.settledError || Vector4.Length(this.component.stats.d_angularVelocity) > 1.0
      || Vector4.Dot(this.component.stats.d_forward2D, Vector4.Normalize2D(this.component.stats.d_velocity)) < 0.95 {
      return "not settled";
    }
    return this.TryManeuver(k, side, this.component.stats.d_position, this.closestNow) ? this.maneuver.GetName() : "does not fit here";
  }

  // The up vector's bank about the nose (0 = level, positive towards the car's right side) and the
  // two directions it is measured from; false with the nose too near vertical to tell.
  private static func BankFrame(fwd: Vector4, out level: Vector4, out side: Vector4) -> Bool {
    level = FlightAttitude.Across(FlightUtils.Up(), fwd);
    if Vector4.Length(level) < 0.2 {
      return false;
    }
    level = Vector4.Normalize(level);
    side = Vector4.Cross(fwd, level);
    return true;
  }

  private static func WrapAngle(a: Float) -> Float {
    let x = a;
    while x > 3.1415927 {
      x -= 6.2831853;
    }
    while x < -3.1415927 {
      x += 6.2831853;
    }
    return x;
  }

  // The wanted up vector with its bank eased: critically damped towards it, the roll rate and roll
  // acceleration limited.
  private func EaseBank(dt: Float, fwd: Vector4, up: Vector4) -> Vector4 {
    let level: Vector4;
    let side: Vector4;
    if !FlightModeAutopilot.BankFrame(fwd, level, side) {
      this.bankRateNow = 0.0;
      return up;
    }
    let wanted = AtanF(Vector4.Dot(up, side), Vector4.Dot(up, level));
    let error = FlightModeAutopilot.WrapAngle(wanted - this.bankAngle);
    let f = this.raceRollFrequency;
    let accelMax = this.raceRollAccelMax;
    let rateMax = this.raceRollRateMax;
    let rollAccel = ClampF(f * f * error - 2.0 * f * this.bankRateNow, -accelMax, accelMax);
    this.bankRateNow = ClampF(this.bankRateNow + rollAccel * dt, -rateMax, rateMax);
    this.bankAngle = FlightModeAutopilot.WrapAngle(this.bankAngle + this.bankRateNow * dt);
    return level * CosF(this.bankAngle) + side * SinF(this.bankAngle);
  }

  // In Race: the nose's pitch eased (critically damped, rate and acceleration limited), on the
  // nose's own heading. On a line the pitch wanted is the line's slope ahead; off it (the
  // take-off and the arrival) the nose's own. Elsewhere the nose stays as it is.
  private func EasePitch(dt: Float, fwd: Vector4) -> Vector4 {
    let level = Vector4.Normalize2D(fwd);
    if Vector4.Length(level) < 0.5 {
      this.SyncPitch(fwd);
      return fwd;
    }
    let wanted = AsinF(ClampF(fwd.Z, -1.0, 1.0));
    if this.onLine {
      let tangent: Vector4;
      let bend: Vector4;
      this.LineFrameOver(this.closestS + this.component.stats.d_speed * this.ribbonLead, this.ribbonSpan, tangent, bend);
      // the same steep-path rule as the nose off the line (the line's own slope on the final drop
      // to a landing spot would hold the nose at the ground)
      let slope = AsinF(ClampF(tangent.Z, -1.0, 1.0));
      let along = this.Unsteep(level * CosF(slope) + FlightUtils.Up() * SinF(slope));
      wanted = AsinF(ClampF(along.Z, -1.0, 1.0));
    }
    let f = this.racePitchFrequency;
    let accel = ClampF(f * f * (wanted - this.pitchAngle) - 2.0 * f * this.pitchRateNow, -this.racePitchAccelMax, this.racePitchAccelMax);
    this.pitchRateNow = ClampF(this.pitchRateNow + accel * dt, -this.racePitchRateMax, this.racePitchRateMax);
    this.pitchAngle = ClampF(this.pitchAngle + this.pitchRateNow * dt, -1.5, 1.5);
    return level * CosF(this.pitchAngle) + FlightUtils.Up() * SinF(this.pitchAngle);
  }

  // A steep path (the drop onto a landing spot): the nose nearer level instead of pointing at the
  // ground, where which way is up has no answer and the bank flicked across +-80 degrees.
  private func Unsteep(fwd: Vector4) -> Vector4 {
    let steep = ClampF((AbsF(fwd.Z) - 0.6) / 0.3, 0.0, 1.0);
    if steep > 0.0 {
      return Vector4.Normalize(fwd * (1.0 - 0.6 * steep) + this.headingRef * (0.6 * steep));
    }
    return fwd;
  }

  // FlightAttitudeControl with the params and the state filled from the fields; the inputs are the caller's.
  // AttitudeControlBack copies the state back.
  private func AttitudeControl() -> ref<FlightAttitudeControl> {
    if !IsDefined(this.attitudeControl) {
      this.attitudeControl = new FlightAttitudeControl();
    }
    let c = this.attitudeControl;
    let fs = FlightSettings.GetInstance();
    c.raceRollFrequency = this.raceRollFrequency;
    c.raceRollRateMax = this.raceRollRateMax;
    c.raceRollAccelMax = this.raceRollAccelMax;
    c.racePitchFrequency = this.racePitchFrequency;
    c.racePitchRateMax = this.racePitchRateMax;
    c.racePitchAccelMax = this.racePitchAccelMax;
    c.attitudeFrequency = this.attitudeFrequency;
    c.attitudeDamping = this.attitudeDamping;
    c.maxAngularAccel = this.maxAngularAccel;
    c.torqueGainPitch = this.torqueGainPitch;
    c.torqueGainRoll = this.torqueGainRoll;
    c.torqueGainYaw = this.torqueGainYaw;
    c.generalDampFactorAngular = fs.generalDampFactorAngular;
    c.generalYawDampFactor = fs.generalYawDampFactor;
    c.generalDampFactorAngularMax = fs.generalDampFactorAngularMax;
    c.standardModeSurgeFactor = this.standardModeSurgeFactor;
    c.standardModeSwayFactor = this.standardModeSwayFactor;
    c.standardModeLiftFactor = this.standardModeLiftFactor;
    c.gravityFactor = this.gravityFactor;
    c.targetRight = this.targetRight;
    c.targetForward = this.targetForward;
    c.targetUp = this.targetUp;
    c.omegaRef = this.omegaRef;
    c.bankAngle = this.bankAngle;
    c.bankRateNow = this.bankRateNow;
    c.pitchAngle = this.pitchAngle;
    c.pitchRateNow = this.pitchRateNow;
    return c;
  }

  private func AttitudeControlBack(c: ref<FlightAttitudeControl>) -> Void {
    this.targetRight = c.targetRight;
    this.targetForward = c.targetForward;
    this.targetUp = c.targetUp;
    this.omegaRef = c.omegaRef;
    this.bankAngle = c.bankAngle;
    this.bankRateNow = c.bankRateNow;
    this.pitchAngle = c.pitchAngle;
    this.pitchRateNow = c.pitchRateNow;
  }

  // EaseBank in C++: the same wanted up vector, the bank state going in and coming back through the fields.
  private func EaseBankNative(dt: Float, fwd: Vector4, up: Vector4) -> Vector4 {
    let c = this.AttitudeControl();
    let eased = c.EaseBank(dt, fwd, up);
    this.AttitudeControlBack(c);
    return eased;
  }

  // EasePitch in C++. The line's tangent is worked out here, as in the script (LineFrameOver is a nav map query),
  // and only on a line.
  private func EasePitchNative(dt: Float, fwd: Vector4) -> Vector4 {
    let tangent: Vector4;
    if this.onLine {
      let bend: Vector4;
      this.LineFrameOver(this.closestS + this.component.stats.d_speed * this.ribbonLead, this.ribbonSpan, tangent, bend);
    }
    let c = this.AttitudeControl();
    let eased = c.EasePitch(dt, fwd, this.onLine, tangent, this.headingRef);
    this.AttitudeControlBack(c);
    return eased;
  }

  private func SyncPitch(fwd: Vector4) -> Void {
    this.pitchAngle = AsinF(ClampF(fwd.Z, -1.0, 1.0));
    this.pitchRateNow = 0.0;
  }

  private func SyncBank(fwd: Vector4, up: Vector4) -> Void {
    let level: Vector4;
    let side: Vector4;
    if FlightModeAutopilot.BankFrame(fwd, level, side) {
      this.bankAngle = AtanF(Vector4.Dot(up, side), Vector4.Dot(up, level));
    }
    this.bankRateNow = 0.0;
  }

  // The landing turn (droneBrakeTurn): the nose keeps to the line until droneLandSlewBefore
  // before the descent by the trajectory's clock, then turns to the landing heading. Returns the
  // yaw wanted, wantedYaw or the turn's.
  private func ChooseYaw(dt: Float, live: Bool, wantedYaw: Float) -> Float {
    if this.droneBrakeTurn && this.landSlewOn && (Equals(this.status, "descending") || Equals(this.status, "landed")) {
      // the landing turn runs on droneLandSlewDuring into the descent
      this.landSlewDescent += dt;
      wantedYaw = this.LandSlewYaw(MaxF(this.droneLandSlewDuring - this.landSlewDescent, 0.0), dt, this.droneYaw);
    } else if this.droneBrakeTurn && this.FollowsTrajectory() && Equals(this.status, "following") {
      let timeLeft = this.landing ? this.TimeToDescent() : 1000.0;
      if this.landSlewOn || timeLeft <= this.droneLandSlewBefore {
        wantedYaw = this.LandSlewYaw(timeLeft + this.droneLandSlewDuring, dt, live ? this.droneYaw : wantedYaw);
      }
    }
    return wantedYaw;
  }

  // Drone style's attitude target: the nose level, along the flight path (the heading at hover
  // speeds, blend as in FlyAerobatic), the body tilted on each axis towards the thrust direction
  // the commanded acceleration and gravity need (DroneBodyAngle). FlyAerobatic's thrust then gives
  // that acceleration in the car's frame: what the tilt leaves along the nose and across goes to
  // surge and sway, which swivel the thrusters.
  private func DroneAttitude(dt: Float, accel: Vector4, blend: Float, live: Bool, out fwd: Vector4, out up: Vector4) -> Void {
    if this.nativeAttitude {
      this.DroneAttitudeNative(dt, accel, blend, live, fwd, up);
      return;
    }
    let stats = this.component.stats;
    let z = FlightUtils.Up();
    let g = 9.81 * this.gravityFactor;
    // where the nose points: on a line along the line a moment ahead, which is smooth by
    // construction; off it the velocity's horizontal part. That part is small and noisy on a
    // steep climb or drop (the nose twitches): it holds the last heading where the direction is
    // near vertical
    // (Flat returns a unit vector: the horizontal part with its length is taken by hand)
    let ahead3 = stats.d_velocity + accel * this.attitudeLead;
    let path = new Vector4(ahead3.X, ahead3.Y, 0.0, 0.0);
    let pathWeight = ClampF(Vector4.Length(path) / 6.0, 0.0, 1.0);
    if this.onLine {
      let tangent: Vector4;
      let bend: Vector4;
      this.LineFrameOver(this.closestS + stats.d_speed * this.droneYawLead, this.lineSpan, tangent, bend);
      path = FlightModeAutopilot.Flat(tangent);
      // turned into the bend as the trajectory plans (its drift), so the surge pushes round it
      if this.FollowsTrajectory() && Equals(this.status, "following") {
        let drift = this.trajectory.Drift(this.closestS + stats.d_speed * this.droneYawLead);
        path = new Vector4(path.X * CosF(drift) - path.Y * SinF(drift), path.X * SinF(drift) + path.Y * CosF(drift), 0.0, 0.0);
      }
      pathWeight = ClampF((Vector4.Length(path) - 0.15) / 0.35, 0.0, 1.0);
    }
    // coming down onto the spot the nose keeps to the hover heading (the landing turn's end), not
    // the drift: at 7 m/s it swung 50 degrees back towards it as the descent took over
    if Equals(this.status, "descending") || Equals(this.status, "landed") {
      pathWeight = 0.0;
    }
    let previous = live ? new Vector4(CosF(this.droneYaw), SinF(this.droneYaw), 0.0, 0.0) : this.headingRef;
    let h = this.headingRef * (1.0 - blend);
    if Vector4.Length(path) > 0.01 {
      h += (Vector4.Normalize(path) * pathWeight + previous * (1.0 - pathWeight)) * blend;
    }
    h = Vector4.Length(h) > 0.1 ? Vector4.Normalize(h) : this.headingRef;
    let wantedYaw = AtanF(h.Y, h.X);
    wantedYaw = this.ChooseYaw(dt, live, wantedYaw);
    // the heading eased like the tilt (EaseAngle): rate and acceleration limited
    if !live {
      this.droneYaw = wantedYaw;
      this.droneYawRate = 0.0;
    }
    let yawRate: Float;
    this.droneYaw = FlightModeAutopilot.WrapAngle(FlightModeAutopilot.EaseAngle(dt, this.droneYaw + FlightModeAutopilot.WrapAngle(wantedYaw - this.droneYaw), this.droneYaw,
      this.droneYawRate, this.droneYawFrequency, this.droneYawRateMax, this.droneYawAccelMax, yawRate));
    this.droneYawRate = yawRate;
    h = new Vector4(CosF(this.droneYaw), SinF(this.droneYaw), 0.0, 0.0);
    // the hover heading starts from the nose: below 12 m/s the nose blends to it, and left behind
    // it swung the nose 70 degrees back at the end of the landing turn
    this.headingRef = h;
    let side = Vector4.Normalize(Vector4.Cross(h, z));
    this.attAccel += (accel - this.attAccel) * ClampF(dt / this.droneTiltTime, 0.0, 1.0);
    let drag = this.surgeHold.Drag(this, stats.d_velocity);
    let thrust = this.attAccel + drag + z * g;
    // bending down, the tilt as if the lift still held the car up (droneTiltFloor): the lift pushes less
    // or down and the thrusters take the rest, rather than the body rolling onto its side
    thrust.Z = MaxF(thrust.Z, this.droneTiltFloor * g);
    let size = Vector4.Length(thrust);
    let ahead = Vector4.Dot(thrust, h);
    // braking: reverse surge and the brake (RaceDecel), much more than forward surge
    let along = ahead >= 0.0 ? this.standardModeSurgeFactor : this.RaceDecel(stats.d_speed) - this.DragAccel(stats.d_speed);
    let cap = Deg2Rad(this.droneMaxTilt);
    let pitch = ClampF(this.DroneBodyAngle(AtanF(ahead, thrust.Z), size, along, ahead >= 0.0 ? this.droneBodyShare : this.droneBrakeShare), -cap, cap);
    // following a route the plan's tilt, which holds the roll for the turn coming (a gentle bend then
    // a sharp one in the same direction is flown as one turn, not two); hovering and descending the
    // tilt the commanded acceleration needs
    let lateral = Vector4.Dot(thrust, side);
    let roll = ClampF(this.DroneBodyAngle(AtanF(lateral, thrust.Z), size, this.standardModeSwayFactor, this.droneBodyShare), -cap, cap);
    if this.FollowsTrajectory() && Equals(this.status, "following") {
      // the planned tilt a moment ahead, plus what the follower's correction adds to the trajectory's
      // own acceleration (both tilts worked out the same way, so on the trajectory that is 0)
      this.attRef += (this.trajectory.Acceleration(this.closestS) - this.attRef) * ClampF(dt / this.droneTiltTime, 0.0, 1.0);
      let thrustRef = this.attRef + drag + z * g;
      thrustRef.Z = MaxF(thrustRef.Z, this.droneTiltFloor * g);
      let sizeRef = Vector4.Length(thrustRef);
      let aheadRef = Vector4.Dot(thrustRef, h);
      let alongRef = aheadRef >= 0.0 ? this.standardModeSurgeFactor : this.RaceDecel(stats.d_speed) - this.DragAccel(stats.d_speed);
      let pitchRef = ClampF(this.DroneBodyAngle(AtanF(aheadRef, thrustRef.Z), sizeRef, alongRef, aheadRef >= 0.0 ? this.droneBodyShare : this.droneBrakeShare), -cap, cap);
      let rollRef = ClampF(this.DroneBodyAngle(AtanF(Vector4.Dot(thrustRef, side), thrustRef.Z), sizeRef, this.standardModeSwayFactor, this.droneBodyShare), -cap, cap);
      let sAtt = this.closestS + stats.d_speed * this.trajectoryAttitudeLead;
      pitch = ClampF(this.trajectory.Pitch(sAtt) + pitch - pitchRef, -cap, cap);
      // the plan's roll is towards (h.y, -h.x), the heading turned right; side may point the other way
      let flip = Vector4.Dot(side, new Vector4(h.Y, -h.X, 0.0, 0.0)) >= 0.0 ? 1.0 : -1.0;
      roll = ClampF(flip * this.trajectory.Roll(sAtt) + roll - rollRef, -cap, cap);
    } else {
      this.attRef = this.attAccel;
    }
    // taking off: a little lean forward, as a player tips the nose into the surge
    if this.droneTakeoffTilt > 0.0 && this.FollowsTrajectory() && Equals(this.status, "following")
      && this.closestS < this.droneTakeoffLength && this.trajectory.Along(this.closestS) > 0.0 {
      let into = ClampF(stats.d_speed / MaxF(this.droneTakeoffIn, 0.1), 0.0, 1.0);
      let out = ClampF((this.droneTakeoffOut - stats.d_speed) / MaxF(this.droneTakeoffOut - this.droneTakeoffFull, 0.1), 0.0, 1.0);
      let w = into * into * (3.0 - 2.0 * into) * out * out * (3.0 - 2.0 * out);
      pitch = MaxF(pitch, Deg2Rad(this.droneTakeoffTilt) * w);
    }
    // eased as the plane's pitch and bank are (EasePitch, EaseBank): followed directly, the tilt
    // flicks with every change of the commanded acceleration and the car overshoots it. What the
    // tilt lags by, the thrust in the car's frame (FlyAerobatic) still gives, up to the surge and
    // sway limits.
    if !live {
      this.dronePitch = pitch;
      this.droneRoll = roll;
      this.dronePitchRate = 0.0;
      this.droneRollRate = 0.0;
    }
    let rate: Float;
    this.dronePitch = FlightModeAutopilot.EaseAngle(dt, pitch, this.dronePitch, this.dronePitchRate, this.racePitchFrequency, this.racePitchRateMax, this.racePitchAccelMax, rate);
    this.dronePitchRate = rate;
    this.droneRoll = FlightModeAutopilot.EaseAngle(dt, roll, this.droneRoll, this.droneRollRate, this.raceRollFrequency, this.raceRollRateMax, this.raceRollAccelMax, rate);
    this.droneRollRate = rate;
    pitch = this.dronePitch;
    roll = this.droneRoll;
    // top towards the thrust: tilted forward is nose down, as a quadcopter speeding up
    up = Vector4.Normalize(z + h * TanF(pitch) + side * TanF(roll));
    fwd = Vector4.Normalize(FlightAttitude.Across(h, up));
  }

  // The follower's params, the inputs it shares between Track and Command and the state, from the fields
  // (the surge hold's dwell timer stays in the tracker: AutopilotSurge keeps it private).
  private func FillTracker(dt: Float, p: Vector4, v: Vector4) -> Void {
    let stats = this.component.stats;
    let t = this.tracker;
    t.replanOffLine = this.replanOffLine;
    t.replanDistance = this.replanDistance;
    t.cornerSpan = this.cornerSpan;
    t.cornerOvershoot = this.cornerOvershoot;
    t.collisionHorizon = this.collisionHorizon;
    t.raceOffLine = this.raceOffLine;
    t.lineLead = this.lineLead;
    t.speedGain = this.speedGain;
    t.brakeAccel = this.brakeAccel;
    t.brakeMax = this.brakeMax;
    t.velocityGain = this.velocityGain;
    t.accelJerk = this.accelJerk;
    t.raceJerk = this.raceJerk;
    t.lineSpring = this.lineSpring;
    t.lineVelocityGain = this.lineVelocityGain;
    t.lineSpan = this.lineSpan;
    t.trajectoryCorrectionMax = this.trajectoryCorrectionMax;
    t.slack = this.surgeHold.slack;
    t.dwell = this.surgeHold.dwell;
    t.fullSurge = this.surgeHold.fullSurge;
    t.dt = dt;
    t.speed = stats.d_speed;
    t.replanTimer = this.replanTimer;
    t.position = p;
    t.velocity = v;
    t.forward = stats.d_forward;
    t.routeLine = this.routeLine;
    t.planned = this.FollowsTrajectory();
    t.recovering = this.recovering > 0.0;
    t.requestPending = this.request != 0;
    t.segment = this.segment;
    t.closestS = this.closestS;
    t.offRouteNow = this.offRouteNow;
    t.remaining = this.remaining;
    t.aheadSmooth = this.aheadSmooth;
    t.curvatureAhead = this.curvatureAhead;
    t.blockedTimer = this.blockedTimer;
    t.targetSpeed = this.targetSpeed;
    t.speedCmd = this.speedCmd;
    t.speedRate = this.speedRate;
    t.closestNow = this.closestNow;
    t.pathCurvature = this.pathCurvature;
    t.accelCmd = this.accelCmd;
    t.blockedAhead = this.blockedAhead;
    t.emergencyBrake = this.emergencyBrake;
    t.surgeHeld = this.surgeHold.held;
    t.surgeChanging = this.surgeHold.changing;
  }

  // The follower's state back into the fields.
  private func TrackerOut() -> Void {
    let t = this.tracker;
    this.segment = t.segment;
    this.closestS = t.closestS;
    this.offRouteNow = t.offRouteNow;
    this.remaining = t.remaining;
    this.aheadSmooth = t.aheadSmooth;
    this.curvatureAhead = t.curvatureAhead;
    this.blockedTimer = t.blockedTimer;
    this.targetSpeed = t.targetSpeed;
    this.speedCmd = t.speedCmd;
    this.speedRate = t.speedRate;
    this.closestNow = t.closestNow;
    this.pathCurvature = t.pathCurvature;
    this.accelCmd = t.accelCmd;
    this.blockedAhead = t.blockedAhead;
    this.emergencyBrake = t.emergencyBrake;
    this.surgeHold.held = t.surgeHeld;
    this.surgeHold.changing = t.surgeChanging;
  }

  // DroneAttitude in C++ (FlightDroneAttitude, Navigation/DroneAttitude.hpp): the params, the inputs and
  // the state go in from the fields, the state comes back to them, so everything that reads them and the
  // plane path are as with the script. The landing turn's choice stays here (ChooseYaw).
  private func DroneAttitudeNative(dt: Float, accel: Vector4, blend: Float, live: Bool, out fwd: Vector4, out up: Vector4) -> Void {
    let stats = this.component.stats;
    let a = this.attitude;
    a.droneBodyShare = this.droneBodyShare;
    a.droneTiltTime = this.droneTiltTime;
    a.attitudeLead = this.attitudeLead;
    a.droneYawLead = this.droneYawLead;
    a.lineSpan = this.lineSpan;
    a.droneYawFrequency = this.droneYawFrequency;
    a.droneYawRateMax = this.droneYawRateMax;
    a.droneYawAccelMax = this.droneYawAccelMax;
    a.racePitchFrequency = this.racePitchFrequency;
    a.racePitchRateMax = this.racePitchRateMax;
    a.racePitchAccelMax = this.racePitchAccelMax;
    a.raceRollFrequency = this.raceRollFrequency;
    a.raceRollRateMax = this.raceRollRateMax;
    a.raceRollAccelMax = this.raceRollAccelMax;
    a.trajectoryAttitudeLead = this.trajectoryAttitudeLead;
    a.droneTakeoffTilt = this.droneTakeoffTilt;
    a.droneTakeoffIn = this.droneTakeoffIn;
    a.droneTakeoffFull = this.droneTakeoffFull;
    a.droneTakeoffOut = this.droneTakeoffOut;
    a.droneTakeoffLength = this.droneTakeoffLength;
    a.dragFeedForward = this.surgeHold.dragFeedForward;
    a.dt = dt;
    a.blend = blend;
    a.speed = stats.d_speed;
    a.closestS = this.closestS;
    a.accel = accel;
    a.velocity = stats.d_velocity;
    a.forward = stats.d_forward;
    a.live = live;
    a.onLine = this.onLine;
    a.following = this.FollowsTrajectory() && Equals(this.status, "following");
    a.descending = Equals(this.status, "descending") || Equals(this.status, "landed");
    a.droneYaw = this.droneYaw;
    a.droneYawRate = this.droneYawRate;
    a.dronePitch = this.dronePitch;
    a.dronePitchRate = this.dronePitchRate;
    a.droneRoll = this.droneRoll;
    a.droneRollRate = this.droneRollRate;
    a.attAccel = this.attAccel;
    a.attRef = this.attRef;
    a.headingRef = this.headingRef;
    let wantedYaw = this.ChooseYaw(dt, live, a.HeadingYaw(this.route, this.trajectory));
    a.Step(wantedYaw, this.route, this.trajectory, this.Vehicle());
    this.droneYaw = a.droneYaw;
    this.droneYawRate = a.droneYawRate;
    this.dronePitch = a.dronePitch;
    this.dronePitchRate = a.dronePitchRate;
    this.droneRoll = a.droneRoll;
    this.droneRollRate = a.droneRollRate;
    this.attAccel = a.attAccel;
    this.attRef = a.attRef;
    this.headingRef = a.headingRef;
    fwd = a.fwd;
    up = a.up;
  }

  // An angle eased towards wanted: critically damped at frequency f, its rate and acceleration
  // limited (EaseBank and EasePitch do the same for the plane). Returns the new angle, the new
  // rate in rateOut.
  private static func EaseAngle(dt: Float, wanted: Float, angle: Float, rateIn: Float, f: Float, rateMax: Float, accelMax: Float, out rateOut: Float) -> Float {
    let accel = ClampF(f * f * (wanted - angle) - 2.0 * f * rateIn, -accelMax, accelMax);
    rateOut = ClampF(rateIn + accel * dt, -rateMax, rateMax);
    return angle + rateOut * dt;
  }

  // The body's tilt on one axis for a thrust direction `angle` from vertical: droneBodyShare of it,
  // or more where the thrusters (`thrusters` m/s2 across the body at most, raceMargin of it planned)
  // cannot point the rest of `thrust` m/s2. Sway is 5 m/s2, so turns lean mostly on the body. The formula is
  // Navigation::DroneBodyAngle (Vehicle.hpp).
  private func DroneBodyAngle(angle: Float, thrust: Float, thrusters: Float, share: Float) -> Float {
    return this.Vehicle().DroneBodyAngle(angle, thrust, thrusters, share);
  }

  // Drone style's envelope: the most horizontal acceleration level flight leaves along one body
  // axis, the body tilted at most droneMaxTilt with `thrusters` m/s2 across it and the lift (at most
  // g + standardModeLiftFactor) holding the car up.
  private func DroneAccel(thrusters: Float) -> Float {
    return this.Vehicle().DroneAccel(thrusters);
  }

  // Flies the commanded acceleration with the car's attitude as a full 3D target: the nose along
  // the flight path (the heading at hover speeds), the top towards the turn and away from gravity
  // (a coordinated turn in any direction), or a manoeuvre's own attitude. Thrust is applied along
  // whichever way the car points, so the attitude never costs the path anything.
  private func FlyAerobatic(timeDelta: Float, accel: Vector4, heading: Vector4) -> Void {
    let stats = this.component.stats;
    let v = stats.d_velocity;
    let z = FlightUtils.Up();
    let dt = MaxF(timeDelta, 0.001);

    // heading at hover speeds: towards the wanted one, eased and at most maxHeadingRate
    if Vector4.Length(heading) > 2.0 {
      let want = Vector4.Normalize2D(heading);
      let angle = AcosF(ClampF(Vector4.Dot(want, this.headingRef), -1.0, 1.0));
      if angle > 0.0001 {
        let f = MinF(ClampF(dt / this.yawSmoothing, 0.0, 1.0), this.maxHeadingRate * dt / angle);
        this.headingRef = FlightModeAutopilot.BlendHeading(this.headingRef, want, f);
      }
    }

    // drone style's eased tilt starts from the wanted one when it was not flown last frame
    let droneLive = this.droneEaseLive;
    this.droneEaseLive = false;
    // a manoeuvre's attitude is taken attitudeDelay ahead: the car answers torque two frames late
    let ahead = IsDefined(this.maneuver) ? MinF(this.maneuver.time + this.attitudeDelay, this.maneuver.duration) : 0.0;
    // the nose follows the flight path from a lower speed than in level flight, or a manoeuvre
    // starts with the nose well off its path
    let blend = ClampF((stats.d_speed - 4.0) / 8.0, 0.0, 1.0);
    blend = blend * blend * (3.0 - 2.0 * blend);
    let fwd: Vector4;
    let up: Vector4;
    if IsDefined(this.maneuver) && !this.maneuver.IsOverlay() {
      let refPos: Vector4;
      let refVel: Vector4;
      let refAcc: Vector4;
      this.maneuver.Sample(ahead, refPos, refVel, refAcc, fwd, up);
      this.SyncPitch(fwd);
    } else if !IsDefined(this.maneuver) {
      this.DroneAttitude(dt, accel, blend, droneLive, fwd, up);
      this.droneEaseLive = true;
    } else {
      // nose: the velocity plus a little of the commanded acceleration, so it leads into turns,
      // climbs and dives
      let path = v + accel * this.attitudeLead;
      fwd = this.headingRef * (1.0 - blend);
      if Vector4.Length(path) > 1.0 {
        fwd += Vector4.Normalize(path) * blend;
      }
      fwd = Vector4.Length(fwd) > 0.1 ? Vector4.Normalize(fwd) : this.headingRef;
      // nearer level on a steep path, then eased
      fwd = this.nativeAerobatics ? this.EasePitchNative(dt, this.Unsteep(fwd)) : this.EasePitch(dt, this.Unsteep(fwd));
      this.attAccel += (accel - this.attAccel) * ClampF(dt / this.raceBankTime, 0.0, 1.0);
      let bankAccel = this.attAccel;
      if this.onLine {
        // the ribbon: the acceleration the line's bend needs ahead, at the speed flown
        let ribbonTangent: Vector4;
        let ribbonBend: Vector4;
        this.LineFrameOver(this.closestS + stats.d_speed * this.ribbonLead, this.ribbonSpan, ribbonTangent, ribbonBend);
        bankAccel = ribbonBend * (stats.d_speed * stats.d_speed);
        this.ribbonNeed = bankAccel;
        // level when slow: the final drop to a landing spot flicked the bank across +-80 degrees
        bankAccel *= ClampF((stats.d_speed - this.ribbonSlow) / 15.0, 0.0, 1.0);
        let need = Vector4.Length(bankAccel);
        bankAccel = need > this.ribbonDeadband ? bankAccel * ((need - this.ribbonDeadband) / need) : new Vector4(0.0, 0.0, 0.0, 0.0);
      }
      // pushing over: past raceTopDown the top would turn towards the ground and the car roll
      // over; it stays upright and the thrust (negative lift) gives the rest
      bankAccel.Z = MaxF(bankAccel.Z, -this.raceTopDown * 9.81 * this.gravityFactor);
      let u = FlightAttitude.Across(bankAccel * blend + z * (9.81 * this.gravityFactor), fwd);
      let held = FlightAttitude.Across(this.targetUp, fwd);
      if Vector4.Length(u) < 1.0 {
        u = held;
      }
      // with the path near vertical, gravity says nothing about which way the top should face and
      // small sideways accelerations swing it from frame to frame: hold the top where it was instead
      let vertical = ClampF((AbsF(fwd.Z) - 0.7) / 0.25, 0.0, 1.0);
      if vertical > 0.0 && Vector4.Length(held) > 0.01 {
        u = Vector4.Normalize(u) * (1.0 - vertical) + Vector4.Normalize(held) * vertical;
      }
      up = this.nativeAerobatics ? this.EaseBankNative(dt, fwd, Vector4.Normalize(u)) : this.EaseBank(dt, fwd, Vector4.Normalize(u));
      if IsDefined(this.maneuver) {
        if Equals(this.maneuver.kind, FlightManeuverKind.AileronRoll) {
          up = this.maneuver.RollUp(ahead, fwd, up);
        } else {
          // barrel roll: top towards the centre of the circle round the route
          let offset: Vector4;
          let offsetVelocity: Vector4;
          let barrelUp: Vector4;
          this.maneuver.BarrelOffset(ahead, offset, offsetVelocity, barrelUp);
          let across = FlightAttitude.Across(barrelUp, fwd);
          if Vector4.Length(across) > 0.5 {
            up = Vector4.Normalize(across);
          }
        }
      } else if this.onLine && this.raceDriftMax > 0.0 {
        // drift: the nose turned into the bend about the car's top, more the harder the bend
        let inward = FlightAttitude.Across(FlightAttitude.Across(this.ribbonNeed, fwd), up);
        let need = Vector4.Length(inward);
        if need > this.raceDriftStart {
          let f = ClampF((need - this.raceDriftStart) / MaxF(this.raceDriftFull - this.raceDriftStart, 0.1), 0.0, 1.0);
          let drift = Deg2Rad(this.raceDriftMax) * f * f * (3.0 - 2.0 * f);
          fwd = Vector4.Normalize(fwd * CosF(drift) + Vector4.Normalize(inward) * SinF(drift));
        }
      }
    }
    let torque: Vector4;
    let local: Vector4;
    let g: Float;
    let rate: Vector4;
    let rateRef: Vector4;
    if this.nativeAerobatics {
      // the target frame, the torque and the thrust in C++ (Navigation::AttitudeControl)
      let c = this.AttitudeControl();
      c.dt = dt;
      c.fwd = fwd;
      c.up = up;
      c.shape = IsDefined(this.maneuver) && !this.maneuver.IsOverlay();
      c.accel = accel;
      c.drag = this.surgeHold.Drag(this, v);
      c.dragAccel = this.DragAccel(stats.d_speed);
      c.raceDecel = this.RaceDecel(stats.d_speed);
      c.thrusterTensor = this.component.thrusterTensor;
      c.inertiaX = this.component.GetWorldInertiaRow(0);
      c.inertiaY = this.component.GetWorldInertiaRow(1);
      c.inertiaZ = this.component.GetWorldInertiaRow(2);
      c.Step(stats);
      this.AttitudeControlBack(c);
      this.attitudeError = c.attitudeError;
      torque = c.torque;
      local = c.thrust;
      rateRef = c.rateRef;
      fwd = c.targetForward;
      up = c.targetUp;
      rate = stats.d_angularVelocity;
      rate.W = 0.0;
      g = 9.81 * this.gravityFactor;
    } else {
      let right = Vector4.Normalize(Vector4.Cross(fwd, up));
      up = Vector4.Cross(right, fwd);
      if IsDefined(this.maneuver) && !this.maneuver.IsOverlay() {
        // a shape sets its own attitude: carry on the bank easing from where it leaves the car
        this.SyncBank(fwd, up);
      }

      // rate of the target, smoothed (it is what the car should be turning at)
      let step = FlightAttitude.RotationError(this.targetRight, this.targetForward, this.targetUp, right, fwd, up);
      let omega = (this.targetRight * step.X + this.targetForward * step.Y + this.targetUp * step.Z) * (1.0 / dt);
      this.omegaRef += (omega - this.omegaRef) * ClampF(dt / 0.05, 0.0, 1.0);
      this.targetRight = right;
      this.targetForward = fwd;
      this.targetUp = up;

      // attitude control in the body frame: angular acceleration from the error and the rate error,
      // then the torque that gives it on each axis
      let error = FlightAttitude.RotationError(stats.d_right, stats.d_forward, stats.d_up, right, fwd, up);
      this.attitudeError = Vector4.Length(error);
      rateRef = new Vector4(Vector4.Dot(this.omegaRef, stats.d_right), Vector4.Dot(this.omegaRef, stats.d_forward), Vector4.Dot(this.omegaRef, stats.d_up), 0.0);
      rate = stats.d_angularVelocity;
      rate.W = 0.0;
      let alpha = error * (this.attitudeFrequency * this.attitudeFrequency) + (rateRef - rate) * (2.0 * this.attitudeDamping * this.attitudeFrequency);
      if Vector4.Length(alpha) > this.maxAngularAccel {
        alpha = Vector4.Normalize(alpha) * this.maxAngularAccel;
      }
      let mass = stats.s_mass;
      let inertia = stats.s_momentOfInertia;
      let tensor = this.component.thrusterTensor;
      // FlightComponent turns the mode torque into an angular impulse: x dt, x mass, x the thruster
      // tensor per axis, into the world frame; PhysX applies it with the body's world inertia. So the
      // torque for alpha is that inertia x alpha, back through the same steps. The per-axis gains
      // below were measured on one car (an Archer Quartz) and are only the fallback without PhysX-owned
      // flight (on other cars they come out too stiff and shake).
      let impulse = this.component.AngularImpulseFor(stats.d_orientation * alpha);
      if impulse.W > 0.5 {
        impulse.W = 0.0;
        let localImpulse = Quaternion.Conjugate(stats.d_orientation) * impulse;
        torque = new Vector4(
          localImpulse.X / MaxF(mass * tensor.X, 0.001),
          localImpulse.Y / MaxF(mass * MaxF(tensor.Y, 1.0), 0.001),
          localImpulse.Z / MaxF(mass * tensor.Z, 0.001), 0.0);
      } else {
        torque = new Vector4(
          alpha.X * inertia.X / MaxF(this.torqueGainPitch * mass * tensor.X, 0.001),
          alpha.Y * inertia.Y / MaxF(this.torqueGainRoll * mass * MaxF(tensor.Y, 1.0), 0.001),
          alpha.Z * inertia.Z / MaxF(this.torqueGainYaw * mass * tensor.Z, 0.001), 0.0);
      }
      // ApplyPhysics damps all rotation: give back what it takes from the rotation wanted
      let fs = FlightSettings.GetInstance();
      let giveBack = rateRef * fs.generalDampFactorAngular;
      giveBack.Z *= fs.generalYawDampFactor;
      if Vector4.Length(giveBack) > fs.generalDampFactorAngularMax {
        giveBack = Vector4.Normalize(giveBack) * fs.generalDampFactorAngularMax; // ApplyPhysics' own cap
      }
      torque += giveBack;

      // thrust: the commanded acceleration, the drag it holds off and gravity, in the car's frame
      local = Quaternion.Conjugate(stats.d_orientation) * (accel + this.surgeHold.Drag(this, v) + z * (9.81 * this.gravityFactor));
      // what the player's inputs give at most: surge along the nose (reverse surge plus the brake
      // when slowing), sway across, lift (on top of hover) along the top
      g = 9.81 * this.gravityFactor;
      local.X = ClampF(local.X, -this.standardModeSwayFactor, this.standardModeSwayFactor);
      local.Y = ClampF(local.Y, this.DragAccel(stats.d_speed) - this.RaceDecel(stats.d_speed), this.standardModeSurgeFactor);
      local.Z = ClampF(local.Z, g - this.standardModeLiftFactor, g + this.standardModeLiftFactor);
    }
    // where the speed is the car's to set, the surge held full along the path
    // (with nativeTracker FlightTracker.Command did it this frame, with the same inputs)
    if !this.nativeTracker {
      this.surgeHold.Update(this, this.closestS + stats.d_speed * this.lineLead, stats.d_speed,
        this.recovering <= 0.0 && stats.d_speed > 1.0 && this.FollowsTrajectory() && Equals(this.status, "following"), timeDelta);
    }
    if this.surgeHold.held {
      local = this.surgeHold.Full(local, Quaternion.Conjugate(stats.d_orientation) * Vector4.Normalize(v), this.standardModeSurgeFactor);
      local.X = ClampF(local.X, -this.standardModeSwayFactor, this.standardModeSwayFactor);
      local.Z = ClampF(local.Z, g - this.standardModeLiftFactor, g + this.standardModeLiftFactor);
    }
    this.force = local;
    this.thrustCmd = local;
    this.torque = torque;
    // inputs for audio and the thrusters' look; no yaw input, ApplyPhysics damps less on an axis
    // with input and the give-back above assumes full damping
    this.commandSurge = this.surgeHold.Ease(this.commandSurge, ClampF(local.Y / this.standardModeSurgeFactor, -1.0, 1.0), timeDelta);
    this.commandSway = this.surgeHold.Ease(this.commandSway, ClampF(local.X / this.standardModeSwayFactor, -1.0, 1.0), timeDelta);
    this.component.surge = this.commandSurge;
    this.component.sway = this.commandSway;
    this.component.yaw = 0.0;
    this.component.lift = 0.0;
    this.component.linearBrake = 0.0;
    this.component.angularBrake = 0.0;

    if this.telemetry {
      this.telemetryTime += timeDelta;
      let p = stats.d_position;
      FlightLog.Info("[AutopilotA] " + FloatToStringPrec(this.telemetryTime, 3) + " " + FloatToStringPrec(p.X, 2) + " " + FloatToStringPrec(p.Y, 2) + " " + FloatToStringPrec(p.Z, 2)
        + " " + FloatToStringPrec(v.X, 2) + " " + FloatToStringPrec(v.Y, 2) + " " + FloatToStringPrec(v.Z, 2)
        + " " + FloatToStringPrec(Rad2Deg(this.attitudeError), 2)
        + " " + FloatToStringPrec(rate.X, 3) + " " + FloatToStringPrec(rate.Y, 3) + " " + FloatToStringPrec(rate.Z, 3)
        + " " + FloatToStringPrec(rateRef.X, 3) + " " + FloatToStringPrec(rateRef.Y, 3) + " " + FloatToStringPrec(rateRef.Z, 3)
        + " " + FloatToStringPrec(up.X, 3) + " " + FloatToStringPrec(up.Y, 3) + " " + FloatToStringPrec(up.Z, 3)
        + " " + FloatToStringPrec(stats.d_up.X, 3) + " " + FloatToStringPrec(stats.d_up.Y, 3) + " " + FloatToStringPrec(stats.d_up.Z, 3)
        + " " + FloatToStringPrec(accel.X, 2) + " " + FloatToStringPrec(accel.Y, 2) + " " + FloatToStringPrec(accel.Z, 2)
        + " " + (IsDefined(this.maneuver) ? ToString(EnumInt(this.maneuver.kind)) + " " + FloatToStringPrec(this.maneuver.time, 2) : "0 0")
        + " " + this.status);
    }
    this.logTimer -= timeDelta;
    if this.logTimer <= 0.0 {
      this.logTimer = 1.0;
      FlightLog.Info("[Autopilot] " + this.status + (IsDefined(this.maneuver) ? " (" + this.maneuver.GetName() + " " + FloatToStringPrec(this.maneuver.time, 1) + " s)" : "")
        + " seg " + ToString(this.segment) + "/" + ToString(this.RouteSize())
        + " speed " + FloatToStringPrec(stats.d_speed, 1) + " target " + FloatToStringPrec(this.targetSpeed, 1)
        + " remaining " + FloatToStringPrec(this.remaining, 0) + " attitude error " + FloatToStringPrec(Rad2Deg(this.attitudeError), 1)
        + " up " + FloatToStringPrec(stats.d_up.Z, 2) + " z " + FloatToStringPrec(stats.d_position.Z, 1));
    }

    if IsDefined(this.maneuver) {
      this.maneuver.time += timeDelta;
      if this.maneuver.IsDone() {
        this.EndManeuver();
      }
    }
  }
}
