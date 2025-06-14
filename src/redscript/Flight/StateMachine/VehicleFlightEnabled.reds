public class VehicleFlightEnabledDecisions extends VehicleFlightTransition {
  
  public const func ToVehicleFlightActivating(const stateContext: ref<StateContext>, const scriptInterface: ref<StateGameScriptInterface>) -> Bool {
      return scriptInterface.IsActionJustTapped(n"Flight_Toggle");
  }

  public const func ToVehicleFlightActive(const stateContext: ref<StateContext>, const scriptInterface: ref<StateGameScriptInterface>) -> Bool {
      return IsDefined(fs().playerComponent) && fs().playerComponent.active;
  }
}

public class VehicleFlightEnabledEvents extends VehicleFlightEventsTransition {

}