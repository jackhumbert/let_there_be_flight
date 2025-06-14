public class VehicleFlightDriverCombatAimContextDecisions extends VehicleFlightDriverCombatContextDecisions {

  protected const func IsAimingEnterCondition() -> Bool {
    return this.m_isAiming;
  }

  protected const func CameraPerspectiveEnterCondition() -> Bool {
    return true;
  }
}

public class VehicleFlightDriverCombatAimContextEvents extends VehicleFlightDriverCombatContextEvents {

  protected func OnEnter(stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
    super.OnEnter(stateContext, scriptInterface);
    this.m_weapon = DefaultTransition.GetActiveWeapon(scriptInterface);
    stateContext.SetPermanentBoolParameter(n"inMeleeDriverCombat", this.m_weapon.IsMelee(), true);
    if this.m_gameplaySettings.GetIsInputHintEnabled() && this.m_isGameplayInputHintManagerInitialized {
      this.ShowVehicleDriverCombatTPPInputHints(stateContext, scriptInterface);
      // this.ShowVehicleFlightInputHints(stateContext, scriptInterface);
    };
    FlightSettings.SetBool("inTPPDriverCombat", true);
  }

  protected func OnExit(stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
    this.RemoveVehicleDriverCombatTPPInputHints(stateContext, scriptInterface);
    stateContext.RemovePermanentBoolParameter(n"inMeleeDriverCombat");
    FlightSettings.SetBool("inTPPDriverCombat", false);
  }
}