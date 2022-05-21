public class FlightFx {
  private let sys: ref<FlightSystem>;
  public let component: ref<FlightComponent>;
  public let fl_fx: ref<FxInstance>;
  public let fr_fx: ref<FxInstance>;
  public let bl_fx: ref<FxInstance>;
  public let br_fx: ref<FxInstance>;
  public let resource: FxResource;
  // let bl_fx_wt: WorldTransform;
  // let br_fx_wt: WorldTransform;
  // let fl_fx_wt: WorldTransform;
  // let fr_fx_wt: WorldTransform;

  public static func Create(component: ref<FlightComponent>) -> ref<FlightFx> {
    return new FlightFx().Initialize(component);
  }

  public func Initialize(component: ref<FlightComponent>) -> ref<FlightFx> {
    this.sys = component.sys;
    this.component = component;
    return this;
  }

  public func Start() {
    // hide wheels, tires & brakes (chassis)
    let vehicleComponent = this.component.GetVehicle().GetVehicleComponent();
    (vehicleComponent.FindComponentByName(n"chassis") as IComponent).Toggle(false);

    (vehicleComponent.FindComponentByName(n"tire_01_fl_a") as IComponent).Toggle(false);
    (vehicleComponent.FindComponentByName(n"tire_01_fl_b") as IComponent).Toggle(false);
    (vehicleComponent.FindComponentByName(n"tire_01_fl_a_shadow") as IComponent).Toggle(false);
    (vehicleComponent.FindComponentByName(n"wheel_01_fl_a") as IComponent).Toggle(false);
    (vehicleComponent.FindComponentByName(n"wheel_01_fl_b") as IComponent).Toggle(false);

    (vehicleComponent.FindComponentByName(n"tire_01_fr_a") as IComponent).Toggle(false);
    (vehicleComponent.FindComponentByName(n"tire_01_fr_b") as IComponent).Toggle(false);
    (vehicleComponent.FindComponentByName(n"tire_01_fr_a_shadow") as IComponent).Toggle(false);
    (vehicleComponent.FindComponentByName(n"wheel_01_fr_a") as IComponent).Toggle(false);
    (vehicleComponent.FindComponentByName(n"wheel_01_fr_b") as IComponent).Toggle(false);

    (vehicleComponent.FindComponentByName(n"tire_01_bl_a") as IComponent).Toggle(false);
    (vehicleComponent.FindComponentByName(n"tire_01_bl_b") as IComponent).Toggle(false);
    (vehicleComponent.FindComponentByName(n"tire_01_bl_a_shadow") as IComponent).Toggle(false);
    (vehicleComponent.FindComponentByName(n"wheel_01_bl_a") as IComponent).Toggle(false);
    (vehicleComponent.FindComponentByName(n"wheel_01_bl_b") as IComponent).Toggle(false);

    (vehicleComponent.FindComponentByName(n"tire_01_br_a") as IComponent).Toggle(false);
    (vehicleComponent.FindComponentByName(n"tire_01_br_b") as IComponent).Toggle(false);
    (vehicleComponent.FindComponentByName(n"tire_01_br_a_shadow") as IComponent).Toggle(false);
    (vehicleComponent.FindComponentByName(n"wheel_01_br_a") as IComponent).Toggle(false);
    (vehicleComponent.FindComponentByName(n"wheel_01_br_b") as IComponent).Toggle(false);

    (vehicleComponent.FindComponentByName(n"tire_02_fl_a") as IComponent).Toggle(false);
    (vehicleComponent.FindComponentByName(n"tire_02_fl_b") as IComponent).Toggle(false);
    (vehicleComponent.FindComponentByName(n"tire_02_fl_a_shadow") as IComponent).Toggle(false);
    (vehicleComponent.FindComponentByName(n"wheel_02_fl_a") as IComponent).Toggle(false);
    (vehicleComponent.FindComponentByName(n"wheel_02_fl_b") as IComponent).Toggle(false);

    (vehicleComponent.FindComponentByName(n"tire_02_fr_a") as IComponent).Toggle(false);
    (vehicleComponent.FindComponentByName(n"tire_02_fr_b") as IComponent).Toggle(false);
    (vehicleComponent.FindComponentByName(n"tire_02_fr_a_shadow") as IComponent).Toggle(false);
    (vehicleComponent.FindComponentByName(n"wheel_02_fr_a") as IComponent).Toggle(false);
    (vehicleComponent.FindComponentByName(n"wheel_02_fr_b") as IComponent).Toggle(false);

    (vehicleComponent.FindComponentByName(n"tire_02_bl_a") as IComponent).Toggle(false);
    (vehicleComponent.FindComponentByName(n"tire_02_bl_b") as IComponent).Toggle(false);
    (vehicleComponent.FindComponentByName(n"tire_02_bl_a_shadow") as IComponent).Toggle(false);
    (vehicleComponent.FindComponentByName(n"wheel_02_bl_a") as IComponent).Toggle(false);
    (vehicleComponent.FindComponentByName(n"wheel_02_bl_b") as IComponent).Toggle(false);

    (vehicleComponent.FindComponentByName(n"tire_02_br_a") as IComponent).Toggle(false);
    (vehicleComponent.FindComponentByName(n"tire_02_br_b") as IComponent).Toggle(false);
    (vehicleComponent.FindComponentByName(n"tire_02_br_a_shadow") as IComponent).Toggle(false);
    (vehicleComponent.FindComponentByName(n"wheel_02_br_a") as IComponent).Toggle(false);
    (vehicleComponent.FindComponentByName(n"wheel_02_br_b") as IComponent).Toggle(false);

    (vehicleComponent.FindComponentByName(n"wheel_bl_a_01") as IComponent).Toggle(false);
    (vehicleComponent.FindComponentByName(n"wheel_br_a_01") as IComponent).Toggle(false);
    (vehicleComponent.FindComponentByName(n"wheel_fl_a_01") as IComponent).Toggle(false);
    (vehicleComponent.FindComponentByName(n"wheel_fr_a_01") as IComponent).Toggle(false);

    (vehicleComponent.FindComponentByName(n"wheel_bl_b_01") as IComponent).Toggle(false);
    (vehicleComponent.FindComponentByName(n"wheel_br_b_01") as IComponent).Toggle(false);
    (vehicleComponent.FindComponentByName(n"wheel_fl_b_01") as IComponent).Toggle(false);
    (vehicleComponent.FindComponentByName(n"wheel_fr_b_01") as IComponent).Toggle(false);

    (vehicleComponent.FindComponentByName(n"wheel_bl_a_02") as IComponent).Toggle(false);
    (vehicleComponent.FindComponentByName(n"wheel_br_a_02") as IComponent).Toggle(false);
    (vehicleComponent.FindComponentByName(n"wheel_fl_a_02") as IComponent).Toggle(false);
    (vehicleComponent.FindComponentByName(n"wheel_fr_a_02") as IComponent).Toggle(false);

    (vehicleComponent.FindComponentByName(n"wheel_bl_b_02") as IComponent).Toggle(false);
    (vehicleComponent.FindComponentByName(n"wheel_br_b_02") as IComponent).Toggle(false);
    (vehicleComponent.FindComponentByName(n"wheel_fl_b_02") as IComponent).Toggle(false);
    (vehicleComponent.FindComponentByName(n"wheel_fr_b_02") as IComponent).Toggle(false);

    let effectTransform: WorldTransform;
    WorldTransform.SetPosition(effectTransform, this.component.stats.d_position);
    // let resource: FxResource = Cast<FxResource>(r"base\\fx\\vehicles\\av\\av_panzer\\av_panzer_thruster_quest.effect");
    // let resource: FxResource = Cast<FxResource>(r"base\\fx\\vehicles\\av_ion_thruster.effect");
    this.resource = Cast<FxResource>(r"base\\fx\\vehicles\\v_av_manticore_thrusters_idle_land.effect");
    // let resource: FxResource = Cast<FxResource>(r"base\\fx\\vehicles\\manticore\\v_av_manticore_medium_thrusters.effect");
    // let resource: FxResource = Cast<FxResource>(r"base\\fx\\vehicles\\av\\av_panzer\\av_panzer_thruster.effect");
    // let resource: FxResource = Cast<FxResource>(r"base\\fx\\vehicles\\av_general_thruster.effect");
    this.bl_fx = GameInstance.GetFxSystem(this.component.GetVehicle().GetGame()).SpawnEffect(this.resource, effectTransform);
    this.br_fx = GameInstance.GetFxSystem(this.component.GetVehicle().GetGame()).SpawnEffect(this.resource, effectTransform);
    this.fl_fx = GameInstance.GetFxSystem(this.component.GetVehicle().GetGame()).SpawnEffect(this.resource, effectTransform);
    this.fr_fx = GameInstance.GetFxSystem(this.component.GetVehicle().GetGame()).SpawnEffect(this.resource, effectTransform);

    this.bl_fx.SetBlackboardValue(n"thruster_amount", 0.0);
    this.br_fx.SetBlackboardValue(n"thruster_amount", 0.0);
    this.fl_fx.SetBlackboardValue(n"thruster_amount", 0.0);
    this.fr_fx.SetBlackboardValue(n"thruster_amount", 0.0);

    // vehicleSlots.GetSlotTransform(n"wheel_back_left", this.bl_fx_wt);
    // this.bl_fx.AttachToComponent(this.component.GetVehicle(), entAttachmentTarget.Transform, n"Chassis", this.bl_fx_wt);
    // vehicleSlots.GetSlotTransform(n"wheel_back_right", this.br_fx_wt);
    // this.br_fx.AttachToComponent(this.component.GetVehicle(), entAttachmentTarget.Transform, n"Chassis", this.br_fx_wt);
    // vehicleSlots.GetSlotTransform(n"wheel_front_left", this.fl_fx_wt);
    // this.fl_fx.AttachToComponent(this.component.GetVehicle(), entAttachmentTarget.Transform, n"Chassis", this.fl_fx_wt);
    // vehicleSlots.GetSlotTransform(n"wheel_front_right", this.fr_fx_wt);
    // this.fr_fx.AttachToComponent(this.component.GetVehicle(), entAttachmentTarget.Transform, n"Chassis", this.fr_fx_wt);

    // let vehicleSlots = this.component.GetVehicle().GetVehicleComponent().FindComponentByName(n"vehicle_slots") as SlotComponent;
    // vehicleSlots.GetSlotTransform(n"wheel_back_left", this.bl_fx_wt);
    // // this.bl_fx.AttachToSlot(this.component.GetVehicle(), entAttachmentTarget.Transform, n"Base", this.bl_fx_wt);
    // vehicleSlots.GetSlotTransform(n"wheel_back_right", this.br_fx_wt);
    // // this.br_fx.AttachToSlot(this.component.GetVehicle(), entAttachmentTarget.Transform, n"Base", this.br_fx_wt);
    // vehicleSlots.GetSlotTransform(n"wheel_front_left", this.fl_fx_wt);
    // // this.fl_fx.AttachToSlot(this.component.GetVehicle(), entAttachmentTarget.Transform, n"Base", this.fl_fx_wt);
    // vehicleSlots.GetSlotTransform(n"wheel_front_right", this.fr_fx_wt);
    // // this.fr_fx.AttachToSlot(this.component.GetVehicle(), entAttachmentTarget.Transform, n"Base", this.fr_fx_wt);

    let wt = new WorldTransform();
    WorldTransform.SetPosition(wt, new Vector4(0.0, 0.0, -0.6, 0.0));
    this.fl_fx.AttachToComponent(this.component.GetVehicle(), entAttachmentTarget.Transform, n"WheelAudioEmitterFL", wt);
    this.fr_fx.AttachToComponent(this.component.GetVehicle(), entAttachmentTarget.Transform, n"WheelAudioEmitterFR", wt);
    this.bl_fx.AttachToComponent(this.component.GetVehicle(), entAttachmentTarget.Transform, n"WheelAudioEmitterBL", wt);
    this.br_fx.AttachToComponent(this.component.GetVehicle(), entAttachmentTarget.Transform, n"WheelAudioEmitterBR", wt);

    
    // let vwt = Matrix.GetInverted(this.component.GetVehicle().GetLocalToWorld());
    // this.component.bl_tire.SetLocalPosition(WorldPosition.ToVector4(WorldTransform.GetWorldPosition(this.bl_fx_wt)) * vwt);
    // this.component.br_tire.SetLocalPosition(WorldPosition.ToVector4(WorldTransform.GetWorldPosition(this.br_fx_wt)) * vwt);
    // this.component.fl_tire.SetLocalPosition(WorldPosition.ToVector4(WorldTransform.GetWorldPosition(this.fl_fx_wt)) * vwt);
    // this.component.fr_tire.SetLocalPosition(WorldPosition.ToVector4(WorldTransform.GetWorldPosition(this.fr_fx_wt)) * vwt);
  }

  public func Stop() {
    this.bl_fx.BreakLoop();
    this.br_fx.BreakLoop();
    this.fl_fx.BreakLoop();
    this.fr_fx.BreakLoop();

    let vehicleComponent = this.component.GetVehicle().GetVehicleComponent();
    (vehicleComponent.FindComponentByName(n"chassis") as IComponent).Toggle(true);

    (vehicleComponent.FindComponentByName(n"tire_01_fl_a") as IComponent).Toggle(true);
    (vehicleComponent.FindComponentByName(n"tire_01_fl_b") as IComponent).Toggle(true);
    (vehicleComponent.FindComponentByName(n"tire_01_fl_a_shadow") as IComponent).Toggle(true);
    (vehicleComponent.FindComponentByName(n"wheel_01_fl_a") as IComponent).Toggle(true);
    (vehicleComponent.FindComponentByName(n"wheel_01_fl_b") as IComponent).Toggle(true);

    (vehicleComponent.FindComponentByName(n"tire_01_fr_a") as IComponent).Toggle(true);
    (vehicleComponent.FindComponentByName(n"tire_01_fr_b") as IComponent).Toggle(true);
    (vehicleComponent.FindComponentByName(n"tire_01_fr_a_shadow") as IComponent).Toggle(true);
    (vehicleComponent.FindComponentByName(n"wheel_01_fr_a") as IComponent).Toggle(true);
    (vehicleComponent.FindComponentByName(n"wheel_01_fr_b") as IComponent).Toggle(true);

    (vehicleComponent.FindComponentByName(n"tire_01_bl_a") as IComponent).Toggle(true);
    (vehicleComponent.FindComponentByName(n"tire_01_bl_b") as IComponent).Toggle(true);
    (vehicleComponent.FindComponentByName(n"tire_01_bl_a_shadow") as IComponent).Toggle(true);
    (vehicleComponent.FindComponentByName(n"wheel_01_bl_a") as IComponent).Toggle(true);
    (vehicleComponent.FindComponentByName(n"wheel_01_bl_b") as IComponent).Toggle(true);

    (vehicleComponent.FindComponentByName(n"tire_01_br_a") as IComponent).Toggle(true);
    (vehicleComponent.FindComponentByName(n"tire_01_br_b") as IComponent).Toggle(true);
    (vehicleComponent.FindComponentByName(n"tire_01_br_a_shadow") as IComponent).Toggle(true);
    (vehicleComponent.FindComponentByName(n"wheel_01_br_a") as IComponent).Toggle(true);
    (vehicleComponent.FindComponentByName(n"wheel_01_br_b") as IComponent).Toggle(true);

    (vehicleComponent.FindComponentByName(n"tire_02_fl_a") as IComponent).Toggle(true);
    (vehicleComponent.FindComponentByName(n"tire_02_fl_b") as IComponent).Toggle(true);
    (vehicleComponent.FindComponentByName(n"tire_02_fl_a_shadow") as IComponent).Toggle(true);
    (vehicleComponent.FindComponentByName(n"wheel_02_fl_a") as IComponent).Toggle(true);
    (vehicleComponent.FindComponentByName(n"wheel_02_fl_b") as IComponent).Toggle(true);

    (vehicleComponent.FindComponentByName(n"tire_02_fr_a") as IComponent).Toggle(true);
    (vehicleComponent.FindComponentByName(n"tire_02_fr_b") as IComponent).Toggle(true);
    (vehicleComponent.FindComponentByName(n"tire_02_fr_a_shadow") as IComponent).Toggle(true);
    (vehicleComponent.FindComponentByName(n"wheel_02_fr_a") as IComponent).Toggle(true);
    (vehicleComponent.FindComponentByName(n"wheel_02_fr_b") as IComponent).Toggle(true);

    (vehicleComponent.FindComponentByName(n"tire_02_bl_a") as IComponent).Toggle(true);
    (vehicleComponent.FindComponentByName(n"tire_02_bl_b") as IComponent).Toggle(true);
    (vehicleComponent.FindComponentByName(n"tire_02_bl_a_shadow") as IComponent).Toggle(true);
    (vehicleComponent.FindComponentByName(n"wheel_02_bl_a") as IComponent).Toggle(true);
    (vehicleComponent.FindComponentByName(n"wheel_02_bl_b") as IComponent).Toggle(true);

    (vehicleComponent.FindComponentByName(n"tire_02_br_a") as IComponent).Toggle(true);
    (vehicleComponent.FindComponentByName(n"tire_02_br_b") as IComponent).Toggle(true);
    (vehicleComponent.FindComponentByName(n"tire_02_br_a_shadow") as IComponent).Toggle(true);
    (vehicleComponent.FindComponentByName(n"wheel_02_br_a") as IComponent).Toggle(true);
    (vehicleComponent.FindComponentByName(n"wheel_02_br_b") as IComponent).Toggle(true);

    (vehicleComponent.FindComponentByName(n"wheel_bl_a_01") as IComponent).Toggle(true);
    (vehicleComponent.FindComponentByName(n"wheel_br_a_01") as IComponent).Toggle(true);
    (vehicleComponent.FindComponentByName(n"wheel_fl_a_01") as IComponent).Toggle(true);
    (vehicleComponent.FindComponentByName(n"wheel_fr_a_01") as IComponent).Toggle(true);

    (vehicleComponent.FindComponentByName(n"wheel_bl_b_01") as IComponent).Toggle(true);
    (vehicleComponent.FindComponentByName(n"wheel_br_b_01") as IComponent).Toggle(true);
    (vehicleComponent.FindComponentByName(n"wheel_fl_b_01") as IComponent).Toggle(true);
    (vehicleComponent.FindComponentByName(n"wheel_fr_b_01") as IComponent).Toggle(true);

    (vehicleComponent.FindComponentByName(n"wheel_bl_a_02") as IComponent).Toggle(true);
    (vehicleComponent.FindComponentByName(n"wheel_br_a_02") as IComponent).Toggle(true);
    (vehicleComponent.FindComponentByName(n"wheel_fl_a_02") as IComponent).Toggle(true);
    (vehicleComponent.FindComponentByName(n"wheel_fr_a_02") as IComponent).Toggle(true);

    (vehicleComponent.FindComponentByName(n"wheel_bl_b_02") as IComponent).Toggle(true);
    (vehicleComponent.FindComponentByName(n"wheel_br_b_02") as IComponent).Toggle(true);
    (vehicleComponent.FindComponentByName(n"wheel_fl_b_02") as IComponent).Toggle(true);
    (vehicleComponent.FindComponentByName(n"wheel_fr_b_02") as IComponent).Toggle(true);

  }

  public func Update(visualForce: Vector4, visualTorque: Vector4) {

    let thrusterAmount = Vector4.Dot(new Vector4(0.0, 0.0, 1.0, 0.0), visualForce);
    // let thrusterAmount = ClampF(this.surge.GetValue(), 0.0, 1.0) * 1.0;

    this.bl_fx.SetBlackboardValue(n"thruster_amount", (thrusterAmount + visualTorque.X + visualTorque.Y) * this.sys.settings.thrusterFactor());
    this.br_fx.SetBlackboardValue(n"thruster_amount", (thrusterAmount + visualTorque.X - visualTorque.Y) * this.sys.settings.thrusterFactor());
    this.fl_fx.SetBlackboardValue(n"thruster_amount", (thrusterAmount - visualTorque.X + visualTorque.Y) * this.sys.settings.thrusterFactor());
    this.fr_fx.SetBlackboardValue(n"thruster_amount", (thrusterAmount - visualTorque.X - visualTorque.Y) * this.sys.settings.thrusterFactor());

    // +x = torwards front
    // +y = rotate left z
    // +z = towards right 
    // let forceQuat = Quaternion.BuildFromDirectionVector(-visualForce, this.component.stats.d_localUp);
    // let quat = EulerAngles.ToQuat(new EulerAngles(
    //   Vector4.GetAngleDegAroundAxis(FlightUtils.Up(), visualForce, FlightUtils.Right()),
    //   Vector4.GetAngleDegAroundAxis(FlightUtils.Up(), visualForce, FlightUtils.Forward()),
    //   0
    // ));
    this.component.bl_tire.SetLocalOrientation(Quaternion.Slerp(this.component.bl_tire.GetLocalOrientation(), EulerAngles.ToQuat(new EulerAngles(
      ClampF(Vector4.GetAngleDegAroundAxis(FlightUtils.Up(), visualForce, FlightUtils.Right()), -45.0, 45.0),
      0.0,
      visualTorque.Z * 0.5 + ClampF(Vector4.GetAngleDegAroundAxis(FlightUtils.Up(), visualForce, FlightUtils.Forward()), -45.0, 45.0)
    )), 0.1));
    this.component.br_tire.SetLocalOrientation(Quaternion.Slerp(this.component.br_tire.GetLocalOrientation(), EulerAngles.ToQuat(new EulerAngles(
      ClampF(Vector4.GetAngleDegAroundAxis(FlightUtils.Up(), visualForce, FlightUtils.Right()), -45.0, 45.0), 
      0.0,
      visualTorque.Z * 0.5 + ClampF(Vector4.GetAngleDegAroundAxis(FlightUtils.Up(), visualForce, FlightUtils.Forward()), -45.0, 45.0)
    )), 0.1));
    this.component.fl_tire.SetLocalOrientation(Quaternion.Slerp(this.component.fl_tire.GetLocalOrientation(), EulerAngles.ToQuat(new EulerAngles(
      ClampF(Vector4.GetAngleDegAroundAxis(FlightUtils.Up(), visualForce, FlightUtils.Right()), -45.0, 45.0), 
      0.0,
      -visualTorque.Z * 0.5 + ClampF(Vector4.GetAngleDegAroundAxis(FlightUtils.Up(), visualForce, FlightUtils.Forward()), -45.0, 45.0)
    )), 0.1));
    this.component.fr_tire.SetLocalOrientation(Quaternion.Slerp(this.component.fr_tire.GetLocalOrientation(), EulerAngles.ToQuat(new EulerAngles(
      ClampF(Vector4.GetAngleDegAroundAxis(FlightUtils.Up(), visualForce, FlightUtils.Right()), -45.0, 45.0), 
      0.0,
      -visualTorque.Z * 0.5 + ClampF(Vector4.GetAngleDegAroundAxis(FlightUtils.Up(), visualForce, FlightUtils.Forward()), -45.0, 45.0)
    )), 0.1));
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
}