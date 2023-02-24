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
    return amount;
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
    return amount + x + y;

    this.instance.SetBlackboardValue(n"thruster_amount", ClampF(amount, -1.0, 1.0));
    return amount;
  }
}

public class SideFlightThrusterFX extends IFlightThrusterFX {
  public func Create(thruster: ref<IFlightThruster>) -> ref<IFlightThrusterFX> {
    super.Create(thruster);
    this.SetResource(r"user\\jackhumbert\\effects\\retro_thruster.effect");
    this.rotation = EulerAngles.ToQuat(new EulerAngles(0.0, 0.0, -90.0));
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