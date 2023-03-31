public abstract class FlightMode {
  protected let sys: ref<FlightSystem>;
  protected let component: ref<FlightComponent>;

  public let force: Vector4;
  public let torque: Vector4;
  public let linearDamp: Float;
  public let angularDamp: Float;

  public static let gravityFactor: Float;

  public let usesRightStickInput: Bool;
  public let collisionPenalty: Float;

  public let dampAccVector: Vector3;
  // public let lastAngularDamp: Vector4;

  // public let enabled: Bool;

  public func Initialize(component: ref<FlightComponent>) -> Void {
    this.component = component;
    this.sys = component.sys;
    // this.gravityFactor = 2.885;
    this.gravityFactor = 1.0;
  }

  public func Deinitialize() -> Void;

  public func Activate() -> Void;
  public func Deactivate() -> Void;
  public func GetDescription() -> String;

  public func Update(timeDelta: Float) -> Void;

  public func ApplyPhysics(timeDelta: Float) -> Void {
    let fs =  FlightSettings.GetInstance();
    
    let velocityDamp: Vector4; // = this.component.stats.d_speed * this.component.stats.d_localVelocity * fs.generalDampFactorLinear * this.component.stats.s_airResistanceFactor;
    // let angularDamp: Vector4 = this.component.stats.d_angularVelocity * fs.generalDampFactorAngular;
    // angularDamp += this.component.stats.d_angularAcceleration;
    
    let angularDamp = 0.0;

    // only damp if no input is being received on that axis
    // angularDamp += (1.0 - SqrtF(AbsF(this.component.pitch)));
    // angularDamp += (1.0 - SqrtF(AbsF(this.component.roll)));
    // angularDamp += (1.0 - SqrtF(AbsF(this.component.yaw)));
    angularDamp += (1.0 - AbsF(this.component.pitch));
    angularDamp += (1.0 - AbsF(this.component.roll));
    angularDamp += (1.0 - AbsF(this.component.yaw));

    // detect when we hit stuff and delay the damping
    // this.dampAccVector.X = ClampF(MaxF(this.dampAccVector.X, AbsF(this.component.stats.d_angularAcceleration.X) / timeDelta / 10.0), 0.0, 1.0);
    // this.dampAccVector.Y = ClampF(MaxF(this.dampAccVector.Y, AbsF(this.component.stats.d_angularAcceleration.Y) / timeDelta / 10.0), 0.0, 1.0);
    // this.dampAccVector.Z = ClampF(MaxF(this.dampAccVector.Z, AbsF(this.component.stats.d_angularAcceleration.Z) / timeDelta / 10.0), 0.0, 1.0);

    // angularDamp.X *= (1.0 - this.dampAccVector.X);
    // angularDamp.Y *= (1.0 - this.dampAccVector.Y);
    // angularDamp.Z *= (1.0 - this.dampAccVector.Z);

    // decay over 200 ms
    // this.dampAccVector.X -= timeDelta / 0.200;
    // this.dampAccVector.Y -= timeDelta / 0.200;
    // this.dampAccVector.Z -= timeDelta / 0.200;

    // clamp the dampening
    // if Vector4.Length(angularDamp) > fs.generalDampFactorAngularMax {
    //   angularDamp = Vector4.Normalize(angularDamp) * fs.generalDampFactorAngularMax;
    // }

    this.angularDamp = fs.generalDampFactorAngular * ClampF(angularDamp, 0.0, 1.0) + this.component.angularBrake * fs.brakeFactorAngular;
    this.linearDamp = fs.generalDampFactorLinear + this.component.linearBrake * fs.brakeFactorLinear;

    // this.lastAngularDamp = angularDamp;

    // let the world throw us around on collisions
    // angularDamp *= this.timeSinceLastCollision;
    // this.timeSinceLastCollision = MinF(this.timeSinceLastCollision + timeDelta, 1.0);

    let direction = this.component.stats.d_direction;
    if Vector4.Dot(this.component.stats.d_direction, this.component.stats.d_forward) < 0.0 {
      direction = -this.component.stats.d_direction;
    }
    let yawDirectionAngle: Float = Vector4.GetAngleDegAroundAxis(direction, this.component.stats.d_forward, this.component.stats.d_up);
    let pitchDirectionAngle: Float = Vector4.GetAngleDegAroundAxis(direction, this.component.stats.d_forward, this.component.stats.d_right);

    // let aeroDynamicYaw = this.component.aeroYawPID.GetCorrectionClamped(yawDirectionAngle, timeDelta, 10.0) * this.component.stats.d_speedRatio;// / 10.0;
    // let aeroDynamicPitch = this.component.pitchAeroPID.GetCorrectionClamped(pitchDirectionAngle, timeDelta, 10.0) * this.component.stats.d_speedRatio;// / 10.0;
    let aeroDynamicYaw = yawDirectionAngle * this.component.stats.d_speedRatio;// / 10.0;
    let aeroDynamicPitch = pitchDirectionAngle * this.component.stats.d_speedRatio;// / 10.0;

    let yawDirectionality: Float = this.component.stats.d_speedRatio * fs.generalYawDirectionalityFactor;
    let pitchDirectionality: Float = this.component.stats.d_speedRatio * fs.generalPitchDirectionalityFactor;
    let aeroFactor = Vector4.Dot(this.component.stats.d_forward, this.component.stats.d_direction);
    // yawDirectionality - redirect non-directional velocity to vehicle forward

    this.force = -velocityDamp;
    
    this.force += FlightUtils.Forward() * AbsF(Vector4.Dot(this.component.stats.d_forward - this.component.stats.d_direction, this.component.stats.d_right)) * yawDirectionality * aeroFactor;
    this.force += -this.component.stats.d_localDirection * AbsF(Vector4.Dot(this.component.stats.d_forward - this.component.stats.d_direction, this.component.stats.d_right)) * yawDirectionality * AbsF(aeroFactor);

    this.force += FlightUtils.Forward() * AbsF(Vector4.Dot(this.component.stats.d_forward - this.component.stats.d_direction, this.component.stats.d_up)) * pitchDirectionality * aeroFactor;
    this.force += -this.component.stats.d_localDirection * AbsF(Vector4.Dot(this.component.stats.d_forward - this.component.stats.d_direction, this.component.stats.d_up)) * pitchDirectionality * AbsF(aeroFactor);

    // this.torque = -angularDamp;
    this.torque.Z -= aeroDynamicYaw * fs.generalYawAeroFactor;
    this.torque.X -= aeroDynamicPitch * fs.generalPitchAeroFactor;
  }
}