enum FlightThrusterType {
  FrontLeft = 0,
  FrontRight = 1,
  BackLeft = 2,
  BackRight = 3,
  FrontLeftB = 4,
  FrontRightB = 5,
  Front = 6,
  Back = 7
}

public class FlightThruster {
  public let flightComponent: ref<FlightComponent>;
  public let bone: Float = 0.0;
  public let boneLerpAmount: Float = 0.25;
  public let maxThrusterAnglePitch: Float = 90.0;
  public let maxThrusterAngleOutside: Float = 60.0;
  public let maxThrusterAngleInside: Float = 15.0;
  public let thrusterAngleAllowance: Float = 15.0;
  public let ogComponents: array<ref<IComponent>>;
  public let meshComponent: ref<MeshComponent>;
  public let mainResRef: ResRef = r"user\\jackhumbert\\effects\\ion_thruster.effect";
  public let mainFxRes: FxResource;
  public let retroResRef: ResRef = r"user\\jackhumbert\\effects\\retro_thruster.effect";
  public let retroFxRes: FxResource;
  public let mainFx: ref<FxInstance>;
  // public let mainThrusterFactor: Float = 0.05;
  public let mainThrusterYawFactor: Float = 0.5;
  public let retroFx: ref<FxInstance>;
  public let retroThrusterFactor: Float = 0.1;
  public let force: Vector4;
  public let torque: Vector4;
  public let isRight: Bool;
  public let isFront: Bool;
  public let isMotorcycle: Bool;
  public let isB: Bool;
  public let id: String;
  public let audioUpdate: ref<FlightAudioUpdate>;
  public let audioPitch: Float;
  public let audioPitchSeparation: Float = 0.001;

  public static func CreateThrusters(fc: ref<FlightComponent>) -> array<ref<FlightThruster>> {
    let thrusters: array<ref<FlightThruster>>;
    let vehicleComponent = fc.GetVehicle().GetVehicleComponent();

    if IsDefined(vehicleComponent.FindComponentByName(n"ThrusterFL") as MeshComponent) {
      ArrayPush(thrusters, new FlightThruster().Initialize(fc, FlightThrusterType.FrontLeft));
    }
    if IsDefined(vehicleComponent.FindComponentByName(n"ThrusterFR") as MeshComponent) {
      ArrayPush(thrusters, new FlightThruster().Initialize(fc, FlightThrusterType.FrontRight));
    }
    if IsDefined(vehicleComponent.FindComponentByName(n"ThrusterBL") as MeshComponent) {
      ArrayPush(thrusters, new FlightThruster().Initialize(fc, FlightThrusterType.BackLeft));
    }
    if IsDefined(vehicleComponent.FindComponentByName(n"ThrusterBR") as MeshComponent) {
      ArrayPush(thrusters, new FlightThruster().Initialize(fc, FlightThrusterType.BackRight));
    }
    if IsDefined(vehicleComponent.FindComponentByName(n"ThrusterFLB") as MeshComponent) {
      ArrayPush(thrusters, new FlightThrusterFLB().Create(fc));
    }
    if IsDefined(vehicleComponent.FindComponentByName(n"ThrusterFRB") as MeshComponent) {
      ArrayPush(thrusters, new FlightThruster().Initialize(fc, FlightThrusterType.FrontRightB));
    }
    if IsDefined(vehicleComponent.FindComponentByName(n"ThrusterF") as MeshComponent) {
      ArrayPush(thrusters, new FlightThruster().Initialize(fc, FlightThrusterType.Front));
    }
    if IsDefined(vehicleComponent.FindComponentByName(n"ThrusterB") as MeshComponent) {
      ArrayPush(thrusters, new FlightThruster().Initialize(fc, FlightThrusterType.Back));
    }

    return thrusters;
  }

