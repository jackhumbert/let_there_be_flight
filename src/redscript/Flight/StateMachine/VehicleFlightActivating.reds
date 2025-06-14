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
      // FlightLog.Info("[VehicleEventsTransition] OnEnterFlight, starting");
    // this.SetSide(stateContext, scriptInterface);
    // this.SetIsVehicleDriver(stateContext, true);
    // this.PlayerStateChange(scriptInterface, 1);
    // this.SetBlackboardIntVariable(scriptInterface, GetAllBlackboardDefs().PlayerStateMachine.Vehicle, 8);
    // this.SendAnimFeature(stateContext, scriptInterface);
    // this.SetVehFppCameraParams(stateContext, scriptInterface, false);
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
      
      // if !VehicleTransition.CanEnterDriverCombat() {
      //   stateContext.SetPermanentBoolParameter(n"ForceEmptyHands", true, true);
      // };
      // FlightController.GetInstance().Activate();
    // if (!vehicle.GetFlightComponent().active) {
      let evt = new VehicleFlightActivationEvent();
      // evt.vehicle = scriptInterface.owner as VehicleObject;
      vehicle.QueueEvent(evt);
    // }
  }
}