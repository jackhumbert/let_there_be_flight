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

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-Autopilot")
  @runtimeProperty("ModSettings.displayName", "UI-Settings-Autopilot-Aerobatics")
  @runtimeProperty("ModSettings.description", "UI-Settings-Autopilot-Aerobatics-Description")
  public let aerobatics: Bool = false;        // full 3D attitude and manoeuvres (FlightAerobatics.reds)

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-Autopilot")
  @runtimeProperty("ModSettings.displayName", "UI-Settings-Autopilot-Stunts")
  @runtimeProperty("ModSettings.description", "UI-Settings-Autopilot-Stunts-Description")
  @runtimeProperty("ModSettings.step", "5.0")
  @runtimeProperty("ModSettings.min", "0.0")
  @runtimeProperty("ModSettings.max", "120.0")
  public let stuntInterval: Float = 0.0;      // s between stunts on long straights with aerobatics on (0 = none)

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
  // tools/cet/torque_probe.lua on an Archer Quartz hovering, 2026-10-26: roll 0.52, yaw 0.95,
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

  // state, public for debugging from CET
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
        if FlightModeAutopilot.GapAt(c, g) && AbsF(g.X - level) < 3.0 && g.Y - g.X > room && this.IsFlat(c, g.X) {
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
  // area or of the map), inside freeRoamRadius of where free roam began, raceRouteHeight above the
  // ground under open sky there.
  private func PickFreeRoamGoal(from: Vector4, heading: Vector4, out goal: Vector4) -> Bool {
    let base = AtanF(heading.Y, heading.X);
    let tries = 0;
    while tries < 24 {
      let spread = MinF(Deg2Rad(this.freeRoamSpread) * (1.0 + Cast<Float>(tries) / 6.0), 3.1415927);
      let angle = base + RandRangeF(-1.0, 1.0) * spread;
      let distance = RandRangeF(this.freeRoamLegMin, this.freeRoamLegMax);
      let c = new Vector4(from.X + distance * CosF(angle), from.Y + distance * SinF(angle), from.Z, 1.0);
      if Vector4.Distance2D(c, this.freeRoamCentre) <= this.freeRoamRadius {
        let gaps = FlightNavMap.GetGaps(c);
        let n = ArraySize(gaps);
        if n > 0 {
          let sky = gaps[n - 1];
          let z = MinF(sky.X + this.raceRouteHeight, sky.Y - 5.0);
          // near the height of the leg before, until nothing like that fits
          if z > sky.X + 5.0 && (AbsF(z - from.Z) <= this.freeRoamMaxRise || tries >= 16) {
            goal = new Vector4(c.X, c.Y, z, 1.0);
            return true;
          }
        }
      }
      tries += 1;
    }
    return false;
  }

  // Plans a leg from `from` towards a new goal; join: it continues the route from route point
  // joinIndex (spliced on by SpliceRoute once planned).
  private func NextFreeRoamLeg(from: Vector4, heading: Vector4, join: Bool) -> Bool {
    let goal: Vector4;
    if !this.PickFreeRoamGoal(from, heading, goal) {
      this.replanTimer = 1.0;
      FlightLog.Warn("[Autopilot] free roam: no point found for the next leg");
      return false;
    }
    this.destination = goal;
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
      let heading = Vector4.Length2D(stats.d_velocity) > 3.0 ? Vector4.Normalize2D(stats.d_velocity) : stats.d_forward2D;
      this.NextFreeRoamLeg(this.takingOff ? this.takeoffPoint : stats.d_position, heading, false);
      return;
    }
    if !Equals(this.status, "following") || this.routeLength[n - 1] - this.closestS > this.freeRoamChainAt {
      return;
    }
    // the point at that distance, not the next route point after it: a plain route's legs can be
    // a kilometre long, and its end was the join every time, always reached before the plan
    // (2026-10-27)
    let s = MinF(this.closestS + MaxF(this.freeRoamJoinMin, stats.d_speed * this.freeRoamJoinAhead), this.routeLength[n - 1]);
    let point = this.PointAt(s);
    let back = point - this.PointAt(s - 10.0);
    let heading = Vector4.Length2D(back) > 0.1 ? Vector4.Normalize2D(back) : stats.d_forward2D;
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

  public func Update(timeDelta: Float) -> Void {
    let stats = this.component.stats;
    let p = stats.d_position;
    let v = stats.d_velocity;

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
      if this.IsAerobatic() {
        this.ChooseManeuver(p, closest, timeDelta);
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
      if this.freeRoam && this.remaining < this.arriveDistance && stats.d_speed < 3.0 {
        // the end of the route before the next leg joined: on from here (ChainFreeRoam)
        this.holdPosition = this.route[ArraySize(this.route) - 1];
        ArrayClear(this.route);
        this.status = "free roam";
        FlightLog.Info("[Autopilot] free roam: end of the route, planning on from here");
      } else if this.remaining < this.arriveDistance && stats.d_speed < 3.0 {
        this.status = this.landing ? "descending" : "arrived";
        this.holdPosition = this.landing ? this.landingSpot : this.route[ArraySize(this.route) - 1];
        if this.landing {
          this.landingStartZ = p.Z;
          this.landingHeading0 = stats.d_forward2D;
          this.landingHeading = this.landingPad >= 0
            ? FlightModeAutopilot.PadHeading(this.landingPad, stats.d_forward2D)
            : this.LandingHeading(this.landingSpot, stats.d_forward2D);
        }
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
        desiredVelocity.Z = -ClampF(0.5 * (p.Z - this.landingSpot.Z), this.touchdownRate, this.descentRate);
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

    // per-frame telemetry for tuning (tools/autopilot_smoothness.py); switch on from CET with
    // FlightSystem.GetInstance().playerComponent:GetFlightMode().telemetry = true
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
    let top = k > 0.0 ? SqrtF(this.standardModeSurgeFactor / k) : this.raceSpeedCap;
    return MinF(0.97 * top, this.freeRoam ? this.freeRoamTopSpeed : this.raceSpeedCap);
  }

  // Slowing at speed v: full reverse surge, the brake (velocity damping, FlightMode_Standard) and drag.
  private func RaceDecel(v: Float) -> Float {
    let brake = FlightSettings.GetInstance().brakeFactorLinear * this.component.stats.s_brakingFrictionFactor * v;
    return this.standardModeSurgeFactor + brake + this.DragAccel(v);
  }

  // Sideways acceleration in a level turn: the full lift banked into it, the rest holding the car up.
  private func RaceLateral() -> Float {
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
        limit = MinF(limit, SqrtF(this.racePushOver / -bend.Z));
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
    let stats = this.component.stats;
    FlightLog.Info("[Autopilot] race: top " + FloatToStringPrec(top, 1) + " m/s, lateral " + FloatToStringPrec(this.RaceLateral(), 1)
      + " m/s2, decel " + FloatToStringPrec(this.RaceDecel(0.0), 1) + " + " + FloatToStringPrec(this.RaceDecel(30.0) - this.RaceDecel(0.0), 1) + " at 30 m/s"
      + " (mass " + FloatToStringPrec(stats.s_mass, 0) + ", air " + FloatToStringPrec(stats.s_airResistanceFactor, 3) + ", brake friction " + FloatToStringPrec(stats.s_brakingFrictionFactor, 2) + ")");
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
    let distance = this.routeLength[k] - s - this.speedCmd * this.brakeLead;
    if k == n - 1 {
      distance -= 3.0;
    }
    return this.RaceBrakeBack(this.routeSpeed[k], MaxF(distance, 0.0), this.RaceTopSpeed());
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
      && Vector4.Dot(Vector4.Normalize2D(ahead), Vector4.Normalize2D(v)) < -0.5 {
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

  // From CET, for testing: kind as FlightManeuverKind (1 aileron roll .. 5 split-S), side +-1.
  public func StartManeuver(kind: Int32, side: Float) -> String {
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
