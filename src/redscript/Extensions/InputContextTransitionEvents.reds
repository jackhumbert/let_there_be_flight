@addMethod(InputContextTransitionEvents)
protected final const func ShowVehicleFlightInputHints(stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
  this.ShowInputHint(scriptInterface, n"Flight_Toggle", n"VehicleFlight", LocKeyToString(n"Input-Hint-Disable-Flight"), inkInputHintHoldIndicationType.FromInputConfig, true, 2);
  stateContext.SetPermanentBoolParameter(n"isFlightInputHintDisplayed", true, true);
}

@addMethod(InputContextTransitionEvents)
protected final const func RemoveVehicleFlightInputHints(stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
  if this.m_isGameplayInputHintManagerInitialized {
    this.RemoveInputHint(scriptInterface, n"Flight_Toggle", n"VehicleFlight");
    this.RemoveInputHintsBySource(scriptInterface, n"VehicleFlight");
  };
  stateContext.RemovePermanentBoolParameter(n"isFlightInputHintDisplayed");
}

@wrapMethod(InputContextTransitionEvents)
protected final func RemoveAllInputHints(stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
  wrappedMethod(stateContext, scriptInterface);
  this.RemoveVehicleFlightInputHints(stateContext, scriptInterface);
}

@wrapMethod(InputContextTransitionEvents)
protected final const func ShowVehicleDriverInputHints(stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
  wrappedMethod(stateContext, scriptInterface);
  this.ShowInputHint(scriptInterface, n"Flight_Toggle", n"VehicleDriver", LocKeyToString(n"Input-Hint-Enable-Flight"), inkInputHintHoldIndicationType.FromInputConfig, true, 2);
}
