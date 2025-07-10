public abstract native class IFlightThruster extends IScriptable {

  @runtimeProperty("offset", "0x40")
  public native let flightComponent: wref<FlightComponent>;

  @runtimeProperty("offset", "0x58")
  public native let slotName: CName;

  @runtimeProperty("offset", "0x90")
  public native let meshComponent: ref<MeshComponent>;

  @runtimeProperty("offset", "0xA0")
  public native let vehicle: wref<VehicleObject>;

  @runtimeProperty("offset", "0xB0")
  public native let attached: Bool;

  public let parentSlotName: CName;
  public let radiusName: CName;
  public let deviationName: CName;

  public let fxs: array<ref<IFlightThrusterFX>>;

  // public let mainResRef: ResRef = r"user\\jackhumbert\\effects\\ion_thruster.effect";
  // public let mainFxRes: FxResource;
  // public let retroResRef: ResRef = r"user\\jackhumbert\\effects\\retro_thruster.effect";
  // public let retroFxRes: FxResource;
  
  public let bone: Float = 0.0;
  public let boneLerpAmount: Float = 0.25;
  public let maxThrusterAnglePitch: Float = 90.0;
  public let maxThrusterAngleOutside: Float = 60.0;
  public let maxThrusterAngleInside: Float = 15.0;
  public let thrusterAngleAllowance: Float = 15.0;
  public let ogComponents: array<ref<IComponent>>;
  public let componentSizeArray: array<Vector3>;
  // public let mainFx: ref<FxInstance>;
  // public let mainThrusterFactor: Float = 0.05;
  public let mainThrusterYawFactor: Float = 0.5;
  public let hasRetroThruster: Bool = true;
  // public let retroFx: ref<FxInstance>;
  // public let retroThrusterFactor: Float = 0.1;
  public let force: Vector4;
  public let torque: Vector4;
  public let isRight: Bool = false;
  public let isFront: Bool = false;
  public let isMotorcycle: Bool = false;
  public let isB: Bool = false;
  public let id: String;
  public let audioUpdate: ref<FlightAudioUpdate>;
  public let audioPitch: Float;
  public let audioPitchSeparation: Float = 0.001;
  public let wheelIndex: Int32;
  public let initialOrientation: Quaternion;

  public func OnSetup(fc : ref<FlightComponent>) {
    this.flightComponent = fc;
    this.attached = true;
    
    if !this.hasRetroThruster {
      this.mainThrusterYawFactor = 30.0;
    } else {
      this.mainThrusterYawFactor = 5.0;
    }

    this.meshComponent.visualScale = new Vector3(0.0, 0.0, 0.0);
    this.meshComponent.Toggle(false);
    // this.initialOrientation = this.meshComponent.GetLocalOrientation();
    this.meshComponent.SetLocalOrientation(this.initialOrientation * EulerAngles.ToQuat(this.GetEulerAngles()));
    this.meshComponent.SetLocalOrientation(EulerAngles.ToQuat(this.GetEulerAngles()));

    this.id = "vehicle";
    // doesn't seem to have the data to get this here
    // this.audioPitch = this.flightComponent.GetPitch();
    if this.isFront {
      this.id += "F";
      // this.audioPitch *= 1.02;
    } else {
      this.id += "B";
      // this.audioPitch *= 0.5;
      // this.audioPitch *= 2.0;
    }
    if this.isRight {
      this.id += "R";
      // this.audioPitch *= (1.0 + this.audioPitchSeparation);
    } else {
      this.id += "L";
      // this.audioPitch /= (1.0 + this.audioPitchSeparation);
    }
    if this.isB {
      this.id += "B";
      // this.audioPitch *= 0.5;
    }
    // not ready yet
    // this.id += this.flightComponent.GetUniqueID();
    this.id += FloatToString(RandRangeF(0.0, 1.0));
    this.audioUpdate = new FlightAudioUpdate();
  
  }

  public func GetLocalToWorld() -> Matrix {
    if IsDefined(this.meshComponent) {
      return this.meshComponent.GetLocalToWorld();
    } else {
      return Matrix.Identity();
    }
  }

  public func GetLocalOrientation() -> Quaternion {
    if IsDefined(this.meshComponent) {
      return this.meshComponent.GetLocalOrientation();
    } else {
      return new Quaternion(0.0, 0.0, 0.0, 1.0);
    }
  }

  public func GetEulerAngles() -> EulerAngles {
    return MakeEulerAngles(this.GetPitch(), this.GetRoll(), this.GetYaw());
  }

  public func GetPitch() -> Float {
    let angle = Vector4.GetAngleDegAroundAxis(FlightUtils.Up(), this.force, FlightUtils.Right());
    if angle < (-this.maxThrusterAnglePitch - this.thrusterAngleAllowance) || angle > (this.maxThrusterAnglePitch + this.thrusterAngleAllowance) {
      angle = 0.0;
    }
    let dir: Float;
    if this.isRight {
      dir = 1.0;
    } else {
      dir = -1.0;
    }
    angle *= (1.0 - AbsF(this.torque.Y) * 0.5);
    return ClampF(angle, -this.maxThrusterAnglePitch, this.maxThrusterAnglePitch) * dir;
  }

  public func GetRoll() -> Float {
    if this.isRight {
      return 0.0;
    } else {
      return 180.0;
    }
  }

  public func GetYaw() -> Float {
    if this.flightComponent.active {
      let dir: Float;
      let outside: Float;
      let inside: Float;
      if this.isRight {
        dir = 1.0;
        outside = this.maxThrusterAngleInside;
        inside = this.maxThrusterAngleOutside;
      } else {
        dir = -1.0;
        outside = this.maxThrusterAngleOutside;
        inside = this.maxThrusterAngleInside;
      }
      let angle = Vector4.GetAngleDegAroundAxis(FlightUtils.Up(), this.force, FlightUtils.Forward());
      if angle < (-inside - this.thrusterAngleAllowance) || angle > (outside + this.thrusterAngleAllowance) {
        angle = 0.0;
      }
      let tor: Float;
      if this.isFront ^ this.isRight { // FL, BR
        tor = this.torque.Z;
      } else { // FR, BL
        tor = -this.torque.Z;
      }
      return tor * this.mainThrusterYawFactor + ClampF(angle, -inside, outside) * dir;
    } else {
      return 180.0;
    }
  }

  public func SetOGComponents() {
    this.ogComponents = this.vehicle.GetComponentsUsingSlot(this.parentSlotName);
  }

  public func Start() {

    this.SetOGComponents();
    this.HideOGComponents();

    if this.attached {
      if IsDefined(this.meshComponent) {
        for fx in this.fxs {
          fx.Start();
        }
        this.meshComponent.Toggle(true);
      }

    // FlightAudio.Get().StartWithPitch(this.id, "vehicle3_TPP", this.audioPitch);
      FlightAudio.Get().StartWithPitch(this.id, "vehicle3_TPP", this.flightComponent.GetPitch());
    }
  }

  let forceThreshold: Float = 10.0;
  let torqueThreshold: Float = 1.0;

  let animDeviation: Float = 0.3;
  let animRadius: Float = 0.0;

  public func Detach() {
    this.attached = false;
    
    FlightAudio.Get().Stop(this.id);
    
    for fx in this.fxs {
      fx.Stop();
    }
  }

  public func Update(force: Vector4, torque: Vector4) {
    if !this.attached {
      return;
    }

    if Vector4.Length(force) > this.forceThreshold {
      this.force = Vector4.Normalize(force);
    } else {
      this.force = force / this.forceThreshold;
    }
    if Vector4.Length(torque) > this.torqueThreshold {
      this.torque = Vector4.Normalize(torque);
    } else {
      this.torque = torque / this.torqueThreshold;
    }
    
    let vec = new Vector4(1.0, 1.0, 1.0, 1.0);
    if !this.flightComponent.active {
      vec = Vector4.EmptyVector();
    }
    
    let rotatedMatrix: Matrix;
    let volume = 1.0;
    this.audioUpdate = this.flightComponent.audioUpdate;
    this.audioUpdate.scrape = ClampF((this.vehicle as WheeledObject).GetDampedSpringForce(this.wheelIndex) / this.vehicle.GetTotalMass(), 0.0, 1.0);

    if IsDefined(this.meshComponent) {

      let vec_og = Vector4.Vector3To4(this.meshComponent.visualScale);

      this.meshComponent.visualScale = Vector4.Vector4To3(Vector4.Interpolate(vec_og, vec, 0.1));
      this.meshComponent.SetLocalOrientation(Quaternion.Slerp(this.meshComponent.GetLocalOrientation(), this.initialOrientation * EulerAngles.ToQuat(this.GetEulerAngles()), 0.1));
      // this.meshComponent.SetLocalOrientation(Quaternion.Slerp(this.meshComponent.GetLocalOrientation(), EulerAngles.ToQuat(this.GetEulerAngles()), 0.1));

      // scale FX to how much "work" the thruster is actually doing
      let amount = Vector4.Dot(Quaternion.GetUp(this.meshComponent.GetLocalOrientation()), this.force);

      for fx in this.fxs {
        amount += fx.UpdateGetDisplacement();
      }

      // -4, 4 / -10, 10
      let animDeviationCenter = 0.0;

      // how much thrusters move opposite of the force effect
      let animDeviationScale = 0.025;

      // 0, 16
      // let animRadiusCenter = 1.0;
      // let animRadiusScale = -1.0;

      // this.bone = LerpF(this.boneLerpAmount, this.bone, -animScale + ClampF(amount, -1.0, 1.0) * animScale);
      this.animDeviation = LerpF(this.boneLerpAmount, this.animDeviation, animDeviationCenter + amount * animDeviationScale);
      // this.animDeviation = animDeviationCenter + amount * animDeviationScale;
      // this.animRadius = animRadiusCenter + amount * animRadiusScale;
      // AnimationControllerComponent.SetInputFloatToReplicate(this.vehicle, this.deviationName, this.animDeviation);
      // AnimationControllerComponent.SetInputFloatToReplicate(this.vehicle, this.GetRadiusName(), this.animRadius);

      let acc = this.flightComponent.FindComponentByName(n"AnimationController") as AnimationControllerComponent;
      if IsDefined(acc) {
        acc.SetInputFloat(this.deviationName, this.animDeviation);
      }
      // AnimationControllerComponent.SetInputFloat(this.vehicle, this.deviationName, this.animDeviation);

      // acc.SetInputFloat(this.GetRadiusName(), this.animRadius);
      
      // amount *= 0.5;
      // this.audioUpdate.surge *= amount;
      // this.audioUpdate.pitch *= amount;
      // this.audioUpdate.yaw *= retroAmount;
      // this.audioUpdate.sway *= retroAmount;
      // this.audioUpdate.lift *= amount;
      // this.audioUpdate.roll *= amount;

      if !this.isFront {
        volume = ClampF(this.flightComponent.stats.d_speed / 100.0, 0.0, 1.0);
      }
      // this.audioUpdate.pitch = retroAmount;
      
      let matrix = this.meshComponent.GetLocalToWorld();
      // rotates the event cone down
      let quat = Matrix.ToQuat(matrix) * new Quaternion(-0.707, 0.0, 0.0, 0.707);
      rotatedMatrix = Quaternion.ToMatrix(quat);
      rotatedMatrix.W = matrix.W;
    }

    FlightAudio.Get().UpdateEvent(this.id, rotatedMatrix, volume, this.audioUpdate);
  }

  public func Stop() {
    if this.attached {
      FlightAudio.Get().Stop(this.id);
      for fx in this.fxs {
        fx.Stop();
      }
    }
    this.ShowOGComponents();
  }

  public func HideOGComponents() {
    for c in this.ogComponents {
      let mc = c as MeshComponent;
      if IsDefined(mc) {
        ArrayPush(this.componentSizeArray, mc.visualScale);
        mc.visualScale = new Vector3(0.0, 0.0, 0.0);
      }
    }
  }

  public func ShowOGComponents() {
    let i = 0;
    for c in this.ogComponents {
      let mc = c as MeshComponent;
      if IsDefined(mc) {
        mc.visualScale = this.componentSizeArray[i];
        i += 1;
      }
    }
    ArrayClear(this.componentSizeArray);
  }
}