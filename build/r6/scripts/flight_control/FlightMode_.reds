public abstract class FlightMode {
  protected let sys: ref<FlightSystem>;
  protected let component: ref<FlightComponent>;

  public let force: Vector4;
  public let torque: Vector4;

  public static let gravityFactor: Float;

  public func Initialize(component: ref<FlightComponent>) -> Void {
    this.component = component;
    this.sys = component.sys;
    // this.gravityFactor = 2.885;
    this.gravityFactor = 1.0;
  }

  public func Activate() -> Void;
  public func Deactivate() -> Void;
  public func GetDescription() -> String;

  public func Update(timeDelta: Float) -> Void;

  public func ApplyPhysics(timeDelta: Float) -> Void {
    
    let velocityDamp: Vector4 = this.component.stats.d_speed * this.component.stats.d_localVelocity * FlightSettings.GetFloat(n"airResistance") * this.component.stats.s_airResistanceFactor;
    let angularDamp: Vector4 = this.component.stats.d_angularVelocity * FlightSettings.GetFloat(n"angularDampFactor");

    let direction = this.component.stats.d_direction;
    if Vector4.Dot(this.component.stats.d_direction, this.component.stats.d_forward) < 0.0 {
      direction = -this.component.stats.d_direction;
    }
    let directionAngle: Float = Vector4.GetAngleDegAroundAxis(direction, this.component.stats.d_forward, this.component.stats.d_up);
    let aeroDynamicYaw = this.component.yawPID.GetCorrectionClamped(directionAngle, timeDelta, 10.0) * this.component.stats.d_speedRatio;// / 10.0;

    let yawDirectionality: Float = this.component.stats.d_speedRatio * FlightSettings.GetFloat(n"yawDirectionalityFactor");
    let aeroFactor = Vector4.Dot(this.component.stats.d_forward, this.component.stats.d_direction);
    // yawDirectionality - redirect non-directional velocity to vehicle forward

    this.force = -velocityDamp;
    
    this.force += FlightUtils.Forward() * AbsF(Vector4.Dot(this.component.stats.d_forward - this.component.stats.d_direction, this.component.stats.d_right)) * yawDirectionality * aeroFactor;
    this.force += -this.component.stats.d_localDirection * AbsF(Vector4.Dot(this.component.stats.d_forward - this.component.stats.d_direction, this.component.stats.d_right)) * yawDirectionality * AbsF(aeroFactor);

    this.torque = -angularDamp;
    this.torque.Z -= aeroDynamicYaw * FlightSettings.GetFloat(n"yawCorrectionFactor");
  }
}