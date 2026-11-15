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

  public func SetInput(input: Float) {
    this.inputFloat = input;
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
}
