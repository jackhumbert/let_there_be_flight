public class FlightModeFly extends FlightModeStandard {
  public static func Create(component: ref<FlightComponent>) -> ref<FlightModeFly> {
    let self = new FlightModeFly();
    self.component = component;
    self.sys = component.sys;
    return self;
  }
  
  public func Update(timeDelta: Float) -> Void {
    let heightDifference = this.component.lift * timeDelta * this.sys.settings.liftFactor() * (1.0 + this.component.stats.d_speedRatio * 2.0);
    let idealNormal = FlightUtils.Up();
    this.UpdateWithNormalDistance(timeDelta, idealNormal, heightDifference);
  }
}