public class VehicleFlightEnabledDecisions extends VehicleFlightTransition {
  
  public const func ToVehicleFlightActivating(const stateContext: ref<StateContext>, const scriptInterface: ref<StateGameScriptInterface>) -> Bool {
      return this.IsFlightToggleRequested(scriptInterface);
  }

  public const func ToVehicleFlightActive(const stateContext: ref<StateContext>, const scriptInterface: ref<StateGameScriptInterface>) -> Bool {
      return IsDefined(fs().playerComponent) && fs().playerComponent.active;
  }
}

public class VehicleFlightEnabledEvents extends VehicleFlightEventsTransition {

}