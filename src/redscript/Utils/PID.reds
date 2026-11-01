public native class PID extends IScriptable {
  private native let valueFloat: Float;
  private native let inputFloat: Float;
  private native let P: Float;
  private native let I: Float;
  private native let D: Float;
  public native let integralFloat: Float;
  private native let lastErrorFloat: Float;
  public native func NativeCorrection(error: Float, timeDelta: Float) -> Float;
  public native func NativeReset(input: Float) -> Void;

  public static func Create(P: Float, I: Float, D: Float) -> ref<PID> {
    let instance: ref<PID> = new PID();
    instance.P = P;
    instance.I = I;
    instance.D = D;
    instance.Reset();
    return instance;
  }
  public static func Create(v: Vector3) -> ref<PID> {
    return PID.Create(v.X, v.Y, v.Z);
  }
  public static func Create(P: Float, I: Float, D: Float, initialValue: Float) -> ref<PID> {
    let instance: ref<PID> = PID.Create(P, I, D);
    instance.valueFloat = initialValue;
    return instance;
  }
  public func Update(P: Float, I: Float, D: Float) -> Void {
    this.P = P;
    this.I = I;
    this.D = D;
  }
  
  public func UpdateP(P: Float) -> Void {
    this.P = P;
  }
  
  public func UpdateI(I: Float) -> Void {
    this.I = I;
  }

  public func UpdateD(D: Float) -> Void {
    this.D = D;
  }

  public func SetInput(input: Float) {
    this.inputFloat = input;
  }
  public func GetValue(timeDelta: Float) -> Float {
    let error: Float = this.inputFloat - this.valueFloat;
    this.valueFloat += this.GetCorrection(error, timeDelta);
    return this.valueFloat;
  } 
  public func GetValue() -> Float {
    return this.valueFloat;
  } 
  public func GetInput() -> Float {
    return this.inputFloat;
  }
  public func GetValue(input: Float, timeDelta: Float) -> Float {
    this.SetInput(input);
    return this.GetValue(timeDelta);
  }
  public func GetCorrection(error: Float, timeDelta: Float) -> Float { 
    return this.NativeCorrection(error, timeDelta);
  }
  public func GetCorrectionClamped(error: Float, timeDelta: Float, clamp: Float) -> Float {
    return ClampF(this.GetCorrection(error, timeDelta), -clamp, clamp);
  }
  public func Reset(opt input: Float) -> Void {
    this.NativeReset(input);
  }
}

public native class InputPID extends PID {
  private native let P_dec: Float;
  public native func NativeInputCorrection(error: Float) -> Float;

  public static func Create(P: Float, P_dec: Float) -> ref<InputPID> {
    let instance: ref<InputPID> = new InputPID();
    instance.P = P;
    instance.P_dec = P_dec;
    instance.Reset();
    return instance;
  }

  public func UpdatePd(Pd: Float) -> Void {
    this.P_dec = Pd;
  }

  public func GetCorrection(error: Float, timeDelta: Float) -> Float { 
    return this.NativeInputCorrection(error);
  }
}
