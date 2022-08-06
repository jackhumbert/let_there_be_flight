public class FlightModeDroneAntiGravity extends FlightModeDrone {

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "Flight Mode Settings")
  @runtimeProperty("ModSettings.displayName", "Anti-Gravity Drone Mode Enabled")
  public let agEnabled: Bool = false;

  public static func Create(component: ref<FlightComponent>) -> ref<FlightModeDroneAntiGravity> {
    let self = new FlightModeDroneAntiGravity();
    self.Initialize(component);
    return self;
  }

  public func GetDescription() -> String = "Anti-Gravity Drone";

  public func Update(timeDelta: Float) -> Void {
    super.Update(timeDelta);
    this.force += this.component.stats.d_localUp *  (9.81000042) * this.gravityFactor;
  }
}