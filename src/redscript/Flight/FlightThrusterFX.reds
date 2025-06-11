// relevant in-game effects
// r"base\\fx\\quest\\q203\\v_av_rayfield_excalibur_thruster_ground.effect"// blue star on ground
// r"base\\fx\\vehicles\\av\\v_av_trauma_team_thruster.effect" // blinking blue streaks with heat
// r"base\\fx\\vehicles\\manticore\\v_av_manticore_medium_thrusters.effect"// blue speckles with little smoke
// r"base\\fx\\vehicles\\av\\av_rayfield_excalibur_thruster.effect"// blinking blue streaks
// r"base\\fx\\vehicles\\av\\av_luxury_thruster.effect"// large heat with little black smoke
// r"base\\fx\\vehicles\\av\\av_panzer\\av_panzer_thruster_quest.effect"// blue dome
// r"base\\fx\\vehicles\\av\\v_av_trauma_team_thruster_start.effect" // short blue streaks
// r"base\\fx\\quest\\q000\\q000_cargo_av_thruster.effect"// giant red circle with center beams and smoke
// r"base\\fx\\quest\\q104\\q104_av_thrusters.effect"// long blue streaks
// r"base\\fx\\vehicles\\manticore\\v_av_manticore_thrusters_idle_land.effect"// blue streaks with heat
// r"base\\fx\\characters\\common\\retro_thrusters\\ch_retro_thrusters.effect" // grey puff of smoke
// r"base\\fx\\vehicles\\manticore\\v_av_manticore_thrusters.effect"// rectangle shape with blue streaks
// r"base\\fx\\quest\\q112\\q112_av_parade_barge_thruster.effect" // large, bright orange block
// r"base\\fx\\devices\\boot_thrusters\\d_boot_thruster.effect"// too tiny
// r"base\\fx\\vehicles\\av\\v_av_valgus_thruster.effect"// medium orange circle
// r"base\\fx\\devices\\boot_thrusters\\d_boot_thruster_burst_rogue_weyland_hack.effect"// too tiny
// r"base\\fx\\vehicles\\av\\av_panzer\\av_panzer_thruster.effect"// orange & blue, long with tan dust
// r"base\\fx\\devices\\boot_thrusters\\d_boot_thruster_burst.effect"// too tiny
// r"base\\fx\\devices\\boot_thrusters\\d_boot_thruster_holo_jump.effect"// too tiny
// r"base\\fx\\vehicles\\av\\av_rayfield_excalibur_thruster_low_power.effect"// tiny blue with heat

public native struct MaterialResource {
  public native let skidMarks: ResRef;
  public native let tireTracks: ResRef;
  public native let loaded: Bool;
}

public native struct MaterialCondition {
  public native let particle: MaterialResource;
  public native let decal: MaterialResource;
}

public native struct MaterialFx {
  public native let normal: MaterialCondition;
  public native let wet: MaterialCondition;
  public native let rain: MaterialCondition;
}

public abstract class IFlightThrusterFX extends IScriptable {
  public let resource: FxResource;
  public let instance: ref<FxInstance>;
  public let thruster: ref<IFlightThruster>;
  public let position: Vector3;
  public let rotation: Quaternion;

  public func SetResource(resource: ResRef) {
    this.resource = Cast<FxResource>(resource);
  }

  public func Create(thruster: ref<IFlightThruster>) -> ref<IFlightThrusterFX> {
    this.thruster = thruster;
    return this;
  }

  public func Start() {    
    let effectTransform: WorldTransform;
    let wt = new WorldTransform();

    WorldTransform.SetPosition(effectTransform, this.thruster.flightComponent.stats.d_position);
    WorldTransform.SetPosition(wt, Vector4.Vector3To4(this.position));
    WorldTransform.SetOrientation(wt, this.rotation);
    this.instance = GameInstance.GetFxSystem(this.thruster.vehicle.GetGame()).SpawnEffect(this.resource, effectTransform);
    this.instance.SetBlackboardValue(n"thruster_amount", 0.0);
    this.instance.AttachToComponent(this.thruster.vehicle, entAttachmentTarget.Transform, this.thruster.meshComponent.name, wt);
  }

  public func Stop() {
    if IsDefined(this.instance) {
      this.instance.BreakLoop();
    }
  }

  // returns how much the bone should be affected
  public func UpdateGetDisplacement() -> Float{
    this.instance.SetBlackboardValue(n"thruster_amount", ClampF(0.0, -1.0, 1.0));
    return 0.0;
  }
}

public class RegularFlightThrusterFX extends IFlightThrusterFX {
  public func Create(thruster: ref<IFlightThruster>) -> ref<IFlightThrusterFX> {
    super.Create(thruster);
    // needs work still
    // this.SetResource(r"user\\jackhumbert\\effects\\flame.effect");
    this.SetResource(r"user\\jackhumbert\\effects\\ion_thruster.effect");
    return this;
  }

  public func UpdateGetDisplacement() -> Float {    
    let amount = Vector4.Dot(Quaternion.GetUp(this.thruster.meshComponent.GetLocalOrientation()), this.thruster.force);
    let x = this.thruster.torque.X;
    if !this.thruster.isFront {
      x *= -1.0;
    }
    amount += x + this.thruster.torque.Y;

    this.instance.SetBlackboardValue(n"thruster_amount", ClampF(amount + this.thruster.torque.Z, -1.0, 1.0));
    return ClampF(amount, -1.0, 1.0);
  }
}

public class MainFlightThrusterFX extends IFlightThrusterFX {
  public func Create(thruster: ref<IFlightThruster>) -> ref<IFlightThrusterFX> {
    super.Create(thruster);
    this.SetResource(r"user\\jackhumbert\\effects\\ion_thruster.effect");
    return this;
  }

  public func UpdateGetDisplacement() -> Float {
    let amount = Vector4.Dot(Quaternion.GetUp(this.thruster.meshComponent.GetLocalOrientation()), this.thruster.force);
    let x = this.thruster.torque.X;
    let y = this.thruster.torque.Y;
    if !this.thruster.isFront {
      x *= -1.0;
    }
    if this.thruster.isRight {
      y *= -1.0;
    }

    this.instance.SetBlackboardValue(n"thruster_amount", ClampF(amount + x + y, -1.0, 1.0));
    return ClampF(amount, -1.0, 1.0);
  }
}

public class SideFlightThrusterFX extends IFlightThrusterFX {
  public func Create(thruster: ref<IFlightThruster>) -> ref<IFlightThrusterFX> {
    super.Create(thruster);
    this.SetResource(r"user\\jackhumbert\\effects\\retro_thruster.effect");
    this.rotation = EulerAngles.ToQuat(MakeEulerAngles(0.0, 0.0, -90.0));
    return this;
  }

  public func UpdateGetDisplacement() -> Float {
    let tor = this.thruster.torque.Z;
    let vec = new Vector4(1.0, 0.0, 0.0, 0.0);
    if this.thruster.isRight {
      tor *= -1.0;
      vec *= -1.0;
    }
    if !this.thruster.isFront {
      vec *= -1.0;
    }
    let amount = (Vector4.Dot(vec, this.thruster.force) + tor) * 0.1;
    this.instance.SetBlackboardValue(n"thruster_amount", ClampF(amount, -1.0, 1.0));
    return 0.0;
  }
}