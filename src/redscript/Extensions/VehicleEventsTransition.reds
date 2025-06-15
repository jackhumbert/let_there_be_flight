@replaceMethod(VehicleEventsTransition)
protected final func HandleCameraInput(scriptInterface: ref<StateGameScriptInterface>) -> Void {
  if scriptInterface.IsActionJustPressed(n"ToggleVehCamera") && !this.IsVehicleCameraChangeBlocked(scriptInterface) && !FlightController.GetInstance().showOptions {
    this.RequestToggleVehicleCamera(scriptInterface);
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

@replaceMethod(VehicleTransition)
protected final func ResetVehicleCamera(scriptInterface: ref<StateGameScriptInterface>) -> Void {
  // let camEvent: ref<vehicleCameraResetEvent> = new vehicleCameraResetEvent();
  // scriptInterface.executionOwner.QueueEvent(camEvent);
  return;
}
