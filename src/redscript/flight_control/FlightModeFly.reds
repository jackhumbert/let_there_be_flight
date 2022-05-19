public class FlightModeFly extends FlightModeStandard {
  public func Update(timeDelta: Float, out force: Vector4, out torque: Vector4) -> Void {
    let heightDifference = this.component.lift * timeDelta * this.sys.settings.liftFactor() * (1.0 + this.component.stats.d_speedRatio * 2.0);
    let idealNormal = FlightUtils.Up();
    this.UpdateWithNormalDistance(timeDelta, force, torque, idealNormal, heightDifference);
  }
}