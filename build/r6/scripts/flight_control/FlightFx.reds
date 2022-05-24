public class FlightFx {
  private let sys: ref<FlightSystem>;
  public let component: ref<FlightComponent>;

  public let f_fx: ref<FxInstance>;
  public let b_fx: ref<FxInstance>;
  public let fl_fx: ref<FxInstance>;
  public let flb_fx: ref<FxInstance>;
  public let fr_fx: ref<FxInstance>;
  public let frb_fx: ref<FxInstance>;
  public let bl_fx: ref<FxInstance>;
  public let br_fx: ref<FxInstance>;

  public let resource: FxResource;

  let f_fx_wt: WorldTransform;
  let b_fx_wt: WorldTransform;
  let bl_fx_wt: WorldTransform;
  let br_fx_wt: WorldTransform;
  let fl_fx_wt: WorldTransform;
  let fr_fx_wt: WorldTransform;
  let flb_fx_wt: WorldTransform;
  let frb_fx_wt: WorldTransform;

  public let chassis: ref<IComponent>;

  public let fc: array<ref<IComponent>>;
  public let bc: array<ref<IComponent>>;
  public let blc: array<ref<IComponent>>;
  public let brc: array<ref<IComponent>>;
  public let flc: array<ref<IComponent>>;
  public let flbc: array<ref<IComponent>>;
  public let frc: array<ref<IComponent>>;
  public let frbc: array<ref<IComponent>>;

  public static func Create(component: ref<FlightComponent>) -> ref<FlightFx> {
    return new FlightFx().Initialize(component);
  }

  public func Initialize(component: ref<FlightComponent>) -> ref<FlightFx> {
    this.sys = component.sys;
    this.component = component;
    this.resource = Cast<FxResource>(r"user\\jackhumbert\\effects\\ion_thruster_with_engine.effect");
    return this;
  }

  public func HasFWheel() -> Bool {
    return ArraySize(this.fc) > 0;
  }
  
  public func HasBWheel() -> Bool {
    return ArraySize(this.bc) > 0;
  }

  public func HasFLWheel() -> Bool {
    return ArraySize(this.flc) > 0;
  }
  
  public func HasFLBWheel() -> Bool {
    return ArraySize(this.flbc) > 0;
  }
  
  public func HasFRWheel() -> Bool {
    return ArraySize(this.frc) > 0;
  }
  
  public func HasFRBWheel() -> Bool {
    return ArraySize(this.frbc) > 0;
  }
  
  public func HasBLWheel() -> Bool {
    return ArraySize(this.blc) > 0;
  }
  
  public func HasBRWheel() -> Bool {
    return ArraySize(this.brc) > 0;
  }

  public func Start() {
    this.fc = this.component.GetVehicle().GetComponentsUsingSlot(n"wheel_front");
    if ArraySize(this.fc) == 0 {
      this.fc = this.component.GetVehicle().GetComponentsUsingSlot(n"wheel_front_rot_set");
    }
    this.bc = this.component.GetVehicle().GetComponentsUsingSlot(n"wheel_back");
    if ArraySize(this.bc) == 0 {
      this.bc = this.component.GetVehicle().GetComponentsUsingSlot(n"axle_back_wheel");
    }
    this.blc = this.component.GetVehicle().GetComponentsUsingSlot(n"wheel_back_left");
    this.brc = this.component.GetVehicle().GetComponentsUsingSlot(n"wheel_back_right");
    this.flc = this.component.GetVehicle().GetComponentsUsingSlot(n"wheel_front_left");
    this.flbc = this.component.GetVehicle().GetComponentsUsingSlot(n"wheel_front_left_b");
    this.frc = this.component.GetVehicle().GetComponentsUsingSlot(n"wheel_front_right");
    this.frbc = this.component.GetVehicle().GetComponentsUsingSlot(n"wheel_front_right_b");

    let vehicleComponent = this.component.GetVehicle().GetVehicleComponent();
    this.chassis = vehicleComponent.FindComponentByName(n"chassis") as IComponent;
    if !IsDefined(this.chassis) {
      this.chassis = vehicleComponent.FindComponentByName(n"chasis") as IComponent;
    }

    this.HideWheelComponents();

    let effectTransform: WorldTransform;
    WorldTransform.SetPosition(effectTransform, this.component.stats.d_position);
    let wt = new WorldTransform();
    WorldTransform.SetPosition(wt, new Vector4(0.0, 0.0, 0.0, 0.0));
    let chassisOffset = (vehicleComponent.FindComponentByName(n"Chassis") as vehicleChassisComponent).GetLocalPosition();
    let vehicleSlots = this.component.GetVehicle().GetVehicleComponent().FindComponentByName(n"vehicle_slots") as SlotComponent;
    let vwt = Matrix.GetInverted(this.component.GetVehicle().GetLocalToWorld());
    if this.HasFWheel() {
      this.f_fx = GameInstance.GetFxSystem(this.component.GetVehicle().GetGame()).SpawnEffect(this.resource, effectTransform);
      this.f_fx.SetBlackboardValue(n"thruster_amount", 0.0);
      vehicleSlots.GetSlotTransform(n"wheel_front", this.f_fx_wt);
      this.component.fl_tire.SetLocalPosition(-chassisOffset + WorldPosition.ToVector4(WorldTransform.GetWorldPosition(this.f_fx_wt)) * vwt);
      this.f_fx.AttachToComponent(this.component.GetVehicle(), entAttachmentTarget.Transform, n"WheelAudioEmitterFront", wt);
    }
    if this.HasBWheel() {
      this.b_fx = GameInstance.GetFxSystem(this.component.GetVehicle().GetGame()).SpawnEffect(this.resource, effectTransform);
      this.b_fx.SetBlackboardValue(n"thruster_amount", 0.0);
      vehicleSlots.GetSlotTransform(n"wheel_back", this.b_fx_wt);
      this.component.bl_tire.SetLocalPosition(-chassisOffset + WorldPosition.ToVector4(WorldTransform.GetWorldPosition(this.b_fx_wt)) * vwt);
      this.b_fx.AttachToComponent(this.component.GetVehicle(), entAttachmentTarget.Transform, n"WheelAudioEmitterBack", wt);
    }
    if this.HasBLWheel() {
      this.bl_fx = GameInstance.GetFxSystem(this.component.GetVehicle().GetGame()).SpawnEffect(this.resource, effectTransform);
      this.bl_fx.SetBlackboardValue(n"thruster_amount", 0.0);
      vehicleSlots.GetSlotTransform(n"wheel_back_left", this.bl_fx_wt);
      this.component.bl_tire.SetLocalPosition(-chassisOffset + WorldPosition.ToVector4(WorldTransform.GetWorldPosition(this.bl_fx_wt)) * vwt);
      this.bl_fx.AttachToComponent(this.component.GetVehicle(), entAttachmentTarget.Transform, n"WheelAudioEmitterBL", wt);
    }
    if this.HasBRWheel() {
      this.br_fx = GameInstance.GetFxSystem(this.component.GetVehicle().GetGame()).SpawnEffect(this.resource, effectTransform);
      this.br_fx.SetBlackboardValue(n"thruster_amount", 0.0);
      vehicleSlots.GetSlotTransform(n"wheel_back_right", this.br_fx_wt);
      this.component.br_tire.SetLocalPosition(-chassisOffset + WorldPosition.ToVector4(WorldTransform.GetWorldPosition(this.br_fx_wt)) * vwt);
      this.br_fx.AttachToComponent(this.component.GetVehicle(), entAttachmentTarget.Transform, n"WheelAudioEmitterBR", wt);
    }
    if this.HasFLWheel() {
      this.fl_fx = GameInstance.GetFxSystem(this.component.GetVehicle().GetGame()).SpawnEffect(this.resource, effectTransform);
      this.fl_fx.SetBlackboardValue(n"thruster_amount", 0.0);
      vehicleSlots.GetSlotTransform(n"wheel_front_left", this.fl_fx_wt);
      this.component.fl_tire.SetLocalPosition(-chassisOffset + WorldPosition.ToVector4(WorldTransform.GetWorldPosition(this.fl_fx_wt)) * vwt);
      this.fl_fx.AttachToComponent(this.component.GetVehicle(), entAttachmentTarget.Transform, n"WheelAudioEmitterFL", wt);
    }
    if this.HasFRWheel() {
      this.fr_fx = GameInstance.GetFxSystem(this.component.GetVehicle().GetGame()).SpawnEffect(this.resource, effectTransform);
      this.fr_fx.SetBlackboardValue(n"thruster_amount", 0.0);
      vehicleSlots.GetSlotTransform(n"wheel_front_right", this.fr_fx_wt);
      this.component.fr_tire.SetLocalPosition(-chassisOffset + WorldPosition.ToVector4(WorldTransform.GetWorldPosition(this.fr_fx_wt)) * vwt);
      this.fr_fx.AttachToComponent(this.component.GetVehicle(), entAttachmentTarget.Transform, n"WheelAudioEmitterFR", wt);
    }
    if this.HasFLBWheel() {
      this.flb_fx = GameInstance.GetFxSystem(this.component.GetVehicle().GetGame()).SpawnEffect(this.resource, effectTransform);
      this.flb_fx.SetBlackboardValue(n"thruster_amount", 0.0);
      vehicleSlots.GetSlotTransform(n"wheel_front_left_b", this.flb_fx_wt);
      this.component.hood.SetLocalPosition(-chassisOffset + WorldPosition.ToVector4(WorldTransform.GetWorldPosition(this.flb_fx_wt)) * vwt);
      this.flb_fx.AttachToComponent(this.component.GetVehicle(), entAttachmentTarget.Transform, n"VehicleHoodEmitter", wt);
    }
    if this.HasFRBWheel() {
      this.frb_fx = GameInstance.GetFxSystem(this.component.GetVehicle().GetGame()).SpawnEffect(this.resource, effectTransform);
      this.frb_fx.SetBlackboardValue(n"thruster_amount", 0.0);
      vehicleSlots.GetSlotTransform(n"wheel_front_right_b", this.frb_fx_wt);
      this.component.trunk.SetLocalPosition(-chassisOffset + WorldPosition.ToVector4(WorldTransform.GetWorldPosition(this.frb_fx_wt)) * vwt);
      this.frb_fx.AttachToComponent(this.component.GetVehicle(), entAttachmentTarget.Transform, n"VehicleTrunkEmitter", wt);
    }
  }

  public func Stop() {
    if IsDefined(this.f_fx) {
      this.f_fx.BreakLoop();
    }
    if IsDefined(this.b_fx) {
      this.b_fx.BreakLoop();
    }
    if IsDefined(this.bl_fx) {
      this.bl_fx.BreakLoop();
    }
    if IsDefined(this.br_fx) {
      this.br_fx.BreakLoop();
    }
    if IsDefined(this.fl_fx) {
      this.fl_fx.BreakLoop();
    }
    if IsDefined(this.fr_fx) {
      this.fr_fx.BreakLoop();
    }
    if IsDefined(this.flb_fx) {
      this.flb_fx.BreakLoop();
    }
    if IsDefined(this.frb_fx) {
      this.frb_fx.BreakLoop();
    }


    this.ShowWheelComponents();
  }

  public func Update(visualForce: Vector4, visualTorque: Vector4) {
    // would be nice to do this periodically, or when the vehicle comes back into the frustum
    this.HideWheelComponents();

    let thrusterAmount = Vector4.Dot(new Vector4(0.0, 0.0, 1.0, 0.0), visualForce);
    // let thrusterAmount = ClampF(this.surge.GetValue(), 0.0, 1.0) * 1.0;
    if this.HasFWheel() {
      this.f_fx.SetBlackboardValue(n"thruster_amount", (thrusterAmount + visualTorque.X + visualTorque.Y) * this.sys.settings.thrusterFactor());
      if thrusterAmount > 0.0 {
        this.component.fl_tire.SetLocalOrientation(Quaternion.Slerp(this.component.fl_tire.GetLocalOrientation(), EulerAngles.ToQuat(new EulerAngles(
          ClampF(Vector4.GetAngleDegAroundAxis(FlightUtils.Up(), visualForce, FlightUtils.Right()), -45.0, 45.0),
          0.0,
          visualTorque.Z * 0.5 + ClampF(Vector4.GetAngleDegAroundAxis(FlightUtils.Up(), visualForce, FlightUtils.Forward()), -45.0, 45.0)
        )), 0.1));
      }
    }
    if this.HasBWheel() {
      this.b_fx.SetBlackboardValue(n"thruster_amount", (thrusterAmount + visualTorque.X - visualTorque.Y) * this.sys.settings.thrusterFactor());
      if thrusterAmount > 0.0 {
        this.component.bl_tire.SetLocalOrientation(Quaternion.Slerp(this.component.bl_tire.GetLocalOrientation(), EulerAngles.ToQuat(new EulerAngles(
          ClampF(Vector4.GetAngleDegAroundAxis(FlightUtils.Up(), visualForce, FlightUtils.Right()), -45.0, 45.0), 
          0.0,
          visualTorque.Z * 0.5 + ClampF(Vector4.GetAngleDegAroundAxis(FlightUtils.Up(), visualForce, FlightUtils.Forward()), -45.0, 45.0)
        )), 0.1));
      }
    }
    if this.HasBLWheel() {
      this.bl_fx.SetBlackboardValue(n"thruster_amount", (thrusterAmount + visualTorque.X + visualTorque.Y) * this.sys.settings.thrusterFactor());
      if thrusterAmount > 0.0 {
        this.component.bl_tire.SetLocalOrientation(Quaternion.Slerp(this.component.bl_tire.GetLocalOrientation(), EulerAngles.ToQuat(new EulerAngles(
          ClampF(Vector4.GetAngleDegAroundAxis(FlightUtils.Up(), visualForce, FlightUtils.Right()), -45.0, 45.0),
          0.0,
          visualTorque.Z * 0.5 + ClampF(Vector4.GetAngleDegAroundAxis(FlightUtils.Up(), visualForce, FlightUtils.Forward()), -45.0, 45.0)
        )), 0.1));
      }
    }
    if this.HasBRWheel() {
      this.br_fx.SetBlackboardValue(n"thruster_amount", (thrusterAmount + visualTorque.X - visualTorque.Y) * this.sys.settings.thrusterFactor());
      if thrusterAmount > 0.0 {
        this.component.br_tire.SetLocalOrientation(Quaternion.Slerp(this.component.br_tire.GetLocalOrientation(), EulerAngles.ToQuat(new EulerAngles(
          ClampF(Vector4.GetAngleDegAroundAxis(FlightUtils.Up(), visualForce, FlightUtils.Right()), -45.0, 45.0), 
          0.0,
          visualTorque.Z * 0.5 + ClampF(Vector4.GetAngleDegAroundAxis(FlightUtils.Up(), visualForce, FlightUtils.Forward()), -45.0, 45.0)
        )), 0.1));
      }
    }
    if this.HasFLWheel() {
      this.fl_fx.SetBlackboardValue(n"thruster_amount", (thrusterAmount - visualTorque.X + visualTorque.Y) * this.sys.settings.thrusterFactor());
      if thrusterAmount > 0.0 {
        this.component.fl_tire.SetLocalOrientation(Quaternion.Slerp(this.component.fl_tire.GetLocalOrientation(), EulerAngles.ToQuat(new EulerAngles(
          ClampF(Vector4.GetAngleDegAroundAxis(FlightUtils.Up(), visualForce, FlightUtils.Right()), -45.0, 45.0), 
          0.0,
          -visualTorque.Z * 0.5 + ClampF(Vector4.GetAngleDegAroundAxis(FlightUtils.Up(), visualForce, FlightUtils.Forward()), -45.0, 45.0)
        )), 0.1));
      }
    }
    if this.HasFRWheel() {
      this.fr_fx.SetBlackboardValue(n"thruster_amount", (thrusterAmount - visualTorque.X - visualTorque.Y) * this.sys.settings.thrusterFactor());
      if thrusterAmount > 0.0 {
        this.component.fr_tire.SetLocalOrientation(Quaternion.Slerp(this.component.fr_tire.GetLocalOrientation(), EulerAngles.ToQuat(new EulerAngles(
          ClampF(Vector4.GetAngleDegAroundAxis(FlightUtils.Up(), visualForce, FlightUtils.Right()), -45.0, 45.0), 
          0.0,
          -visualTorque.Z * 0.5 + ClampF(Vector4.GetAngleDegAroundAxis(FlightUtils.Up(), visualForce, FlightUtils.Forward()), -45.0, 45.0)
        )), 0.1));
      }
    }
    if this.HasFLBWheel() {
      this.flb_fx.SetBlackboardValue(n"thruster_amount", (thrusterAmount - visualTorque.X + visualTorque.Y) * this.sys.settings.thrusterFactor());
      if thrusterAmount > 0.0 {
        this.component.hood.SetLocalOrientation(Quaternion.Slerp(this.component.hood.GetLocalOrientation(), EulerAngles.ToQuat(new EulerAngles(
          ClampF(Vector4.GetAngleDegAroundAxis(FlightUtils.Up(), visualForce, FlightUtils.Right()), -45.0, 45.0), 
          0.0,
          -visualTorque.Z * 0.5 + ClampF(Vector4.GetAngleDegAroundAxis(FlightUtils.Up(), visualForce, FlightUtils.Forward()), -45.0, 45.0)
        )), 0.1));
      }
    }
    if this.HasFRBWheel() {
      this.frb_fx.SetBlackboardValue(n"thruster_amount", (thrusterAmount - visualTorque.X - visualTorque.Y) * this.sys.settings.thrusterFactor());
      if thrusterAmount > 0.0 {
        this.component.trunk.SetLocalOrientation(Quaternion.Slerp(this.component.trunk.GetLocalOrientation(), EulerAngles.ToQuat(new EulerAngles(
          ClampF(Vector4.GetAngleDegAroundAxis(FlightUtils.Up(), visualForce, FlightUtils.Right()), -45.0, 45.0), 
          0.0,
          -visualTorque.Z * 0.5 + ClampF(Vector4.GetAngleDegAroundAxis(FlightUtils.Up(), visualForce, FlightUtils.Forward()), -45.0, 45.0)
        )), 0.1));
      }
    }

    // +x = torwards front
    // +y = rotate left z
    // +z = towards right 
    // let forceQuat = Quaternion.BuildFromDirectionVector(-visualForce, this.component.stats.d_localUp);
    // let quat = EulerAngles.ToQuat(new EulerAngles(
    //   Vector4.GetAngleDegAroundAxis(FlightUtils.Up(), visualForce, FlightUtils.Right()),
    //   Vector4.GetAngleDegAroundAxis(FlightUtils.Up(), visualForce, FlightUtils.Forward()),
    //   0
    // ));
    // this.component.bl_tire.SetLocalOrientation(EulerAngles.ToQuat(new EulerAngles(
    //   -visualForce.Y,
    //   0.0, 
    //   visualTorque.Z - visualForce.X
    // )));
    // this.component.br_tire.SetLocalOrientation(EulerAngles.ToQuat(new EulerAngles(
    //   -visualForce.Y, 
    //   0.0, 
    //   visualTorque.Z - visualForce.X
    // )));
    // this.component.fl_tire.SetLocalOrientation(EulerAngles.ToQuat(new EulerAngles(
    //   -visualForce.Y, 
    //   0.0, 
    //   -visualTorque.Z - visualForce.X
    // )));
    // this.component.fr_tire.SetLocalOrientation(EulerAngles.ToQuat(new EulerAngles(
    //   -visualForce.Y, 
    //   0.0, 
    //   -visualTorque.Z - visualForce.X
    // )));
    // +x = torwards front
    // +y = rotate left z
    // +z = towards right 
    // WorldTransform.SetPosition(this.bl_fx_wt, this.audio.GetPosition(n"wheel_back_left"));
    // WorldTransform.SetPosition(this.br_fx_wt, this.audio.GetPosition(n"wheel_back_right"));
    // WorldTransform.SetPosition(this.fl_fx_wt, this.audio.GetPosition(n"wheel_front_left"));
    // WorldTransform.SetPosition(this.fr_fx_wt, this.audio.GetPosition(n"wheel_front_right"));

    // WorldTransform.SetOrientationEuler(this.bl_fx_wt, new EulerAngles(
    //   Vector4.Dot(visualForce, -this.stats.d_forward), 
    //   0.0, 
    //   visualTorque.Z + Vector4.Dot(visualForce, -this.stats.d_right)
    // ));
    // WorldTransform.SetOrientationEuler(this.br_fx_wt, new EulerAngles(
    //   Vector4.Dot(visualForce, -this.stats.d_forward), 
    //   0.0, 
    //   visualTorque.Z + Vector4.Dot(visualForce, -this.stats.d_right)
    // ));
    // WorldTransform.SetOrientationEuler(this.fl_fx_wt, new EulerAngles(
    //   Vector4.Dot(visualForce, -this.stats.d_forward), 
    //   0.0, 
    //   -visualTorque.Z + Vector4.Dot(visualForce, -this.stats.d_right)
    // ));
    // WorldTransform.SetOrientationEuler(this.fr_fx_wt, new EulerAngles(
    //   Vector4.Dot(visualForce, -this.stats.d_forward), 
    //   0.0, 
    //   -visualTorque.Z + Vector4.Dot(visualForce, -this.stats.d_right)
    // ));
    
    // this.bl_fx.UpdateTransform(this.bl_fx_wt);
    // this.br_fx.UpdateTransform(this.br_fx_wt);
    // this.fl_fx.UpdateTransform(this.fl_fx_wt);
    // this.fr_fx.UpdateTransform(this.fr_fx_wt);
  }

  public func HideWheelComponents() {
    // hide wheels, tires & brakes (chassis)

    if this.chassis.IsEnabled() {
      this.chassis.Toggle(false);
    }

    for c in this.fc {
      if c.IsEnabled() {
        c.Toggle(false);
      }
      // ArrayRemove(this.blc, c);
    }
    for c in this.bc {
      if c.IsEnabled() {
        c.Toggle(false);
      }
      // ArrayRemove(this.blc, c);
    }
    for c in this.blc {
      if c.IsEnabled() {
        c.Toggle(false);
      }
      // ArrayRemove(this.blc, c);
    }
    for c in this.brc {
      if c.IsEnabled() {
        c.Toggle(false);
      }
      // ArrayRemove(this.brc, c);
    }
    for c in this.flc {
      if c.IsEnabled() {
        c.Toggle(false);
      }
      // ArrayRemove(this.flc, c);
    }
    for c in this.frc {
      if c.IsEnabled() {
        c.Toggle(false);
      }
      // ArrayRemove(this.frc, c);
    }
    for c in this.flbc {
      if c.IsEnabled() {
        c.Toggle(false);
      }
      // ArrayRemove(this.frc, c);
    }
    for c in this.frbc {
      if c.IsEnabled() {
        c.Toggle(false);
      }
      // ArrayRemove(this.frc, c);
    }
  }

  public func ShowWheelComponents() {
    this.chassis.Toggle(true);

    for c in this.fc {
        c.Toggle(true);
    }
    for c in this.bc {
        c.Toggle(true);
    }
    for c in this.blc {
        c.Toggle(true);
    }
    for c in this.brc {
        c.Toggle(true);
    }
    for c in this.flc {
        c.Toggle(true);
    }
    for c in this.frc {
        c.Toggle(true);
    }
    for c in this.flbc {
        c.Toggle(true);
    }
    for c in this.frbc {
        c.Toggle(true);
    }
  }
}