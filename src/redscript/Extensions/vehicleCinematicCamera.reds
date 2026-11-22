// Vanilla's cinematic camera (autodrive's, and the Autopilot's) was made for cars on a road. In
// flight:
// - the shots attached to the car are placed in a level frame that turns with the car's heading,
//   eased (Cinematic::LevelHeading): the car banks and pitches in the frame and never swings the
//   camera round it (vanilla's stabilised frame crosses world up with the car's right vector, which
//   has no answer at 90 degrees of bank; its suspension-tracking one follows the whole attitude);
// - every shot aims ahead of the car's centre along its motion with the horizon level, from far
//   enough out that the car fits the frame at any bank (Cinematic::FitDistanceAnyBank), the
//   distance eased (out quickly, in slowly), so the car sits off centre with room in front and the
//   camera does not pump in and out as it banks;
// - the shots fixed in the world become flybys beside the route ahead (Cinematic::PlanFlyby): far
//   enough out that the car crosses the view at a steady pan, drifting along with a fast car, clear
//   of buildings and with a clear view, on the side the camera already is (no crossing the line);
// - a shot is picked less often for a while after it ran, and an attached one not where its
//   camera would hit something now or in the next 1.5 s.
// The shot maths in C++ (src/red4ext/Camera/FlightCinematicShot.cpp): the game's own WorldTransform natives are
// called from there. OrientationFromDir and TransformPoint are the assumed conventions, kept to compare with the natives.
public native class FlightCinematicShot extends IScriptable {
  public static native func AimAtCar(xform: WorldTransform, centre: Vector4, velocity: Vector4, minDistance: Float) -> WorldTransform;
  public static native func FitDistance(camera: Vector4, centre: Vector4, car: Quaternion, half: Vector4, fov: Float, prev: Float, dt: Float) -> Float;
  public static native func LevelHeading(prev: Vector4, car: Quaternion, velocity: Vector4, dt: Float) -> Vector4;
  public static native func FlybyStraight(position: Vector4, velocity: Vector4, offset: Vector4, side: Int32) -> ref<FlightCinematicFlyby>;
  public static native func OrientationFromDir(dir: Vector4) -> Quaternion;
  public static native func TransformPoint(xform: WorldTransform, point: Vector4) -> Vector4;
}

// A flyby's plan: the camera starts at `start` and moves at `velocity` (m/s); the car passes it `pass` s after the
// start, on `side` (+1 right of its travel, -1 left). Not `ok`: no clear spot beside the route.
public native class FlightCinematicFlyby extends IScriptable {
  public native let ok: Bool;
  public native let start: Vector4;
  public native let velocity: Vector4;
  public native let pass: Float;
  public native let side: Int32;
}

public abstract class FlightCinematic {
  public static func Flying() -> Bool {
    return FlightController.GetInstance().IsActive();
  }

  // The camera far enough out that the car fits the frame at any bank, the distance eased from `prev` (0 on the
  // shot's first frame) over dt (in C++: Cinematic::FitDistanceAnyBank, Cinematic::EaseDistance).
  public static func FitDistance(xform: WorldTransform, context: vehicleCinematicCameraShotUpdateContext, prev: Float, dt: Float) -> Float {
    let box = context.vehicleBoundingBox;
    let half = (box.Max - box.Min) * 0.5;
    if half.X < 0.25 || half.Y < 0.5 || half.Z < 0.15 {
      half = new Vector4(0.0, 0.0, 0.0, 0.0);
    }
    return FlightCinematicShot.FitDistance(WorldPosition.ToVector4(WorldTransform.GetWorldPosition(xform)), FlightCinematic.Centre(context), WorldTransform.GetOrientation(context.vehicleTransform), half, context.cameraComponent.GetFOV(), prev, dt);
  }

