// Vanilla (2.3+) HandleCameraInput cycles the camera on ToggleVehCamera *release* and
// toggles the cinematic (autodrive) camera on a *hold* of HoldCinematicCamera, both bound
// to the same key. This used to be an @replaceMethod carrying the pre-2.3 body
// (IsActionJustPressed + RequestToggleVehicleCamera only), which is why the cinematic
// camera stopped working with LTBF installed (GitHub #93). Wrap instead: outside of flight
// the vanilla handler runs untouched; while flight is active the VehicleFlight state
// machine owns the camera toggle (VehicleFlightEventsTransition.HandleFlightCameraInput).
@wrapMethod(VehicleEventsTransition)
protected final func HandleCameraInput(scriptInterface: ref<StateGameScriptInterface>) -> Void {
  if !FlightController.GetInstance().IsActive() {
    wrappedMethod(scriptInterface);
  };
}

// @wrapMethod(vehicleVisualCustomizationHotkeyController)
// protected cb func OnAction(action: ListenerAction, consumer: ListenerActionConsumer) -> Bool {
//   let isVehicleCustomizationAvailable: Bool = this.m_player.GetMountedVehicle().GetVehicleComponent().GetIsVehicleVisualCustomizationEnabled();
//   if Equals(ListenerAction.GetName(action), n"VehicleVisualCustomization") {
//     if VehicleSystem.IsPlayerInVehicle(this.GetPlayer().GetGame()) && isVehicleCustomizationAvailable {
//       return true;
//     };
//     if Equals(ListenerAction.GetType(action), gameinputActionType.BUTTON_HOLD_COMPLETE) {
//       if IsDefined(this.m_animationProxy) && this.m_animationProxy.IsPlaying() {
//         this.m_animationProxy.GotoEndAndStop(true);
//         this.m_animationProxy = null;
//       };
//       this.m_animationProxy = this.PlayLibraryAnimation(n"onFailUse_carMod");
//     };
//   };
// }

// not the right place
// @replaceMethod(VehicleTransition)
// protected final func ResetVehicleCamera(scriptInterface: ref<StateGameScriptInterface>) -> Void {
//   // let camEvent: ref<vehicleCameraResetEvent> = new vehicleCameraResetEvent();
//   // scriptInterface.executionOwner.QueueEvent(camEvent);
//   return;
// }
