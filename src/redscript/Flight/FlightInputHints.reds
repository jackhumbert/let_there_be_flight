@addMethod(InputContextTransitionEvents)
protected final const func ShowVehicleFlightInputHints(stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
  // this.ShowInputHint(scriptInterface, n"Flight_Toggle", n"VehicleFlight", "Input-Hint-Disable-Flight", inkInputHintHoldIndicationType.FromInputConfig, true, 2);
  stateContext.SetPermanentBoolParameter(n"isFlightInputHintDisplayed", true, true);
}

@addMethod(InputContextTransitionEvents)
protected final const func RemoveVehicleFlightInputHints(stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
  if this.m_isGameplayInputHintManagerInitialized {
    // this.RemoveInputHint(scriptInterface, n"Flight_Toggle", n"VehicleFlight");
    // this.RemoveInputHintsBySource(scriptInterface, n"VehicleFlight");
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
  // this.ShowInputHint(scriptInterface, n"Flight_Toggle", n"VehicleDriver", "Input-Hint-Enable-Flight", inkInputHintHoldIndicationType.FromInputConfig, true, 2);
}


// should control the flight input hints
public class VehicleFlightContextEvents extends InputContextTransitionEvents {

  protected final func OnUpdate(timeDelta: Float, stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
    if this.m_gameplaySettings.GetIsInputHintEnabled() && this.m_isGameplayInputHintManagerInitialized {
      this.UpdateVehicleFlightInputHints(stateContext, scriptInterface);
    };
  }

  protected final func UpdateVehicleFlightInputHints(stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
    // let isExitVehicleBlocked: Bool;
    // let isVehicleCombatModeBlocked: Bool;
    if this.ShouldForceRefreshInputHints(stateContext) {
      this.RemoveVehicleFlightInputHints(stateContext, scriptInterface);
      this.m_isGameplayInputHintRefreshRequired = false;
    };
    if stateContext.GetBoolParameter(n"isFlightInputHintDisplayed", true) {
      // isVehicleCombatModeBlocked = this.IsVehicleBlockingCombat(scriptInterface) || this.IsEmptyHandsForced(stateContext, scriptInterface);
      // isExitVehicleBlocked = this.IsExitVehicleBlocked(scriptInterface);
      // if NotEquals(isVehicleCombatModeBlocked, stateContext.GetBoolParameter(n"IsVehicleCombatModeBlocked", true)) {
      //   this.ShowVehicleDrawWeaponInputHint(stateContext, scriptInterface);
      // };
      // if NotEquals(isExitVehicleBlocked, stateContext.GetBoolParameter(n"IsExitVehicleBlocked", true)) {
      //   this.ShowVehicleExitInputHint(stateContext, scriptInterface, n"VehicleFlight");
      // };
    } else {
      this.ShowVehicleFlightInputHints(stateContext, scriptInterface);
    };
  }


  protected func OnEnter(stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
    FlightLog.Info("[VehicleFlightContextEvents] OnEnter");
    this.RemoveVehicleDriverInputHints(stateContext, scriptInterface);
    this.ShowVehicleFlightInputHints(stateContext, scriptInterface);
  }

  protected func OnExit(stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
    FlightLog.Info("[VehicleFlightContextEvents] OnExit");
    this.RemoveVehicleFlightInputHints(stateContext, scriptInterface);
  }
}