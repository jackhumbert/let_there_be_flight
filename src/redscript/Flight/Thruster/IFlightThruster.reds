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

  public native func SetupMotion() -> Bool;
  public native func UpdateNative(force: Vector4, torque: Vector4) -> Bool;
  public native func MotionState() -> String;

  public let motionKind: Int32 = 0; // Flight::ThrusterShape: 0 car, 1 bike front, 2 bike back (set at Create)

  public let parentSlotName: CName;
  public let deviationName: CName;

  public let fxs: array<ref<IFlightThrusterFX>>;
  public let boneLerpAmount: Float = 0.25;
  public let maxThrusterAnglePitch: Float = 90.0;
  public let maxThrusterAngleOutside: Float = 60.0;
  public let maxThrusterAngleInside: Float = 15.0;
  public let thrusterAngleAllowance: Float = 15.0;
  public let ogComponents: array<ref<IComponent>>;
  public let componentSizeArray: array<Vector3>;
  public let mainThrusterYawFactor: Float = 0.5;
  public let hasRetroThruster: Bool = true;
  public let force: Vector4;
  public let torque: Vector4;
  public let isRight: Bool = false;
  public let isFront: Bool = false;
  public let isB: Bool = false;
  public let id: String;
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

    if IsDefined(this.meshComponent) {
      this.meshComponent.visualScale = new Vector3(0.0, 0.0, 0.0);
      this.meshComponent.Toggle(false);
      this.initialOrientation = this.meshComponent.GetLocalOrientation();
      this.meshComponent.SetLocalOrientation(this.initialOrientation * EulerAngles.ToQuat(this.GetEulerAngles()));
    } else {
      FlightLog.Warn("[IFlightThruster] OnSetup: thruster " + NameToString(this.slotName) + " has no mesh component");
      this.initialOrientation = new Quaternion(0.0, 0.0, 0.0, 1.0);
    }

    this.id = "vehicle";
    if this.isFront {
      this.id += "F";
    } else {
      this.id += "B";
    }
    if this.isRight {
      this.id += "R";
    } else {
      this.id += "L";
    }
    if this.isB {
      this.id += "B";
    }
    this.id += FloatToString(RandRangeF(0.0, 1.0));
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

  // the mesh's starting angles at OnSetup (no force yet); each frame's are worked out in C++ (Flight::Thruster*, with
  // the per-type differences as motionKind)
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

      FlightAudio.Get().StartEngine(this.id, this.flightComponent.GetPitch(), Cast<Uint64>(EntityID.GetHash(this.vehicle.GetEntityID())), this.flightComponent.isPlayerMounted);
    }

    this.SetupMotion();
  }

  let forceThreshold: Float = 10.0;
  let torqueThreshold: Float = 1.0;

  let animDeviation: Float = 0.3;

  public func Detach() {
    this.attached = false;
    
    FlightAudio.Get().StopEngine(this.id);
    
    for fx in this.fxs {
      fx.Stop();
    }
  }

  public func Update(force: Vector4, torque: Vector4) {
    if !this.attached {
      return;
    }

    // in C++ (UpdateNative); without a game function it needs it logs once and the thruster is not animated
    this.UpdateNative(force, torque);
  }

  public func Stop() {
    if this.attached {
      FlightAudio.Get().StopEngine(this.id);
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