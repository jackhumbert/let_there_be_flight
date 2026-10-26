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
  public let enabled: Bool = true;

  // tuning (plain fields for now; not in Mod Settings yet)
  public let maxSpeed: Float = 45.0;          // m/s
  public let lateralAccel: Float = 5.0;       // m/s2 allowed in corners
  public let brakeAccel: Float = 6.0;         // m/s2 planned deceleration
  public let velocityGain: Float = 1.2;       // 1/s, velocity error -> acceleration
  public let maxAccel: Float = 12.0;          // m/s2
  public let yawGain: Float = 0.04;           // full yaw input at 25 degrees heading error
  public let yawRateDamping: Float = 0.25;
  public let maxBankDeg: Float = 25.0;
  public let lateralCorrection: Float = 1.0;  // 1/s, sideways velocity error -> extra bank accel
  public let bankTime: Float = 0.4;           // s, smoothing of the bank target
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
    if this.component.isPlayerMounted {
      this.PublishAutodriveUI(true);
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
    this.request = FlightNavMap.PlanAsync(from, this.destination, false);
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
      this.RequestRoute(this.component.stats.d_position);
    }
    if this.request > 0 && FlightNavMap.IsPlanReady(this.request) {
      let points = FlightNavMap.TakePlan(this.request);
      this.request = 0;
      if ArraySize(points) >= 2 {
        this.route = points;
        ArrayClear(this.routeLength);
        let total = 0.0;
        let i = 0;
        while i < ArraySize(points) {
          if i > 0 {
            total += Vector4.Distance(points[i - 1], points[i]);
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

  // Closest point on the route from the current segment on; advances the segment index.
  private func ClosestOnRoute(p: Vector4, out closest: Vector4, out distance: Float) -> Void {
    let n = ArraySize(this.route);
    let best = 1000000000.0;
    let bestSegment = this.segment;
    let i = this.segment;
    while i < n - 1 && i <= this.segment + 4 {
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
    let allowed = MinF(this.maxSpeed, SqrtF(2.0 * this.brakeAccel * MaxF(this.remaining - 2.0, 0.0)));
    // each corner is judged by the route's direction a stretch before and after it, so a run of
    // small steps reads as the bend it really is instead of many tight corners
    let distance = toCorner;
    let start = this.routeLength[this.segment + 1] - toCorner; // route distance of the car
    k = this.segment + 1;
    while k < n - 1 && distance < 600.0 {
      let s = this.routeLength[k];
      let before = this.PointAt(s - this.cornerSpan);
      let after = this.PointAt(s + this.cornerSpan);
      let inDir = this.route[k] - before;
      let outDir = after - this.route[k];
      if Vector4.Length(inDir) > 0.5 && Vector4.Length(outDir) > 0.5 {
        let angle = AcosF(ClampF(Vector4.Dot(Vector4.Normalize(inDir), Vector4.Normalize(outDir)), -1.0, 1.0));
        if angle > 0.05 {
          let tangent = MinF(Vector4.Length(inDir), Vector4.Length(outDir));
          let radius = tangent / TanF(angle * 0.5);
          let cornerSpeed = SqrtF(this.lateralAccel * MaxF(radius, 1.0));
          allowed = MinF(allowed, SqrtF(cornerSpeed * cornerSpeed + 2.0 * this.brakeAccel * MaxF(s - start - tangent, 0.0)));
        }
      }
      distance = s - start;
      k += 1;
    }
    return allowed;
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
      this.UpdateWithNormalLift(timeDelta, FlightUtils.Up(), 9.81 * this.gravityFactor + this.component.lift * this.standardModeLiftFactor - v.Z);
      return;
    }

    // where to go
    let target = this.holdPosition;
    this.targetSpeed = 0.0;
    let following = Equals(this.status, "following") && ArraySize(this.route) >= 2;
    if following {
      let closest: Vector4;
      let offRoute: Float;
      this.ClosestOnRoute(p, closest, offRoute);
      this.offRouteNow = offRoute;
      if offRoute > this.replanDistance && this.replanTimer <= 0.0 && this.request == 0 {
        this.RequestRoute(p);
      }
      let ahead = ClampF(0.7 * stats.d_speed + 12.0, 15.0, 90.0);
      // don't aim far past the next corner: a long lookahead cuts inside corners, and route legs
      // graze obstacles there
      if this.segment + 2 < ArraySize(this.route) {
        ahead = MinF(ahead, Vector4.Distance(closest, this.route[this.segment + 1]) + this.cornerOvershoot);
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
      this.curvatureAhead = this.CurvatureAhead(closest, ahead);
      this.targetSpeed = this.AllowedSpeed(closest);
      // off the route: slow down while getting back, the route's margins assume the car is on it
      this.targetSpeed = MinF(this.targetSpeed, MaxF(8.0, this.maxSpeed - this.offRouteSlowdown * MaxF(offRoute - 2.0, 0.0)));
      // where the current velocity is taking the car must be free on the map; brake to stop short
      // of the first blocked point otherwise
      if stats.d_speed > 3.0 {
        let reach = v * this.collisionHorizon;
        let clear = FlightNavMap.SegmentClear(FlightNavMap.IsFree(p) ? p : closest, p + reach);
        if clear < 1.0 {
          let room = MaxF(clear * Vector4.Length(reach) - 5.0, 0.0);
          this.targetSpeed = MinF(this.targetSpeed, SqrtF(2.0 * this.brakeAccel * room));
          this.blockedAhead = true;
        } else {
          this.blockedAhead = false;
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
        this.status = "arrived";
        this.holdPosition = this.route[ArraySize(this.route) - 1];
        ArrayClear(this.route);
        FlightLog.Info("[Autopilot] arrived");
        if this.component.isPlayerMounted {
          // like vanilla autodrive: "destination reached", then hand the controls back (the
          // previous mode takes over and hovers here)
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
      desiredVelocity = Vector4.Normalize(toTarget) * this.targetSpeed;
    } else {
      // hold: approach the hold point, slowing down near it
      desiredVelocity = toTarget * 0.8;
      if Vector4.Length(desiredVelocity) > 10.0 {
        desiredVelocity = Vector4.Normalize(desiredVelocity) * 10.0;
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

    // fly like a drone when slow, like a plane when fast
    let blend = ClampF((stats.d_speed - 8.0) / 17.0, 0.0, 1.0);
    blend = blend * blend * (3.0 - 2.0 * blend);

    // heading: along the desired velocity, or keep the current one when hovering
    let heading = new Vector4(desiredVelocity.X, desiredVelocity.Y, 0.0, 0.0);
    let yaw = 0.0;
    if Vector4.Length(heading) > 2.0 {
      let error = Vector4.GetAngleDegAroundAxis(Vector4.Normalize(heading), stats.d_forward2D, FlightUtils.Up());
      // positive yaw input turns right, which is a negative angular velocity about Z, so the
      // damping term adds the rate back in (it opposes the turn already under way)
      yaw = ClampF(error * this.yawGain + stats.d_angularVelocity.Z * this.yawRateDamping, -1.0, 1.0);
    }

    // acceleration in the car's frame
    let local = Quaternion.Conjugate(stats.d_orientation) * accel;
    let surge = ClampF(local.Y / this.standardModeSurgeFactor, -1.0, 1.0);
    let brake = local.Y < -2.0 ? ClampF(-local.Y / 12.0, 0.0, 1.0) : 0.0;
    // sideways thrust keeps the car on the line at every speed (fading to half at plane speeds):
    // banking alone only redirects the velocity slowly through the nose at mid speeds
    let sway = ClampF(local.X / this.standardModeSwayFactor, -1.0, 1.0) * (1.0 - 0.5 * blend);

    // coordinated bank at speed: the route's own turn (v^2 x curvature) plus a small correction
    // for sideways velocity error, smoothed; nose pitch along the climb
    let lateralError = Vector4.Dot(desiredVelocity - v, stats.d_right);
    let lateral = stats.d_speed2D * stats.d_speed2D * this.curvatureAhead + this.lateralCorrection * lateralError * stats.d_speed2D * 0.1;
    let bankWanted = ClampF(AtanF(lateral, 9.81), -Deg2Rad(this.maxBankDeg), Deg2Rad(this.maxBankDeg)) * blend;
    this.bankTarget += (bankWanted - this.bankTarget) * ClampF(timeDelta / this.bankTime, 0.0, 1.0);
    let bank = this.bankTarget;
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
