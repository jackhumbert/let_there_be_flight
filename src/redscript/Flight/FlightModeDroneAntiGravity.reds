// Drone with gravity cancelled; in the mode list behind Extra Flight Modes (FlightSettings.extraModesEnabled).
public class FlightModeDroneAntiGravity extends FlightModeDrone {

  public func GetDescription() -> String = "Anti-Gravity Drone";

  public func IsEnabled() -> Bool = FlightSettings.GetInstance().extraModesEnabled;

  public func Update(timeDelta: Float) -> Void {
    super.Update(timeDelta);
    this.force += this.component.stats.d_localUp *  (9.81000042) * this.gravityFactor;
  }
}
