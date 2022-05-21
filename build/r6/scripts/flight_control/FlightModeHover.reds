public class FlightModeHover extends FlightModeStandard {
  public static func Create(component: ref<FlightComponent>) -> ref<FlightModeHover> {
    let self = new FlightModeHover();
    self.Initialize(component);
    return self;
  }
  
  public func Update(timeDelta: Float) -> Void {
    this.component.hoverHeight = MaxF(this.sys.settings.minHoverHeight(), this.component.hoverHeight);

    let findWater: TraceResult;
    let heightDifference = 1.0;
    let normal: Vector4;
    let idealNormal = FlightUtils.Up();

    this.component.sqs.SyncRaycastByCollisionGroup(this.component.stats.d_position, this.component.stats.d_position - this.sys.settings.lookDown(), n"Water", findWater, true, false);
    if !TraceResult.IsValid(findWater) {
      if (this.component.FindGround(normal)) {
          heightDifference = this.component.hoverHeight - this.component.distance;
          idealNormal = normal;
      }
    }

    this.UpdateWithNormalDistance(timeDelta, idealNormal, heightDifference);
  }
}