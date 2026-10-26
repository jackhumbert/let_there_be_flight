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
    this.takingOff = this.TakeoffPoint(this.holdPosition, this.takeoffPoint);
    this.takeoffStartZ = this.holdPosition.Z;
    this.takeoffHeading0 = this.component.stats.d_forward2D;
    this.aheadSmooth = 15.0;
    if this.takingOff {
      this.holdPosition = this.takeoffPoint;
    }
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

  // Near the floor: climb straight up first, clear of traffic and street furniture, then fly off.
  private func TakeoffPoint(p: Vector4, out point: Vector4) -> Bool {
    let gap: Vector2;
    if !FlightNavMap.IsLoaded() || !FlightModeAutopilot.GapAt(p, gap) {
      return false;
    }
    let z = MinF(gap.X + this.takeoffHeight, gap.Y - 2.0);
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
        if FlightModeAutopilot.GapAt(c, g) && AbsF(g.X - level) < 3.0 && g.Y - g.X > 6.0 && this.IsFlat(c, g.X) {
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
    if !enabled && bb.GetBool(GetAllBlackboardDefs().UI_AutodriveData.CinematicCameraActive) {
      // leave the cinematic camera with the autopilot, as vanilla does when autodrive ends
      GetPlayer(game).QueueEvent(new vehicleCinematicCameraToggleEvent());
    }
  }

  private func PublishProgress() -> Void {
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
    this.request = FlightNavMap.PlanAsync(from, this.routeGoal, false);
    this.replanTimer = 3.0;
    if ArraySize(this.route) < 2 {
      // nothing to follow meanwhile: hold here (a replan keeps following the old route)
      this.status = "planning";
      this.holdPosition = from;
    }
  }

  private func UpdateRoute(timeDelta: Float) -> Void {
    this.replanTimer -= timeDelta;
    let target: Vector4;
    let has = this.CurrentDestination(target);
    if !has && (Equals(this.status, "descending") || Equals(this.status, "landed")) {
      // the game untracks a map pin once the player reaches it, mid-descent: finish the landing
      return;
    }
    if !has {
      if this.hasDestination {
        ArrayClear(this.route);
        this.holdPosition = this.component.stats.d_position;
      }
      this.hasDestination = false;
      this.status = "no destination";
      return;
    }
    if !this.hasDestination || Vector4.Distance(target, this.destination) > 20.0 {
      this.destination = target;
      this.hasDestination = true;
      ArrayClear(this.route);
      this.ChooseRouteGoal();
      this.RequestRoute(this.takingOff ? this.takeoffPoint : this.component.stats.d_position);
    }
    if this.request > 0 && FlightNavMap.IsPlanReady(this.request) {
      let points = FlightNavMap.TakePlan(this.request);
      this.request = 0;
      if ArraySize(points) >= 2 {
        this.route = this.RoundCorners(points);
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
        this.status = "following";
      } else {
        this.status = "no route";
        this.holdPosition = this.component.stats.d_position;
      }
      FlightLog.Info("[Autopilot] " + FlightNavMap.GetLastPlanInfo());
    }
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
      let cut = this.cornerRound;
      while !rounded && cut >= 0.5 * this.cornerRound {
        let d1 = MinF(0.4 * Vector4.Distance(a, b), cut);
        let d2 = MinF(0.4 * Vector4.Distance(b, c), cut);
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
        cut *= 0.5;
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
    this.emergencyBrake = false;
    let following = Equals(this.status, "following") && ArraySize(this.route) >= 2 && !this.takingOff;
    if following {
      let closest: Vector4;
      let offRoute: Float;
      this.ClosestOnRoute(p, closest, offRoute);
      this.offRouteNow = offRoute;
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
      this.targetSpeed = this.AllowedSpeed(closest);
      // off the route: slow down while getting back, the route's margins assume the car is on it
      this.targetSpeed = MinF(this.targetSpeed, MaxF(8.0, this.maxSpeed - this.offRouteSlowdown * MaxF(offRoute - 2.0, 0.0)));
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
        if blockedAt && travelled > 10.0 && this.DistanceToRoute(hit) > 2.0 && this.RouteClear(closest, Vector4.Length(reach)) {
          let room = MaxF(travelled - 5.0, 0.0);
          this.targetSpeed = MinF(this.targetSpeed, SqrtF(2.0 * this.brakeAccel * room));
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
      let wantedRate = ClampF((this.targetSpeed - this.speedCmd) * (slowing ? 2.0 * this.speedGain : this.speedGain), -this.brakeMax, this.accelUp);
      let jerk = (slowing ? this.brakeJerk : this.speedJerk) * timeDelta;
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
      if this.remaining < this.arriveDistance && stats.d_speed < 3.0 {
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
      desiredVelocity = Vector4.Normalize(toTarget) * this.speedCmd;
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
    let horizontal = new Vector4(accel.X, accel.Y, 0.0, 0.0);
    if Vector4.Length(horizontal) > this.maxAccel {
      horizontal = Vector4.Normalize(horizontal) * this.maxAccel;
    }
    accel.X = horizontal.X;
    accel.Y = horizontal.Y;
    accel.Z = ClampF(accel.Z, -this.maxAccel, this.maxAccel);
    // ease changes of the acceleration itself (it is what is felt): rate-limit the command,
    // except in an emergency stop for blocked space
    let change = accel - this.accelCmd;
    let maxChange = this.accelJerk * timeDelta;
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
}
