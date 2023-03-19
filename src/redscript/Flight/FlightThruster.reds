public abstract native class IFlightThruster extends IScriptable {

  @runtimeProperty("offset", "0x40")
  public native let flightComponent: ref<FlightComponent>;

  @runtimeProperty("offset", "0x90")
  public native let meshComponent: ref<MeshComponent>;

  @runtimeProperty("offset", "0xA0")
  public native let vehicle: ref<VehicleObject>;

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

  public func OnSetup(fc : ref<FlightComponent>) {
    this.flightComponent = fc;
    
    if !this.hasRetroThruster {
      this.mainThrusterYawFactor = 30.0;
    } else {
      this.mainThrusterYawFactor = 5.0;
    }

    this.meshComponent.visualScale = new Vector3(0.0, 0.0, 0.0);
    this.meshComponent.Toggle(false);
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

  public func GetEulerAngles() -> EulerAngles {
    return new EulerAngles(this.GetPitch(), this.GetRoll(), this.GetYaw());
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

    for fx in this.fxs {
      fx.Start();
    }

    this.meshComponent.Toggle(true);

    // FlightAudio.Get().StartWithPitch(this.id, "vehicle3_TPP", this.audioPitch);
    FlightAudio.Get().StartWithPitch(this.id, "vehicle3_TPP", this.flightComponent.GetPitch());

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

    for fx in this.fxs {
      amount += fx.UpdateGetDisplacement();
    }

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
    // AnimationControllerComponent.SetInputFloatToReplicate(this.vehicle, this.deviationName, this.animDeviation);
    // AnimationControllerComponent.SetInputFloatToReplicate(this.vehicle, this.GetRadiusName(), this.animRadius);

    let acc = this.flightComponent.FindComponentByName(n"AnimationController") as AnimationControllerComponent;
    if IsDefined(acc) {
      acc.SetInputFloat(this.deviationName, this.animDeviation);
    }
    // AnimationControllerComponent.SetInputFloat(this.vehicle, this.deviationName, this.animDeviation);

    // acc.SetInputFloat(this.GetRadiusName(), this.animRadius);
    
    this.audioUpdate = this.flightComponent.audioUpdate;
    // amount *= 0.5;
    // this.audioUpdate.surge *= amount;
    // this.audioUpdate.pitch *= amount;
    // this.audioUpdate.yaw *= retroAmount;
    // this.audioUpdate.sway *= retroAmount;
    // this.audioUpdate.lift *= amount;
    // this.audioUpdate.roll *= amount;
    this.audioUpdate.scrape = ClampF((this.vehicle as WheeledObject).GetDampedSpringForce(this.wheelIndex) / this.vehicle.GetTotalMass(), 0.0, 1.0);
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
    for fx in this.fxs {
      fx.Stop();
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

// FRONT

public class FlightThrusterFront extends IFlightThruster {
  public func Create(vehicle: ref<VehicleObject>, meshComponent: ref<MeshComponent>) -> ref<IFlightThruster> {
    this.vehicle = vehicle;
    this.vehicle.AddSlot(n"suspension_front_offset", n"thruster_front", new Vector3(0.0, 0.0, -0.5), new Quaternion(0.22627002, 0.0, 0.0, -0.974064708));

    this.meshComponent = meshComponent;
    this.meshComponent.name = n"ThrusterF"; 
    this.meshComponent.SetParentTransform(n"vehicle_slots", n"thruster_front");

    this.isFront = true;
    this.parentSlotName = n"wheel_front_spring";
    this.radiusName = n"None";
    this.deviationName = n"None";
    return this;
  }
  
  public func SetOGComponents() {
    let comp: ref<IComponent>;
    ArrayClear(this.ogComponents);

    comp = this.flightComponent.FindComponentByName(n"axel_f_01");
    if IsDefined(comp) {
      ArrayPush(this.ogComponents, comp);
    }
    let comps = this.vehicle.GetComponentsUsingSlot(n"wheel_front_rot_set");
    for c in comps {
      ArrayPush(this.ogComponents, c);
    }
    comp = this.flightComponent.FindComponentByName(n"wheel_f_01");
    if IsDefined(comp) {
      ArrayPush(this.ogComponents, comp);
    }
    comp = this.flightComponent.FindComponentByName(n"tire_f_01");
    if IsDefined(comp) {
      ArrayPush(this.ogComponents, comp);
    }
  }

  public func GetPitch() -> Float {
    let angle = Vector4.GetAngleDegAroundAxis(FlightUtils.Up(), this.force, FlightUtils.Right());
    if angle < (-this.maxThrusterAnglePitch - this.thrusterAngleAllowance) || angle > (this.maxThrusterAnglePitch + this.thrusterAngleAllowance) {
      angle = 0.0;
    }
    angle *= (1.0 - AbsF(this.torque.Y) * 0.5);
    return -ClampF(angle, -this.maxThrusterAnglePitch, this.maxThrusterAnglePitch);
  }

  public func GetYaw() -> Float {
    if this.flightComponent.active {
      let outside: Float;
      let inside: Float;
      outside = this.maxThrusterAngleOutside;
      inside = this.maxThrusterAngleOutside;
      let angle = Vector4.GetAngleDegAroundAxis(FlightUtils.Up(), this.force, FlightUtils.Forward());
      if angle < (-inside - this.thrusterAngleAllowance) || angle > (outside + this.thrusterAngleAllowance) {
        angle = 0.0;
      }
      return this.torque.Z * 30.0 + ClampF(angle, -inside, outside);
    } else {
      return 180.0;
    }
  }
}

// BACK

public class FlightThrusterBack extends IFlightThruster {
  public func Create(vehicle: ref<VehicleObject>, meshComponent: ref<MeshComponent>) -> ref<IFlightThruster> {
    this.vehicle = vehicle;
    this.vehicle.AddSlot(n"suspension_back", n"thruster_back", new Vector3(0.0, 0.0, -0.5), new Quaternion(0.0, 0.0, 0.0, 1.0));

    this.meshComponent = meshComponent;
    this.meshComponent.name = n"ThrusterB"; 
    this.meshComponent.SetParentTransform(n"vehicle_slots", n"thruster_back");

    this.parentSlotName = n"axel_back";
    this.radiusName = n"None";
    this.deviationName = n"None";
    return this;
  }
  
  public func SetOGComponents() {
    let comp: ref<IComponent>;
    ArrayClear(this.ogComponents);

    comp = this.flightComponent.FindComponentByName(n"wheel_b_01");
    if IsDefined(comp) {
      ArrayPush(this.ogComponents, comp);
    }
    comp = this.flightComponent.FindComponentByName(n"tire_b_01");
    if IsDefined(comp) {
      ArrayPush(this.ogComponents, comp);
    }
    let comps = this.vehicle.GetComponentsUsingSlot(n"axel_back_wheel");
    for c in comps {
      ArrayPush(this.ogComponents, c);
    }
  }

  public func GetPitch() -> Float {
    let angle = Vector4.GetAngleDegAroundAxis(FlightUtils.Up(), this.force, FlightUtils.Right());
    if angle < (-this.maxThrusterAnglePitch - this.thrusterAngleAllowance) || angle > (this.maxThrusterAnglePitch + this.thrusterAngleAllowance) {
      angle = 0.0;
    }
    angle *= (1.0 - AbsF(this.torque.Y) * 0.5);
    return -ClampF(angle, -this.maxThrusterAnglePitch, this.maxThrusterAnglePitch);
  }

  public func GetYaw() -> Float {
    if this.flightComponent.active {
      let outside: Float;
      let inside: Float;
      outside = this.maxThrusterAngleOutside;
      inside = this.maxThrusterAngleOutside;
      let angle = Vector4.GetAngleDegAroundAxis(FlightUtils.Up(), this.force, FlightUtils.Forward());
      if angle < (-inside - this.thrusterAngleAllowance) || angle > (outside + this.thrusterAngleAllowance) {
        angle = 0.0;
      }
      return this.torque.Z * -30.0 - ClampF(angle, -inside, outside);
    } else {
      return 180.0;
    }
  }
}

// FRONT LEFT

public class FlightThrusterFL extends IFlightThruster {
  public func Create(vehicle: ref<VehicleObject>, meshComponent: ref<MeshComponent>) -> ref<IFlightThruster> {
    this.vehicle = vehicle;
    this.vehicle.AddSlot(n"swingarm_front_left", n"thruster_front_left", new Vector3(0.0, 0.0, 0.0), new Quaternion(0.0, 0.0, 0.0, 1.0));

    this.meshComponent = meshComponent;
    this.meshComponent.name = n"ThrusterFL"; 
    this.meshComponent.SetParentTransform(n"vehicle_slots", n"thruster_front_left");

    this.isFront = true;
    this.parentSlotName = n"wheel_front_left";
    this.radiusName = n"veh_rad_w_f_l";
    this.deviationName = n"veh_press_w_f_l";
    return this;
  }
}

public class FlightThrusterFR extends IFlightThruster {
  public func Create(vehicle: ref<VehicleObject>, meshComponent: ref<MeshComponent>) -> ref<IFlightThruster> {
    this.vehicle = vehicle;
    this.vehicle.AddSlot(n"swingarm_front_right", n"thruster_front_right", new Vector3(0.0, 0.0, 0.0), new Quaternion(0.0, 0.0, 0.0, 1.0));

    this.meshComponent = meshComponent;
    this.meshComponent.name = n"ThrusterFR"; 
    this.meshComponent.SetParentTransform(n"vehicle_slots", n"thruster_front_right");

    this.isFront = true;
    this.isRight = true;
    this.parentSlotName = n"wheel_front_right";
    this.radiusName = n"veh_rad_w_f_r";
    this.deviationName = n"veh_press_w_f_r";
    return this;
  }
}

// BACK RIGHT

public class FlightThrusterBR extends IFlightThruster {
  public func Create(vehicle: ref<VehicleObject>, meshComponent: ref<MeshComponent>) -> ref<IFlightThruster> {
    this.vehicle = vehicle;
    this.vehicle.AddSlot(n"swingarm_back_right", n"thruster_back_right", new Vector3(0.0, 0.0, 0.0), new Quaternion(0.0, 0.0, 0.0, 1.0));

    this.meshComponent = meshComponent;
    this.meshComponent.name = n"ThrusterBR"; 
    this.meshComponent.SetParentTransform(n"vehicle_slots", n"thruster_back_right");

    this.isRight = true;
    this.parentSlotName = n"wheel_back_right";
    this.radiusName = n"veh_rad_w_b_r";
    this.deviationName = n"veh_press_w_b_r";
    return this;
  }
}

// BACK LEFT

public class FlightThrusterBL extends IFlightThruster {
  public func Create(vehicle: ref<VehicleObject>, meshComponent: ref<MeshComponent>) -> ref<IFlightThruster> {
    this.vehicle = vehicle;
    this.vehicle.AddSlot(n"swingarm_back_left", n"thruster_back_left", new Vector3(0.0, 0.0, 0.0), new Quaternion(0.0, 0.0, 0.0, 1.0));

    this.meshComponent = meshComponent;
    this.meshComponent.name = n"ThrusterBL"; 
    this.meshComponent.SetParentTransform(n"vehicle_slots", n"thruster_back_left");

    this.parentSlotName = n"wheel_back_left";
    this.radiusName = n"veh_rad_w_b_l";
    this.deviationName = n"veh_press_w_b_l";
    return this;
  }
}

// FRONT LEFT B

public class FlightThrusterFLB extends IFlightThruster {
  public func Create(vehicle: ref<VehicleObject>, meshComponent: ref<MeshComponent>) -> ref<IFlightThruster> {
    this.vehicle = vehicle;
    this.vehicle.AddSlot(n"swingarm_front_left_b", n"thruster_front_left_b", new Vector3(0.0, 0.0, 0.0), new Quaternion(0.0, 0.0, 0.0, 1.0));

    this.meshComponent = meshComponent;
    this.meshComponent.name = n"ThrusterFLB"; 
    this.meshComponent.SetParentTransform(n"vehicle_slots", n"thruster_front_left_b");

    this.isFront = true;
    this.isB = true;
    this.parentSlotName = n"wheel_front_left_b";
    this.radiusName = n"veh_rad_w_1_l";
    this.deviationName = n"veh_press_w_1_l";
    return this;
  }
}

// FRONT RIGHT B

public class FlightThrusterFRB extends IFlightThruster {
  public func Create(vehicle: ref<VehicleObject>, meshComponent: ref<MeshComponent>) -> ref<IFlightThruster> {
    this.vehicle = vehicle;
    this.vehicle.AddSlot(n"swingarm_front_right_b", n"thruster_front_right_b", new Vector3(0.0, 0.0, 0.0), new Quaternion(0.0, 0.0, 0.0, 1.0));

    this.meshComponent = meshComponent;
    this.meshComponent.name = n"ThrusterFRB"; 
    this.meshComponent.SetParentTransform(n"vehicle_slots", n"thruster_front_right_b");

    this.isFront = true;
    this.isRight = true;
    this.isB = true;
    this.parentSlotName = n"wheel_front_right_b";
    this.radiusName = n"veh_rad_w_1_r";
    this.deviationName = n"veh_press_w_1_r";
    return this;
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