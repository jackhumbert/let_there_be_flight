// Vanilla's cinematic camera (autodrive's, and the Autopilot's) was made for cars on a road. In
// flight the shots attached to the car follow its whole attitude (vanilla's suspension-tracking
// ones already do; the stabilised ones cross world up with the car's right vector, which has no
// answer at 90 degrees of bank), so the camera swings round a banking car and stays outside it
// (kept level, the close side shots had the car roll its roof into the lens). Every shot then aims
// at the car's centre of mass with the horizon level, from at least MinDistance: the shots' own
// angles were made for a level car and lost a banking one off the frame.
public abstract class FlightCinematic {
  public static func Flying() -> Bool {
    return FlightController.GetInstance().IsActive();
  }

  // closest a camera comes to the centre of mass, m (the shot context's bounding box is no use for
  // this: it read -2.75 m long for a 5.8 m car in Update)
  public static func MinDistance() -> Float {
    return 4.5;
  }

  public static func AimAtCar(xform: WorldTransform, context: vehicleCinematicCameraShotUpdateContext) -> WorldTransform {
    let vehicle = GetMountedVehicle(GetPlayer(GetGameInstance()));
    let com = WorldPosition.ToVector4(WorldTransform.GetWorldPosition(context.vehicleTransform));
    if IsDefined(vehicle) {
      com = WorldPosition.ToVector4(WorldTransform.TransformPoint(context.vehicleTransform, Vector4.Vector3To4(vehicle.GetCenterOfMass())));
    }
    let camera = WorldPosition.ToVector4(WorldTransform.GetWorldPosition(xform));
    let toCar = com - camera;
    let distance = Vector4.Length(toCar);
    if distance < 0.01 {
      return xform;
    }
    let least = FlightCinematic.MinDistance();
    if distance < least {
      WorldTransform.SetPosition(xform, com - toCar * (least / distance));
    }
    if AbsF(toCar.Z) / distance < 0.98 {
      WorldTransform.SetOrientationFromDir(xform, toCar);
    }
    return xform;
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
// car went past in a third of a second. Their offset is scaled with the speed at the shot's start,
// so the car takes about as long to reach the camera as at road speed (FlightCinematic.RoadSpeed).
@addField(cameraShotRoot_FixedShot)
private let flightScale: Float;

// (vehicleCinematicCameraShotRoot.Reset is empty, and the fixed shot does not override it)
@addMethod(cameraShotRoot_FixedShot)
public func Reset() -> Void {
  this.flightScale = 0.0;
}

@wrapMethod(cameraShotRoot_FixedShot)
public func GetRootTransform(data: CinematicCameraData) -> WorldTransform {
  if !FlightCinematic.Flying() {
    return wrappedMethod(data);
  }
  if this.flightScale <= 0.0 {
    this.flightScale = ClampF(AbsF(data.vehicleSpeed) / FlightCinematic.RoadSpeed(), 1.0, 5.0);
  }
  let offset = CinematicCameraData.ScaleTransformForVehicle(data, this.offsetFromInitialPosition);
  Transform.SetPosition(offset, Transform.GetPosition(offset) * this.flightScale);
  return WorldTransform.TransformXForm(this.GetShotSpaceTransform(data), offset);
}

// Which shot the camera picks while flying, one line each.
@wrapMethod(vehicleCinematicCameraShot)
public func Start(context: vehicleCinematicCameraShotUpdateContext) -> Void {
  wrappedMethod(context);
  if FlightCinematic.Flying() {
    FlightLog.Info("[CinematicCamera] shot " + this.name + " (" + NameToString(this.root.GetClassName()) + ") at " + FloatToStringPrec(context.vehicleSpeed, 1) + " m/s");
  }
}
