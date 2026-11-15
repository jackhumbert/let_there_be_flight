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
    this.ExitCustomCamera(scriptInterface);
      
      let evt = new VehicleFlightDeactivationEvent();
      evt.silent = false;
      vehicle.QueueEvent(evt);
  }
}
