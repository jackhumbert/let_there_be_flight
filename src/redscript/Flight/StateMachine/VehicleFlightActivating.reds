public class VehicleFlightActivatingDecisions extends VehicleFlightTransition {

}

public class VehicleFlightActivatingEvents extends VehicleFlightEventsTransition {

  protected func OnEnter(stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
    super.OnEnter(stateContext, scriptInterface);
    
    // FlightLog.Info("Owner : " + NameToString(scriptInterface.owner.GetRecordID()));
    // FlightLog.Info("EOwner: " + NameToString(scriptInterface.executionOwner.GetRecordID()));

    let vehicle = scriptInterface.owner as VehicleObject;
    if !IsDefined(vehicle) {
      FlightLog.Info("[VehicleFlightActivatingEvents] No vehicle defined in enter");
      return;
    }
    /*
    switch vehicle.GetCameraManager().GetActivePerspective() {
      case vehicleCameraPerspective.FPP:
        FlightSystem.GetInstance().cameraIndex = 0;
        break;
      case vehicleCameraPerspective.TPPClose:
        FlightSystem.GetInstance().cameraIndex = 2;
        break;
      case vehicleCameraPerspective.TPPFar:
        FlightSystem.GetInstance().cameraIndex = 3;
    };
    */
      
      let evt = new VehicleFlightActivationEvent();
      vehicle.QueueEvent(evt);
  }
}