  // The camera at least `distance` from the car's centre, facing ahead of it (in C++: Cinematic::AimAtCarLead).
  public static func AimAtCar(xform: WorldTransform, context: vehicleCinematicCameraShotUpdateContext, distance: Float) -> WorldTransform {
    return FlightCinematicShot.AimAtCar(xform, FlightCinematic.Centre(context), FlightCinematic.Velocity(), distance);
  }

  // The player's car's velocity, m/s (zero without a car).
  public static func Velocity() -> Vector4 {
    let vehicle = GetMountedVehicle(GetPlayer(GetGameInstance()));
    let velocity: Vector4;
    if IsDefined(vehicle) {
      velocity = vehicle.GetLinearVelocity();
    }
    return velocity;
  }

  // The middle of the car's bounding box in the world; the centre of mass when the box is degenerate.
  public static func Centre(context: vehicleCinematicCameraShotUpdateContext) -> Vector4 {
    let box = context.vehicleBoundingBox;
    let size = box.Max - box.Min;
    if size.X > 0.5 && size.Y > 1.0 && size.Z > 0.3 {
      return WorldPosition.ToVector4(WorldTransform.TransformPoint(context.vehicleTransform, (box.Min + box.Max) * 0.5));
    }
    let vehicle = GetMountedVehicle(GetPlayer(GetGameInstance()));
    if IsDefined(vehicle) {
      return WorldPosition.ToVector4(WorldTransform.TransformPoint(context.vehicleTransform, Vector4.Vector3To4(vehicle.GetCenterOfMass())));
    }
    return WorldPosition.ToVector4(WorldTransform.GetWorldPosition(context.vehicleTransform));
  }

  // how much more often a shot fixed in the world (a flyby) is picked in flight: 6 flybys against
  // about 18 attached shots
  public static func FixedShotWeight() -> Int32 {
    return 3;
  }

  // how much more often a shot is picked when it is not among the last RecentShots run
  public static func FreshShotWeight() -> Int32 {
    return 4;
  }

  public static func RecentShots() -> Int32 {
    return 3;
  }

  // Shots made for a car on a road that do not work in flight: the wheel and the hood seen from
  // under a metre away (the car filled the frame, half of it out of it).
  public static func Unused(name: String) -> Bool {
    return Equals(name, "S_WheelShot_Static") || Equals(name, "S_AttachedFarHoodShot1_Static");
  }

  // s ahead a shot attached to the car is checked to stay in open air, at its start and twice between
  public static func ClearAhead() -> Float {
    return 1.5;
  }

  // degrees the view of the car must turn at a cut, at least (less reads as a jump cut)
  public static func MinCutAngle() -> Float {
    return 30.0;
  }

  // The view of the car from `from` and from `to` differ by less than MinCutAngle.
  public static func JumpCut(from: Vector4, to: Vector4, car: Vector4) -> Bool {
    let a = car - from;
    let b = car - to;
    if Vector4.Length(a) < 0.1 || Vector4.Length(b) < 0.1 {
      return false;
    }
    return Vector4.GetAngleBetween(a, b) < FlightCinematic.MinCutAngle();
  }

  // Shots cut short in flight, s (-1: as made). AttachedFront1 (ahead of the car, looking back at
  // its nose) looked odd for its 6 s.
  public static func MaxDuration(name: String) -> Float {
    if Equals(name, "S_AttachedFront1_Static") {
      return 2.5;
    }
    return -1.0;
  }

