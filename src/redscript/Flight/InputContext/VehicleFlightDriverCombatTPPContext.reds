public class VehicleFlightDriverCombatTPPContextDecisions extends VehicleFlightDriverCombatContextDecisions {

  protected const func CameraPerspectiveEnterCondition() -> Bool {
    return this.m_inTpp;
  }
}

public class VehicleFlightDriverCombatTPPContextEvents extends VehicleFlightDriverCombatContextEvents {

  protected func UpdateVehicleDriverCombatInputHintsInternal(stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
    this.ShowVehicleDriverCombatTPPInputHints(stateContext, scriptInterface);
  }

  protected func RemoveVehicleDriverCombatInputHintsInternal(stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
    this.RemoveVehicleDriverCombatTPPInputHints(stateContext, scriptInterface);
  }

  protected func OnEnter(stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
    FlightLog.Info("[InputContext] " + NameToString(this.GetStateName()) + " from " + NameToString(stateContext.GetStateMachineCurrentState(n"InputContext")));
    this.m_weapon = DefaultTransition.GetActiveWeapon(scriptInterface);
    stateContext.SetPermanentBoolParameter(n"inMeleeDriverCombat", this.m_weapon.IsMelee(), true);
    FlightSettings.SetBool("inTPPDriverCombat", true);
    if this.m_gameplaySettings.GetIsInputHintEnabled() && this.m_isGameplayInputHintManagerInitialized {
      this.ShowVehicleDriverCombatTPPInputHints(stateContext, scriptInterface);
      this.ShowVehicleFlightInputHints(stateContext, scriptInterface);
    };
  }

  protected func OnExit(stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
    super.OnExit(stateContext, scriptInterface);
    this.RemoveVehicleDriverCombatTPPInputHints(stateContext, scriptInterface);
    stateContext.RemovePermanentBoolParameter(n"inMeleeDriverCombat");
    FlightSettings.SetBool("inTPPDriverCombat", false);
  }
}