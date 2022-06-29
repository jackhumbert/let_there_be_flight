public class FlightModeFly extends FlightModeStandard {
  public static func Create(component: ref<FlightComponent>) -> ref<FlightModeFly> {
    let self = new FlightModeFly();
    self.Initialize(component);
    return self;
  }

  public func GetDescription() -> String = "Fly";

  public func Update(timeDelta: Float) -> Void {
    let idealNormal = FlightUtils.Up();  
    let liftForce: Float = FlightSettings.GetFloat(n"flyModeLiftFactor") * this.component.lift + (9.81000042) * this.gravityFactor;
    this.UpdateWithNormalLift(timeDelta, idealNormal, liftForce);
  }
}