  public func Initialize(fc: ref<FlightComponent>, type: FlightThrusterType) -> ref<FlightThruster> {
    this.flightComponent = fc;
    this.mainFxRes = Cast<FxResource>(this.mainResRef);
    this.retroFxRes = Cast<FxResource>(this.retroResRef);

    if Equals(type, FlightThrusterType.FrontRight) {
      this.isRight = true;
      this.isFront = true;
    }
    if Equals(type, FlightThrusterType.FrontLeft) {
      this.isRight = false;
      this.isFront = true;
    }
    if Equals(type, FlightThrusterType.FrontRightB) {
      this.isRight = true;
      this.isFront = true;
      this.isB = true;
    }
    if Equals(type, FlightThrusterType.FrontLeftB) {
      this.isRight = false;
      this.isFront = true;
      this.isB = true;
    }
    if Equals(type, FlightThrusterType.BackRight) {
      this.isRight = true;
      this.isFront = false;
    }
    if Equals(type, FlightThrusterType.BackLeft) {
      this.isRight = false;
      this.isFront = false;
    }
    if Equals(type, FlightThrusterType.Front) {
      this.isFront = true;
      this.isMotorcycle = true;
    }
    if Equals(type, FlightThrusterType.Back) {
      this.isMotorcycle = true;
    }

    let vehicleComponent = this.flightComponent.GetVehicle().GetVehicleComponent();

    this.meshComponent = vehicleComponent.FindComponentByName(this.GetComponentName()) as MeshComponent;
    this.meshComponent.visualScale = new Vector3(0.0, 0.0, 0.0);
    this.meshComponent.Toggle(false);
    this.meshComponent.SetLocalOrientation(EulerAngles.ToQuat(this.GetEulerAngles()));

    this.id = "vehicle";
    this.audioPitch = this.flightComponent.GetPitch();
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
    this.id += this.flightComponent.GetUniqueID();
    this.audioUpdate = new FlightAudioUpdate();

    return this;
  }

  public func Start() {
    let vehicle = this.flightComponent.GetVehicle();
    let effectTransform: WorldTransform;
    let wt = new WorldTransform();

    this.ogComponents = this.flightComponent.GetVehicle().GetComponentsUsingSlot(this.GetSlotName());
    this.HideOGComponents();

    WorldTransform.SetPosition(effectTransform, this.flightComponent.stats.d_position);
    this.mainFx = GameInstance.GetFxSystem(vehicle.GetGame()).SpawnEffect(this.mainFxRes, effectTransform);
    this.mainFx.SetBlackboardValue(n"thruster_amount", 0.0);
    this.mainFx.AttachToComponent(vehicle, entAttachmentTarget.Transform, this.GetComponentName(), wt);
    this.meshComponent.Toggle(true);

    let wt_retro: WorldTransform;
    WorldTransform.SetOrientation(wt_retro, EulerAngles.ToQuat(new EulerAngles(0.0, 0.0, -90.0)));
    this.retroFx =  GameInstance.GetFxSystem(vehicle.GetGame()).SpawnEffect(this.retroFxRes, effectTransform);
    // this.retroFx.AttachToSlot(this.component.GetVehicle(), entAttachmentTarget.Transform, n"Base", wt_retro);
    this.retroFx.AttachToComponent(vehicle, entAttachmentTarget.Transform, this.GetComponentName(), wt_retro);

    FlightAudio.Get().StartWithPitch(this.id, "vehicle3_TPP", this.audioPitch);

  }

  let forceThreshold: Float = 10.0;
  let torqueThreshold: Float = 1.0;

  let animDeviation: Float = 0.3;
  let animRadius: Float = 0.0;


