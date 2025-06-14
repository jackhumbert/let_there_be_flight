public class VehicleFlightContextDecisions extends InputContextTransitionDecisions {

  protected let m_stateCallbackID: ref<CallbackHandle>;
  protected let m_flightCallbackID: ref<CallbackHandle>;

  protected let m_psmVehicle: Int32;
  protected let m_isFlying: Bool;

  protected let m_context: ref<StateContext>;

  protected func OnAttach(const stateContext: ref<StateContext>, const scriptInterface: ref<StateGameScriptInterface>) -> Void {
    // FlightLog.Info("[VehicleFlightContextDecisions] OnAttach");
    this.m_context = stateContext;
    let allBlackboardDef = GetAllBlackboardDefs();
      
    if IsDefined(scriptInterface.localBlackboard) {
      this.m_stateCallbackID = scriptInterface.localBlackboard.RegisterListenerInt(allBlackboardDef.PlayerStateMachine.Vehicle, this, n"OnVehicleStateChanged");
      this.OnVehicleStateChanged(scriptInterface.localBlackboard.GetInt(allBlackboardDef.PlayerStateMachine.Vehicle));
      
      FlightController.GetInstance().SetupMountedToCallback(scriptInterface.localBlackboard);
    }
      
    let bb = FlightController.GetInstance().GetBlackboard();

    if IsDefined(bb) {
      this.m_flightCallbackID = bb.RegisterListenerBool(allBlackboardDef.VehicleFlight.IsActive, this, n"OnVehicleFlightChanged");
      this.OnVehicleFlightChanged(bb.GetBool(allBlackboardDef.VehicleFlight.IsActive));
    };

    (scriptInterface.owner as VehicleObject).ToggleFlightComponent(true);
  }

  protected func OnDetach(const stateContext: ref<StateContext>, const scriptInterface: ref<StateGameScriptInterface>) -> Void {
    (scriptInterface.owner as VehicleObject).ToggleFlightComponent(false);

    let allBlackboardDef: ref<AllBlackboardDefinitions> = GetAllBlackboardDefs();

    if IsDefined(scriptInterface.localBlackboard) {
      scriptInterface.localBlackboard.UnregisterListenerInt(allBlackboardDef.PlayerStateMachine.Vehicle, this.m_stateCallbackID);
    };

    let bb = FlightController.GetInstance().GetBlackboard();

    if IsDefined(bb) {
      scriptInterface.localBlackboard.UnregisterListenerBool(allBlackboardDef.VehicleFlight.IsActive, this.m_flightCallbackID);
    };

    this.m_stateCallbackID = null;
    this.m_flightCallbackID = null;
  }

  protected func UpdateEnterConditionForFlight() -> Void {
    let value = this.m_psmVehicle == EnumInt(gamePSMVehicle.Driving) && this.m_isFlying;
    // let value = this.m_isFlying;
    this.EnableOnEnterCondition(value);
    // FlightLog.Info("[InputContext] " + this.GetContextName() + " EnterCondition: " + ToString(value));
    
    // let inputState = this.m_context.GetStateMachineCurrentState(n"InputContext");
    // FlightLog.Info("[InputContext] Current: " + NameToString(inputState));
  }

  protected cb func OnVehicleStateChanged(value: Int32) -> Bool {
    this.m_psmVehicle = value;
    this.UpdateEnterConditionForFlight();
  }

  protected cb func OnVehicleFlightChanged(value: Bool) -> Bool {
    this.m_isFlying = value;
    this.UpdateEnterConditionForFlight();
  }

  protected const func DriverCombatTypeEnterCondition(const stateContext: ref<StateContext>) -> Bool {
    let driverCombatType: gamedataDriverCombatType = this.GetDriverCombatType(stateContext);
    return NotEquals(driverCombatType, gamedataDriverCombatType.MountedWeapons);
  }

  protected const func EnterCondition(const stateContext: ref<StateContext>, const scriptInterface: ref<StateGameScriptInterface>) -> Bool {
    if !this.DriverCombatTypeEnterCondition(stateContext) {
      return false;
    };
    if StatusEffectSystem.ObjectHasStatusEffectWithTag(scriptInterface.executionOwner, n"VehicleOnlyForward") {
      return false;
    };
    if StatusEffectSystem.ObjectHasStatusEffectWithTag(scriptInterface.executionOwner, n"NoDriving") {
      return false;
    };
    return true;
  }
}

public class VehicleFlightContextEvents extends InputContextTransitionEvents {

  protected final func OnUpdate(timeDelta: Float, stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
    if this.m_gameplaySettings.GetIsInputHintEnabled() && this.m_isGameplayInputHintManagerInitialized {
      this.UpdateVehicleFlightInputHints(stateContext, scriptInterface);
    };
  }

  protected func OnEnter(stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
    FlightLog.Info("[InputContext] " + NameToString(this.GetStateName()) + " from " + NameToString(stateContext.GetStateMachineCurrentState(n"InputContext")));
    this.RemoveVehicleDriverInputHints(stateContext, scriptInterface);
    this.ShowVehicleFlightInputHints(stateContext, scriptInterface);
  }

  protected func OnExit(stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
    this.RemoveVehicleFlightInputHints(stateContext, scriptInterface);
  }
}