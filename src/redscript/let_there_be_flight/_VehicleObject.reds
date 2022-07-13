@addField(VehicleObject)
private let m_flightComponent: wref<FlightComponent>;

@wrapMethod(VehicleObject)
protected cb func OnRequestComponents(ri: EntityRequestComponentsInterface) -> Bool {
  EntityRequestComponentsInterface.RequestComponent(ri, n"flightComponent", n"FlightComponent", true);
  EntityRequestComponentsInterface.RequestComponent(ri, n"flight_ui", n"worlduiWidgetComponent", true);
  EntityRequestComponentsInterface.RequestComponent(ri, n"flight_ui_info", n"worlduiWidgetComponent", true);
  wrappedMethod(ri);
}

@wrapMethod(VehicleObject)
protected cb func OnTakeControl(ri: EntityResolveComponentsInterface) -> Bool {
  //FlightLog.Info("[VehicleObject] OnTakeControl: " + this.GetDisplayName());
  this.m_flightComponent = EntityResolveComponentsInterface.GetComponent(ri, n"flightComponent") as FlightComponent;
  // this.m_flightComponent.ui = EntityResolveComponentsInterface.GetComponent(ri, n"flight_ui") as worlduiWidgetComponent;
  this.m_flightComponent.ui_info = EntityResolveComponentsInterface.GetComponent(ri, n"flight_ui_info") as worlduiWidgetComponent;
  // this.m_flightComponent.Toggle(false);
  wrappedMethod(ri);
}

@addMethod(VehicleObject)
public const func GetFlightComponent() -> ref<FlightComponent> {
  return this.m_flightComponent;
}

@addMethod(VehicleObject)
public func ToggleFlightComponent(state: Bool) -> Void {
  this.m_flightComponent.Toggle(state);
}

@addMethod(VehicleObject)
public func GetLocalToWorld() -> Matrix {
  return WorldTransform.ToMatrix(this.GetWorldTransform());
}

@addField(VehicleObject)
public let chassis: ref<vehicleChassisComponent>;

@addField(VehicleObject)
@runtimeProperty("offset", "0x24C")
public native let isOnGround: Bool;

@addField(VehicleObject)
public native let acceleration: Float;

@addField(VehicleObject)
public native let deceleration: Float;

@addField(VehicleObject)
public native let handbrake: Float;

@addField(VehicleObject)
public native let turnX: Float;

@addField(VehicleObject)
public native let turnX2: Float;

@addField(VehicleObject)
public native let turnX3: Float;

@addField(VehicleObject)
public native let turnX4: Float;

@addField(VehicleObject)
public native let tracePosition: Vector3;

@addMethod(VehicleObject)
public native func UsesInertiaTensor() -> Bool;

@addMethod(VehicleObject)
public native func GetInertiaTensor() -> Matrix;

// @addMethod(VehicleObject)
// public native func GetWorldInertiaTensor() -> Matrix;

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

@addMethod(VehicleObject)
public native func GetComponentsUsingSlot(slotName: CName) -> array<ref<IComponent>>;

@addMethod(VehicleObject)
public native func GetWeaponPlaceholderOrientation(index: Int32) -> Quaternion;

// working
// @addMethod(VehicleObject)
// protected cb func OnPhysicalCollision(evt: ref<PhysicalCollisionEvent>) -> Bool {
//   FlightLog.Info("[VehicleObject] OnPhysicalCollision");
//   let vehicle = evt.otherEntity as VehicleObject;
//   if IsDefined(vehicle) {
//     let gameInstance: GameInstance = this.GetGame();
//     let player: ref<PlayerPuppet> = GetPlayer(gameInstance);
//     let isPlayerMounted = VehicleComponent.IsMountedToProvidedVehicle(gameInstance, player.GetEntityID(), vehicle);
//     if isPlayerMounted {
//       // FlightController.GetInstance().ProcessImpact(evt.attackData.vehicleImpactForce);
//     } else {
//       let impulseEvent: ref<PhysicalImpulseEvent> = new PhysicalImpulseEvent();
//       impulseEvent.radius = 1.0;
//       impulseEvent.worldPosition = Vector4.Vector4To3(evt.worldPosition);
//       impulseEvent.worldImpulse = new Vector3(0.0, 0.0, 10000.0);
//       vehicle.QueueEvent(impulseEvent);
//     }
//   }
// }

@wrapMethod(VehicleObject)
public final func IsOnPavement() -> Bool {
  return wrappedMethod() || FlightController.GetInstance().IsActive();
}

// @wrapMethod(VehicleObject)
// protected cb func OnLookedAtEvent(evt: ref<LookedAtEvent>) -> Bool {
//   wrappedMethod(evt);
//   if this.IsDestroyed() && this.IsCurrentlyScanned() {
//     let player: ref<PlayerPuppet> = GetPlayer(this.GetGame());
//     let uiSystem: ref<UISystem> = GameInstance.GetUISystem(this.GetGame());
//     if evt.isLookedAt {
//         player.RegisterInputListener(this.m_vehicleComponent, n"Choice1");
//         uiSystem.QueueEvent(FlightController.ShowHintHelper("Repair Vehicle", n"Choice1", n"RepairVehicle"));
//     } else {
//         player.UnregisterInputListener(this.m_vehicleComponent, n"Choice1");
//         uiSystem.QueueEvent(FlightController.HideHintFromSource(n"RepairVehicle"));
//     }
//   }
// } 

// @addMethod(VehicleObject)
// public const func IsQuickHackAble() -> Bool {
//   return true;
// }

// @addMethod(VehicleObject)
// public const func IsQuickHacksExposed() -> Bool {
//   return true;
// }