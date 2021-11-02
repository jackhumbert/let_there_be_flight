public class PID {
  private let valueFloat: Float;
  // private let valueVector: Vector4;
  private let inputFloat: Float;
  // private let inputVector: Vector4;
  private let P: Float;
  private let I: Float;
  private let D: Float;
  public let integralFloat: Float;
  // private let integralVector: Vector4;
  private let lastErrorFloat: Float;
  // private let lastErrorVector: Vector4;
  public static func Create(P: Float, I: Float, D: Float) -> ref<PID> {
    let instance: ref<PID> = new PID();
    instance.P = P;
    instance.I = I;
    instance.D = D;
    instance.Reset();
    return instance;
  }
  public static func Create(P: Float, I: Float, D: Float, initialValue: Float) -> ref<PID> {
    let instance: ref<PID> = PID.Create(P, I, D);
    instance.valueFloat = initialValue;
    return instance;
  }
  // public static func Create(P: Float, I: Float, D: Float, initialValue: Vector4) -> ref<PID> {
  //   let instance: ref<PID> = PID.Create(P, I, D);
  //   instance.valueVector = initialValue;
  //   return instance;
  // }
  public func Update(P: Float, I: Float, D: Float) -> Void {
    this.P = P;
    this.I = I;
    this.D = D;
  }
  public func SetInput(input: Float) {
    this.inputFloat = input;
  }
  // public func SetInput(input: Vector4) {
  //   this.inputVector = input;
  // }
  public func GetValue(timeDelta: Float) -> Float {
    let error: Float = this.inputFloat - this.valueFloat;
    this.valueFloat += this.GetCorrection(error);
    return this.valueFloat;
  } 
  public func GetValue() -> Float {
    return this.valueFloat;
  } 
  // public func GetValue(timeDelta: Float) -> Vector4 {
  //   let error: Vector4 = this.inputVector - this.valueVector;
  //   this.valueVector += this.GetCorrection(error)
  //   return this.valueVector;
  // }
  public func GetValue(input: Float, timeDelta: Float) -> Float {
    this.SetInput(input);
    return this.GetValue(timeDelta);
  }
  // public func GetValue(input: Vector4, timeDelta: Float) -> Vector4 {
  //   this.SetInput(input);
  //   return GetValue(timeDelta);
  // }
  public func GetCorrection(error: Float, timeDelta: Float) -> Float { 
    let derivative: Float = (error - this.lastErrorFloat) / timeDelta;
    // if error < 0.01 || error * this.lastErrorFloat < 0.0 {
    //   this.integralFloat = 0.0;
    // } else {
    this.integralFloat = ClampF(error * timeDelta + this.integralFloat, -100.0, 100.0) * 0.95;
    // }
    this.lastErrorFloat = error;
    return this.P * error + this.I * this.integralFloat + this.D * derivative;
  }
  // public func GetCorrection(error: Vector4, timeDelta: Float) -> Vector4 { 
  //   let derivative: Vector4 = (error - this.lastErrorVector) / timeDelta;
  //   this.integralVector = Vector4.ClampLength(error * timeDelta + this.integralVector, -100.0, 100.0);
  //   this.lastErrorVector = error;
  //   return this.P * error + this.I * this.integralFloat + this.D * derivative;
  // }
  public func GetCorrectionClamped(error: Float, timeDelta: Float, clamp: Float) -> Float {
    return ClampF(this.GetCorrection(error, timeDelta), -clamp, clamp);
  }
  public func Reset() -> Void {
    this.inputFloat = 0.0;
    this.integralFloat = 0.0;
    this.lastErrorFloat = 0.0;
  }
}