  public func Update(force: Vector4, torque: Vector4) {
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
    this.meshComponent.visualScale = Vector4.Vector4To3(Vector4.Interpolate(Vector4.Vector3To4(this.meshComponent.visualScale), vec, 0.1));

    this.meshComponent.SetLocalOrientation(Quaternion.Slerp(this.meshComponent.GetLocalOrientation(), EulerAngles.ToQuat(this.GetEulerAngles()), 0.1));

    let amount = Vector4.Dot(Quaternion.GetUp(this.meshComponent.GetLocalOrientation()), this.force);
    amount += this.GetMainThrusterTorqueAmount();
    // let amount = Vector4.Dot(Quaternion.GetUp(this.meshComponent.GetLocalOrientation()), Quaternion.GetUp(EulerAngles.ToQuat(this.GetEulerAngles())));
    // amount *= this.mainThrusterFactor;
    amount = ClampF(amount, -1.0, 1.0);
    this.mainFx.SetBlackboardValue(n"thruster_amount", amount);

    // -4, 4 / -10, 10
    let animDeviationCenter = 0.0;
    let animDeviationScale = 0.1;
    // 0, 16
    // let animRadiusCenter = 1.0;
    // let animRadiusScale = -1.0;

    // this.bone = LerpF(this.boneLerpAmount, this.bone, -animScale + ClampF(amount, -1.0, 1.0) * animScale);
    this.animDeviation = LerpF(this.boneLerpAmount, this.animDeviation, animDeviationCenter + amount * animDeviationScale);
    // this.animDeviation = animDeviationCenter + amount * animDeviationScale;
    // this.animRadius = animRadiusCenter + amount * animRadiusScale;
    // AnimationControllerComponent.SetInputFloatToReplicate(this.flightComponent.GetVehicle(), this.GetDeviationName(), this.animDeviation);
    // AnimationControllerComponent.SetInputFloatToReplicate(this.flightComponent.GetVehicle(), this.GetRadiusName(), this.animRadius);

    // let acc = this.flightComponent.FindComponentByName(n"AnimationController") as AnimationControllerComponent;
    // if IsDefined(acc) {
    //   acc.SetInputFloat(this.GetDeviationName(), this.animDeviation);
    // }
    // AnimationControllerComponent.SetInputFloat(this.flightComponent.GetVehicle(), this.GetDeviationName(), this.animDeviation);

    // acc.SetInputFloat(this.GetRadiusName(), this.animRadius);

    let retroAmount = this.GetRetroThrusterAmount();
    this.retroFx.SetBlackboardValue(n"thruster_amount", retroAmount);
    
    this.audioUpdate = this.flightComponent.audioUpdate;
    // amount *= 0.5;
    // this.audioUpdate.surge *= amount;
    // this.audioUpdate.pitch *= amount;
    // this.audioUpdate.yaw *= retroAmount;
    // this.audioUpdate.sway *= retroAmount;
    // this.audioUpdate.lift *= amount;
    // this.audioUpdate.roll *= amount;
    let volume = 1.0;
    if !this.isFront {
      volume = ClampF(this.flightComponent.stats.d_speed / 100.0, 0.0, 1.0);
    }
    // this.audioUpdate.pitch = retroAmount;
    
    let matrix = this.meshComponent.GetLocalToWorld();
    // rotates the event cone down
    let quat = Matrix.ToQuat(matrix) * new Quaternion(-0.707, 0.0, 0.0, 0.707);
    let rotatedMatrix = Quaternion.ToMatrix(quat);
    rotatedMatrix.W = matrix.W;
    FlightAudio.Get().UpdateEvent(this.id, rotatedMatrix, volume, this.audioUpdate);
  }

  public func Stop() {
    FlightAudio.Get().Stop(this.id);
    if IsDefined(this.mainFx) {
      this.mainFx.BreakLoop();
    }
    if IsDefined(this.retroFx) {
      this.retroFx.BreakLoop();
    }
    this.ShowOGComponents();
  }

  public func GetEulerAngles() -> EulerAngles {
    return new EulerAngles(this.GetPitch(), this.GetRoll(), this.GetYaw());
  }

  public let componentSizeArray: array<Vector3>;

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

