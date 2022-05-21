public class FlightModeHoverFly extends FlightModeStandard {
  protected let hovering: Bool;
  protected let referenceZ: Float;

  public static func Create(component: ref<FlightComponent>) -> ref<FlightModeHoverFly> {
    let self = new FlightModeHoverFly();
    self.Initialize(component);
    self.hovering = true;
    return self;
  }

  public func Activate() -> Void {
    let normal: Vector4;
    this.referenceZ = this.component.stats.d_position.Z;
    this.component.FindGround(normal);
    this.component.hoverHeight = this.component.distance;
  }

  public func Update(timeDelta: Float) -> Void {
    this.component.hoverHeight += this.component.lift * timeDelta * this.sys.settings.liftFactor(); // * (1.0 + this.component.stats.d_speedRatio * 2.0);

    if this.hovering {
      this.component.hoverHeight = MaxF(this.sys.settings.minHoverHeight(), this.component.hoverHeight);
    }

    let findWater: TraceResult;
    let heightDifference = 0.1;
    let idealNormal = FlightUtils.Up();

    this.component.sqs.SyncRaycastByCollisionGroup(this.component.stats.d_position, this.component.stats.d_position - this.sys.settings.lookDown(), n"Water", findWater, true, false);
    if !TraceResult.IsValid(findWater) {
      let normal: Vector4;
      let foundGround = this.component.FindGround(normal);
      if ((this.component.distance > this.sys.settings.maxHoverHeight() && this.hovering) || (this.hovering && !foundGround)) {
        this.hovering = false;
        this.referenceZ = this.component.stats.d_position.Z;
        this.component.hoverHeight = 0.0;
      }
      if (this.component.distance <= this.sys.settings.maxHoverHeight() && !this.hovering && foundGround) {
        this.hovering = true;
        this.component.hoverHeight = MaxF(this.component.distance, this.sys.settings.minHoverHeight());
      }
    // would be cool to fade between these instead of using a boolean
      if this.hovering {
        // close to ground, use as reference
        heightDifference = this.component.hoverHeight - this.component.distance;
        // idealNormal = this.normal;
        idealNormal = Vector4.Interpolate(normal, idealNormal, (this.component.distance - this.sys.settings.minHoverHeight()) / (this.sys.settings.maxHoverHeight() - this.sys.settings.minHoverHeight()));
      } else {
        // use absolute Z if too high
        heightDifference = this.referenceZ + this.component.hoverHeight - this.component.stats.d_position.Z;
      }
    }

    this.UpdateWithNormalDistance(timeDelta, idealNormal, heightDifference);
  }
}