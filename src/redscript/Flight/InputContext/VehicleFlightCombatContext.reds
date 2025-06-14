public class VehicleFlightCombatContextDecisions extends VehicleFlightContextDecisions {

  protected func UpdateEnterConditionForFlight() -> Void {
    let value = this.m_psmVehicle == EnumInt(gamePSMVehicle.Combat) && this.m_isFlying;
    this.EnableOnEnterCondition(value);
  }

  protected const func EnterCondition(const stateContext: ref<StateContext>, const scriptInterface: ref<StateGameScriptInterface>) -> Bool {
    return true;
  }
}

public class VehicleFlightCombatContextEvents extends VehicleCombatContextEvents {

  protected final func OnUpdate(timeDelta: Float, stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
    super.OnUpdate(timeDelta, stateContext, scriptInterface);
    if this.m_gameplaySettings.GetIsInputHintEnabled() && this.m_isGameplayInputHintManagerInitialized {
      this.UpdateVehicleFlightInputHints(stateContext, scriptInterface);
    };
  }

  protected func OnEnter(stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
    super.OnEnter(stateContext, scriptInterface);
    this.RemoveVehicleDriverInputHints(stateContext, scriptInterface);
    this.ShowVehicleFlightInputHints(stateContext, scriptInterface);
  }

  protected func OnExit(stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
    super.OnExit(stateContext, scriptInterface);
    this.RemoveVehicleFlightInputHints(stateContext, scriptInterface);
  }
}