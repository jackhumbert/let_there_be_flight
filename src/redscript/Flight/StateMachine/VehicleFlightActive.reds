public class VehicleFlightActiveDecisions extends VehicleFlightTransition {

  public const func ToVehicleFlightDeactivating(const stateContext: ref<StateContext>, const scriptInterface: ref<StateGameScriptInterface>) -> Bool {
    return scriptInterface.IsActionJustTapped(n"Flight_Toggle");
  }

  public const func ToVehicleFlightDisabled(const stateContext: ref<StateContext>, const scriptInterface: ref<StateGameScriptInterface>) -> Bool {
    let stopFlight = stateContext.GetTemporaryBoolParameter(n"stopVehicleFlight");
    if stopFlight.valid {
      return stopFlight.value;
    }
    return false;
  }
}

public class VehicleFlightActiveEvents extends VehicleFlightEventsTransition {

  protected func OnEnter(stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
    super.OnEnter(stateContext, scriptInterface);
    this.SetIsInFlight(stateContext, true);

  }

  protected func OnExit(stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
    this.SetIsInFlight(stateContext, false);
  }
  
  public final func OnUpdateFlight(timeDelta: Float, stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
    // let fc = FlightController.GetInstance();
    // fc.OnUpdate(timeDelta, stateContext, scriptInterface);
    // fc.sys.OnUpdate(timeDelta);
    // this.SetSide(stateContext, scriptInterface);
    // this.SendAnimFeature(stateContext, scriptInterface);
    if (!FlightController.GetInstance().showOptions) {
      this.HandleFlightCameraInput(scriptInterface);
    }
    // this.HandleFlightExitRequest(stateContext, scriptInterface);
  }

}
