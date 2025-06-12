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
    FlightLog.Info("[InputContext] VehicleFlight Enter");
    this.RemoveVehicleDriverInputHints(stateContext, scriptInterface);
    this.ShowVehicleFlightInputHints(stateContext, scriptInterface);
  }

  protected func OnExit(stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
    FlightLog.Info("[InputContext] VehicleFlight Exit");
    this.RemoveVehicleFlightInputHints(stateContext, scriptInterface);
  }
}

public class VehicleFlightDriverCombatContextEvents extends VehicleDriverCombatContextEvents {

  protected final func OnUpdate(timeDelta: Float, stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
    super.OnUpdate(timeDelta, stateContext, scriptInterface);
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
    FlightLog.Info("[InputContext] VehicleFlightDriverCombat Enter");
    super.OnEnter(stateContext, scriptInterface);
    this.RemoveVehicleDriverInputHints(stateContext, scriptInterface);
    this.ShowVehicleFlightInputHints(stateContext, scriptInterface);
  }

  protected func OnExit(stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
    FlightLog.Info("[InputContext] VehicleFlightDriverCombat Exit");
    super.OnExit(stateContext, scriptInterface);
    this.RemoveVehicleFlightInputHints(stateContext, scriptInterface);
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
    FlightLog.Info("[InputContext] VehicleFlightDriverCombatTPP Enter");
    this.m_weapon = DefaultTransition.GetActiveWeapon(scriptInterface);
    stateContext.SetPermanentBoolParameter(n"inMeleeDriverCombat", this.m_weapon.IsMelee(), true);
    if this.m_gameplaySettings.GetIsInputHintEnabled() && this.m_isGameplayInputHintManagerInitialized {
      this.ShowVehicleDriverCombatTPPInputHints(stateContext, scriptInterface);
      this.ShowVehicleFlightInputHints(stateContext, scriptInterface);
    };
  }

  protected func OnExit(stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
    FlightLog.Info("[InputContext] VehicleFlightDriverCombatTPP Exit");
    this.RemoveVehicleDriverCombatTPPInputHints(stateContext, scriptInterface);
    this.RemoveVehicleFlightInputHints(stateContext, scriptInterface);
    stateContext.RemovePermanentBoolParameter(n"inMeleeDriverCombat");
  }
}

public class VehicleFlightDriverCombatAimContextEvents extends VehicleFlightDriverCombatContextEvents {

  protected func OnEnter(stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
    FlightLog.Info("[InputContext] VehicleFlightDriverCombatAim Enter");
    this.m_weapon = DefaultTransition.GetActiveWeapon(scriptInterface);
    stateContext.SetPermanentBoolParameter(n"inMeleeDriverCombat", this.m_weapon.IsMelee(), true);
    if this.m_gameplaySettings.GetIsInputHintEnabled() && this.m_isGameplayInputHintManagerInitialized {
      this.ShowVehicleDriverCombatTPPInputHints(stateContext, scriptInterface);
      this.ShowVehicleFlightInputHints(stateContext, scriptInterface);
    };
  }

  protected func OnExit(stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
    FlightLog.Info("[InputContext] VehicleFlightDriverCombatAim Exit");
    this.RemoveVehicleDriverCombatTPPInputHints(stateContext, scriptInterface);
    this.RemoveVehicleFlightInputHints(stateContext, scriptInterface);
    stateContext.RemovePermanentBoolParameter(n"inMeleeDriverCombat");
  }
}

public class VehicleFlightDriverCombatMountedWeaponsContextEvents extends VehicleFlightDriverCombatContextEvents {

  protected func OnEnter(stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
    FlightLog.Info("[InputContext] VehicleFlightDriverCombatMountedWeapons Enter");
    super.OnEnter(stateContext, scriptInterface);
  }

  protected func OnExit(stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
    FlightLog.Info("[InputContext] VehicleFlightDriverCombatMountedWeapons Exit");
    super.OnExit(stateContext, scriptInterface);
    this.RemoveVehicleFlightInputHints(stateContext, scriptInterface);
  }
}
