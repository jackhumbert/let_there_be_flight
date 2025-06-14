public class VehicleFlightDeactivatingDecisions extends VehicleFlightTransition {

}

public class VehicleFlightDeactivatingEvents extends VehicleFlightEventsTransition {

  protected func OnEnter(stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
    super.OnEnter(stateContext, scriptInterface);
    let vehicle = scriptInterface.owner as VehicleObject;
    if !IsDefined(vehicle) {
      FlightLog.Info("[VehicleFlightDeactivatingEvents] No vehicle defined in exit");
      return;
    }
      // FlightLog.Info("[VehicleEventsTransition] OnExitFlight, stopping");
    // this.ExitCustomCamera(scriptInterface);
    // this.SetIsVehicleDriver(stateContext, false);
    // this.SendAnimFeature(stateContext, scriptInterface);
    // vehicle.ToggleFlightComponent(false);
    // FlightController.GetInstance().Deactivate(false);
    // stateContext.SetPermanentBoolParameter(n"ForceEmptyHands", false, true);
      
    // if (vehicle.GetFlightComponent().active) {
      let evt = new VehicleFlightDeactivationEvent();
      evt.silent = false;
      // evt.vehicle = scriptInterface.owner as VehicleObject;
      vehicle.QueueEvent(evt);
    // }
  }
}
