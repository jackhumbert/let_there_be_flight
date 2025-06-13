public class VehicleFlightContextEvents extends InputContextTransitionEvents {

  protected const func GetContextName() -> String {
    return "VehicleFlight";
  }

  protected final func OnUpdate(timeDelta: Float, stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
    if this.m_gameplaySettings.GetIsInputHintEnabled() && this.m_isGameplayInputHintManagerInitialized {
      this.UpdateVehicleFlightInputHints(stateContext, scriptInterface);
    };
  }

  protected func OnEnter(stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
    FlightLog.Info("[InputContext] " + this.GetContextName() + " from " + NameToString(stateContext.GetStateMachineCurrentState(n"InputContext")));
    this.RemoveVehicleDriverInputHints(stateContext, scriptInterface);
    this.ShowVehicleFlightInputHints(stateContext, scriptInterface);
  }

  protected func OnExit(stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
    this.RemoveVehicleFlightInputHints(stateContext, scriptInterface);
  }
}

public class VehicleFlightDriverMountedWeaponsContextEvents extends VehicleFlightContextEvents {

  protected const func GetContextName() -> String {
    return "VehicleFlightDriverMountedWeapons";
  }
}

public class VehicleFlightDriverCombatContextEvents extends VehicleDriverCombatContextEvents {

  protected const func GetContextName() -> String {
    return "VehicleFlightDriverCombat";
  }

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

public class VehicleFlightDriverCombatTPPContextEvents extends VehicleFlightDriverCombatContextEvents {

  protected const func GetContextName() -> String {
    return "VehicleFlightDriverCombatTPP";
  }

  protected func UpdateVehicleDriverCombatInputHintsInternal(stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
    this.ShowVehicleDriverCombatTPPInputHints(stateContext, scriptInterface);
  }

  protected func RemoveVehicleDriverCombatInputHintsInternal(stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
    this.RemoveVehicleDriverCombatTPPInputHints(stateContext, scriptInterface);
  }

  protected func OnEnter(stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
    FlightLog.Info("[InputContext] " + this.GetContextName() + " from " + NameToString(stateContext.GetStateMachineCurrentState(n"InputContext")));
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

public class VehicleFlightDriverCombatAimContextEvents extends VehicleFlightDriverCombatContextEvents {

  protected const func GetContextName() -> String {
    return "VehicleFlightDriverCombatAim";
  }

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

public class VehicleFlightDriverCombatMountedWeaponsContextEvents extends VehicleFlightDriverCombatContextEvents {

  protected const func GetContextName() -> String {
    return "VehicleFlightDriverCombatMountedWeapons";
  }
}
