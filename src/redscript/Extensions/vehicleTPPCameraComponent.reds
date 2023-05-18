public native class vehicleChassisComponent extends IPlacedComponent {
    public native func GetComOffset() -> Transform;
}

native class vehicleTPPCameraComponent extends CameraComponent {
    // public native let isInAir: Bool;
    public native let drivingDirectionCompensationAngleSmooth: Float;
    public native let drivingDirectionCompensationSpeedCoef: Float;
    public native let lockedCamera: Bool;
    public native let initialTransform: WorldTransform;
    public native let pitch: Float;
    public native let yaw: Float;
    // public native let pitchDelta: Float; // positive moves camera down
    // public native let yawDelta: Float; // positive moves camera right
    // public native let chassis: ref<vehicleChassisComponent>;
}

// @addMethod(vehicleChassisComponent)
// protected cb func OnRequestComponents(ri: EntityRequestComponentsInterface) -> Bool {
//     super.OnRequestComponents(ri);
//     EntityRequestComponentsInterface.RequestComponent(ri, n"Chassis", n"vehicleChassisComponent", false);
// }


public native class vehicleDriveToPointEvent extends Event {
    public native let targetPos: Vector3;
    public native let useTraffic: Bool;
    public native let speedInTraffic: Float;
}

// public importonly class EffectSpawnerComponent extends IVisualComponent {
//     public native func AddEffect() -> Void;
// }


// @addField(ColliderComponent)
// public native let mass: Float;

// @addField(ColliderComponent)
// public native let massOverride: Float;

// @addField(ColliderComponent)
// public native let inertia: Vector3;

// @addField(ColliderComponent)
// public native let comOffset: Transform;

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


// @addMethod(FxSystem)
// public final native func SpawnEffect(resource: ResRef, transform: WorldTransform, opt ignoreTimeDilation: Bool) -> ref<FxInstance>;

// @addField(FxResource)
// public native let effect: ResRef;

// @addMethod(Entity)
// public native func AddComponent(component: ref<IComponent>) -> Bool;

// @addMethod(Entity)
// public native func AddWorldWidgetComponent() -> Bool;

// @addMethod(IPlacedComponent)
// public native func UpdateHardTransformBinding(bindName: CName, slotName: CName) -> Bool;