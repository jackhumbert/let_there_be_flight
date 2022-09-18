public abstract class FlightMode {
  protected let sys: ref<FlightSystem>;
  protected let component: ref<FlightComponent>;

  public let force: Vector4;
  public let torque: Vector4;

  public static let gravityFactor: Float;

  public let usesRightStickInput: Bool;
  public let collisionPenalty: Float;

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
    
    let velocityDamp: Vector4 = this.component.stats.d_speed * this.component.stats.d_localVelocity * FlightSettings.GetInstance().generalDampFactorLinear * this.component.stats.s_airResistanceFactor;
    let angularDamp: Vector4 = this.component.stats.d_angularVelocity * FlightSettings.GetInstance().generalDampFactorAngular;
    // if angularDamp.X < 0.0 {
    //   angularDamp.X *= angularDamp.X * -1.0;
    // } else {
    //   angularDamp.X *= angularDamp.X;
    // }
    // if angularDamp.Y < 0.0 {
    //   angularDamp.Y *= angularDamp.Y * -1.0;
    // } else {
    //   angularDamp.Y *= angularDamp.Y;
    // }
    // if angularDamp.Z < 0.0 {
    //   angularDamp.Z *= angularDamp.Z * -1.0;
    // } else {
    //   angularDamp.Z *= angularDamp.Z;
    // }
    // if angularDamp.W < 0.0 {
    //   angularDamp.W *= angularDamp.W * -1.0;
    // } else {
    //   angularDamp.W *= angularDamp.W;
    // }
    

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

    let yawDirectionality: Float = this.component.stats.d_speedRatio * FlightSettings.GetInstance().generalYawDirectionalityFactor;
    let pitchDirectionality: Float = this.component.stats.d_speedRatio * FlightSettings.GetInstance().generalPitchDirectionalityFactor;
    let aeroFactor = Vector4.Dot(this.component.stats.d_forward, this.component.stats.d_direction);
    // yawDirectionality - redirect non-directional velocity to vehicle forward

    this.force = -velocityDamp;
    
    this.force += FlightUtils.Forward() * AbsF(Vector4.Dot(this.component.stats.d_forward - this.component.stats.d_direction, this.component.stats.d_right)) * yawDirectionality * aeroFactor;
    this.force += -this.component.stats.d_localDirection * AbsF(Vector4.Dot(this.component.stats.d_forward - this.component.stats.d_direction, this.component.stats.d_right)) * yawDirectionality * AbsF(aeroFactor);

    this.force += FlightUtils.Forward() * AbsF(Vector4.Dot(this.component.stats.d_forward - this.component.stats.d_direction, this.component.stats.d_up)) * pitchDirectionality * aeroFactor;
    this.force += -this.component.stats.d_localDirection * AbsF(Vector4.Dot(this.component.stats.d_forward - this.component.stats.d_direction, this.component.stats.d_up)) * pitchDirectionality * AbsF(aeroFactor);

    this.torque = -angularDamp;
    this.torque.Z -= aeroDynamicYaw * FlightSettings.GetInstance().generalYawAeroFactor;
    this.torque.X -= aeroDynamicPitch * FlightSettings.GetInstance().generalPitchAeroFactor;
  }
}