  public static func List(camera: ref<vehicleCinematicCameraComponent>, context: vehicleCinematicCameraShotUpdateContext) -> Void {
    let g = 0;
    while g < camera.GetGroupCount() {
      let group = camera.GetGroup(g);
      let text = "[CinematicCamera] group " + group.name + (group.enabled ? "" : " (disabled)") + (group.CanBeSelected(context) ? "" : " (not now)");
      let c = 0;
      while c < group.GetConditionCount() {
        let condition = group.GetCondition(c);
        text += " " + NameToString(condition.GetClassName());
        let minSpeed = condition as vehicleCinematicCameraShotStartCondition_MinSpeed;
        if IsDefined(minSpeed) { text += " " + FloatToStringPrec(minSpeed.minSpeed, 0) + " km/h"; }
        let maxSpeed = condition as vehicleCinematicCameraShotStartCondition_MaxSpeed;
        if IsDefined(maxSpeed) { text += " " + FloatToStringPrec(maxSpeed.maxSpeed, 0) + " km/h"; }
        let type = condition as vehicleCinematicCameraShotStartCondition_VehicleType;
        if IsDefined(type) { text += " " + ToString(type.vehicleType); }
        c += 1;
      }
      FlightLog.Info(text);
      let i = 0;
      while i < group.GetShotCount() {
        let shot = group.GetShot(i);
        let line = "[CinematicCamera]   " + shot.name + (shot.enabled ? "" : " (disabled)") + " p " + ToString(shot.probability) + ", " + FloatToStringPrec(shot.duration, 0) + " s, " + NameToString(shot.root.GetClassName()) + ", stops:";
        for stop in shot.stopConditions {
          line += " " + NameToString(stop.GetClassName());
          let far = stop as vehicleCinematicCameraShotStopCondition_VehicleDistanceFromCamera;
          if IsDefined(far) { line += " " + FloatToStringPrec(far.maxDistanceFromCamera, 0) + " m"; }
          let hidden = stop as vehicleCinematicCameraShotStopCondition_VehicleNotVisible;
          if IsDefined(hidden) { line += " " + FloatToStringPrec(hidden.timeOutOfSightBeforeStop, 2) + " s"; }
        }
        FlightLog.Info(line);
        i += 1;
      }
      g += 1;
    }
  }

  // The side of the car the camera is on now: +1 right, -1 left, 0 within a metre of its middle.
  public static func CameraSide(camera: Vector4, vehicleTransform: WorldTransform) -> Int32 {
    let local = WorldTransform.TransformInvPoint(vehicleTransform, camera);
    if AbsF(local.X) < 1.0 {
      return 0;
    }
    return local.X > 0.0 ? 1 : -1;
  }

  // The Autopilot flying the player's car, if it is.
  public static func Autopilot() -> ref<FlightModeAutopilot> {
    let pc = FlightSystem.GetInstance().playerComponent;
    if !IsDefined(pc) {
      return null;
    }
    return pc.GetFlightMode() as FlightModeAutopilot;
  }
}

// The level heading an attached shot is placed by, and when it was last eased (the frame is asked for
// more than once a frame; a heading older than half a second is from the shot's last run).
@addField(cameraShotRoot_FixedOnVehicle)
private let flightHeading: Vector4;

@addField(cameraShotRoot_FixedOnVehicle)
private let flightHeadingTime: Float;

@wrapMethod(cameraShotRoot_FixedOnVehicle)
public func Reset() -> Void {
  wrappedMethod();
  this.flightHeading = new Vector4(0.0, 0.0, 0.0, 0.0);
  this.flightHeadingTime = 0.0;
}

@addMethod(cameraShotRoot_FixedOnVehicle)
public func FlightLevelFrame(data: CinematicCameraData) -> WorldTransform {
  let dt = data.currentTime - this.flightHeadingTime;
  if dt < 0.0 || dt > 0.5 {
    this.flightHeading = new Vector4(0.0, 0.0, 0.0, 0.0);
    dt = 0.0;
  }
  if dt > 0.0 || Vector4.Length2D(this.flightHeading) < 0.5 {
    // in C++ (Cinematic::LevelHeading)
    this.flightHeading = FlightCinematicShot.LevelHeading(this.flightHeading, WorldTransform.GetOrientation(data.vehicleTransform), FlightCinematic.Velocity(), dt);
    this.flightHeadingTime = data.currentTime;
  }
  let frame = data.vehicleTransform;
  WorldTransform.SetOrientationFromDir(frame, this.flightHeading);
  return frame;
}

