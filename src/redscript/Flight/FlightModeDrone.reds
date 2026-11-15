public class FlightModeDrone extends FlightMode {

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-Flight-Mode-Settings")
  @runtimeProperty("ModSettings.category.order", "3")
  @runtimeProperty("ModSettings.order", "4")
  @runtimeProperty("ModSettings.displayName", "UI-Settings-Drone-Mode-Enabled")
  public let enabled: Bool = true;

  // Scales the three turn factors below.
  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-Flight-Mode-Settings")
  @runtimeProperty("ModSettings.category.order", "3")
  @runtimeProperty("ModSettings.order", "5")
  @runtimeProperty("ModSettings.displayName", "UI-Settings-Drone-Turn-Speed")
  @runtimeProperty("ModSettings.description", "UI-Settings-Drone-Turn-Speed-Description")
  @runtimeProperty("ModSettings.step", "0.05")
  @runtimeProperty("ModSettings.min", "0.25")
  @runtimeProperty("ModSettings.max", "3.0")
  public let droneTurnSpeed: Float = 1.0;

  // Scales the three thrust factors below.
  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-Flight-Mode-Settings")
  @runtimeProperty("ModSettings.category.order", "3")
  @runtimeProperty("ModSettings.order", "6")
  @runtimeProperty("ModSettings.displayName", "UI-Settings-Drone-Thrust")
  @runtimeProperty("ModSettings.description", "UI-Settings-Drone-Thrust-Description")
  @runtimeProperty("ModSettings.step", "0.05")
  @runtimeProperty("ModSettings.min", "0.25")
  @runtimeProperty("ModSettings.max", "3.0")
  public let droneThrust: Float = 1.0;

  // Developer values (no longer Mod Settings, docs/design/settings-audit.md): thrust, m/s2 at full input, and
  // turn, torque per unit of input. Drone Thrust and Drone Turn Speed scale them.
  public let droneModeLiftFactor: Float = 40.0;
  public let droneModePitchFactor: Float = 5.0;
  public let droneModeRollFactor: Float = 8.0;
  public let droneModeSurgeFactor: Float = 15.0;
  public let droneModeYawFactor: Float = 5.0;
  public let droneModeSwayFactor: Float = 15.0;

  public func Initialize(component: ref<FlightComponent>) -> Void {
    super.Initialize(component);
    this.usesRightStickInput = true;
    LTBF_RegisterListener(this);
  }

  public func Deinitialize() -> Void {
    LTBF_UnregisterListener(this);
  }

  public func IsEnabled() -> Bool = this.enabled;

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
      if this.component.nativeModes {
        let k = this.Control(timeDelta);
        k.droneModeLiftFactor = this.droneModeLiftFactor * this.droneThrust;
        k.droneModePitchFactor = this.droneModePitchFactor * this.droneTurnSpeed;
        k.droneModeRollFactor = this.droneModeRollFactor * this.droneTurnSpeed;
        k.droneModeSurgeFactor = this.droneModeSurgeFactor * this.droneThrust;
        k.droneModeYawFactor = this.droneModeYawFactor * this.droneTurnSpeed;
        k.droneModeSwayFactor = this.droneModeSwayFactor * this.droneThrust;
        k.Drone(this.component.stats, this.component);
        this.force = k.force;
        this.torque = k.torque;
        return;
      }
      let velocityDamp: Vector4 = this.component.stats.d_localVelocity * this.component.linearBrake * FlightSettings.GetInstance().LinearBrake() * this.component.stats.s_brakingFrictionFactor;   
      // let velocityDamp: Vector4;
      let angularDamp: Vector4 = this.component.stats.d_angularVelocity * this.component.angularBrake * FlightSettings.GetInstance().AngularBrake() * this.component.stats.s_brakingFrictionFactor;
      // let angularDamp: Vector4;

      this.force = new Vector4(0.0, 0.0, 0.0, 0.0);
      // lift
      this.force += FlightUtils.Up() * this.component.lift * this.droneModeLiftFactor * this.droneThrust;
      // surge
      this.force += FlightUtils.Forward() * this.component.surge * this.droneModeSurgeFactor * this.droneThrust;
      // sway
      this.force += FlightUtils.Right() * this.component.sway * this.droneModeSwayFactor * this.droneThrust;
      // directional brake
      this.force -= velocityDamp;

      this.torque = new Vector4(0.0, 0.0, 0.0, 0.0);
      // pitch correction
      this.torque.X = -(this.component.pitch * this.droneModePitchFactor * this.droneTurnSpeed + angularDamp.X);
      // roll correction
      this.torque.Y = (this.component.roll * this.droneModeRollFactor * this.droneTurnSpeed - angularDamp.Y);
      // yaw correction
      this.torque.Z = -(this.component.yaw * this.droneModeYawFactor * this.droneTurnSpeed + angularDamp.Z);
  }
}