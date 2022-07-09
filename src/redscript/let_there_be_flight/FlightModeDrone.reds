public class FlightModeDrone extends FlightMode {
  public static func Create(component: ref<FlightComponent>) -> ref<FlightModeDrone> {
    let self = new FlightModeDrone();
    self.Initialize(component);
    return self;
  }

  public func Initialize(component: ref<FlightComponent>) -> Void {
    super.Initialize(component);
    this.usesRightStickInput = true;
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
      let velocityDamp: Vector4 = this.component.stats.d_localVelocity * this.component.linearBrake * FlightSettings.GetFloat("brakeFactorLinear") * this.component.stats.s_brakingFrictionFactor;   
      let angularDamp: Vector4 = this.component.stats.d_angularVelocity * this.component.angularBrake * FlightSettings.GetFloat("brakeFactorAngular") * this.component.stats.s_brakingFrictionFactor;

      this.force = new Vector4(0.0, 0.0, 0.0, 0.0);
      // lift
      this.force += FlightUtils.Up() * this.component.lift * FlightSettings.GetFloat("droneModeLiftFactor");
      // surge
      this.force += FlightUtils.Forward() * this.component.surge * FlightSettings.GetFloat("droneModeSurgeFactor");
      // sway
      this.force += FlightUtils.Right() * this.component.sway * FlightSettings.GetFloat("droneModeSwayFactor");
      // directional brake
      this.force -= velocityDamp;

      this.torque = new Vector4(0.0, 0.0, 0.0, 0.0);
      // pitch correction
      this.torque.X = -(this.component.pitch * FlightSettings.GetFloat("droneModePitchFactor") + angularDamp.X);
      // roll correction
      this.torque.Y = (this.component.roll * FlightSettings.GetFloat("droneModeRollFactor") - angularDamp.Y);
      // yaw correction
      this.torque.Z = -(this.component.yaw * FlightSettings.GetFloat("droneModeYawFactor") + angularDamp.Z);
  }
}