@wrapMethod(cameraShotRoot_FixedOnVehicleNonSuspensionTracking)
public func GetShotSpaceTransform(data: CinematicCameraData) -> WorldTransform {
  if FlightCinematic.Flying() {
    return this.FlightLevelFrame(data);
  }
  return wrappedMethod(data);
}

@wrapMethod(cameraShotRoot_FixedOnVehicleSuspensionTracking)
public func GetShotSpaceTransform(data: CinematicCameraData) -> WorldTransform {
  if FlightCinematic.Flying() {
    return this.FlightLevelFrame(data);
  }
  return wrappedMethod(data);
}

@wrapMethod(vehicleCinematicCameraShot)
public func Update(context: vehicleCinematicCameraShotUpdateContext) -> WorldTransform {
  let result = wrappedMethod(context);
  if FlightCinematic.Flying() {
    let dt = context.engineTime - this.flightFitTime;
    if this.flightFitTime <= 0.0 || dt < 0.0 || dt > 0.5 {
      this.flightFit = 0.0;
      dt = 0.0;
    }
    this.flightFit = FlightCinematic.FitDistance(result, context, this.flightFit, dt);
    this.flightFitTime = context.engineTime;
    return FlightCinematic.AimAtCar(result, context, this.flightFit);
  }
  return result;
}

// The distance from the car the shot keeps (FlightCinematic.FitDistance), eased, and when it was last
// eased; dropped at the shot's start and when older than half a second.
@addField(vehicleCinematicCameraShot)
private let flightFit: Float;

@addField(vehicleCinematicCameraShot)
private let flightFitTime: Float;

// The shots fixed in the world are flybys in flight (Cinematic::PlanFlyby): made for road speeds,
// a flying car went past in a blink, close enough to whip the camera round. The plan puts the camera beside the
// route the Autopilot will fly (a straight line without one), on the side the camera is on, so the
// car passes it in up to 3 s at a pan of 0.75 radians a second as planned; faster than 15 m/s the camera
// drifts along with the car. A shot with no clear spot is not picked (GetCanBeSelectedFromContext).
@addField(cameraShotRoot_FixedShot)
public let flightFlyby: ref<FlightCinematicFlyby>;

// how far the camera starts from the car, m (for the distance stop condition)
@addField(cameraShotRoot_FixedShot)
public let flightReach: Float;

// (vehicleCinematicCameraShotRoot.Reset is empty, and the fixed shot does not override it)
@addMethod(cameraShotRoot_FixedShot)
public func Reset() -> Void {
  this.flightFlyby = null;
  this.flightReach = 0.0;
}

@addMethod(cameraShotRoot_FixedShot)
public func FlightFlyby(data: CinematicCameraData) -> ref<FlightCinematicFlyby> {
  if !IsDefined(this.flightFlyby) {
    let offset = Transform.GetPosition(CinematicCameraData.ScaleTransformForVehicle(data, this.offsetFromInitialPosition));
    let car = WorldPosition.ToVector4(WorldTransform.GetWorldPosition(data.vehicleInitialTransform));
    let side = FlightCinematic.CameraSide(WorldPosition.ToVector4(data.cameraComponent.GetWorldPosition()), data.vehicleInitialTransform);
    if side == 0 {
      side = offset.X < 0.0 ? -1 : 1;
    }
    let velocity = FlightCinematic.Velocity();
    let autopilot = FlightCinematic.Autopilot();
    // in C++ (FlightModeAutopilot.CinematicFlyby, Cinematic::PlanFlyby)
    if IsDefined(autopilot) {
      this.flightFlyby = autopilot.CinematicFlyby(car, velocity, offset, side);
    } else {
      this.flightFlyby = FlightCinematicShot.FlybyStraight(car, velocity, offset, side);
    }
    this.flightReach = Vector4.Distance(this.flightFlyby.start, car);
  }
  return this.flightFlyby;
}

