// Vanilla's cinematic camera (autodrive's, and the Autopilot's) was made for cars on a road. In
// flight the shots attached to the car follow its whole attitude (vanilla's suspension-tracking
// ones already do; the stabilised ones cross world up with the car's right vector, which has no
// answer at 90 degrees of bank), so the camera swings round a banking car and stays outside it
// (kept level, the close side shots had the car roll its roof into the lens). Every shot then aims
// at the car's centre of mass with the horizon level, from at least 4.5 m (Cinematic::kMinDistance): the shots' own
// angles were made for a level car and lost a banking one off the frame.
// The shot maths in C++ (src/red4ext/Camera/FlightCinematicShot.cpp): the game's own WorldTransform natives are
// called from there. OrientationFromDir and TransformPoint are the assumed conventions, for the bridge's comparison.
public native class FlightCinematicShot extends IScriptable {
  public static native func AimAtCar(xform: WorldTransform, centreOfMass: Vector4) -> WorldTransform;
  public static native func FitAhead(shot: WorldTransform, offset: Vector4, scale: Float) -> Float;
  public static native func OrientationFromDir(dir: Vector4) -> Quaternion;
  public static native func TransformPoint(xform: WorldTransform, point: Vector4) -> Vector4;
}

public abstract class FlightCinematic {
  public static func Flying() -> Bool {
    return FlightController.GetInstance().IsActive();
  }

  public static func AimAtCar(xform: WorldTransform, context: vehicleCinematicCameraShotUpdateContext) -> WorldTransform {
    let vehicle = GetMountedVehicle(GetPlayer(GetGameInstance()));
    let com = WorldPosition.ToVector4(WorldTransform.GetWorldPosition(context.vehicleTransform));
    if IsDefined(vehicle) {
      com = WorldPosition.ToVector4(WorldTransform.TransformPoint(context.vehicleTransform, Vector4.Vector3To4(vehicle.GetCenterOfMass())));
    }
    // in C++ (Cinematic::AimAtCar): no nearer than Cinematic::kMinDistance, facing the centre of mass
    return FlightCinematicShot.AimAtCar(xform, com);
  }

  // how much more often a shot fixed in the world (a flyby) is picked in flight: 5 flybys against
  // 18 attached shots (at 10, with flybys free of the side rule, nothing else was picked)
  public static func FixedShotWeight() -> Int32 {
    return 3;
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

  // a shot offset with its distance ahead (shot-space Y) scaled
  public static func Ahead(offset: Vector4, scale: Float) -> Vector4 {
    offset.Y *= scale;
    return offset;
  }

  // the speed vanilla's shots were made for, m/s
  public static func RoadSpeed() -> Float {
    return 20.0;
  }
}

@wrapMethod(cameraShotRoot_FixedOnVehicleNonSuspensionTracking)
public func GetShotSpaceTransform(data: CinematicCameraData) -> WorldTransform {
  if FlightCinematic.Flying() {
    return data.vehicleTransform;
  }
  return wrappedMethod(data);
}

@wrapMethod(vehicleCinematicCameraShot)
public func Update(context: vehicleCinematicCameraShotUpdateContext) -> WorldTransform {
  let result = wrappedMethod(context);
  if FlightCinematic.Flying() {
    return FlightCinematic.AimAtCar(result, context);
  }
  return result;
}

// The shots fixed in the world start about 28 m ahead of the car: made for road speeds, a flying
// car went past in a third of a second. Their distance ahead is scaled with the speed at the shot's
// start, so the car takes about as long to reach the camera as at road speed (FlightCinematic.
// RoadSpeed); to the side and up they stay as made, so the car still passes close (scaled whole,
// the camera ended up 30 to 40 m off the path and the car was a speck).
@addField(cameraShotRoot_FixedShot)
private let flightScale: Float;

// how far the camera starts from the car, m (for the distance stop condition)
@addField(cameraShotRoot_FixedShot)
public let flightReach: Float;

// (vehicleCinematicCameraShotRoot.Reset is empty, and the fixed shot does not override it)
@addMethod(cameraShotRoot_FixedShot)
public func Reset() -> Void {
  this.flightScale = 0.0;
  this.flightReach = 0.0;
}

@wrapMethod(cameraShotRoot_FixedShot)
public func GetRootTransform(data: CinematicCameraData) -> WorldTransform {
  if !FlightCinematic.Flying() {
    return wrappedMethod(data);
  }
  let offset = CinematicCameraData.ScaleTransformForVehicle(data, this.offsetFromInitialPosition);
  if this.flightScale <= 0.0 {
    // pulled back towards the shot's own place until the camera is in open air (the nav map's
    // free space): scaled out into a building, a flyby ended as it began
    let scale = ClampF(AbsF(data.vehicleSpeed) / FlightCinematic.RoadSpeed(), 1.0, 5.0);
    // in C++ (Cinematic::FitAhead): shrunk by 0.8 a try while the nav map has the point solid
    scale = FlightCinematicShot.FitAhead(this.GetShotSpaceTransform(data), Transform.GetPosition(offset), scale);
    this.flightScale = scale;
  }
  Transform.SetPosition(offset, FlightCinematic.Ahead(Transform.GetPosition(offset), this.flightScale));
  if this.flightReach <= 0.0 {
    this.flightReach = Vector4.Length(Transform.GetPosition(offset));
  }
  return WorldTransform.TransformXForm(this.GetShotSpaceTransform(data), offset);
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
  wrappedMethod(context);
  if FlightCinematic.Flying() {
    FlightLog.Info("[CinematicCamera] shot " + this.name + " (" + NameToString(this.root.GetClassName()) + ") at " + FloatToStringPrec(context.vehicleSpeed, 1) + " m/s");
  }
}

// Shot weights in flight: the shots fixed in the world (the flybys) were rare next to the ones
// attached to the car.
@wrapMethod(vehicleCinematicCameraShot)
public const func GetShotProbabilityFromContext(context: vehicleCinematicCameraShotUpdateContext) -> Int32 {
  let probability = wrappedMethod(context);
  if FlightCinematic.Flying() && IsDefined(this.root as cameraShotRoot_FixedShot) {
    return probability * FlightCinematic.FixedShotWeight();
  }
  return probability;
}

// A shot is only picked on the side of the car the camera is on (no crossing the line), and four
// of the five flybys are on the right: while the camera was on the left there were none. A flyby
// watches from far ahead, where the cut across does not jar, so in flight they skip that rule.
@wrapMethod(vehicleCinematicCameraShot)
public const func GetCanBeSelectedFromContext(context: vehicleCinematicCameraShotUpdateContext) -> Bool {
  if FlightCinematic.Flying() && IsDefined(this.root as cameraShotRoot_FixedShot) {
    return true;
  }
  return wrappedMethod(context);
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
// about 30 m out (RightAheadHigh at 31 m, so it never lasted even on a road) and further in flight:
// they ended within 0.1 s and the camera drew one after another (2026-10-27). In flight their
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
