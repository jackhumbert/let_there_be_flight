public class FlightModeDrone extends FlightMode {

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-Flight-Mode-Settings")
  @runtimeProperty("ModSettings.displayName", "UI-Settings-Drone-Mode-Enabled")
  public let enabled: Bool = true;

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-Drone-Mode")
  @runtimeProperty("ModSettings.displayName", "Drone Mode Name")
  public let droneModeName: CName = n"Drone Mode";

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-Drone-Mode")
  @runtimeProperty("ModSettings.displayName", "UI-Settings-Lift-Factor")
  @runtimeProperty("ModSettings.step", "0.5")
  @runtimeProperty("ModSettings.min", "0")
  @runtimeProperty("ModSettings.max", "200")
  public let droneModeLiftFactor: Float = 40.0;
  
  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-Drone-Mode")
  @runtimeProperty("ModSettings.displayName", "UI-Settings-Pitch-Factor")
  @runtimeProperty("ModSettings.step", "0.1")
  @runtimeProperty("ModSettings.min", "0.0")
  @runtimeProperty("ModSettings.max", "200")
  public let droneModePitchFactor: Float = 5.0;
  
  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-Drone-Mode")
  @runtimeProperty("ModSettings.displayName", "UI-Settings-Roll-Factor")
  @runtimeProperty("ModSettings.step", "0.1")
  @runtimeProperty("ModSettings.min", "0.0")
  @runtimeProperty("ModSettings.max", "200")
  public let droneModeRollFactor: Float = 12.0;
  
  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-Drone-Mode")
  @runtimeProperty("ModSettings.displayName", "UI-Settings-Surge-Factor")
  @runtimeProperty("ModSettings.step", "0.5")
  @runtimeProperty("ModSettings.min", "0")
  @runtimeProperty("ModSettings.max", "200")
  public let droneModeSurgeFactor: Float = 15.0;
  
  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-Drone-Mode")
  @runtimeProperty("ModSettings.displayName", "UI-Settings-Yaw-Factor")
  @runtimeProperty("ModSettings.step", "0.1")
  @runtimeProperty("ModSettings.min", "0.0")
  @runtimeProperty("ModSettings.max", "200")
  public let droneModeYawFactor: Float = 5.0;
  
  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-Drone-Mode")
  @runtimeProperty("ModSettings.displayName", "UI-Settings-Sway-Factor")
  @runtimeProperty("ModSettings.step", "0.5")
  @runtimeProperty("ModSettings.min", "0")
  @runtimeProperty("ModSettings.max", "200")
  public let droneModeSwayFactor: Float = 15.0;

  public static func Create(component: ref<FlightComponent>) -> ref<FlightModeDrone> {
    let self = new FlightModeDrone();
    self.Initialize(component);
    return self;
  }

  public func Initialize(component: ref<FlightComponent>) -> Void {
    super.Initialize(component);
    this.usesRightStickInput = true;
    LTBF_RegisterListener(this);
  }

  public func Deinitialize() -> Void {
    LTBF_UnregisterListener(this);
  }

  public func Activate() -> Void {
    // let camera = this.component.sys.player.GetFPPCameraComponent();
    // if IsDefined(camera) {
    //   let slotT: WorldTransform;
    //   let vehicleSlots = this.component.GetVehicle().GetVehicleComponent().FindComponentByName(n"OccupantSlots") as SlotComponent;
    //   vehicleSlots.GetSlotTransform(n"seat_front_left", slotT);
    //   let vwt = Matrix.GetInverted(this.component.GetVehicle().GetLocalToWorld());
    //   let v = WorldPosition.ToVector4(WorldTransform.GetWorldPosition(slotT)) * vwt;
    //   camera.SetLocalPosition(new Vector4(0.0, FlightSettings.GetFloat("FPVCameraOffsetY"), FlightSettings.GetFloat("FPVCameraOffsetZ"), 0.0) - v);
    //   // camera.Activate(1.0);
    // }
  }

  public func Deactivate() -> Void {
    // let camera = this.component.sys.player.GetFPPCameraComponent();
    // if IsDefined(camera) {
    //   camera.SetLocalPosition(new Vector4(0.0, 0.0, 0.0, 0.0));
    // }
  }

  public func GetDescription() -> String = "Drone";

  public func Update(timeDelta: Float) -> Void {
      let velocityDamp: Vector4 = this.component.stats.d_localVelocity * this.component.linearBrake * FlightSettings.GetInstance().brakeFactorLinear * this.component.stats.s_brakingFrictionFactor;   
      let angularDamp: Vector4 = this.component.stats.d_angularVelocity * this.component.angularBrake * FlightSettings.GetInstance().brakeFactorAngular * this.component.stats.s_brakingFrictionFactor;

      this.force = new Vector4(0.0, 0.0, 0.0, 0.0);
      // lift
      this.force += FlightUtils.Up() * this.component.lift * this.droneModeLiftFactor;
      // surge
      this.force += FlightUtils.Forward() * this.component.surge * this.droneModeSurgeFactor;
      // sway
      this.force += FlightUtils.Right() * this.component.sway * this.droneModeSwayFactor;
      // directional brake
      this.force -= velocityDamp;

      this.torque = new Vector4(0.0, 0.0, 0.0, 0.0);
      // pitch correction
      this.torque.X = -(this.component.pitch * this.droneModePitchFactor + angularDamp.X);
      // roll correction
      this.torque.Y = (this.component.roll * this.droneModeRollFactor - angularDamp.Y);
      // yaw correction
      this.torque.Z = -(this.component.yaw * this.droneModeYawFactor + angularDamp.Z);
  }
}