@wrapMethod(cameraShotRoot_FixedShot)
public func GetRootTransform(data: CinematicCameraData) -> WorldTransform {
  if !FlightCinematic.Flying() {
    return wrappedMethod(data);
  }
  let flyby = this.FlightFlyby(data);
  if !flyby.ok {
    return wrappedMethod(data);
  }
  let camera = flyby.start + flyby.velocity * MaxF(data.currentTime - data.shotStartTime, 0.0);
  let xform = this.GetShotSpaceTransform(data);
  WorldTransform.SetPosition(xform, camera);
  WorldTransform.SetOrientationFromDir(xform, WorldPosition.ToVector4(WorldTransform.GetWorldPosition(data.vehicleTransform)) - camera);
  return xform;
}

// Which shot the camera picks while flying, one line each.
@wrapMethod(vehicleCinematicCameraShot)
public func Start(context: vehicleCinematicCameraShotUpdateContext) -> Void {
  // a fixed shot's scale is taken anew at its start: the camera also places every candidate while
  // choosing, which left the losers' scale from then
  let fixed = this.root as cameraShotRoot_FixedShot;
  if IsDefined(fixed) {
    fixed.Reset();
  }
  this.flightFit = 0.0;
  this.flightFitTime = 0.0;
  wrappedMethod(context);
  if FlightCinematic.Flying() {
    context.cameraComponent.FlightRan(this.name);
    let text = "[CinematicCamera] shot " + this.name + " (" + NameToString(this.root.GetClassName()) + ") at " + FloatToStringPrec(context.vehicleSpeed, 1) + " m/s";
    if IsDefined(fixed) && IsDefined(fixed.flightFlyby) {
      let flyby = fixed.flightFlyby;
      if flyby.ok {
        text += " flyby side " + ToString(flyby.side) + " pass " + FloatToStringPrec(flyby.pass, 1) + " s drift " + FloatToStringPrec(Vector4.Length(flyby.velocity), 1) + " m/s";
      } else {
        text += " flyby: no clear spot";
      }
    }
    FlightLog.Info(text);
  }
}

// The shots run last, newest first (FlightCinematic.RecentShots of them).
@addField(vehicleCinematicCameraComponent)
private let flightRecent: array<String>;

@addMethod(vehicleCinematicCameraComponent)
public func FlightRan(name: String) -> Void {
  ArrayInsert(this.flightRecent, 0, name);
  while ArraySize(this.flightRecent) > FlightCinematic.RecentShots() {
    ArrayPop(this.flightRecent);
  }
}

@addMethod(vehicleCinematicCameraComponent)
public func FlightRanLately(name: String) -> Bool {
  return ArrayContains(this.flightRecent, name);
}

// Shot weights in flight: the shot's own, the flybys' more (they were rare next to the shots
// attached to the car), and the shots not run lately more again. Vanilla's bonuses are left out:
// the one for a shot starting near the camera picked the shot next to the last one (a cut to
// nearly the same view) and kept the camera on one side; the one for a static shot after a static
// shot applies to all but one.
@wrapMethod(vehicleCinematicCameraShot)
public const func GetShotProbabilityFromContext(context: vehicleCinematicCameraShotUpdateContext) -> Int32 {
  if !FlightCinematic.Flying() {
    return wrappedMethod(context);
  }
  let probability = this.probability;
  if IsDefined(this.root as cameraShotRoot_FixedShot) {
    probability *= FlightCinematic.FixedShotWeight();
  }
  if !context.cameraComponent.FlightRanLately(this.name) {
    probability *= FlightCinematic.FreshShotWeight();
  }
  return probability;
}

