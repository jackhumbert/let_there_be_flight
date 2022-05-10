public native class vehicleChassisComponent extends IPlacedComponent {
    public native func GetComOffset() -> Transform;
}

native class vehicleTPPCameraComponent extends CameraComponent {
    // public native let isInAir: Bool;
    public native let drivingDirectionCompensationAngleSmooth: Float;
    public native let drivingDirectionCompensationSpeedCoef: Float;
    public native let lockedCamera: Bool;
    public native let worldPosition: WorldPosition;
    public native let worldTransform2: WorldTransform;
    public native let pitch: Float;
    public native let yaw: Float;
    public native let pitchDelta: Float; // positive moves camera down
    public native let yawDelta: Float; // positive moves camera right
    // public native let chassis: ref<vehicleChassisComponent>;
}

@addField(VehicleObject)
public native let isOnGround: Bool;

@addField(VehicleObject)
public let chassis: ref<vehicleChassisComponent>;

@addMethod(VehicleObject)
public native func UsesInertiaTensor() -> Bool;

@addMethod(VehicleObject)
public native func GetInertiaTensor() -> Matrix;

@addMethod(VehicleObject)
public native func GetMomentOfInertiaScale() -> Vector3;

@addMethod(VehicleObject)
public native func GetCenterOfMass() -> Vector3;

@addMethod(VehicleObject)
public native func GetAngularVelocity() -> Vector3;

@addMethod(VehicleObject)
public native func TurnOffAirControl() -> Bool;

public native class vehicleFlightHelper extends IScriptable {
    public native let force: Vector4;
    public native let torque: Vector4;
}

@addMethod(VehicleObject)
public native func AddFlightHelper() -> ref<vehicleFlightHelper>;

// @addMethod(vehicleChassisComponent)
// protected cb func OnRequestComponents(ri: EntityRequestComponentsInterface) -> Bool {
//     super.OnRequestComponents(ri);
//     EntityRequestComponentsInterface.RequestComponent(ri, n"Chassis", n"vehicleChassisComponent", false);
// }

@wrapMethod(MinimapContainerController)
protected final func InitializePlayer(playerPuppet: ref<GameObject>) -> Void {
  wrappedMethod(playerPuppet);
  FlightController.GetInstance().ui.mmcc = this;
}

@addField(MinimapContainerController)
public native let questPoints: array<Vector4>;

@addField(MinimapContainerController)
public native let playerPoints: array<Vector4>;

public native class vehicleDriveToPointEvent extends Event {
    public native let targetPos: Vector3;
    public native let useTraffic: Bool;
    public native let speedInTraffic: Float;
}

public importonly class EffectSpawnerComponent extends IVisualComponent {
    public native func AddEffect() -> Void;
}


@addField(ColliderComponent)
public native let mass: Float;

@addField(ColliderComponent)
public native let massOverride: Float;

@addField(ColliderComponent)
public native let inertia: Vector3;

@addField(ColliderComponent)
public native let comOffset: Transform;

// public native class exEntitySpawner {
//     public native static func Spawn(entityPath: ResRef, worldTransform: WorldTransform, opt appearance: CName, opt recordID: TweakDBID) -> EntityID;
//     public native static func SpawnRecord(recordID: TweakDBID, worldTransform: WorldTransform, opt appearance: CName) -> EntityID;
//     public native static func Despawn(entity: ref<Entity>) -> Void;
// }

// @addField(MappinSystem)
// public native let worldMappins: Array<Ptr<>>;

// @addField(entCameraComponent)
// native let fov: Float;

// @addField(entCameraComponent)
// native let zoom: Float;

// @addField(entCameraComponent)
// native let nearPlaneOverride: Float;

// @addField(entCameraComponent)
// native let farPlaneOverride: Float;

// @addField(entCameraComponent)
// native let motionBlurScale: Float;

//FindVehicleCameraManager