// Vanilla (2.3+) HandleCameraInput cycles the camera on ToggleVehCamera *release* and
// toggles the cinematic (autodrive) camera on a *hold* of HoldCinematicCamera, both bound
// to the same key. This used to be an @replaceMethod carrying the pre-2.3 body
// (IsActionJustPressed + RequestToggleVehicleCamera only), which is why the cinematic
// camera stopped working with LTBF installed (GitHub #93). Wrap instead: outside of flight
// the vanilla handler runs untouched; while flight is active the VehicleFlight state
// machine owns the camera toggle (VehicleFlightEventsTransition.HandleFlightCameraInput).
// 2.5 added the stateContext parameter (the hold is skipped while the item wheel is open).
@wrapMethod(VehicleEventsTransition)
protected final func HandleCameraInput(const stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
  if !FlightController.GetInstance().IsActive() {
    wrappedMethod(stateContext, scriptInterface);
  };
}

