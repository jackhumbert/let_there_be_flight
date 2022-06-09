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
  // public func SetInput(input: Vector4) {
  //   this.inputVector = input;
  // }
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
    this.integralFloat = ClampF(error * timeDelta + this.integralFloat, -100.0, 100.0);
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
  public func Reset(opt input: Float) -> Void {
    this.inputFloat = input;
    this.integralFloat = 0.0;
    this.lastErrorFloat = 0.0;
  }
}

public class InputPID extends PID {
  private let P_dec: Float;
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
    if AbsF(this.inputFloat) > AbsF(this.valueFloat) || this.inputFloat * this.valueFloat < 0.0 {
      return this.P * error;
    } else {
      return this.P_dec * error;
    }
  }
}

public class DualPID extends PID {
  private let P_aux: Float;
  private let I_aux: Float;
  private let D_aux: Float;
  private let ratio: Float;
  public static func Create(P: Float, I: Float, D: Float, P_aux: Float, I_aux: Float, D_aux: Float) -> ref<DualPID> {
    let instance: ref<DualPID> = new DualPID();
    instance.P = P;
    instance.I = I;
    instance.D = D;
    instance.P_aux = P_aux;
    instance.I_aux = I_aux;
    instance.D_aux = D_aux;
    instance.Reset();
    return instance;
  }
  public static func Create(P: Float, I: Float, D: Float, P_aux: Float, I_aux: Float, D_aux: Float, initialValue: Float) -> ref<DualPID> {
    let instance: ref<DualPID> = DualPID.Create(P, I, D, P_aux, I_aux, D_aux);
    instance.valueFloat = initialValue;
    return instance;
  }
  public func SetRatio(ratio: Float) {
    this.ratio = ratio;
  }
  public func GetCorrection(error: Float, timeDelta: Float) -> Float { 
    let derivative: Float = (error - this.lastErrorFloat) / timeDelta;
    // if error < 0.01 || error * this.lastErrorFloat < 0.0 {
    //   this.integralFloat = 0.0;
    // } else {
    this.integralFloat = ClampF(error * timeDelta + this.integralFloat, -100.0, 100.0) * 0.95;
    // }
    this.lastErrorFloat = error;
    let pri = this.P * error + this.I * this.integralFloat + this.D * derivative;
    let aux = this.P_aux * error + this.I_aux * this.integralFloat + this.D_aux * derivative;
    return pri * (1.0 - this.ratio) + aux * (this.ratio);
  }
  public func UpdateAux(P_aux: Float, I_aux: Float, D_aux: Float) -> Void {
    this.P_aux = P_aux;
    this.I_aux = I_aux;
    this.D_aux = D_aux;
  }
}