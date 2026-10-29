// Autopilot: flies the planned route to the GPS destination (or FlightNavMap's target override).
//
// Route: FlightNavMap.PlanAsync (C++, src/red4ext/Navigation/Planner.cpp) gives car-centre
// corner points over the baked nav map. Each frame:
//   - follow: closest point on the route, then a lookahead point further along it (distance grows
//     with speed), desired velocity towards it;
//   - speed: capped by max speed, by each corner ahead (v = sqrt(lateral accel x corner radius),
//     with room to brake for it) and by the distance left to the goal;
//   - control: desired acceleration -> yaw towards the desired heading, surge / brake along the
//     nose, a coordinated bank (atan(lateral accel / g)) and nose pitch along the climb at speed,
//     sway for corrections when slow, and lift for the vertical part.
// The outputs go into the same inputs the player drives (surge, yaw, sway, brake), so thrusters,
// audio and the standard attitude control react as usual. Any player input wins for that frame
// and holds the autopilot off briefly; the route is replanned if the car ends up far from it.
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

  // Manoeuvres (FlightAerobatics.reds: rolls, loop, Immelmann, split-S, as stunts on straights or
  // to turn round where the route doubles back) are disconnected since 2026-10-29 (Jack): nothing
  // starts one, and these are no longer Mod Settings. The code is kept.
  public let aerobatics: Bool = false;        // full 3D attitude (Race implies it)
  public let stuntInterval: Float = 0.0;      // s between stunts on long straights (unused while disconnected)

  // The Race flies like a quadcopter (DroneAttitude), the only style since 2026-10-29 (Jack): the
  // plane attitude in FlyAerobatic is no longer flown (manoeuvres aside) and is kept for reference.
  // Not a Mod Setting, and not switched anywhere.
  public let droneStyle: Bool = true;
  // drone style: hard stops swing the nose round instead of pulling back (not in Mod Settings:
  // LtbfBridge.DroneBrakeTurn; a Mod Setting is written back from its stored value when the mode
  // is engaged)
  public let droneBrakeTurn: Bool = false;

  // drone style (2026-10-28): the body tilts droneBodyShare of the way to the thrust direction the
  // commanded acceleration and gravity need, the surge and sway thrusters point the rest (Jack:
  // with lift and surge both there, 22.5 degrees of body for travelling at 45 degrees), so the
  // thrusters keep moving; no stunts
  public let droneBodyShare: Float = 0.5;
  public let droneMaxTilt: Float = 55.0;      // deg the body tilts at most, either axis (sets the speed envelope)
  public let droneTiltTime: Float = 0.2;      // s, smoothing of the acceleration the tilt follows
  // m/s2 bending the path downwards, in place of racePushOver: the body stays upright (DroneAttitude
  // keeps the thrust from pointing down) and the lift pushes down: gravity plus
  // (standardModeLiftFactor - g) of lift downwards, 25 m/s2, planned at raceMargin
  public let dronePushOver: Float = 21.0;
  public let droneYawLead: Float = 0.3;       // s along the racing line the nose aims (DroneAttitude)
  public let droneYawFrequency: Float = 3.0;  // rad/s, heading easing
  public let droneYawRateMax: Float = 1.5;    // rad/s
  public let droneYawAccelMax: Float = 5.0;   // rad/s2
  public let droneBrakeYaw: Float = 45.0;     // deg the nose swings towards the turn coming in a hard stop (droneBrakeTurn), at most
  public let droneLandSlewYaw: Float = 120.0; // deg the nose has come round by the end of a stop into the landing
  public let droneStopMinDrop: Float = 15.0;  // m/s the speed flown falls by in a stop (PlanStops)
  public let droneStopMinDecel: Float = 6.0;  // m/s2 of braking that counts towards a stop
  public let droneStopArrive: Float = 250.0;  // m before the route's end where the arrival's braking is part of a stop
  public let droneStopJoin: Float = 120.0;    // m between two stops that are flown as one
  public let droneStopTurnAhead: Float = 30.0; // m past a stop's end its turn is judged at
  public let droneStopLead: Float = 0.3;      // s ahead the planned slew is read
  public let droneSlewBendOff: Float = 0.0067;  // 1/m (a 150 m radius): the line bending more is a turn, flown unslewed (PlanStops)
  public let droneSlewMinStraight: Float = 80.0; // m a straight stretch of a stop must run to be slewed
  public let droneSlewMargin: Float = 30.0;   // m before a turn the slew is over, at least
  public let droneSlewRelease: Float = 0.8;   // s at the flown speed before a turn the slew is over, at least
  public let droneSlewHeadingOff: Float = 5.0; // deg the line may turn over a slewed stretch into a turn
  public let droneSlewMinTime: Float = 2.0;   // s at the flown speed a slewed stretch must last, at least
  public let speedHumpMerge: Float = 6.0;     // m/s a rise of the profile between two slowings may be at most to be held down (FlattenSpeedHumps)
  public let speedHumpSpan: Float = 150.0;    // m either side the slowings are looked for
  public let droneRollLook: Float = 1.5;     // s of the line ahead the roll is held for (DroneTurnAhead; 1.0 still eased 31 to 24 degrees before the Dogtown gap)
  public let droneRollStep: Float = 0.2;      // s between the points it looks at

  // Race is how the Autopilot flies (2026-10-27): the racing line at the vehicle's full thrust,
  // banking the lift into turns; implies aerobatics. No longer a Mod Setting (a saved "off" would
  // have kept players on the old way); false is the archived normal mode, straight legs between
  // the planner's waypoints with rounded corners.
  public let raceMode: Bool = true;

  // takeoff and landing
  public let takeoffHeight: Float = 10.0;     // m above the floor climbed straight up before flying off
  public let climbRate: Float = 5.0;          // m/s during the takeoff climb
  public let approachHeight: Float = 20.0;    // m above the landing spot the route ends at
  public let descentRate: Float = 4.0;        // m/s at most during the landing descent
  public let touchdownRate: Float = 0.8;      // m/s in the last metres
  public let landingSearch: Int32 = 12;       // rings of 2 m columns searched for a landing spot
  public let landingShaft: Float = 4.0;       // m around a landing spot that must be open above it too (ShaftClear)
  // m from the spot across within which the descent sinks: the clear shaft (ShaftClear), so the
  // car slides the last metres over the spot while coming down instead of stopping above it first
  public let landingOverSpot: Float = 4.0;
  // the descent takes over from the route this far from its end (m) and below this speed (m/s); it
  // keeps closing on the spot and sinks once within landingOverSpot. Taken over 4 m out, the car
  // hung still for 2 s above the spot first (2026-10-29)
  public let landingEntry: Float = 15.0;
  public let landingEntrySpeed: Float = 10.0;
  public let landingRecheck: Float = 300.0;   // m before the route's end the landing spot is checked for vehicles again
  public let landingShiftSpan: Float = 150.0; // m of the route's end moved over to a new spot
  public let spotCheckTimer: Float;
  public let padSnapDistance: Float = 25.0;   // m: a destination this close to a landing pad (and at its level) is on it
  public let padSnapLevel: Float = 6.0;       // m between the destination's height and the pad's deck

  // easing
  public let accelUp: Float = 7.0;            // m/s2 at most when speeding up
  public let brakeMax: Float = 9.0;           // m/s2 at most when slowing (planned at brakeAccel)
  public let speedGain: Float = 1.0;          // 1/s, speed error -> rate of change of the flown speed
  public let speedJerk: Float = 6.0;          // m/s3, how quickly that rate may change
  public let accelJerk: Float = 20.0;         // m/s3, rate limit of the acceleration command
  public let yawSmoothing: Float = 0.25;      // s, low-pass of the yaw input
  public let yawSlew: Float = 1.5;            // 1/s, most the yaw input may change per second
  public let bankRate: Float = 30.0;          // deg/s, most the bank target may change per second
  public let verticalTurnYaw: Float = 0.2;    // most yaw input while turning during takeoff / landing
  public let maxYawTurn: Float = 0.3;         // most yaw input for turning (about 0.9 rad/s)
  public let rollDamping: Float = 0.35;       // s, measured roll rate taken out of the bank command
  // m cut back from each route corner for a Bezier arc (at most 40% of each leg; halved until the
  // arc is clear on the nav map); 0 = sharp corners. The arcs' bends are judged from their own
  // geometry (routeArc): judged like sharp corners they read as gentle, 2026-10-26
  public let cornerRound: Float = 40.0;
  public let brakeLead: Float = 0.6;          // s of the flown speed by which planned braking starts earlier
  public let brakeJerk: Float = 12.0;         // m/s3, how quickly braking may build up

  // controller tuning, not in Mod Settings
  public let velocityGain: Float = 1.2;       // 1/s, velocity error -> acceleration
  public let maxAccel: Float = 12.0;          // m/s2
  public let yawGain: Float = 0.04;           // full yaw input at 25 degrees heading error
  public let yawRateDamping: Float = 0.25;
  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-Autopilot")
  @runtimeProperty("ModSettings.displayName", "UI-Settings-Autopilot-Max-Bank")
  @runtimeProperty("ModSettings.description", "UI-Settings-Autopilot-Max-Bank-Description")
  @runtimeProperty("ModSettings.step", "1.0")
  @runtimeProperty("ModSettings.min", "0.0")
  @runtimeProperty("ModSettings.max", "45.0")
  public let maxBankDeg: Float = 25.0;
  public let lateralCorrection: Float = 0.6;  // 1/s, sideways velocity error -> extra bank accel (1.0 slammed the bank between +-25 deg, 0.3 let the car drift 9 m wide after bends)
  public let bankTime: Float = 0.5;           // s, smoothing of the bank target
  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-Autopilot")
  @runtimeProperty("ModSettings.displayName", "UI-Settings-Autopilot-Max-Pitch")
  @runtimeProperty("ModSettings.description", "UI-Settings-Autopilot-Max-Pitch-Description")
  @runtimeProperty("ModSettings.step", "1.0")
  @runtimeProperty("ModSettings.min", "0.0")
  @runtimeProperty("ModSettings.max", "45.0")
  public let maxPitchDeg: Float = 20.0;
  public let bankSign: Float = 1.0;           // flip if the car banks out of turns
  public let pitchSign: Float = 1.0;          // flip (-1) if the nose drops when climbing
  public let overrideHold: Float = 1.5;       // s the autopilot stays off after player input
  public let replanDistance: Float = 30.0;    // m off the route before replanning
  public let arriveDistance: Float = 6.0;
  public let cornerOvershoot: Float = 10.0;   // m the lookahead may reach past the next corner
  public let cornerSpan: Float = 15.0;        // m of route before / after a corner that set its radius
  public let offRouteSlowdown: Float = 3.0;   // m/s less target speed per metre off the route (past 2 m)
  public let collisionHorizon: Float = 1.5;   // s of current velocity checked against the map

  // aerobatics tuning, not in Mod Settings
  public let attitudeFrequency: Float = 5.0;  // rad/s, natural frequency of the attitude control
  public let attitudeDamping: Float = 0.8;
  public let maxAngularAccel: Float = 20.0;   // rad/s2
  // rad/s2 of angular acceleration per unit of mode torque x mass x thruster tensor / inertia
  // (FlightComponent scales the torque by those, PhysX divides by its own inertia), per axis.
  // the torque probe (tools/bridge, TorqueProbe) on an Archer Quartz hovering, 2026-10-26: roll 0.52, yaw 0.95,
  // pitch 2.5 to 2.8 over the first frames, then falling off while still pushed; all respond two
  // frames late (at 32 fps)
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
  public let raceMargin: Float = 0.85;        // share of the envelope the speed profile plans for
  public let raceJerk: Float = 60.0;          // m/s3, rate limit of the acceleration command
  public let raceSpeedCap: Float = 120.0;     // m/s, whatever the drag would allow
  // race: corners rounded much wider (the bend's radius sets its speed); the cut steps down by
  // raceRoundStep until the arc is clear, to at least raceRoundMin
  public let raceCornerRound: Float = 150.0;  // m
  public let raceRoundShare: Float = 0.5;     // of each leg at most (two corners can meet midway)
  public let raceRoundStep: Float = 0.7;
  public let raceRoundMin: Float = 8.0;       // m
  public let raceBankTime: Float = 0.12;      // s, smoothing of the acceleration the attitude follows
  public let lineRadiusSpan: Float = 15.0;    // m either side of a racing line point its bend radius is taken over
  public let raceOffLine: Float = 4.0;        // m off a racing line before the car is treated as off it
  public let raceTakeoffHeight: Float = 3.0;  // m above the floor before the racing line takes over
  public let coverLimit: Float = 6.0;         // m the map's free space may start above the car before it is under cover
  // race route character (planner, Navigation/Planner.hpp; tools/navplan/laptime.py compares them):
  // between the buildings (40 m, climbing costly, 8 m passages at least: city centre to Dogtown
  // about 80 s, the street start in ManualSave-181 to Dogtown 33 s) or over the rooftops (100 m,
  // 0.5, 0: about 71 s and 38 s). 12 m passages found no route at all from that street start.
  public let raceRouteHeight: Float = 40.0;   // m above the local floor the route would like to fly
  // 0.5 (2026-10-27): at 2 every rise of the floor-following height in the city cost so much that
  // Northside to the Pacifica coast went round the city over the water (5.45 km, against 5.24 km
  // through it) once the doorway search could find the cheapest route
  public let raceClimbCost: Float = 0.5;      // per metre of climbing, against 1 per metre flown
  // 12 (2026-10-27): at 8 west-east threaded an 11.5 m slot between a deck and a skybridge at 85 m/s,
  // kinking the line (laptime 53.6 s, 47.4 s over it). The "no route at all" at 12 above was before
  // the take-off was part of the plan; a start with no 12 m way out still gets the retry without it.
  // The doorways in navmap.bin are baked with this value (tools/heightmap/doorways.py): rebake if it changes.
  public let raceMinPassage: Float = 12.0;    // m of free height the route may squeeze through at least (0 = any)
  public let raceSearchWeight: Float = 1.2;   // A* heuristic weight, 1 = optimal (slower to plan)
  // per 45 degrees the route turns over a few cells (as metres flown): straighter, more open
  // corridors, whose bends the racing line takes wide. At 5 the tightest bend from the street
  // start to Dogtown widened from 28 to 46 m (tools/navplan, 2026-10-27)
  public let raceTurnCost: Float = 5.0;
  // drift: in a bend the nose turns into it, up to raceDriftMax at raceDriftFull sideways (none
  // below raceDriftStart), so the surge thrust pushes round the corner along with the banked lift
  public let raceDriftMax: Float = 20.0;      // deg
  public let raceDriftStart: Float = 8.0;     // m/s2
  public let raceDriftFull: Float = 28.0;     // m/s2
  // m/s2 the speed profile plans slowing with at most: the brake grows with speed (60 m/s2 and
  // more at 40 m/s), but the flown speed eases into it, and planned at full the car reached the
  // destination at 43 m/s and hit the ground, 2026-10-26
  public let racePlanDecel: Float = 18.0;     // what the car reached following the profile (19 m/s2), 2026-10-26
  public let raceBrakeGripMin: Float = 0.3;   // share of racePlanDecel planned for in a bend at the full lateral envelope
  public let raceRateProbe: Float = 0.25;     // s ahead the allowed speed's rate of fall is read from (RaceTargetRate)
  // descents: the line can drop 100 m in its last 150 m (Dogtown: 140 m buildings, then a low
  // courtyard); flown at full speed the car fell at 44 m/s, rolled inverted pushing over into the
  // dive, and reached the approach point at 41 m/s, 2026-10-26
  public let raceMaxSink: Float = 15.0;       // m/s down at most, on the approach only:
  public let raceSinkApproach: Float = 250.0; // m before the route's end (elsewhere it slowed the car for nothing)
  public let raceArriveLength: Float = 120.0; // m before the route's end braked at most raceArriveDecel
  public let raceArriveDecel: Float = 5.0;    // m/s2
  // m/s2 at most bending the path downwards: the car stays upright and pushes with negative lift
  // (FlyAerobatic keeps the top from turning over past raceTopDown); at half a g, three of the five
  // slowdowns on the street start to Dogtown were crests, down to 15 m/s, 2026-10-27
  public let racePushOver: Float = 15.0;
  public let raceTopDown: Float = 0.8;        // g of downward pull the attitude follows; the thrust gives the rest
  // racing line tracking: the line's direction at the flown speed, its curvature fed forward
  // lineLead ahead (the bank takes a moment to build), a damped spring onto the line across it.
  // (Aiming at a lookahead point as on other routes cut inside every bend at speed: 60 m ahead at
  // 75 m/s, the car swung 7 to 20 m off the line and into buildings, 2026-10-26)
  public let lineLead: Float = 0.25;          // s
  public let lineSpring: Float = 1.0;         // 1/s, distance off the line -> speed back towards it
  public let lineVelocityGain: Float = 2.5;   // 1/s, velocity error -> acceleration
  public let lineSpan: Float = 10.0;          // m either side a line point's direction and bend are taken over
  // ribbon: on a racing line the bank comes from the line's own bend (over ribbonSpan either side,
  // ribbonLead seconds ahead) at the speed flown, not from the commanded acceleration, which
  // carries every small correction back onto the line and rolled the car for each of them
  public let ribbonSpan: Float = 50.0;        // m
  public let ribbonLead: Float = 0.8;         // s (0.3 with the quicker roll)
  // sideways acceleration the ribbon ignores: at 80 m/s a 2 km bend in the line already asks for
  // 3 m/s2 (an 18 degree bank), which read as rolling for nothing; only what is above it banks
  public let ribbonDeadband: Float = 3.0;     // m/s2
  public let ribbonSlow: Float = 10.0;        // m/s under which the ribbon banks nothing (full from 25 m/s)
  // bank easing with aerobatics: the bank follows its target through a critically damped second
  // order filter with limited roll rate and roll acceleration (the target flicks as route points
  // pass; followed directly, the car rolled at 21 rad/s2 and more, 2026-10-26)
  public let rollFrequency: Float = 4.0;      // rad/s
  public let raceRollFrequency: Float = 2.5;  // rad/s
  public let rollRateMax: Float = 2.5;        // rad/s
  public let rollAccelMax: Float = 10.0;      // rad/s2
  // Race rolls are the gentlest movements: at 6 rad/s, 2.5 rad/s and 10 rad/s2 the bank swung up to
  // 90 degrees within a second between the Dogtown towers (2026-10-27); the ribbon looks further
  // ahead (ribbonLead) so the slower roll still starts in time
  public let raceRollRateMax: Float = 1.0;    // rad/s
  public let raceRollAccelMax: Float = 3.0;   // rad/s2
  // pitch in Race, the same way as the bank (off the line too: the take-off and the arrival); on a
  // racing line from the line's own slope ribbonLead ahead
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
  public let gateStep: Float = 5.0;           // m between the run's points (a racing line's spacing)
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
  // m/s at most: on and on into new areas at 89 m/s the world could not stream in fast enough and
  // the game stopped on its loading overlay (1.6 km past Watson heading east, 2026-10-27)
  public let freeRoamTopSpeed: Float = 60.0;
  // m a leg's end may differ in height from where it starts: from a tower's top (387 m) down to a
  // street (28 m) the line crested and dived 350 m, the car slowed to 9 m/s and rolled over pushing
  // into the dive (2026-10-27)
  public let freeRoamMaxRise: Float = 100.0;

  // state, public for tests (tools/bridge/ltbf_bridge.reds)
  public let status: String = "idle";
  public let route: array<Vector4>;
  public let routeLength: array<Float>;       // route distance of each corner from the start
  public let routeArc: array<Bool>;           // the point is on a rounded corner's arc
  public let aheadSmooth: Float;              // m, lookahead distance (grows back smoothly)
  public let segment: Int32;
  public let request: Int32;
  public let destination: Vector4;
  public let hasDestination: Bool;
  public let holdPosition: Vector4;
  public let lookahead: Vector4;
  public let targetSpeed: Float;
  public let targetRate: Float;               // m/s2, how fast targetSpeed is falling along the route (race; fed forward)
  public let remaining: Float;
  public let overrideTimer: Float;
  public let replanTimer: Float;
  public let logTimer: Float;
  public let bankTarget: Float;               // rad, smoothed
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
  public let takingOff: Bool;
  public let underCover: Bool;                // engaged somewhere covered the map does not have: holding
  public let takeoffPoint: Vector4;
  public let landing: Bool;                   // the destination is on the ground: descend and touch down there
  public let landingSpot: Vector4;            // car-centre resting height (the bake's floor)
  public let landingPad: Int32;               // FlightNavMap helipad index landed on, -1: a spot found on the map (set in Activate)
  public let touchdownTimer: Float;
  public let takeoffStartZ: Float;
  public let takeoffHeading0: Vector4;
  public let landingStartZ: Float;
  public let landingHeading0: Vector4;
  public let landingHeading: Vector4;
  public let touchdownLow: Float;
  public let speedCmd: Float;                 // speed flown along the route (eased towards targetSpeed)
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
  public let routeSpeed: array<Float>;        // race: fastest speed at each route point (backward pass)
  public let pathCurvature: Vector4;          // 1/m, towards the centre of the route's bend ahead (race feed-forward)
  public let bankAngle: Float;                // rad, eased bank of the attitude target about the nose
  public let requestRace: Bool;               // the pending plan asked for a racing line
  public let routeLine: Bool;                 // the route is a racing line (evenly spaced, smooth)
  public let closestS: Float;                 // route distance of the closest point
  public let onLine: Bool;                    // following a racing line this frame (the ribbon applies)
  public let ribbonNeed: Vector4;             // m/s2, the line's bend ahead at the speed flown (no deadband)
  public let bankRateNow: Float;
  public let pitchAngle: Float;               // rad, eased nose pitch on a racing line
  public let pitchRateNow: Float;             // rad/s
  public let commandSurge: Float;             // -1..1, the thrust it commands, for the engine sound
  public let commandSway: Float;
  public let dronePitch: Float;               // rad, drone style's eased body tilt forward (DroneAttitude)
  public let droneRoll: Float;                // rad, and to the right
  public let dronePitchRate: Float;
  public let droneRollRate: Float;
  public let droneEaseLive: Bool;             // drone style's attitude was flown last frame
  public let droneYaw: Float;                 // rad, drone style's eased heading
  public let droneYawRate: Float;
  public let droneBrakeSide: Float;           // +1 / -1 the nose swings for the stop under way, 0 none
  public let droneBrakeAmount: Float;         // 0..1 of droneBrakeYaw
  public let stopStartS: array<Float>;        // route distance where each planned stop begins (PlanStops)
  public let stopEndS: array<Float>;
  public let stopSide: array<Float>;          // +1 slews right, -1 left
  public let stopAmount: array<Float>;        // 0..1 of droneBrakeYaw
  public let stopToEnd: array<Bool>;          // the slewed stretch runs on into the landing
  public let landSlewOn: Bool;                // a landing turn is under way (LandSlewYaw)
  public let landSlewFrom: Float;             // the start of its stretch
  public let landSlewP: Float;                // 0..1 through it, before easing
  public let landSlewLastS: Float;            // route distance landSlewP was carried to
  public let landSlewStartYaw: Float;         // rad, the nose when it began
  public let landSlewDelta: Float;            // rad, signed, the whole turn to the landing heading
  public let landSlewHeading: Vector4;        // the heading it ends on, landed on by the descent
  public let keepSlew: Bool;                // the next PlanStops carries on a slew under way (the route's end moved)
  public let thrustCmd: Vector4;             // m/s2 in the car's frame (sway, surge, lift) FlyAerobatic set (force is redone by ApplyPhysics)
  public let freeRoam: Bool;                  // flying free roam legs (no destination tracked)
  public let freeRoamCentre: Vector4;
  public let requestJoin: Bool;               // the pending plan is the next free roam leg, from joinPoint
  public let joinS: Float;                    // route distance of joinPoint
  public let joinPoint: Vector4;
  private let cinematicCallback: ref<CallbackHandle>;
  private let cinematicContext: Bool;         // UIGameContext.CinematicCamera pushed              // rad/s

  public static func Create(component: ref<FlightComponent>) -> ref<FlightModeAutopilot> {
    let self = new FlightModeAutopilot();
    self.Initialize(component);
    return self;
  }

  public func GetDescription() -> String = "Autopilot";

  public func Activate() -> Void {
    this.holdPosition = this.component.stats.d_position;
    ArrayClear(this.route);
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
    // In Race the take-off is part of the plan: the route starts at the car, and the planner finds
    // the way out from under a deck or between buildings itself (PlanParams.startRadius). The
    // straight climb and the hold under cover are the archived normal mode's.
    this.underCover = !this.raceMode && this.UnderCover(this.holdPosition);
    if this.underCover {
      // somewhere covered the map does not have (a car park, under a structure): the nearest free
      // space is above the roof, and climbing to it went 47 m up inside a building, 2026-10-27
      this.status = "under cover";
      FlightLog.Warn("[Autopilot] under cover at " + FloatToStringPrec(this.holdPosition.X, 1) + ", " + FloatToStringPrec(this.holdPosition.Y, 1)
        + ", " + FloatToStringPrec(this.holdPosition.Z, 1) + ": holding, move out into the open first");
    }
    this.takingOff = !this.raceMode && !this.underCover && this.TakeoffPoint(this.holdPosition, this.takeoffPoint);
    this.takeoffStartZ = this.holdPosition.Z;
    this.takeoffHeading0 = this.component.stats.d_forward2D;
    this.aheadSmooth = 15.0;
    if this.takingOff {
      this.holdPosition = this.takeoffPoint;
    }
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

  // The car is below the free space the map has here by more than coverLimit: under something the
  // map closed (a car park, an arcade under a tower). Not when the map has nothing here at all.
  private func UnderCover(p: Vector4) -> Bool {
    let gap: Vector2;
    if !FlightNavMap.IsLoaded() || FlightNavMap.IsFree(p) || !FlightModeAutopilot.GapAt(p, gap) {
      return false;
    }
    return gap.X - p.Z > this.coverLimit;
  }

  // Near the floor: climb straight up first, clear of traffic and street furniture, then fly off.
  private func TakeoffPoint(p: Vector4, out point: Vector4) -> Bool {
    let gap: Vector2;
    if !FlightNavMap.IsLoaded() || !FlightModeAutopilot.GapAt(p, gap) {
      return false;
    }
    // Race: only clear of the ground, then the racing line climbs away while speeding up
    let z = MinF(gap.X + (this.raceMode ? this.raceTakeoffHeight : this.takeoffHeight), gap.Y - 2.0);
    if z - p.Z < 2.0 {
      return false;
    }
    point = new Vector4(p.X, p.Y, z, 1.0);
    return true;
  }

  // A destination on the ground (a GPS pin on a street): the nearest column within reach that is
  // flat, at the same level, and open far enough above to come straight down into. The spot is
  // the bake's floor there, which is where a resting car's position sits.
  private func FindLandingSpot(target: Vector4, out spot: Vector4, out top: Float) -> Bool {
    // first a spot with room to come straight down from the approach height: one with just 6 m
    // over it was under a canopy in Dogtown, and the car hit the canopy arriving, 2026-10-27
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
          && this.ShaftClear(c, g.X, room) && !this.SpotTaken(new Vector4(c.X, c.Y, g.X, 1.0)) {
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
  // the car comes down a shaft, not a hole: the bake grows solids by 3 m for the car, and in
  // Dogtown a spot whose neighbours all had a walkway over them at 6 to 12 m let a Hellhound
  // (longer than that, and turning on the way down) catch the walkway's edge (2026-10-29).
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
  // centre and on rings of 2 and 4 m. Parked cars are not on the nav map: the first landing with a
  // clear shaft in Dogtown came down on a parked convertible (2026-10-29). Cars only exist where
  // the world has streamed in, so the spot is checked again on arrival.
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

  // the four neighbouring columns have their floor within half a metre of this one
  private func IsFlat(c: Vector4, floor: Float) -> Bool {
    let offsets: array<Vector4> = [new Vector4(2.0, 0.0, 0.0, 0.0), new Vector4(-2.0, 0.0, 0.0, 0.0), new Vector4(0.0, 2.0, 0.0, 0.0), new Vector4(0.0, -2.0, 0.0, 0.0)];
    for o in offsets {
      let g: Vector2;
      if !FlightModeAutopilot.GapAt(c + o, g) || AbsF(g.X - floor) > 0.5 {
        return false;
      }
    }
    return true;
  }

  // A landing pad (FlightNavMap helipads, docs/helipads.md) to land on for this destination: the
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
    ArrayClear(this.route);
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
    if !FlightNavMap.IsLoaded() {
      this.status = "no nav map";
      return;
    }
    // Race: the planner spreads the route into a racing line of wide bends
    this.requestRace = this.raceMode;
    this.requestJoin = false;
    // a replan towards a gate still ahead (off the route on the way there): through it again. Without
    // the run the leg ended at the gate's entry, and the next one was joined before it (2026-10-28).
    if ArraySize(this.gateTail) == 0 && ArraySize(this.routeGateTail) > 0 && this.gateEndS > this.closestS
      && Vector4.Distance(this.routeGoal, this.routeGateEntry) < 1.0 {
      this.gateTail = this.routeGateTail;
    }
    this.request = this.raceMode
      ? FlightNavMap.PlanRaceAsync(from, this.routeGoal, this.raceRouteHeight, this.raceClimbCost, this.raceMinPassage, this.raceSearchWeight, this.raceTurnCost)
      : FlightNavMap.PlanAsync(from, this.routeGoal, false);
    this.replanTimer = 3.0;
    if ArraySize(this.route) < 2 {
      // nothing to follow meanwhile: hold here (a replan keeps following the old route)
      this.status = "planning";
      this.holdPosition = from;
    }
  }

  private func UpdateRoute(timeDelta: Float) -> Void {
    if this.underCover {
      return;
    }
    this.replanTimer -= timeDelta;
    let target: Vector4;
    let has = this.CurrentDestination(target);
    if !has && !this.freeRoam && (Equals(this.status, "descending") || Equals(this.status, "landed") || Equals(this.status, "arrived")) {
      // the game untracks a map pin once the player reaches it, mid-descent: finish the landing
      // (or the hand-back), not free roam from there
      return;
    }
    if has {
      if this.freeRoam {
        // a destination tracked during free roam: fly there instead
        this.freeRoam = false;
        this.hasDestination = false;
        this.request = 0;
        this.requestJoin = false;
        this.PublishFreeRoamUI();
        FlightLog.Info("[Autopilot] destination tracked, leaving free roam");
      }
      if !this.hasDestination || Vector4.Distance(target, this.destination) > 20.0 {
        this.destination = target;
        this.hasDestination = true;
        ArrayClear(this.route);
        this.ChooseRouteGoal();
        this.RequestRoute(this.takingOff ? this.takeoffPoint : this.component.stats.d_position);
      }
    } else {
      // no destination tracked (none when engaged, or untracked on the way): free roam
      if !this.freeRoam {
        this.BeginFreeRoam();
      }
      this.ChainFreeRoam();
    }
    if this.request > 0 && FlightNavMap.IsPlanReady(this.request) {
      let points = FlightNavMap.TakePlan(this.request);
      this.request = 0;
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
      // planner's 45 degree limit does not reach them: legs into gates among tall buildings came
      // over the roofs and dropped to the gate at 80 degrees (2026-10-28). Another leg instead.
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
        // a racing line has its points lineSpacing (5 m) apart; when the planner could not make
        // one free all the way it sends the plain route, corners and all
        this.routeLine = this.requestRace && ArraySize(points) > 2
          && Vector4.Distance(points[0], points[ArraySize(points) - 1]) / Cast<Float>(ArraySize(points) - 1) < 10.0;
        if this.routeLine {
          // already smooth: no corners to round
          this.route = points;
          ArrayClear(this.routeArc);
          for q in points {
            ArrayPush(this.routeArc, false);
          }
        } else {
          this.route = this.RoundCorners(points);
        }
        ArrayClear(this.routeLength);
        let total = 0.0;
        let i = 0;
        while i < ArraySize(this.route) {
          if i > 0 {
            total += Vector4.Distance(this.route[i - 1], this.route[i]);
          }
          ArrayPush(this.routeLength, total);
          i += 1;
        }
        this.segment = 0;
        this.closestS = 0.0;
        this.landSlewOn = false;
        this.gateEndS = gateLeg ? total : 0.0;
        this.status = "following";
        ArrayClear(this.routeSpeed);
        if this.raceMode {
          this.BuildSpeedProfile();
        }
      } else {
        this.status = "no route";
        this.holdPosition = this.component.stats.d_position;
      }
      FlightLog.Info("[Autopilot] " + FlightNavMap.GetLastPlanInfo());
    }
  }

  // --- free roam ------------------------------------------------------------------------------

  private func BeginFreeRoam() -> Void {
    let stats = this.component.stats;
    this.freeRoam = true;
    this.hasDestination = false;
    this.landing = false;
    this.landingPad = -1;
    this.request = 0;
    this.requestJoin = false;
    this.freeRoamCentre = stats.d_position;
    ArrayClear(this.route);
    this.holdPosition = this.takingOff ? this.takeoffPoint : stats.d_position;
    this.replanTimer = 0.0;
    this.status = "free roam";
    this.PublishFreeRoamUI();
    FlightLog.Info("[Autopilot] free roam around " + FloatToStringPrec(stats.d_position.X, 1) + ", " + FloatToStringPrec(stats.d_position.Y, 1));
  }

  // A point for the next leg to end at: freeRoamLegMin to freeRoamLegMax away from `from`, within
  // freeRoamSpread of `heading` (wider for each candidate that does not fit, at the edge of the
  // area or of the map, up to maxTurn degrees), inside freeRoamRadius of where free roam began,
  // raceRouteHeight above the ground under open sky there.
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
          let z = MinF(sky.X + this.raceRouteHeight, sky.Y - 5.0);
          // near the height of the leg before, until nothing like that fits; over land: a leg out to
          // sea west of the city crossed the playable area's edge about 300 m offshore, and the game
          // put the car back where flight was switched on (2026-10-28)
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
  // doorways by tools/heightmap/gates.py): freeRoamGateMin to freeRoamLegMax away, within
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
    let n = ArraySize(this.route);
    if n < 2 {
      let heading = Vector4.Length2D(stats.d_velocity) > 3.0 ? FlightModeAutopilot.Flat(stats.d_velocity) : FlightModeAutopilot.Flat(stats.d_forward);
      this.NextFreeRoamLeg(this.takingOff ? this.takeoffPoint : stats.d_position, heading, false);
      return;
    }
    if !Equals(this.status, "following") || this.routeLength[n - 1] - this.closestS > this.freeRoamChainAt {
      return;
    }
    // the point at that distance, not the next route point after it: a plain route's legs can be
    // a kilometre long, and its end was the join every time, always reached before the plan
    // (2026-10-27)
    // and not before the end of a gate's straight run still ahead: the next leg would cut it off
    let s = MinF(MaxF(this.closestS + MaxF(this.freeRoamJoinMin, stats.d_speed * this.freeRoamJoinAhead), this.gateEndS), this.routeLength[n - 1]);
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
    let oldArc = this.routeArc;
    let tail: array<Vector4>;
    let tailArc: array<Bool>;
    if line {
      tail = points;
      for q in points {
        ArrayPush(tailArc, false);
      }
    } else {
      tail = this.RoundCorners(points);
      tailArc = this.routeArc;
    }
    let first = Max(this.segment - 2, 0);
    let trimmed = this.routeLength[first];
    let route: array<Vector4>;
    let arc: array<Bool>;
    let i = first;
    while i < ArraySize(this.route) && this.routeLength[i] < this.joinS - 0.5 {
      ArrayPush(route, this.route[i]);
      ArrayPush(arc, i < ArraySize(oldArc) && oldArc[i]);
      i += 1;
    }
    ArrayPush(route, this.joinPoint);
    ArrayPush(arc, false);
    let join = ArraySize(route) - 1;
    i = 1;
    while i < ArraySize(tail) {
      ArrayPush(route, tail[i]);
      ArrayPush(arc, tailArc[i]);
      i += 1;
    }
    // the legs meet at an angle: ease the points round the join towards their neighbours where
    // the map allows (a point only moves if both its segments stay clear), leaving the stretch
    // the car is on alone. Racing line points are about 5 m apart.
    let centre = route[join];
    let span = Cast<Int32>(this.freeRoamSmoothSpan / 5.0);
    let lo = Max(join - span, this.segment - first + 2);
    if this.gateEndS >= this.joinS - 1.0 {
      lo = Max(lo, join); // joined at the end of a gate's run: the run stays straight
    }
    let hi = Min(join + span, ArraySize(route) - 2);
    let pass = 0;
    while pass < 20 {
      let k = lo;
      while k <= hi {
        if Vector4.Distance(route[k], centre) < this.freeRoamSmoothSpan {
          let q = route[k] * 0.5 + (route[k - 1] + route[k + 1]) * 0.25;
          if FlightNavMap.SegmentClear(route[k - 1], q) >= 1.0 && FlightNavMap.SegmentClear(q, route[k + 1]) >= 1.0 {
            route[k] = q;
          }
        }
        k += 1;
      }
      pass += 1;
    }
    this.route = route;
    this.routeArc = arc;
    this.routeLine = this.routeLine && line;
    this.segment -= first;
    this.closestS -= trimmed;
    ArrayClear(this.routeLength);
    let total = 0.0;
    i = 0;
    while i < ArraySize(this.route) {
      if i > 0 {
        total += Vector4.Distance(this.route[i - 1], this.route[i]);
      }
      ArrayPush(this.routeLength, total);
      i += 1;
    }
    this.gateEndS = this.spliceGate ? total : MaxF(this.gateEndS - trimmed, 0.0);
    ArrayClear(this.routeSpeed);
    if this.raceMode {
      this.BuildSpeedProfile();
    }
    FlightLog.Info("[Autopilot] free roam: next leg joined, " + FloatToStringPrec(total - this.closestS, 0) + " m ahead");
  }

  // The planner's route is straight legs meeting at corners. Replace each corner with a short
  // quadratic Bezier arc (cut back up to cornerRound metres, and at most 30% of each leg) so the
  // car starts turning before the corner instead of at it. An arc is kept only where it is free on
  // the nav map; otherwise the corner stays as it was.
  // Also fills routeArc: true for points on an arc, whose bend AllowedSpeed takes from the arc's
  // own geometry (the circle through it and its neighbours) instead of the cornerSpan estimate
  // meant for sharp corners. The cut steps down (full, half, quarter) until the arc is clear.
  private func RoundCorners(points: array<Vector4>) -> array<Vector4> {
    let n = ArraySize(points);
    let out: array<Vector4>;
    ArrayClear(this.routeArc);
    ArrayPush(out, points[0]);
    ArrayPush(this.routeArc, false);
    let i = 1;
    while i < n - 1 {
      let a = points[i - 1];
      let b = points[i];
      let c = points[i + 1];
      let rounded = false;
      // full cut, then half; below that the corner stays sharp: tiny arcs made for very slow
      // corners (8 m/s), where the lookahead cutting a sharp corner flew it faster and fine
      let cut = this.raceMode ? this.raceCornerRound : this.cornerRound;
      let share = this.raceMode ? this.raceRoundShare : 0.4;
      let smallest = this.raceMode ? this.raceRoundMin : 0.5 * this.cornerRound;
      while !rounded && cut >= smallest {
        let d1 = MinF(share * Vector4.Distance(a, b), cut);
        let d2 = MinF(share * Vector4.Distance(b, c), cut);
        if d1 > 1.0 && d2 > 1.0 {
          let q1 = b + Vector4.Normalize(a - b) * d1;
          let q2 = b + Vector4.Normalize(c - b) * d2;
          let arc: array<Vector4>;
          let k = 0;
          while k <= 8 {
            let t = Cast<Float>(k) / 8.0;
            ArrayPush(arc, q1 * ((1.0 - t) * (1.0 - t)) + b * (2.0 * t * (1.0 - t)) + q2 * (t * t));
            k += 1;
          }
          let clear = FlightNavMap.SegmentClear(out[ArraySize(out) - 1], arc[0]) >= 1.0;
          k = 0;
          while clear && k < 8 {
            clear = FlightNavMap.SegmentClear(arc[k], arc[k + 1]) >= 1.0;
            k += 1;
          }
          if clear {
            for q in arc {
              ArrayPush(out, q);
              ArrayPush(this.routeArc, true);
            }
            rounded = true;
          }
        }
        cut *= this.raceMode ? this.raceRoundStep : 0.5;
      }
      if !rounded {
        ArrayPush(out, b);
        ArrayPush(this.routeArc, false);
      }
      i += 1;
    }
    ArrayPush(out, points[n - 1]);
    ArrayPush(this.routeArc, false);
    return out;
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

  // radius of the circle through a, b, c (1e6 when they are in a line)
  // The horizontal unit vector of v (zero if v is vertical). Vector4.Normalize2D keeps v's Z as it
  // is: a dot product of two of those added the Zs' product, 64 for two opposite headings while
  // descending, and free roam took a gate straight behind the car for one ahead (2026-10-28).
  public static func Flat(v: Vector4) -> Vector4 {
    let length = SqrtF(v.X * v.X + v.Y * v.Y);
    return length > 0.0001 ? new Vector4(v.X / length, v.Y / length, 0.0, 0.0) : new Vector4(0.0, 0.0, 0.0, 0.0);
  }

  private static func Circumradius(a: Vector4, b: Vector4, c: Vector4) -> Float {
    let ab = b - a;
    let bc = c - b;
    let area2 = Vector4.Length(Vector4.Cross(ab, bc));
    if area2 < 0.0001 {
      return 1000000.0;
    }
    return Vector4.Length(ab) * Vector4.Length(bc) * Vector4.Distance(a, c) / (2.0 * area2);
  }

  // Route distance from the car to the next bend worth slowing the lookahead for: a point where
  // the route's direction cornerSpan before and after it differs by more than 25 degrees (a
  // rounded corner's arc points each turn only a little, so they are judged as one bend).
  private func DistanceToSharpBend(closest: Vector4) -> Float {
    let n = ArraySize(this.route);
    let start = this.routeLength[this.segment + 1] - Vector4.Distance(closest, this.route[this.segment + 1]);
    let k = this.segment + 1;
    while k < n - 1 && this.routeLength[k] - start < 300.0 {
      let s = this.routeLength[k];
      let inDir = this.route[k] - this.PointAt(s - this.cornerSpan);
      let outDir = this.PointAt(s + this.cornerSpan) - this.route[k];
      if Vector4.Length(inDir) > 0.5 && Vector4.Length(outDir) > 0.5
        && Vector4.Dot(Vector4.Normalize(inDir), Vector4.Normalize(outDir)) < 0.906 {
        return s - start;
      }
      k += 1;
    }
    return 1000000.0;
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
    let n = ArraySize(this.route);
    let best = 1000000000.0;
    let end = this.routeLength[this.segment] + this.component.stats.d_speed * this.collisionHorizon + 50.0;
    let i = this.segment;
    while i < n - 1 && this.routeLength[i] <= end {
      let a = this.route[i];
      let ab = this.route[i + 1] - a;
      let len2 = Vector4.Dot(ab, ab);
      let t = len2 > 0.0001 ? ClampF(Vector4.Dot(q - a, ab) / len2, 0.0, 1.0) : 0.0;
      best = MinF(best, Vector4.Distance(q, a + ab * t));
      i += 1;
    }
    return best;
  }

  // Closest point on the route from the current segment on; advances the segment index.
  private func ClosestOnRoute(p: Vector4, out closest: Vector4, out distance: Float) -> Void {
    let n = ArraySize(this.route);
    let best = 1000000000.0;
    let bestSegment = this.segment;
    let i = this.segment;
    while i < n - 1 && i <= this.segment + 12 {
      let a = this.route[i];
      let b = this.route[i + 1];
      let ab = b - a;
      let len2 = Vector4.Dot(ab, ab);
      let t = len2 > 0.0001 ? ClampF(Vector4.Dot(p - a, ab) / len2, 0.0, 1.0) : 0.0;
      let q = a + ab * t;
      let d = Vector4.Distance(p, q);
      if d < best {
        best = d;
        bestSegment = i;
        closest = q;
      }
      i += 1;
    }
    this.segment = bestSegment;
    distance = best;
  }

  // Point `along` metres further along the route from `from` (which lies on segment this.segment).
  private func AlongRoute(from: Vector4, along: Float) -> Vector4 {
    let n = ArraySize(this.route);
    let p = from;
    let left = along;
    let i = this.segment;
    while i < n - 1 {
      let b = this.route[i + 1];
      let d = Vector4.Distance(p, b);
      if d >= left {
        return p + Vector4.Normalize(b - p) * left;
      }
      left -= d;
      p = b;
      i += 1;
    }
    return this.route[n - 1];
  }

  // Sideways acceleration corner speeds are planned for: the Cornering setting, but no more than
  // 80% of what full sideways thrust gives (standardModeSwayFactor, 5 m/s2 by default), which is
  // all that turns the car (the bank does not tilt the lift); planned above it, the car slid
  // 9 to 14 m wide out of bends (2026-10-26)
  private func CorneringAccel() -> Float {
    return MinF(this.lateralAccel, 0.8 * this.standardModeSwayFactor);
  }

  // Speed allowed now: max speed, corners ahead (with braking room) and the goal.
  private func AllowedSpeed(closest: Vector4) -> Float {
    let n = ArraySize(this.route);
    let toCorner = Vector4.Distance(closest, this.route[this.segment + 1]);
    this.remaining = toCorner;
    let k = this.segment + 1;
    while k < n - 1 {
      this.remaining += Vector4.Distance(this.route[k], this.route[k + 1]);
      k += 1;
    }
    if this.raceMode {
      return this.RaceAllowedSpeed(toCorner);
    }
    // braking curves start brakeLead seconds early, the time the eased speed takes to ramp into
    // full braking
    let lead = this.speedCmd * this.brakeLead;
    let allowed = MinF(this.maxSpeed, SqrtF(2.0 * this.brakeAccel * MaxF(this.remaining - 2.0 - lead, 0.0)));
    // each corner is judged by the route's direction a stretch before and after it, so a run of
    // small steps reads as the bend it really is instead of many tight corners
    let distance = toCorner;
    let start = this.routeLength[this.segment + 1] - toCorner; // route distance of the car
    k = this.segment + 1;
    while k < n - 1 && distance < 600.0 {
      let s = this.routeLength[k];
      if ArraySize(this.routeArc) == n && this.routeArc[k] {
        // on a rounded corner: the bend is the arc's own
        let radius = FlightModeAutopilot.Circumradius(this.route[k - 1], this.route[k], this.route[k + 1]);
        let arcSpeed = SqrtF(this.CorneringAccel() * MaxF(radius, 1.0));
        allowed = MinF(allowed, SqrtF(arcSpeed * arcSpeed + 2.0 * this.brakeAccel * MaxF(s - start - lead, 0.0)));
      } else {
        let before = this.PointAt(s - this.cornerSpan);
        let after = this.PointAt(s + this.cornerSpan);
        let inDir = this.route[k] - before;
        let outDir = after - this.route[k];
        if Vector4.Length(inDir) > 0.5 && Vector4.Length(outDir) > 0.5 {
          let angle = AcosF(ClampF(Vector4.Dot(Vector4.Normalize(inDir), Vector4.Normalize(outDir)), -1.0, 1.0));
          if angle > 0.05 {
            let tangent = MinF(Vector4.Length(inDir), Vector4.Length(outDir));
            // (x 0.75: at the sharp corners left unrounded the car turned tighter than this
            // estimate, 9 m/s2 sideways against a 5 m/s2 limit; 0.55 crawled at 8 m/s, 2026-10-26)
            let radius = 0.75 * tangent / TanF(angle * 0.5);
            let cornerSpeed = SqrtF(this.CorneringAccel() * MaxF(radius, 1.0));
            allowed = MinF(allowed, SqrtF(cornerSpeed * cornerSpeed + 2.0 * this.brakeAccel * MaxF(s - start - tangent - lead, 0.0)));
          }
        }
      }
      distance = s - start;
      k += 1;
    }
    return allowed;
  }

  // The route from `from` (on segment this.segment) is free on the nav map for `along` metres.
  private func RouteClear(from: Vector4, along: Float) -> Bool {
    let n = ArraySize(this.route);
    let p = from;
    let left = along;
    let i = this.segment;
    while i < n - 1 && left > 0.0 {
      let b = this.route[i + 1];
      let d = Vector4.Distance(p, b);
      let end = d > left ? p + Vector4.Normalize(b - p) * left : b;
      if FlightNavMap.SegmentClear(p, end) < 1.0 {
        return false;
      }
      left -= d;
      p = b;
      i += 1;
    }
    return true;
  }

  // Point at route distance s from the start (clamped to the ends).
  private func PointAt(s: Float) -> Vector4 {
    let n = ArraySize(this.route);
    if s <= 0.0 {
      return this.route[0];
    }
    let i = 0;
    while i < n - 1 {
      if this.routeLength[i + 1] >= s {
        let len = this.routeLength[i + 1] - this.routeLength[i];
        let t = len > 0.001 ? (s - this.routeLength[i]) / len : 0.0;
        return this.route[i] + (this.route[i + 1] - this.route[i]) * t;
      }
      i += 1;
    }
    return this.route[n - 1];
  }

  // Signed horizontal curvature of the route around the lookahead: the heading change between
  // the stretch just ahead and the stretch beyond the lookahead point, over the distance between.
  private func CurvatureAhead(closest: Vector4, ahead: Float) -> Float {
    let a = this.AlongRoute(closest, 0.5 * ahead);
    let b = this.AlongRoute(closest, ahead);
    let c = this.AlongRoute(closest, 1.5 * ahead);
    let d1 = new Vector4(b.X - a.X, b.Y - a.Y, 0.0, 0.0);
    let d2 = new Vector4(c.X - b.X, c.Y - b.Y, 0.0, 0.0);
    if Vector4.Length(d1) < 1.0 || Vector4.Length(d2) < 1.0 {
      return 0.0;
    }
    let turn = Deg2Rad(Vector4.GetAngleDegAroundAxis(Vector4.Normalize(d1), Vector4.Normalize(d2), FlightUtils.Up()));
    return -turn / (0.5 * (Vector4.Length(d1) + Vector4.Length(d2)));
  }

  // A racing line's direction at route distance s and its bend there (towards the centre, 1 / radius
  // long), both taken over lineSpan either side.
  private func LineFrame(s: Float, out tangent: Vector4, out bend: Vector4) -> Void {
    this.LineFrameOver(s, this.lineSpan, tangent, bend);
  }

  private func LineFrameOver(s: Float, span: Float, out tangent: Vector4, out bend: Vector4) -> Void {
    let a = this.PointAt(s - span);
    let b = this.PointAt(s);
    let c = this.PointAt(s + span);
    let ab = b - a;
    let bc = c - b;
    let l1 = Vector4.Length(ab);
    let l2 = Vector4.Length(bc);
    tangent = Vector4.Length(c - a) > 0.01 ? Vector4.Normalize(c - a) : this.component.stats.d_forward;
    bend = new Vector4(0.0, 0.0, 0.0, 0.0);
    if l1 > 0.5 && l2 > 0.5 {
      bend = (bc * (1.0 / l2) - ab * (1.0 / l1)) * (2.0 / (l1 + l2));
    }
  }

  // Curvature of the route over the first part of the lookahead, 3D: towards the bend's centre,
  // 1 / radius long.
  private func CurvatureVector(closest: Vector4, ahead: Float) -> Vector4 {
    let a = this.AlongRoute(closest, 0.1 * ahead);
    let b = this.AlongRoute(closest, 0.4 * ahead);
    let c = this.AlongRoute(closest, 0.7 * ahead);
    let d1 = b - a;
    let d2 = c - b;
    let l1 = Vector4.Length(d1);
    let l2 = Vector4.Length(d2);
    if l1 < 1.0 || l2 < 1.0 {
      return new Vector4(0.0, 0.0, 0.0, 0.0);
    }
    return (d2 * (1.0 / l2) - d1 * (1.0 / l1)) * (2.0 / (l1 + l2));
  }

  // --- control -----------------------------------------------------------------------------

  private func PlayerInput() -> Bool {
    return AbsF(this.component.surge) > 0.15 || AbsF(this.component.yaw) > 0.15 || AbsF(this.component.lift) > 0.15
      || AbsF(this.component.sway) > 0.15 || this.component.linearBrake > 0.15;
  }

  // The car's position, with a jump further than it could have flown held off for up to
  // positionJumpHold (then taken as a real move, a teleport). In free roam over the Badlands edge
  // the car froze for 4 s and its position read where flight was switched on, 2 km back, for a
  // moment: off the route by that much, the Autopilot replanned from there and braked for the end
  // of the new route (2026-10-28).
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
  // tools/autopilot_telemetry.py summarises a file.
  private func WriteTelemetry(p: Vector4, v: Vector4, timeDelta: Float) -> Void {
    if !this.telemetry {
      if this.telemetryFile != 0 {
        FlightTelemetry.End(this.telemetryFile);
        this.telemetryFile = 0;
      }
      return;
    }
    if this.telemetryFile == 0 {
      this.telemetryFile = FlightTelemetry.Begin("autopilot", "t,x,y,z,vx,vy,vz,speed,speed2d,target,target_rate,cmd,off,following,maneuver,on_line,segment,"
        + "plan_x,plan_y,plan_z,cmd_fwd_x,cmd_fwd_y,cmd_fwd_z,cmd_up_x,cmd_up_y,cmd_up_z,fwd_x,fwd_y,fwd_z,up_x,up_y,up_z,"
        + "attitude_error,plan_cmd,cmd_car,up_cmd_car,plan_pitch,cmd_pitch,car_pitch,collision_timer,"
        + "drone,cmd_tilt,car_tilt,thrust_right,thrust_fwd,thrust_up,brake_slew");
      this.telemetryClock = 0.0;
      if this.telemetryFile == 0 {
        return;
      }
    }
    this.telemetryClock += timeDelta;
    let stats = this.component.stats;
    let n = ArraySize(this.route);
    let plan = new Vector4(0.0, 0.0, 0.0, 0.0);
    if this.segment + 1 < n {
      plan = this.route[this.segment + 1] - this.route[this.segment];
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
    ArrayPush(row, this.targetRate);
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
    ArrayPush(row, this.droneBrakeAmount * this.droneBrakeSide);
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
    let stats = this.component.stats;
    let p = this.CheckedPosition(stats.d_position, timeDelta);
    let v = stats.d_velocity;
    this.WriteTelemetry(p, v, timeDelta);

    this.UpdateRoute(timeDelta);

    if this.PlayerInput() {
      this.overrideTimer = this.overrideHold;
    }
    if this.overrideTimer > 0.0 {
      // manual: player inputs as given, altitude held by damping vertical speed
      this.overrideTimer -= timeDelta;
      this.holdPosition = p;
      this.takingOff = false;
      this.speedCmd = stats.d_speed;
      this.speedRate = 0.0;
      this.accelCmd = new Vector4(0.0, 0.0, 0.0, 0.0);
      this.yawCmd = 0.0;
      if IsDefined(this.maneuver) {
        FlightLog.Info("[Autopilot] " + this.maneuver.GetName() + " cut short by player input");
        this.maneuver = null;
      }
      this.headingRef = stats.d_forward2D;
      if this.underCover && !this.UnderCover(p) {
        this.underCover = false; // flown out into the open by hand: plan from here
        this.status = "idle";
      }
      this.UpdateWithNormalLift(timeDelta, FlightUtils.Up(), 9.81 * this.gravityFactor + this.component.lift * this.standardModeLiftFactor - v.Z);
      return;
    }

    // where to go
    // takeoff: up the column to the takeoff point first (the route starts there)
    if this.takingOff && Vector4.Distance(p, this.takeoffPoint) < 1.5 && AbsF(v.Z) < 2.0 {
      this.takingOff = false;
      FlightLog.Info("[Autopilot] takeoff done at " + FloatToStringPrec(p.Z, 1));
    }
    let target = this.holdPosition;
    this.targetSpeed = 0.0;
    this.targetRate = 0.0;
    this.emergencyBrake = false;
    // a pitch manoeuvre flies its own shape; the route is picked up again after it
    if !this.IsAerobatic() && IsDefined(this.maneuver) {
      this.EndManeuver(); // switched off in Mod Settings mid-manoeuvre
    }
    let shaped = IsDefined(this.maneuver) && !this.maneuver.IsOverlay();
    this.overlayVelocity = new Vector4(0.0, 0.0, 0.0, 0.0);
    let following = Equals(this.status, "following") && ArraySize(this.route) >= 2 && !this.takingOff && !shaped;
    if following {
      let closest: Vector4;
      let offRoute: Float;
      this.ClosestOnRoute(p, closest, offRoute);
      this.offRouteNow = offRoute;
      this.closestNow = closest;
      this.closestS = this.routeLength[this.segment] + Vector4.Distance(this.route[this.segment], closest);
      if offRoute > this.replanDistance && this.replanTimer <= 0.0 && this.request == 0 {
        this.RequestRoute(p);
      }
      let ahead = ClampF(0.7 * stats.d_speed + 12.0, 15.0, 90.0);
      // don't aim far past the next sharp bend: a long lookahead cuts inside it, and route legs
      // graze obstacles there
      if this.segment + 2 < ArraySize(this.route) {
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
      let curvature = this.raceMode ? this.CurvatureVector(closest, ahead) : new Vector4(0.0, 0.0, 0.0, 0.0);
      this.pathCurvature += (curvature - this.pathCurvature) * ClampF(timeDelta / 0.25, 0.0, 1.0);
      // barrel roll: aim at the route point moved round the roll's circle
      if IsDefined(this.maneuver) && Equals(this.maneuver.kind, FlightManeuverKind.BarrelRoll) {
        let offset: Vector4;
        let barrelUp: Vector4;
        this.maneuver.BarrelOffset(this.maneuver.time, offset, this.overlayVelocity, barrelUp);
        target += offset;
      }
      this.targetSpeed = this.AllowedSpeed(closest);
      this.targetRate = this.raceMode ? this.RaceTargetRate(stats.d_speed) : 0.0;
      // off the route: slow down while getting back, the route's margins assume the car is on it
      let slackOff = this.routeLine ? this.raceOffLine + 1.0 : 2.0;
      let slowdown = this.routeLine ? 0.5 * this.offRouteSlowdown : this.offRouteSlowdown;
      this.targetSpeed = MinF(this.targetSpeed, MaxF(8.0, this.TopSpeed() - slowdown * MaxF(offRoute - slackOff, 0.0)));
      // where the current velocity is taking the car must be free on the map; brake to stop short
      // of the first blocked point otherwise. Only when the car is leaving a route that is clear
      // over the same distance: where the route itself grazes the map's margin (passing a ledge
      // or under a deck) the velocity test fails too, and braking there stalls the car on the line
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
        // (on a racing line, which the planner checked free all the way, only once the car has
        // left it: at speed the straight-line prediction runs into the walls past every bend, and
        // braking for those cost 24 s on the Dogtown route, 2026-10-26)
        let lineHeld = this.routeLine && offRoute < this.raceOffLine;
        if blockedAt && !lineHeld && travelled > 10.0 && this.DistanceToRoute(hit) > 2.0 && this.RouteClear(closest, Vector4.Length(reach)) {
          let room = MaxF(travelled - 5.0, 0.0);
          this.targetSpeed = MinF(this.targetSpeed, SqrtF(2.0 * (this.raceMode ? this.raceMargin * this.RaceDecel(0.0) : this.brakeAccel) * room));
          this.blockedAhead = true;
        }
      }
      // the speed actually flown follows the target through an S-curve: the rate of change eases
      // in and out (jerk-limited) instead of stepping to full acceleration, and the target's
      // frame-to-frame flicker around corners is filtered out. Braking for blocked space is not
      // eased.
      // (slowing responds faster than speeding up: lagging behind a braking curve means arriving
      // too fast; AllowedSpeed also starts its curves brakeLead earlier for the ramp-in)
      let slowing = this.targetSpeed < this.speedCmd;
      let rateDown = this.raceMode ? this.RaceDecel(this.speedCmd) : this.brakeMax;
      let rateUp = this.raceMode ? MaxF(this.standardModeSurgeFactor - this.DragAccel(this.speedCmd), 0.5) : this.accelUp;
      let wantedRate = ClampF(this.targetRate + (this.targetSpeed - this.speedCmd) * (slowing ? 2.0 * this.speedGain : this.speedGain), -rateDown, rateUp);
      let jerk = (this.raceMode ? this.raceJerk : (slowing ? this.brakeJerk : this.speedJerk)) * timeDelta;
      this.speedRate += ClampF(wantedRate - this.speedRate, -jerk, jerk);
      this.speedCmd = MaxF(this.speedCmd + this.speedRate * timeDelta, 0.0);
      // blocked space: ease into it like any other limit while there is room to stop at brakeMax,
      // snap down (and brake hard) only when there is not and it has been blocked for a moment.
      // At speed the 1.5 s velocity ray often pokes into geometry the route is about to bend
      // over or around for a frame or two; braking at 40 m/s2 for those was the harshest jolt.
      this.emergencyBrake = false;
      // once clear again, carry on from the speed the car has: the emergency snapped the flown
      // speed down, and easing back up from there kept braking hard after the danger had gone
      if !this.blockedAhead && this.blockedTimer > 0.2 {
        this.speedCmd = MaxF(this.speedCmd, stats.d_speed);
        this.speedRate = MaxF(this.speedRate, 0.0);
      }
      this.blockedTimer = this.blockedAhead ? this.blockedTimer + timeDelta : 0.0;
      if this.blockedAhead && this.blockedTimer > 0.2 && this.speedCmd > this.targetSpeed {
        let room = MaxF(this.targetSpeed * this.targetSpeed / (2.0 * this.brakeAccel), 0.5);
        if stats.d_speed * stats.d_speed / (2.0 * room) > this.brakeMax {
          this.speedCmd = this.targetSpeed;
          this.speedRate = MinF(this.speedRate, 0.0);
          this.emergencyBrake = true;
        }
      }
      // barely moving while it should fly: wedged against something the map doesn't know about
      // (or thrown somewhere enclosed by a collision); stop pushing and hold
      if stats.d_speed < 1.5 && this.targetSpeed > 5.0 {
        this.stuckTimer += timeDelta;
      } else {
        this.stuckTimer = 0.0;
      }
      if this.stuckTimer > 4.0 {
        this.status = "stuck";
        this.holdPosition = p;
        ArrayClear(this.route);
        this.stuckTimer = 0.0;
        FlightLog.Warn("[Autopilot] stuck at " + FloatToStringPrec(p.X, 1) + ", " + FloatToStringPrec(p.Y, 1) + ", " + FloatToStringPrec(p.Z, 1)
          + " (in free space: " + ToString(FlightNavMap.IsFree(p)) + ")");
      }
      if this.component.isPlayerMounted {
        this.PublishProgress();
      }
      if ArraySize(this.route) >= 2 {
        this.RecheckLandingSpot(timeDelta);
      }
      if this.freeRoam && this.remaining < this.arriveDistance && stats.d_speed < 3.0 {
        // the end of the route before the next leg joined: on from here (ChainFreeRoam)
        this.holdPosition = this.route[ArraySize(this.route) - 1];
        ArrayClear(this.route);
        this.status = "free roam";
        FlightLog.Info("[Autopilot] free roam: end of the route, planning on from here");
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
        this.holdPosition = this.landing ? this.landingSpot : this.route[ArraySize(this.route) - 1];
        if this.landing {
          this.landingStartZ = p.Z;
          this.landingHeading0 = stats.d_forward2D;
          this.landingHeading = this.landingPad >= 0
            ? FlightModeAutopilot.PadHeading(this.landingPad, stats.d_forward2D)
            : this.LandingHeading(this.landingSpot, stats.d_forward2D);
          // a landing turn under way (LandSlewYaw) ends on its own heading
          if this.landSlewOn {
            this.landingHeading = this.landSlewHeading;
          }
        }
        this.landSlewOn = false;
        ArrayClear(this.route);
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
      if this.takingOff {
        desiredVelocity.Z = ClampF(desiredVelocity.Z, -this.climbRate, this.climbRate);
      } else if Equals(this.status, "descending") {
        // over the spot first, at the height the descent began: moved to another spot on arrival
        // (the first one taken by a parked car), the car slid 28 m across while sinking and hit a
        // walkway at 9 m above the floor on the way (2026-10-29)
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
    this.onLine = this.raceMode && following && this.routeLine;
    if this.onLine {
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
      // plus the flown speed's own rate along the line: on velocity error alone the car trailed a
      // braking curve by rate / lineVelocityGain (7 m/s at 18 m/s2), more than brakeLead covers, and
      // reached the Petrochem gap at 45 m/s for a 33 m/s bend, into the wall (2026-10-28)
      accel = (desiredVelocity - v) * this.lineVelocityGain + bend * (this.speedCmd * this.speedCmd) + tangent * this.speedRate;
      target = this.PointAt(this.closestS + 30.0);
      this.lookahead = target;
      jerk = this.raceJerk;
    } else if this.raceMode && following {
      // the bend's own acceleration ahead of time (velocity error alone turns too late at speed);
      // the thrust limits are applied in the car's frame (FlyAerobatic)
      accel += this.pathCurvature * (this.speedCmd * this.speedCmd);
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
    // ease changes of the acceleration itself (it is what is felt): rate-limit the command,
    // except in an emergency stop for blocked space
    let change = accel - this.accelCmd;
    let maxChange = jerk * timeDelta;
    if !this.emergencyBrake && Vector4.Length(change) > maxChange {
      change = Vector4.Normalize(change) * maxChange;
    }
    this.accelCmd += change;
    accel = this.accelCmd;

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
    if this.takingOff {
      let goal = this.takeoffHeading0;
      if ArraySize(this.route) >= 2 {
        goal = new Vector4(this.route[1].X - this.route[0].X, this.route[1].Y - this.route[0].Y, 0.0, 0.0);
      }
      let f = ClampF((p.Z - this.takeoffStartZ) / MaxF(this.takeoffPoint.Z - this.takeoffStartZ, 0.5), 0.0, 1.0);
      heading = FlightModeAutopilot.BlendHeading(this.takeoffHeading0, goal, f * f * (3.0 - 2.0 * f)) * 10.0;
    } else if Equals(this.status, "descending") || Equals(this.status, "landed") {
      let f = ClampF((this.landingStartZ - p.Z) / MaxF(this.landingStartZ - this.landingSpot.Z, 0.5), 0.0, 1.0);
      heading = FlightModeAutopilot.BlendHeading(this.landingHeading0, this.landingHeading, f * f * (3.0 - 2.0 * f)) * 10.0;
    }
    if this.IsAerobatic() {
      this.FlyAerobatic(timeDelta, accel, heading);
      return;
    }
    // heading error part of the yaw input, eased into and out of turns (low-pass and a limit on how
    // fast it may change); the yaw-rate damping below stays instant, delaying it made the heading
    // swing back and forth (+-4 rad/s) when the car started facing away from the route
    let yawTurn = 0.0;
    if Vector4.Length(heading) > 2.0 {
      let error = Vector4.GetAngleDegAroundAxis(Vector4.Normalize(heading), stats.d_forward2D, FlightUtils.Up());
      // capped to what the path needs (about twice speed x curvature ahead, as yaw input at
      // roughly 3 rad/s per unit) and at least maxYawTurn: uncapped, the heading chased the
      // swinging aim direction coming out of sharp corners at up to 1.5 rad/s; capped flat at
      // 0.3, a 12 m/s bend needing 0.6 rad/s ran 9 m wide
      let yawCap = ClampF(MaxF(this.maxYawTurn, 0.67 * stats.d_speed2D * AbsF(this.curvatureAhead)), 0.0, 1.0);
      yawTurn = ClampF(error * this.yawGain, -yawCap, yawCap);
      // turning with the climb or descent: gently, the heading may trail the height a little and
      // finish the turn at the end (a short climb, or a descent losing most height early, made
      // for 1.5 rad/s swings)
      if this.takingOff || Equals(this.status, "descending") || Equals(this.status, "landed") {
        yawTurn = ClampF(yawTurn, -this.verticalTurnYaw, this.verticalTurnYaw);
      }
    }
    let yawStep = (yawTurn - this.yawCmd) * ClampF(timeDelta / this.yawSmoothing, 0.0, 1.0);
    this.yawCmd += ClampF(yawStep, -this.yawSlew * timeDelta, this.yawSlew * timeDelta);
    // positive yaw input turns right, which is a negative angular velocity about Z, so the damping
    // term adds the rate back in (it opposes the turn already under way)
    let yaw = 0.0;
    if Vector4.Length(heading) > 2.0 || AbsF(this.yawCmd) > 0.01 {
      yaw = ClampF(this.yawCmd + stats.d_angularVelocity.Z * this.yawRateDamping, -1.0, 1.0);
    }

    // acceleration in the car's frame
    let local = Quaternion.Conjugate(stats.d_orientation) * accel;
    let surge = ClampF(local.Y / this.standardModeSurgeFactor, -1.0, 1.0);
    // the brake adds a lot of deceleration at once: ease it in and keep it to half, full only in
    // an emergency stop
    let brake = this.emergencyBrake ? ClampF(-local.Y / 12.0, 0.0, 1.0) : ClampF((-local.Y - 2.0) / 24.0, 0.0, 0.5);
    // sideways thrust is what turns the car: in this flight mode the lift stays along world up
    // whatever the bank (d_localUp), so the bank is for looks; full sway at every speed (halved at
    // plane speeds, the car slid 14 m wide out of bends at full bank, 2026-10-26)
    let sway = ClampF(local.X / this.standardModeSwayFactor, -1.0, 1.0);

    // coordinated bank at speed: the route's own turn (v^2 x curvature) plus a small correction
    // for sideways velocity error, smoothed; nose pitch along the climb
    let lateralError = Vector4.Dot(desiredVelocity - v, stats.d_right);
    let lateral = stats.d_speed2D * stats.d_speed2D * this.curvatureAhead + this.lateralCorrection * lateralError * stats.d_speed2D * 0.1;
    let bankWanted = ClampF(AtanF(lateral, 9.81), -Deg2Rad(this.maxBankDeg), Deg2Rad(this.maxBankDeg)) * blend;
    // smoothed, and limited in how fast it may change, so the car rolls into and out of a turn
    // instead of flicking between full bank either way
    let bankStep = (bankWanted - this.bankTarget) * ClampF(timeDelta / this.bankTime, 0.0, 1.0);
    this.bankTarget += ClampF(bankStep, -Deg2Rad(this.bankRate) * timeDelta, Deg2Rad(this.bankRate) * timeDelta);
    // roll damping: the attitude control tracks the target with a lag and swings past it (rocking
    // by 10 to 20 degrees); take the measured roll rate out of the command so it eases in instead
    let rollNow = -AsinF(ClampF(Vector4.Dot(stats.d_right, FlightUtils.Up()), -1.0, 1.0)); // same sense as bankTarget
    if timeDelta > 0.0001 {
      let rate = (rollNow - this.lastRoll) / timeDelta;
      this.rollRate += (rate - this.rollRate) * ClampF(timeDelta / 0.1, 0.0, 1.0);
    }
    this.lastRoll = rollNow;
    let bank = this.bankTarget - this.rollDamping * this.rollRate;
    let climb = ClampF(AtanF(v.Z, MaxF(stats.d_speed2D, 1.0)), -Deg2Rad(this.maxPitchDeg), Deg2Rad(this.maxPitchDeg)) * blend;
    let normal = FlightUtils.Up();
    // UpdateWithNormalLift adds yaw x generalRollWithYaw of roll; take it back out, the bank is ours
    normal = Vector4.RotateAxis(normal, stats.d_forward, this.bankSign * bank - yaw * FlightSettings.GetInstance().generalRollWithYaw);
    normal = Vector4.RotateAxis(normal, stats.d_right, -this.pitchSign * climb);

    let upright = MaxF(Vector4.Dot(stats.d_up, FlightUtils.Up()), 0.5);
    let liftForce = (9.81 * this.gravityFactor + accel.Z) / upright;

    this.component.surge = surge;
    this.component.yaw = yaw;
    this.component.sway = sway;
    this.component.lift = 0.0;
    this.component.linearBrake = brake;
    this.component.angularBrake = 0.0;
    this.UpdateWithNormalLift(timeDelta, normal, liftForce);

    // per-frame telemetry for tuning (tools/autopilot_smoothness.py); switch on with
    // python ../cp2077_tools/bridge.py call LtbfBridge.Telemetry true
    if this.telemetry {
      this.telemetryTime += timeDelta;
      FlightLog.Info("[AutopilotT] " + FloatToStringPrec(this.telemetryTime, 3) + " " + FloatToStringPrec(p.X, 2) + " " + FloatToStringPrec(p.Y, 2) + " " + FloatToStringPrec(p.Z, 2)
        + " " + FloatToStringPrec(v.X, 2) + " " + FloatToStringPrec(v.Y, 2) + " " + FloatToStringPrec(v.Z, 2) + " " + FloatToStringPrec(stats.d_angularVelocity.Z, 3)
        + " " + FloatToStringPrec(this.targetSpeed, 2) + " " + FloatToStringPrec(accel.X, 2) + " " + FloatToStringPrec(accel.Y, 2) + " " + FloatToStringPrec(accel.Z, 2)
        + " " + FloatToStringPrec(yaw, 3) + " " + FloatToStringPrec(Rad2Deg(this.bankTarget), 2)
        + " " + FloatToStringPrec(Rad2Deg(AsinF(ClampF(Vector4.Dot(stats.d_right, FlightUtils.Up()), -1.0, 1.0))), 2)
        + " " + FloatToStringPrec(this.speedCmd, 2) + " " + FloatToStringPrec(this.offRouteNow, 2)
        + " " + (this.blockedAhead ? "1" : "0") + " " + (this.emergencyBrake ? "1" : "0")
        + " " + FloatToStringPrec(this.lookahead.X, 1) + " " + FloatToStringPrec(this.lookahead.Y, 1) + " " + FloatToStringPrec(this.lookahead.Z, 1)
        + " " + FloatToStringPrec(this.aheadSmooth, 1) + " " + this.status);
    }

    this.logTimer -= timeDelta;
    if this.logTimer <= 0.0 {
      this.logTimer = 1.0;
      FlightLog.Info("[Autopilot] " + this.status + " seg " + ToString(this.segment) + "/" + ToString(ArraySize(this.route))
        + " speed " + FloatToStringPrec(stats.d_speed, 1) + " target " + FloatToStringPrec(this.targetSpeed, 1)
        + " remaining " + FloatToStringPrec(this.remaining, 0) + " yaw " + FloatToStringPrec(yaw, 2)
        + " surge " + FloatToStringPrec(surge, 2) + " bank " + FloatToStringPrec(Rad2Deg(bank), 1)
        + " z " + FloatToStringPrec(p.Z, 1) + " -> " + FloatToStringPrec(target.Z, 1));
    }
  }

  // --- race ---------------------------------------------------------------------------------

  private func IsAerobatic() -> Bool {
    return this.aerobatics || this.raceMode;
  }

  // Air drag at speed v (FlightMode.ApplyPhysics), m/s2.
  private func DragAccel(v: Float) -> Float {
    let stats = this.component.stats;
    if !FlightSettings.GetInstance().physxFlightPhysics || stats.s_mass <= 0.0 {
      return 0.0;
    }
    return 1.2 * stats.s_airResistanceFactor * v * v / stats.s_mass;
  }

  // Where full surge equals the drag.
  private func RaceTopSpeed() -> Float {
    let stats = this.component.stats;
    let k = 1.2 * stats.s_airResistanceFactor / MaxF(stats.s_mass, 1.0);
    let push = this.droneStyle ? this.DroneAccel(this.standardModeSurgeFactor) : this.standardModeSurgeFactor;
    let top = k > 0.0 ? SqrtF(push / k) : this.raceSpeedCap;
    return MinF(0.97 * top, this.freeRoam ? this.freeRoamTopSpeed : this.raceSpeedCap);
  }

  // Slowing at speed v: full reverse surge, the brake (velocity damping, FlightMode_Standard) and drag.
  private func RaceDecel(v: Float) -> Float {
    let brake = FlightSettings.GetInstance().brakeFactorLinear * this.component.stats.s_brakingFrictionFactor * v;
    return this.standardModeSurgeFactor + brake + this.DragAccel(v);
  }

  // Sideways acceleration in a level turn: the full lift banked into it, the rest holding the car up.
  private func RaceLateral() -> Float {
    if this.droneStyle {
      return this.DroneAccel(this.standardModeSwayFactor);
    }
    let g = 9.81 * this.gravityFactor;
    let lift = g + this.standardModeLiftFactor;
    // plus the surge's share with the nose turned into the bend (drift)
    return SqrtF(MaxF(lift * lift - g * g, 1.0)) + this.standardModeSurgeFactor * SinF(Deg2Rad(this.raceDriftMax));
  }

  private func TopSpeed() -> Float {
    return this.raceMode ? this.RaceTopSpeed() : this.maxSpeed;
  }

  // Bend radius at route point k (3D): from an arc's own geometry, or for a sharp corner from the
  // route's direction cornerSpan before and after it, as AllowedSpeed judges them.
  private func PointRadius(k: Int32) -> Float {
    let n = ArraySize(this.route);
    if k <= 0 || k >= n - 1 {
      return 1000000.0;
    }
    let s = this.routeLength[k];
    if this.routeLine {
      return FlightModeAutopilot.Circumradius(this.PointAt(s - this.lineRadiusSpan), this.route[k], this.PointAt(s + this.lineRadiusSpan));
    }
    if ArraySize(this.routeArc) == n && this.routeArc[k] {
      return FlightModeAutopilot.Circumradius(this.route[k - 1], this.route[k], this.route[k + 1]);
    }
    let inDir = this.route[k] - this.PointAt(s - this.cornerSpan);
    let outDir = this.PointAt(s + this.cornerSpan) - this.route[k];
    if Vector4.Length(inDir) < 0.5 || Vector4.Length(outDir) < 0.5 {
      return 1000000.0;
    }
    let angle = AcosF(ClampF(Vector4.Dot(Vector4.Normalize(inDir), Vector4.Normalize(outDir)), -1.0, 1.0));
    if angle < 0.05 {
      return 1000000.0;
    }
    return 0.75 * MinF(Vector4.Length(inDir), Vector4.Length(outDir)) / TanF(angle * 0.5);
  }

  // Speed allowed at route point k for its descent: the sink rate held to raceMaxSink, and where the
  // path bends downwards (over a crest, into a dive) at most racePushOver of that bend.
  private func DescentSpeed(k: Int32) -> Float {
    let s = this.routeLength[k];
    let a = this.PointAt(s - this.lineSpan);
    let c = this.PointAt(s + this.lineSpan);
    let along = Vector4.Distance(a, c);
    let limit = 1000.0;
    if along > 1.0 && c.Z < a.Z && this.routeLength[ArraySize(this.route) - 1] - s < this.raceSinkApproach {
      limit = this.raceMaxSink / MaxF((a.Z - c.Z) / along, 0.05);
    }
    if this.routeLine {
      let tangent: Vector4;
      let bend: Vector4;
      this.LineFrame(s, tangent, bend);
      if bend.Z < -0.0001 {
        limit = MinF(limit, SqrtF((this.droneStyle ? this.dronePushOver : this.racePushOver) / -bend.Z));
      }
    }
    return limit;
  }

  // Speed after braking back over `distance` metres from speed v at the race decel (stepped: the
  // brake and the drag grow with speed), at most top.
  private func RaceBrakeBack(v: Float, distance: Float, top: Float) -> Float {
    let vv = v;
    let left = distance;
    while left > 0.0 && vv < top {
      let step = MinF(left, 5.0);
      vv = SqrtF(vv * vv + 2.0 * MinF(this.raceMargin * this.RaceDecel(vv), this.racePlanDecel) * step);
      left -= step;
    }
    return MinF(vv, top);
  }

  // RaceBrakeBack in a bend of the given radius: the lift banked into the bend is not there for
  // braking, so the braking share shrinks as the bend takes more of the lateral envelope (a
  // friction circle), down to raceBrakeGripMin. Braking and turning at once into the Petrochem gap
  // the car slowed at 7 to 11 m/s2 against the 18 planned (2026-10-28).
  private func RaceBrakeBackCornering(v: Float, distance: Float, top: Float, radius: Float, lateral: Float) -> Float {
    let vv = v;
    let left = distance;
    while left > 0.0 && vv < top {
      let step = MinF(left, 5.0);
      let use = MinF(vv * vv / MaxF(radius, 1.0) / MaxF(lateral, 1.0), 1.0);
      let grip = MaxF(SqrtF(1.0 - use * use), this.raceBrakeGripMin);
      vv = SqrtF(vv * vv + 2.0 * MinF(this.raceMargin * this.RaceDecel(vv), this.racePlanDecel) * grip * step);
      left -= step;
    }
    return MinF(vv, top);
  }

  // Fastest speed at each route point: what its bend allows with the lift banked into it, then a
  // backward pass so every point can still be slowed for the ones after it; 0 at the goal.
  private func BuildSpeedProfile() -> Void {
    ArrayClear(this.routeSpeed);
    let n = ArraySize(this.route);
    let top = this.RaceTopSpeed();
    let lateral = this.raceMargin * this.RaceLateral();
    let i = 0;
    while i < n {
      ArrayPush(this.routeSpeed, MinF(MinF(top, SqrtF(lateral * MaxF(this.PointRadius(i), 1.0))), this.DescentSpeed(i)));
      i += 1;
    }
    this.routeSpeed[n - 1] = 0.0;
    // the final approach braked gently: planned at the full race braking the car reached the
    // approach point at 19 m/s, 2026-10-27
    let total = this.routeLength[n - 1];
    i = 0;
    while i < n {
      let left = total - this.routeLength[i];
      if left < this.raceArriveLength {
        this.routeSpeed[i] = MinF(this.routeSpeed[i], SqrtF(2.0 * this.raceArriveDecel * MaxF(left, 0.0)));
      }
      i += 1;
    }
    i = n - 2;
    let radiusAfter = this.PointRadius(n - 1);
    while i >= 0 {
      let d = this.routeLength[i + 1] - this.routeLength[i];
      let radius = this.PointRadius(i);
      this.routeSpeed[i] = MinF(this.routeSpeed[i], this.RaceBrakeBackCornering(this.routeSpeed[i + 1], d, top, MinF(radius, radiusAfter), lateral));
      radiusAfter = radius;
      i -= 1;
    }
    this.FlattenSpeedHumps();
    this.PlanStops();
    let stats = this.component.stats;
    FlightLog.Info("[Autopilot] race" + (this.droneStyle ? " (drone)" : "") + ": " + ToString(ArraySize(this.stopStartS)) + " stops, top " + FloatToStringPrec(top, 1) + " m/s, lateral " + FloatToStringPrec(this.RaceLateral(), 1)
      + " m/s2, decel " + FloatToStringPrec(this.RaceDecel(0.0), 1) + " + " + FloatToStringPrec(this.RaceDecel(30.0) - this.RaceDecel(0.0), 1) + " at 30 m/s"
      + " (mass " + FloatToStringPrec(stats.s_mass, 0) + ", air " + FloatToStringPrec(stats.s_airResistanceFactor, 3) + ", brake friction " + FloatToStringPrec(stats.s_brakingFrictionFactor, 2) + ")");
  }

  // Small rises of the profile between two slowings held down: where the speed climbs less than
  // speedHumpMerge above the lower of the slowest points within speedHumpSpan either side, it stays
  // at that. Into Dogtown the profile braked 109 to 41 m/s, rose to 45 and braked again to 30: two
  // stops, flown as two, for 4 m/s (2026-10-29). Only lowers speeds, so every point can still be
  // slowed for the ones after it.
  private func FlattenSpeedHumps() -> Void {
    let n = ArraySize(this.routeSpeed);
    if n < 3 {
      return;
    }
    let flat: array<Float>;
    let i = 0;
    while i < n {
      let left = this.routeSpeed[i];
      let j = i;
      while j > 0 && this.routeLength[i] - this.routeLength[j - 1] <= this.speedHumpSpan {
        j -= 1;
        left = MinF(left, this.routeSpeed[j]);
      }
      let right = this.routeSpeed[i];
      j = i;
      while j < n - 1 && this.routeLength[j + 1] - this.routeLength[i] <= this.speedHumpSpan {
        j += 1;
        right = MinF(right, this.routeSpeed[j]);
      }
      let low = MaxF(left, right);
      ArrayPush(flat, this.routeSpeed[i] > low && this.routeSpeed[i] - low < this.speedHumpMerge ? low : this.routeSpeed[i]);
      i += 1;
    }
    this.routeSpeed = flat;
  }

  // The route's stops: stretches where the speed the car will fly falls at droneStopMinDecel or
  // more, joined when less than droneStopJoin apart, and kept when the speed falls by
  // droneStopMinDrop over one. The speed flown is the profile capped by what the car can speed up
  // to (surge less drag, from its speed now): the profile alone is the speed allowed, which falls
  // long before the car gets there, and the first try made one stop of the last 1000 m of the route
  // (2026-10-29). Gentle slowing (the arrival's raceArriveDecel) is no stop. Each stop gets the side
  // its slew swings to (towards the turn at its end, the racing line's bend droneStopTurnAhead past
  // it) and a strength from its mean braking. Planned, the slew starts with the braking and is one
  // action per stop: reacting to the commanded braking frame by frame it began 2.5 s late and swung
  // once per dip of the profile.
  private func PlanStops() -> Void {
    // a slew under way when the route's end moves (a parked car on the landing spot) carries on to
    // the new end: replanned, the moved end's bend dropped it and the nose swung back 50 m out
    let keepSide = 0.0;
    let keepAmount = 0.0;
    let keepStart = 0.0;
    let keepEnd = 0.0;
    let keepToEnd = false;
    let k = 0;
    while this.keepSlew && k < ArraySize(this.stopStartS) {
      if this.closestS >= this.stopStartS[k] && this.closestS <= this.stopEndS[k] {
        keepSide = this.stopSide[k];
        keepAmount = this.stopAmount[k];
        keepStart = this.stopStartS[k];
        keepEnd = this.stopEndS[k];
        keepToEnd = this.stopToEnd[k];
      }
      k += 1;
    }
    this.keepSlew = false;
    ArrayClear(this.stopStartS);
    ArrayClear(this.stopEndS);
    ArrayClear(this.stopSide);
    ArrayClear(this.stopAmount);
    ArrayClear(this.stopToEnd);
    let n = ArraySize(this.routeSpeed);
    if n < 3 || !this.routeLine {
      return;
    }
    // from where the car is now, at its speed (the profile can be rebuilt mid-route)
    let first = Clamp(this.segment, 0, n - 1);
    let flown: array<Float>;
    let i = 0;
    while i <= first {
      ArrayPush(flown, i == first ? MinF(this.routeSpeed[i], this.component.stats.d_speed) : this.routeSpeed[i]);
      i += 1;
    }
    while i < n {
      let v = flown[i - 1];
      let d = this.routeLength[i] - this.routeLength[i - 1];
      let up = MaxF(this.standardModeSurgeFactor - this.DragAccel(v), 0.5);
      ArrayPush(flown, MinF(this.routeSpeed[i], SqrtF(v * v + 2.0 * up * d)));
      i += 1;
    }
    let starts: array<Int32>;
    let ends: array<Int32>;
    i = 0;
    while i < n - 1 {
      let d = MaxF(this.routeLength[i + 1] - this.routeLength[i], 0.01);
      let decel = (flown[i] * flown[i] - flown[i + 1] * flown[i + 1]) / (2.0 * d);
      // the arrival's gentler braking counts near the end: the stop into a landing is one with it
      let arriving = this.routeLength[n - 1] - this.routeLength[i] < this.droneStopArrive;
      if decel >= (arriving ? 0.8 * this.raceArriveDecel : this.droneStopMinDecel) {
        let k = ArraySize(starts);
        if k > 0 && this.routeLength[i] - this.routeLength[ends[k - 1]] < this.droneStopJoin {
          ends[k - 1] = i + 1;
        } else {
          ArrayPush(starts, i);
          ArrayPush(ends, i + 1);
        }
      }
      i += 1;
    }
    // a stop that ends near the route's end runs on to it (into the landing), as one
    let last = ArraySize(ends) - 1;
    if last >= 0 && this.routeLength[n - 1] - this.routeLength[ends[last]] < this.droneStopArrive {
      ends[last] = n - 1;
    }
    k = 0;
    let kept = 0;
    while k < ArraySize(starts) {
      let a = starts[k];
      let b = ends[k];
      let va = flown[a];
      let vb = flown[b];
      if va - vb >= this.droneStopMinDrop {
        let length = MaxF(this.routeLength[b] - this.routeLength[a], 1.0);
        let decel = (va * va - vb * vb) / (2.0 * length);
        let f = ClampF((decel - 3.0) / 8.0, 0.0, 1.0);
        f = f * f * (3.0 - 2.0 * f);
        FlightLog.Info("[Autopilot] stop " + ToString(kept) + ": " + FloatToStringPrec(this.routeLength[a], 0) + " to " + FloatToStringPrec(this.routeLength[b], 0)
          + " m, " + FloatToStringPrec(va, 0) + " to " + FloatToStringPrec(vb, 0) + " m/s (" + FloatToStringPrec(decel, 1) + " m/s2)");
        // slewed only on its straight stretches (droneSlewMinStraight long at least), unwound
        // droneSlewMargin before a bend: turns are flown with the nose along the line (Jack liked
        // the turn into the Dogtown gap that way, 2026-10-29). Faded by the bend point by point
        // instead, the slew swung in and out three times through that stop.
        // A gentle bend leading into a turn is part of the turn: a stretch running into a bend
        // ends where the line has turned droneSlewHeadingOff from the stretch's start, unwound
        // droneSlewRelease before that at the flown speed, and must last droneSlewMinTime. Unwound
        // only 30 m before the sharp part, the nose swung 45 degrees towards the Dogtown gap 4 s
        // before it and back out as the car turned in; that brake is now flown nose along the line,
        // like the turn before slewing stops. A stretch running to the stop's end (the landing)
        // ends with it, however the line turns there.
        let s = this.routeLength[a];
        let runStart = -1.0;
        let runSpeed = 0.0;
        let runHeading: Vector4;
        let turnedS = -1.0;
        let j = a;
        let cosOff = CosF(Deg2Rad(this.droneSlewHeadingOff));
        while s <= this.routeLength[b] + 0.01 {
          while j < b && this.routeLength[j + 1] < s {
            j += 1;
          }
          let tangent: Vector4;
          let bend: Vector4;
          this.LineFrameOver(s, this.lineSpan, tangent, bend);
          let flat = Vector4.Normalize(new Vector4(tangent.X, tangent.Y, 0.0, 0.0));
          let straight = Vector4.Length(new Vector4(bend.X, bend.Y, 0.0, 0.0)) < this.droneSlewBendOff;
          if straight && runStart < 0.0 {
            runStart = s;
            runSpeed = flown[j];
            runHeading = flat;
            turnedS = -1.0;
          }
          if runStart >= 0.0 && turnedS < 0.0 && Vector4.Dot(flat, runHeading) < cosOff {
            turnedS = s;
          }
          let last = s + 5.0 > this.routeLength[b] + 0.01;
          if runStart >= 0.0 && (!straight || last) {
            // the run ends here; up to a turn it is unwound before it
            let runEnd = straight ? s : (turnedS >= 0.0 ? turnedS : s) - MaxF(this.droneSlewMargin, flown[j] * this.droneSlewRelease);
            if runEnd - runStart >= MaxF(this.droneSlewMinStraight, runSpeed * this.droneSlewMinTime) {
              this.LineFrameOver(runEnd + this.droneStopTurnAhead, this.lineSpan, tangent, bend);
              let cross = Vector4.Cross(tangent, bend);
              let turn = cross.Z; // positive: the line turns left
              ArrayPush(this.stopStartS, runStart);
              ArrayPush(this.stopEndS, runEnd);
              ArrayPush(this.stopSide, turn > 0.0 ? -1.0 : 1.0);
              ArrayPush(this.stopAmount, f);
              let toEnd = straight && b == n - 1;
              ArrayPush(this.stopToEnd, toEnd);
              FlightLog.Info("[Autopilot]   slew " + FloatToStringPrec(runStart, 0) + " to " + FloatToStringPrec(runEnd, 0) + " m " + (turn > 0.0 ? "left" : "right")
                + " " + FloatToStringPrec(f, 2) + (toEnd ? " into the landing" : ""));
            }
            runStart = -1.0;
          }
          s += 5.0;
        }
        kept += 1;
      }
      k += 1;
    }
    if keepSide != 0.0 && this.PlannedSlew(this.closestS) == 0.0 {
      // it ran to the old end: it runs to the new one (from its own start, so the swing goes on)
      let end = keepToEnd ? this.routeLength[n - 1] : keepEnd;
      ArrayPush(this.stopStartS, keepStart);
      ArrayPush(this.stopEndS, end);
      ArrayPush(this.stopSide, keepSide);
      ArrayPush(this.stopAmount, keepAmount);
      ArrayPush(this.stopToEnd, keepToEnd);
      FlightLog.Info("[Autopilot]   slew kept " + FloatToStringPrec(keepStart, 0) + " to " + FloatToStringPrec(end, 0) + " m");
    }
  }
  // The route's end moved over to a landing spot `to` (the end keeping its height above the floor):
  // the last landingShiftSpan metres shifted by the same offset, eased in, each moved point free
  // on the map; false (and nothing changed) where one is not. The speed profile is rebuilt.
  private func ShiftRouteEnd(to: Vector4) -> Bool {
    let n = ArraySize(this.route);
    if n < 3 {
      return false;
    }
    let total = this.routeLength[n - 1];
    let end = this.route[n - 1];
    let offset = new Vector4(to.X - end.X, to.Y - end.Y, to.Z - this.landingSpot.Z, 0.0);
    let moved: array<Vector4>;
    let i = 0;
    while i < n {
      let q = this.route[i];
      let left = total - this.routeLength[i];
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
    this.route = moved;
    ArrayClear(this.routeLength);
    let sum = 0.0;
    i = 0;
    while i < n {
      if i > 0 {
        sum += Vector4.Distance(this.route[i - 1], this.route[i]);
      }
      ArrayPush(this.routeLength, sum);
      i += 1;
    }
    this.routeGoal = this.route[n - 1];
    ArrayClear(this.routeSpeed);
    this.keepSlew = true;
    return true;
  }

  // The landing spot again on the approach, once the cars around it have streamed in: taken, the
  // route's end is moved to the nearest free one (ShiftRouteEnd), so the car flies into it in one
  // go. Checked on arrival only, the car came to a stop above a parked car's spot and then moved
  // 28 m over before coming down, two actions where there should be one (2026-10-29).
  private func RecheckLandingSpot(timeDelta: Float) -> Void {
    if !this.landing || this.landingPad >= 0 || this.remaining > this.landingRecheck || this.remaining < 15.0 {
      return;
    }
    this.spotCheckTimer -= timeDelta;
    if this.spotCheckTimer > 0.0 {
      return;
    }
    this.spotCheckTimer = 0.5;
    if !this.SpotTaken(this.landingSpot) {
      return;
    }
    let top: Float;
    let spot: Vector4;
    if this.FindLandingSpot(this.destination, spot, top) && Vector4.Distance2D(spot, this.landingSpot) > 2.0 {
      let over = Vector4.Distance2D(spot, this.landingSpot);
      if this.ShiftRouteEnd(spot) {
        this.landingSpot = spot;
        FlightLog.Info("[Autopilot] landing spot taken by a vehicle, " + FloatToStringPrec(this.remaining, 0) + " m out: route end moved "
          + FloatToStringPrec(over, 1) + " m to " + FloatToStringPrec(spot.X, 1) + ", " + FloatToStringPrec(spot.Y, 1));
      }
    }
  }

  // The slewed stretch (PlanStops) at route distance s, -1 none.
  private func SlewIndexAt(s: Float) -> Int32 {
    let k = 0;
    while k < ArraySize(this.stopStartS) {
      if s >= this.stopStartS[k] && s <= this.stopEndS[k] {
        return k;
      }
      k += 1;
    }
    return -1;
  }

  // Progress 0..1 through stretch k at s: into the landing up to where the descent takes over.
  // Eased in over the first fifth, then even with the distance, so the nose turns slower as the
  // car slows.
  private func SlewProgress(k: Int32, s: Float) -> Float {
    let end = this.stopToEnd[k] ? this.stopEndS[k] - this.landingEntry : this.stopEndS[k];
    let p = ClampF((s - this.stopStartS[k]) / MaxF(end - this.stopStartS[k], 1.0), 0.0, 1.0);
    return (p < 0.2 ? p * p / 0.4 : p - 0.1) / 0.9;
  }

  // The slew planned at route distance s (signed: positive swings the nose right), in
  // droneBrakeYaw, 0 elsewhere: out and back in one arc, peaking halfway (held at an angle, it
  // looked locked, Jack 2026-10-29). Into the landing it is the progress of the landing turn
  // (LandSlewYaw), not an angle off the line.
  private func PlannedSlew(s: Float) -> Float {
    let k = this.SlewIndexAt(s);
    if k < 0 {
      return 0.0;
    }
    let p = ClampF((s - this.stopStartS[k]) / MaxF(this.stopEndS[k] - this.stopStartS[k], 1.0), 0.0, 1.0);
    return this.stopSide[k] * this.stopAmount[k] * (this.stopToEnd[k] ? this.SlewProgress(k, s) : SinF(3.14159265 * p));
  }

  // Into the landing the nose makes one turn, from where it was when the stretch began to the
  // heading it lands on, the descent finishing it (one motion, Jack 2026-10-29: as an angle off
  // the line it followed the moved end's wiggles, and below 12 m/s the nose fell back to the old
  // hover heading, then turned again as it sank: 70 degrees back, 70 on, 80 back). The landing
  // heading is the spot's most open line (LandingHeading) or the pad's, whichever of its two ways
  // makes a turn in the slew's direction nearest droneLandSlewYaw. Returns the nose yaw wanted.
  // Started by a stretch into the landing (k), it then runs on its own to where the descent takes
  // over, however the stops are planned again: chosen again for a moved spot it went back 75
  // degrees at 110 deg/s, and restarted with a replanned stretch's start it went back 50.
  private func LandSlewYaw(k: Int32, s: Float, yaw: Float) -> Float {
    if !this.landSlewOn {
      this.landSlewOn = true;
      this.landSlewFrom = this.stopStartS[k];
      this.landSlewP = 0.0;
      this.landSlewLastS = MinF(s, this.landSlewFrom);
      this.landSlewStartYaw = yaw;
      let start = this.landSlewStartYaw;
      let startHeading = new Vector4(CosF(start), SinF(start), 0.0, 0.0);
      let base = this.landingPad >= 0 ? FlightModeAutopilot.PadHeading(this.landingPad, startHeading) : this.LandingHeading(this.landingSpot, startHeading);
      // a positive slew swings the nose right, a falling yaw
      let direction = this.stopSide[k] > 0.0 ? -1.0 : 1.0;
      let want = Deg2Rad(this.droneLandSlewYaw);
      let best = -1.0;
      let i = 0;
      while i < (this.landingPad >= 0 ? 1 : 2) {
        let heading = i == 0 ? base : base * -1.0;
        let turn = FlightModeAutopilot.WrapAngle(AtanF(heading.Y, heading.X) - start) * direction;
        if turn < 0.0 {
          turn += 6.2831853;
        }
        if best < 0.0 || AbsF(turn - want) < AbsF(best - want) {
          best = turn;
          this.landSlewHeading = heading;
        }
        i += 1;
      }
      this.landSlewDelta = best * direction;
      FlightLog.Info("[Autopilot]   landing turn " + FloatToStringPrec(Rad2Deg(this.landSlewDelta), 0) + " deg");
    }
    // progress carried on over the distance left, so a moved end (26 m further for a parked car)
    // stretches what is left of the turn instead of setting it back (17 degrees, a stall)
    let end = this.routeLength[ArraySize(this.routeLength) - 1] - this.landingEntry;
    let ds = s - this.landSlewLastS;
    if ds > 0.0 {
      this.landSlewP = ClampF(this.landSlewP + (1.0 - this.landSlewP) * ds / MaxF(end - this.landSlewLastS, ds), 0.0, 1.0);
      this.landSlewLastS = s;
    }
    let p = this.landSlewP;
    return this.landSlewStartYaw + this.landSlewDelta * (p < 0.2 ? p * p / 0.4 : p - 0.1) / 0.9;
  }

  // Speed allowed now: the next route point's profile speed, braked back over the distance to it
  // (started early by the flown speed's ramp); towards the goal, stopping 3 m short of it.
  private func RaceAllowedSpeed(toCorner: Float) -> Float {
    if ArraySize(this.routeSpeed) != ArraySize(this.route) {
      this.BuildSpeedProfile();
    }
    let lead = this.speedCmd * this.brakeLead;
    let top = this.RaceTopSpeed();
    let next = this.routeSpeed[this.segment + 1];
    let distance = toCorner - lead;
    if this.segment + 2 == ArraySize(this.route) {
      distance -= 3.0;
    }
    return this.RaceBrakeBack(next, MaxF(distance, 0.0), top);
  }

  // RaceAllowedSpeed at route distance s instead of the car's (the profile is built already).
  private func RaceAllowedAt(s: Float) -> Float {
    let n = ArraySize(this.route);
    let k = Max(this.segment + 1, 1);
    while k < n - 1 && this.routeLength[k] <= s {
      k += 1;
    }
    let lead = this.speedCmd * this.brakeLead;
    let distance = this.routeLength[k] - s - lead;
    if k == n - 1 {
      distance -= 3.0;
    }
    let allowed = this.RaceBrakeBack(this.routeSpeed[k], MaxF(distance, 0.0), this.RaceTopSpeed());
    // between two points the profile's own slope (v^2 linear along the segment): braked back at
    // the full race decel from the next point instead, the allowed speed was a sawtooth over each
    // 5 m segment wherever the profile slows gently (the arrival's raceArriveDecel), and its rate
    // read ahead (RaceTargetRate) flicked between -2.9 and -5.8 m/s2 frame to frame, the thrust
    // with it (2026-10-28). Read at s + lead, in the segment there: the lead (0.6 s of the flown
    // speed) is longer than a segment, so in the car's own one it always read the next point.
    let sl = s + lead;
    let j = k;
    while j < n - 1 && this.routeLength[j] <= sl {
      j += 1;
    }
    let len = this.routeLength[j] - this.routeLength[j - 1];
    if len > 0.01 {
      let t = ClampF((sl - this.routeLength[j - 1]) / len, 0.0, 1.0);
      let a = this.routeSpeed[j - 1];
      let b = this.routeSpeed[j];
      allowed = MinF(allowed, SqrtF(MaxF(a * a + (b * b - a * a) * t, 0.0)));
    }
    return allowed;
  }

  // How fast the allowed speed falls as the car flies on (m/s2, 0 or less), from the profile
  // raceRateProbe seconds ahead. Fed forward into the flown speed: on the speed error alone it
  // trailed a braking curve by rate / (2 speedGain), 5 to 7 m/s over the profile into the
  // Petrochem gap even with the car on the flown speed (2026-10-28).
  private func RaceTargetRate(speed: Float) -> Float {
    if ArraySize(this.routeSpeed) != ArraySize(this.route) || ArraySize(this.route) < 2 {
      return 0.0;
    }
    let ahead = this.RaceAllowedAt(this.closestS + MaxF(speed, 1.0) * this.raceRateProbe);
    return MinF((ahead - this.RaceAllowedAt(this.closestS)) / this.raceRateProbe, 0.0);
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
    let reach = stats.d_speed * FlightManeuver.Create(kind, side, p, v, this.maneuverAccel, this.maneuverRollTime).duration;
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
    let m = FlightManeuver.Create(kind, side, p, stats.d_velocity, this.maneuverAccel, this.maneuverRollTime);
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
    if !this.IsAerobatic() {
      return "aerobatics off";
    }
    if IsDefined(this.maneuver) {
      return "busy with " + this.maneuver.GetName();
    }
    let k = IntEnum<FlightManeuverKind>(kind);
    let following = Equals(this.status, "following") && ArraySize(this.route) >= 2 && !this.takingOff;
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
    let f = this.raceMode ? this.raceRollFrequency : this.rollFrequency;
    let accelMax = this.raceMode ? this.raceRollAccelMax : this.rollAccelMax;
    let rateMax = this.raceMode ? this.raceRollRateMax : this.rollRateMax;
    let rollAccel = ClampF(f * f * error - 2.0 * f * this.bankRateNow, -accelMax, accelMax);
    this.bankRateNow = ClampF(this.bankRateNow + rollAccel * dt, -rateMax, rateMax);
    this.bankAngle = FlightModeAutopilot.WrapAngle(this.bankAngle + this.bankRateNow * dt);
    return level * CosF(this.bankAngle) + side * SinF(this.bankAngle);
  }

  // In Race: the nose's pitch eased (critically damped, rate and acceleration limited), on the
  // nose's own heading. On a racing line the pitch wanted is the line's slope ahead; off it (the
  // take-off and the arrival) the nose's own. Elsewhere the nose stays as it is.
  private func EasePitch(dt: Float, fwd: Vector4) -> Vector4 {
    let level = Vector4.Normalize2D(fwd);
    if !this.raceMode || Vector4.Length(level) < 0.5 {
      this.SyncPitch(fwd);
      return fwd;
    }
    let wanted = AsinF(ClampF(fwd.Z, -1.0, 1.0));
    if this.onLine {
      let tangent: Vector4;
      let bend: Vector4;
      this.LineFrameOver(this.closestS + this.component.stats.d_speed * this.ribbonLead, this.ribbonSpan, tangent, bend);
      // the same steep-path rule as the nose off the line: on the final drop to a landing spot
      // the line's own slope held the nose at the ground for 4 s (2026-10-27)
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

  // Drone style's attitude target: the nose level, along the flight path (the heading at hover
  // speeds, blend as in FlyAerobatic), the body tilted on each axis towards the thrust direction
  // the commanded acceleration and gravity need (DroneBodyAngle). FlyAerobatic's thrust then gives
  // that acceleration in the car's frame: what the tilt leaves along the nose and across goes to
  // surge and sway, which swivel the thrusters.
  private func DroneAttitude(dt: Float, accel: Vector4, blend: Float, live: Bool, out fwd: Vector4, out up: Vector4) -> Void {
    let stats = this.component.stats;
    let z = FlightUtils.Up();
    let g = 9.81 * this.gravityFactor;
    // where the nose points: on a racing line along the line a moment ahead, which is smooth by
    // construction; off it the velocity's horizontal part. That part is small and noisy on a
    // steep climb or drop, and the nose twitched there (yaw jitter 7 deg/s RMS, the command's far
    // more, over four legs with 16 to 32% of the route steeper than 45 degrees, 2026-10-29): it
    // holds the last heading where the direction is near vertical
    // (Flat returns a unit vector: the horizontal part with its length is taken by hand)
    let ahead3 = stats.d_velocity + accel * this.attitudeLead;
    let path = new Vector4(ahead3.X, ahead3.Y, 0.0, 0.0);
    let pathWeight = ClampF(Vector4.Length(path) / 6.0, 0.0, 1.0);
    if this.onLine {
      let tangent: Vector4;
      let bend: Vector4;
      this.LineFrameOver(this.closestS + stats.d_speed * this.droneYawLead, this.lineSpan, tangent, bend);
      path = FlightModeAutopilot.Flat(tangent);
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
    // braking turned into a slew (droneBrakeTurn): the nose swings towards the turn coming, up to
    // droneBrakeYaw, so the stop is a tilt across as much as back
    let swing = 0.0;
    let wantedYaw = AtanF(h.Y, h.X);
    if this.droneBrakeTurn && this.onLine && Equals(this.status, "following") {
      // from the plan (PlanStops), droneStopLead seconds ahead, so the swing starts with the braking
      let sAhead = this.closestS + stats.d_speed * this.droneStopLead;
      let slew = this.PlannedSlew(sAhead);
      this.droneBrakeAmount = AbsF(slew);
      this.droneBrakeSide = slew > 0.0 ? 1.0 : (slew < 0.0 ? -1.0 : 0.0);
      let k = this.SlewIndexAt(sAhead);
      if this.landSlewOn || k >= 0 && this.stopToEnd[k] {
        wantedYaw = this.LandSlewYaw(k, sAhead, live ? this.droneYaw : wantedYaw);
      } else {
        swing = slew * Deg2Rad(this.droneBrakeYaw);
      }
    }
    // the heading eased like the tilt (EaseAngle): rate and acceleration limited
    wantedYaw -= swing;
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
    let thrust = this.attAccel + z * g;
    // pushing down harder than gravity is the lift's job (negative lift), not a body turned over
    thrust.Z = MaxF(thrust.Z, 0.3 * g);
    let size = Vector4.Length(thrust);
    let ahead = Vector4.Dot(thrust, h);
    // braking: reverse surge and the brake (RaceDecel), much more than forward surge
    let along = ahead >= 0.0 ? this.standardModeSurgeFactor : this.RaceDecel(stats.d_speed) - this.DragAccel(stats.d_speed);
    let cap = Deg2Rad(this.droneMaxTilt);
    let pitch = ClampF(this.DroneBodyAngle(AtanF(ahead, thrust.Z), size, along), -cap, cap);
    // the roll for the turn coming, not only the one now (DroneTurnAhead): braking from 71 to 31 m/s
    // into the gap into Dogtown, a gentle bend then a sharp one in the same direction banked 28
    // degrees, eased to 14 and banked 55, one turn flown as two (2026-10-28)
    let lateral = Vector4.Dot(thrust, side);
    let coming = this.DroneTurnAhead(side);
    if AbsF(coming) > AbsF(lateral) && (coming * lateral >= 0.0 || AbsF(lateral) < 2.0) {
      lateral = coming;
    }
    let roll = ClampF(this.DroneBodyAngle(AtanF(lateral, thrust.Z), size, this.standardModeSwayFactor), -cap, cap);
    // eased as the plane's pitch and bank are (EasePitch, EaseBank): followed directly, the tilt
    // flicked with every change of the commanded acceleration and the car overshot it, rolling to
    // 85 degrees for a 50 degree command into Dogtown (2026-10-28). What the tilt lags by, the
    // thrust in the car's frame (FlyAerobatic) still gives, up to the surge and sway limits.
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

  // The strongest sideways acceleration (m/s2 along side, signed) the racing line needs over the
  // next droneRollLook seconds, each point at the speed the plan allows there; 0 off a line.
  private func DroneTurnAhead(side: Vector4) -> Float {
    if !this.onLine || ArraySize(this.routeSpeed) != ArraySize(this.route) {
      return 0.0;
    }
    let speed = this.component.stats.d_speed;
    let strongest = 0.0;
    let t = this.droneRollStep;
    while t <= this.droneRollLook {
      let s = this.closestS + speed * t;
      let tangent: Vector4;
      let bend: Vector4;
      this.LineFrameOver(s, this.lineSpan, tangent, bend);
      let v = MinF(speed, this.RaceAllowedAt(s));
      let need = Vector4.Dot(bend, side) * v * v;
      if AbsF(need) > AbsF(strongest) {
        strongest = need;
      }
      t += this.droneRollStep;
    }
    return strongest;
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
  // cannot point the rest of `thrust` m/s2. Sway is 5 m/s2, so turns lean mostly on the body.
  private func DroneBodyAngle(angle: Float, thrust: Float, thrusters: Float) -> Float {
    let reach = AsinF(ClampF(this.raceMargin * thrusters / MaxF(thrust, 0.1), 0.0, 1.0));
    let body = angle * this.droneBodyShare;
    let rest = angle - body;
    if rest > reach {
      return angle - reach;
    }
    if rest < -reach {
      return angle + reach;
    }
    return body;
  }

  // Drone style's envelope: the most horizontal acceleration level flight leaves along one body
  // axis, the body tilted at most droneMaxTilt with `thrusters` m/s2 across it and the lift (at most
  // g + standardModeLiftFactor) holding the car up.
  private func DroneAccel(thrusters: Float) -> Float {
    let g = 9.81 * this.gravityFactor;
    let liftMax = g + this.standardModeLiftFactor;
    let best = thrusters;
    let deg = 1.0;
    while deg <= this.droneMaxTilt {
      let b = Deg2Rad(deg);
      let lift = (g + thrusters * SinF(b)) / CosF(b);
      if lift > liftMax {
        break;
      }
      best = MaxF(best, lift * SinF(b) + thrusters * CosF(b));
      deg += 1.0;
    }
    return best;
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
      // nearer level on a steep path (2026-10-27), then eased
      fwd = this.EasePitch(dt, this.Unsteep(fwd));
      this.attAccel += (accel - this.attAccel) * ClampF(dt / (this.raceMode ? this.raceBankTime : this.bankTime), 0.0, 1.0);
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
      if this.raceMode {
        // pushing over: past raceTopDown the top would turn towards the ground and the car roll
        // over; it stays upright and the thrust (negative lift) gives the rest
        bankAccel.Z = MaxF(bankAccel.Z, -this.raceTopDown * 9.81 * this.gravityFactor);
      }
      let u = FlightAttitude.Across(bankAccel * blend + z * (9.81 * this.gravityFactor), fwd);
      let held = FlightAttitude.Across(this.targetUp, fwd);
      if Vector4.Length(u) < 1.0 {
        u = held;
      }
      // with the path near vertical, gravity says nothing about which way the top should face and
      // small sideways accelerations swung it from frame to frame (yaw chatter in dives and steep
      // climbs, 2026-10-27): hold the top where it was instead
      let vertical = ClampF((AbsF(fwd.Z) - 0.7) / 0.25, 0.0, 1.0);
      if vertical > 0.0 && Vector4.Length(held) > 0.01 {
        u = Vector4.Normalize(u) * (1.0 - vertical) + Vector4.Normalize(held) * vertical;
      }
      up = this.EaseBank(dt, fwd, Vector4.Normalize(u));
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
    let rateRef = new Vector4(Vector4.Dot(this.omegaRef, stats.d_right), Vector4.Dot(this.omegaRef, stats.d_forward), Vector4.Dot(this.omegaRef, stats.d_up), 0.0);
    let rate = stats.d_angularVelocity;
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
    // flight: on a Thorton Colby the yaw came out about three times too stiff and shook at 7 Hz
    // (2026-10-27).
    let torque: Vector4;
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

    // thrust: the commanded acceleration and gravity, in the car's frame
    let local = Quaternion.Conjugate(stats.d_orientation) * (accel + z * (9.81 * this.gravityFactor));
    if this.raceMode {
      // what the player's inputs give at most: surge along the nose (reverse surge plus the brake
      // when slowing), sway across, lift (on top of hover) along the top
      let g = 9.81 * this.gravityFactor;
      local.X = ClampF(local.X, -this.standardModeSwayFactor, this.standardModeSwayFactor);
      local.Y = ClampF(local.Y, this.DragAccel(stats.d_speed) - this.RaceDecel(stats.d_speed), this.standardModeSurgeFactor);
      local.Z = ClampF(local.Z, g - this.standardModeLiftFactor, g + this.standardModeLiftFactor);
    }
    this.force = local;
    this.thrustCmd = local;
    this.torque = torque;
    // inputs for audio and the thrusters' look; no yaw input, ApplyPhysics damps less on an axis
    // with input and the give-back above assumes full damping
    this.commandSurge = ClampF(local.Y / this.standardModeSurgeFactor, -1.0, 1.0);
    this.commandSway = ClampF(local.X / this.standardModeSwayFactor, -1.0, 1.0);
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
        + " seg " + ToString(this.segment) + "/" + ToString(ArraySize(this.route))
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
