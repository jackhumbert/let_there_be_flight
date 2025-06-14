public class VehicleFlightDriverCombatContextDecisions extends VehicleFlightContextDecisions {

  protected let m_tppCallbackID: ref<CallbackHandle>;
  protected let m_upperBodyCallbackID: ref<CallbackHandle>;

  protected let m_inTpp: Bool;
  protected let m_isAiming: Bool;

  protected func OnAttach(const stateContext: ref<StateContext>, const scriptInterface: ref<StateGameScriptInterface>) -> Void {
    super.OnAttach(stateContext, scriptInterface);

    let allBlackboardDef = GetAllBlackboardDefs();
      
    if IsDefined(scriptInterface.localBlackboard) {
      this.m_tppCallbackID = scriptInterface.localBlackboard.RegisterListenerBool(allBlackboardDef.PlayerStateMachine.IsDriverCombatInTPP, this, n"OnVehiclePerspectiveChanged", true);
      this.OnVehiclePerspectiveChanged(scriptInterface.localBlackboard.GetBool(allBlackboardDef.PlayerStateMachine.IsDriverCombatInTPP));

      this.m_upperBodyCallbackID = scriptInterface.localBlackboard.RegisterListenerInt(allBlackboardDef.PlayerStateMachine.UpperBody, this, n"OnUpperBodyStateChanged", true);
      this.OnUpperBodyStateChanged(scriptInterface.localBlackboard.GetInt(allBlackboardDef.PlayerStateMachine.UpperBody));
    }
  }

  protected func OnDetach(const stateContext: ref<StateContext>, const scriptInterface: ref<StateGameScriptInterface>) -> Void {
    super.OnDetach(stateContext, scriptInterface);

    let allBlackboardDef: ref<AllBlackboardDefinitions> = GetAllBlackboardDefs();

    if IsDefined(scriptInterface.localBlackboard) {
      scriptInterface.localBlackboard.UnregisterListenerBool(allBlackboardDef.PlayerStateMachine.IsDriverCombatInTPP, this.m_tppCallbackID);
      scriptInterface.localBlackboard.UnregisterListenerInt(allBlackboardDef.PlayerStateMachine.UpperBody, this.m_upperBodyCallbackID);
    };

    this.m_tppCallbackID = null;
    this.m_upperBodyCallbackID = null;
  }

  protected func UpdateEnterConditionForFlight() -> Void {
    let value = this.m_psmVehicle == EnumInt(gamePSMVehicle.DriverCombat) && this.m_isFlying;
    // let value = this.m_isFlying;
    this.EnableOnEnterCondition(value);
    // FlightLog.Info("[InputContext] " + this.GetContextName() + " EnterCondition: " + ToString(value));
  }

  protected cb func OnVehiclePerspectiveChanged(value: Bool) -> Bool {
    this.m_inTpp = value;
  }

  protected cb func OnUpperBodyStateChanged(value: Int32) -> Bool {
    this.m_isAiming = value == EnumInt(gamePSMUpperBodyStates.Aim);
  }

  protected const func CameraPerspectiveEnterCondition() -> Bool {
    return !this.m_inTpp;
  }

  protected const func IsAimingEnterCondition() -> Bool {
    return !this.m_isAiming;
  }
  
  protected const func DriverCombatTypeEnterCondition(const stateContext: ref<StateContext>) -> Bool {
    let driverCombatType: gamedataDriverCombatType = this.GetDriverCombatType(stateContext);
    return Equals(driverCombatType, gamedataDriverCombatType.Standard) || Equals(driverCombatType, gamedataDriverCombatType.Doors);
  }

  protected const func EnterCondition(const stateContext: ref<StateContext>, const scriptInterface: ref<StateGameScriptInterface>) -> Bool {
    if !this.CameraPerspectiveEnterCondition() || !this.IsAimingEnterCondition() || !this.DriverCombatTypeEnterCondition(stateContext) {
      return false;
    };
    if StatusEffectSystem.ObjectHasStatusEffectWithTag(scriptInterface.executionOwner, n"VehicleOnlyForward") {
      return false;
    };
    if StatusEffectSystem.ObjectHasStatusEffectWithTag(scriptInterface.executionOwner, n"NoDriving") {
      return false;
    };
    // FlightLog.Info("[InputContext] " + this.GetContextName() + " Entering");
    return true;
  }
}

public class VehicleFlightDriverCombatContextEvents extends VehicleDriverCombatContextEvents {

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