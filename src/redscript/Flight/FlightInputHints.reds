@addMethod(InputContextTransitionEvents)
protected final const func ShowVehicleFlightInputHints(stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
  // this.ShowInputHint(scriptInterface, n"Reload", n"VehicleFlight", "LocKey#36198");
  // this.ShowInputHint(scriptInterface, n"WeaponWheel", n"VehicleFlight", "LocKey#36199");
}

@addMethod(InputContextTransitionEvents)
protected final const func RemoveVehicleFlightInputHints(stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
  // this.RemoveInputHintsBySource(scriptInterface, n"VehicleFlight");
}

// should control the flight input hints
public class VehicleFlightContextEvents extends InputContextTransitionEvents {

  protected func OnEnter(stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
    FlightLog.Info("[VehicleFlightContextEvents] OnEnter");
    this.ShowVehicleFlightInputHints(stateContext, scriptInterface);
  }

  protected func OnExit(stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
    FlightLog.Info("[VehicleFlightContextEvents] OnExit");
    this.RemoveVehicleFlightInputHints(stateContext, scriptInterface);
  }
}