public class FlightDriverCombatDecisions extends DriverCombatMountedWeaponsDecisions {

  public final func EnterCondition(const stateContext: ref<StateContext>, const scriptInterface: ref<StateGameScriptInterface>) -> Bool {
    // FlightLog.Info("[FlightDriverCombatDecisions] EnterCondition");
    // return scriptInterface.IsActionJustPressed(n"Flight_Toggle");
    // let fc = scriptInterface.owner.FindComponentByName(n"flightComponent") as FlightComponent;
    // return IsDefined(fc) && fc.configuration.CanActivate();
    return true;
    // return scriptInterface.IsActionJustPressed(n"Flight_Toggle");
  }

  public final func ToDriverCombat(const stateContext: ref<StateContext>, const scriptInterface: ref<StateGameScriptInterface>) -> Bool {
    // FlightLog.Info("[FlightDriverCombatDecisions] ToDrive");
    return scriptInterface.IsActionJustPressed(n"Flight_Toggle");
  }

  public final const func ToFlight(const stateContext: ref<StateContext>, const scriptInterface: ref<StateGameScriptInterface>) -> Bool {
    return this.ToDrive(stateContext, scriptInterface);
  }
}
