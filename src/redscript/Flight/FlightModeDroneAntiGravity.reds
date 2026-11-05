public class FlightModeDroneAntiGravity extends FlightModeDrone {

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-Flight-Mode-Settings")
  @runtimeProperty("ModSettings.displayName", "UI-Settings-Anti-Gravity-Drone-Mode-Enabled")
  public let agEnabled: Bool = false;


  public func GetDescription() -> String = "Anti-Gravity Drone";

  public func IsEnabled() -> Bool = this.agEnabled;

  public func Update(timeDelta: Float) -> Void {
    super.Update(timeDelta);
    this.force += this.component.stats.d_localUp *  (9.81000042) * this.gravityFactor;
  }
}