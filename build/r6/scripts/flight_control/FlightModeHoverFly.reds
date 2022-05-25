public class FlightModeHoverFly extends FlightModeStandard {
  protected let hovering: Float;
  protected let referenceZ: Float;

  public static func Create(component: ref<FlightComponent>) -> ref<FlightModeHoverFly> {
    let self = new FlightModeHoverFly();
    self.Initialize(component);
    self.hovering = 1.0;
    return self;
  }

  public func Activate() -> Void {
    let normal: Vector4;
    this.referenceZ = this.component.stats.d_position.Z;
    this.component.FindGround(normal);
    this.component.hoverHeight = MaxF(this.component.distance, FlightSettings.GetFloat(n"minHoverHeight"));
  }
  
  public func GetDescription() -> String = "Hover & Fly";

  public func Update(timeDelta: Float) -> Void {
    let lastHovering = this.hovering;
    let normal: Vector4;
    let foundGround = this.component.FindGround(normal);
    if foundGround {
      this.hovering = ClampF(1.0 - (this.component.distance - FlightSettings.GetFloat(n"minHoverHeight")) / (FlightSettings.GetFloat(n"maxHoverHeight") - FlightSettings.GetFloat(n"minHoverHeight")), 0.0, 1.0);
    } else {
      this.hovering = 0.0;
    }

    if lastHovering == 0.0 && this.hovering > 0.0 {
      this.component.hoverHeight = MaxF(this.component.distance + this.component.lift * timeDelta * FlightSettings.GetFloat(n"liftFactor"), FlightSettings.GetFloat(n"minHoverHeight"));
    } else {
      this.component.hoverHeight = MaxF(this.component.hoverHeight + this.component.lift * timeDelta * FlightSettings.GetFloat(n"liftFactor"), FlightSettings.GetFloat(n"minHoverHeight"));
    }

    let heightDifference = this.component.hoverHeight - this.component.distance;
    let idealNormal = Vector4.Interpolate(FlightUtils.Up(), normal, this.hovering);

    let hoverCorrection = this.component.hoverGroundPID.GetCorrectionClamped(heightDifference, timeDelta, FlightSettings.GetFloat(n"hoverClamp"));// / FlightSettings.GetFloat(n"hoverClamp");
    let liftFactor = LerpF(this.hovering, this.component.lift - this.component.stats.d_velocity.Z * 0.1, hoverCorrection);

    this.UpdateWithNormalLift(timeDelta, idealNormal, liftFactor * FlightSettings.GetFloat(n"hoverFactor") + (9.81000042) * this.gravityFactor);
  }
}