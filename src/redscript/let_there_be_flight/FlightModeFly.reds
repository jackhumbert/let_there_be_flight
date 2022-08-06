public class FlightModeFly extends FlightModeStandard {

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "Flight Mode Settings")
  @runtimeProperty("ModSettings.displayName", "Fly Mode Enabled")
  public let enabled: Bool = false;

  public static func Create(component: ref<FlightComponent>) -> ref<FlightModeFly> {
    let self = new FlightModeFly();
    self.Initialize(component);
    return self;
  }

  public func GetDescription() -> String = "Fly";

  public func Update(timeDelta: Float) -> Void {
    let idealNormal = FlightUtils.Up();  
    let liftForce: Float = FlightSettings.GetFloat("flyModeLiftFactor") * this.component.lift + (9.81000042) * this.gravityFactor;
    this.UpdateWithNormalLift(timeDelta, idealNormal, liftForce);
  }
}