// Which shots can be picked in flight. A flyby: when it has a clear spot beside the route (its
// plan takes the side the camera is on, so it keeps vanilla's no-crossing rule its own way). An
// attached shot: vanilla's rule (on the side of the car the camera is on), not one of the road
// shots (FlightCinematic.Unused), and with its camera clear of collision (the camera's own test,
// as vanilla's CollisionWithEnvironment stop) now and over the next 1.5 s at the car's velocity.
// (The nav map is no test for a camera: it has space blocked wherever the car would not fit.)
// Either kind: its view of the car at the start turns at least FlightCinematic.MinCutAngle from
// the camera's now (no jump cut).
@wrapMethod(vehicleCinematicCameraShot)
public const func GetCanBeSelectedFromContext(context: vehicleCinematicCameraShotUpdateContext) -> Bool {
  if !FlightCinematic.Flying() {
    return wrappedMethod(context);
  }
  if FlightCinematic.Unused(this.name) {
    return false;
  }
  let fixed = this.root as cameraShotRoot_FixedShot;
  let car = WorldPosition.ToVector4(WorldTransform.GetWorldPosition(context.vehicleTransform));
  let now = WorldPosition.ToVector4(context.cameraComponent.GetWorldPosition());
  if IsDefined(fixed) {
    // placing the camera plans the flyby, anew: a plan kept from an earlier time the shot was weighed is stale
    fixed.Reset();
    let at = WorldPosition.ToVector4(WorldTransform.GetWorldPosition(this.GetRootTransformFromContext(context)));
    return IsDefined(fixed.flightFlyby) && fixed.flightFlyby.ok && !FlightCinematic.JumpCut(now, at, car);
  }
  if !wrappedMethod(context) {
    return false;
  }
  let start = WorldPosition.ToVector4(WorldTransform.GetWorldPosition(this.GetRootTransformFromContext(context)));
  if FlightCinematic.JumpCut(now, start, car) {
    return false;
  }
  let velocity = FlightCinematic.Velocity();
  let ahead = FlightCinematic.ClearAhead();
  let camera = context.cameraComponent;
  return !camera.CheckCollisionsAtPosition(Vector4.Vector4To3(start))
    && !camera.CheckCollisionsAtPosition(Vector4.Vector4To3(start + velocity * (0.5 * ahead)))
    && !camera.CheckCollisionsAtPosition(Vector4.Vector4To3(start + velocity * ahead));
}

// Every group and shot the camera has, logged once a session when it first runs.
@addField(vehicleCinematicCameraComponent)
private let flightListed: Bool;

@wrapMethod(vehicleCinematicCameraComponent)
public func UpdateCameraState(context: vehicleCinematicCameraShotUpdateContext) -> Void {
  if !this.flightListed {
    this.flightListed = true;
    FlightCinematic.List(this, context);
  }
  wrappedMethod(context);
}

// Every shot stops once the car is 30 m from the camera, but the shots fixed in the world start
// about 30 m out (RightAheadHigh at 31 m, so it never lasted even on a road) and further as
// flybys: they ended within 0.1 s and the camera drew one after another. In flight their
// limit is where the camera started plus a margin, so a flyby ends once the car is past and away.
@wrapMethod(vehicleCinematicCameraShotStopCondition_VehicleDistanceFromCamera)
public func Evaluate(const data: CinematicCameraData, const finalTransform: WorldTransform) -> Bool {
  let fixed = data.currentShot.root as cameraShotRoot_FixedShot;
  if !FlightCinematic.Flying() || !IsDefined(fixed) {
    return wrappedMethod(data, finalTransform);
  }
  let limit = MaxF(this.maxDistanceFromCamera, 1.25 * fixed.flightReach);
  let camera = WorldPosition.ToVector4(WorldTransform.GetWorldPosition(finalTransform));
  return Vector4.Distance(WorldPosition.ToVector4(WorldTransform.GetWorldPosition(data.vehicleTransform)), camera) >= limit;
}

@wrapMethod(vehicleCinematicCameraShot)
public func ShouldStop(const finalTransform: WorldTransform) -> Bool {
  if FlightCinematic.Flying() {
    let most = FlightCinematic.MaxDuration(this.name);
    if most > 0.0 && this.runtimeData.currentTime - this.runtimeData.shotStartTime >= most {
      return true;
    }
  }
  return wrappedMethod(finalTransform);
}
