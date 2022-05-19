public class FlightModeHover extends FlightModeStandard {
  public func Update(timeDelta: Float, out force: Vector4, out torque: Vector4) -> Void {
    this.component.hoverHeight = MaxF(this.sys.settings.minHoverHeight(), this.component.hoverHeight);

    let findWater: TraceResult;
    let heightDifference = 1.0;
    let normal: Vector4;
    let idealNormal = FlightUtils.Up();

    this.component.sqs.SyncRaycastByCollisionGroup(this.component.stats.d_position, this.component.stats.d_position - this.sys.settings.lookDown(), n"Water", findWater, true, false);
    if !TraceResult.IsValid(findWater) {
      if (this.component.FindGround(timeDelta, normal)) {
          heightDifference = this.component.hoverHeight - this.component.distance;
          idealNormal = normal;
      }
    }

    this.UpdateWithNormalDistance(timeDelta, force, torque, idealNormal, heightDifference);
  }
}