  public func GetMainThrusterTorqueAmount() -> Float {
    let amount: Float = 0;
    if this.isFront {
      amount += this.torque.X;
    } else {
      amount -= this.torque.X;
    }
    if this.isRight {
      amount -= this.torque.Y;
    } else {
      amount += this.torque.Y;
    }
    return amount;
  }

  public func GetRetroThrusterAmount() -> Float {
    let vec: Vector4;
    if this.isRight {
      vec = new Vector4(-1.0, 0.0, 0.0, 0.0);
    } else {
      vec = new Vector4(1.0, 0.0, 0.0, 0.0);
    }
    let tor: Float;
    if this.isFront ^ this.isRight { // FL, BR
      tor = -this.torque.Z;
    } else { // FR, BL
      tor = this.torque.Z;
    }
    return (Vector4.Dot(vec, this.force) + tor) * this.retroThrusterFactor;
  }

  public func GetComponentName() -> CName {
    if this.isMotorcycle {
        if this.isFront {
          return n"ThrusterF";
        } else {
          return n"ThrusterB";
        }
    } else {
      if this.isRight {
        if this.isFront {
          if this.isB {
            return n"ThrusterFRB";
          } else {
            return n"ThrusterFR";
          }
        } else {
          return n"ThrusterBR";
        }
      } else {
        if this.isFront {
          if this.isB {
            return n"ThrusterFLB";
          } else {
            return n"ThrusterFL";
          }
        } else {
          return n"ThrusterBL";
        }
      }
    }
  }

  public func GetSlotName() -> CName {
    if this.isMotorcycle {
        if this.isFront {
          return n"wheel_front_spring";
        } else {
          return n"axel_back";
        }
    } else {
      if this.isRight {
        if this.isFront {
          if this.isB {
            return n"wheel_front_right_b";
          } else {
            return n"wheel_front_right";
          }
        } else {
          return n"wheel_back_right";
        }
      } else {
        if this.isFront {
          if this.isB {
            return n"wheel_front_left_b";
          } else {
            return n"wheel_front_left";
          }
        } else {
          return n"wheel_back_left";
        }
      }
    }
  }

  public func GetRadiusName() -> CName {
    if this.isRight {
      if this.isFront {
        if this.isB {
          return n"veh_rad_w_1_r";
        } else {
          return n"veh_rad_w_f_r";
        }
      } else {
        return n"veh_rad_w_b_r";
      }
    } else {
      if this.isFront {
        if this.isB {
          return n"veh_rad_w_1_l";
        } else {
          return n"veh_rad_w_f_l";
        }
      } else {
        return n"veh_rad_w_b_l";
      }
    }
  }

  public func GetDeviationName() -> CName {
    if this.isRight {
      if this.isFront {
        if this.isB {
          return n"veh_press_w_1_r";
        } else {
          return n"veh_press_w_f_r";
        }
      } else {
        return n"veh_press_w_b_r";
      }
    } else {
      if this.isFront {
        if this.isB {
          return n"veh_press_w_1_l";
        } else {
          return n"veh_press_w_f_l";
        }
      } else {
        return n"veh_press_w_b_l";
      }
    }
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
}


public class FlightThrusterFLB extends FlightThruster {
  public func Create(fc: ref<FlightComponent>) -> ref<FlightThruster> {
    return this.Initialize(fc, FlightThrusterType.FrontLeftB);
  }

  public func GetComponentName() -> CName {
    return n"ThrusterFLB";
  }

  public func GetSlotName() -> CName {
    return n"wheel_front_left_b";
  }

  public func GetRadiusName() -> CName {
    return n"veh_rad_w_1_l";
  }

  public func GetDeviationName() -> CName {
    return n"veh_press_w_1_l";
  }
}

public class Vector3Wrapper {
  public let vector: Vector3;
  public static func Create(v: Vector3) -> ref<Vector3Wrapper> {
    let vw = new Vector3Wrapper();
    vw.vector = v;
    return vw;
  }
}