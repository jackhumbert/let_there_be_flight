@wrapMethod(VehicleDriverContextDecisions)
protected const func EnterCondition(const stateContext: ref<StateContext>, const scriptInterface: ref<StateGameScriptInterface>) -> Bool {
  // FlightLog.Info("[VehicleDriverContextDecisions] EnterCondition");
  let old = wrappedMethod(stateContext, scriptInterface);
  let currentState: Int32 = scriptInterface.localBlackboard.GetInt(GetAllBlackboardDefs().PlayerStateMachine.Vehicle);
  return old && currentState != 8;
}

@wrapMethod(VehiclePassengerContextDecisions)
protected const func EnterCondition(const stateContext: ref<StateContext>, const scriptInterface: ref<StateGameScriptInterface>) -> Bool {
  let currentState: Int32 = scriptInterface.localBlackboard.GetInt(GetAllBlackboardDefs().PlayerStateMachine.Vehicle);
  return currentState != 8;
}

public class VehicleFlightContextDecisions extends InputContextTransitionDecisions {

  private let m_callbackID: ref<CallbackHandle>;

  protected final func OnAttach(const stateContext: ref<StateContext>, const scriptInterface: ref<StateGameScriptInterface>) -> Void {
    FlightLog.Info("[VehicleFlightContextDecisions] OnAttach");
    let allBlackboardDef: ref<AllBlackboardDefinitions>;
    if IsDefined(scriptInterface.localBlackboard) {
      allBlackboardDef = GetAllBlackboardDefs();
      this.m_callbackID = scriptInterface.localBlackboard.RegisterListenerInt(allBlackboardDef.PlayerStateMachine.Vehicle, this, n"OnVehicleStateChanged");
      this.OnVehicleStateChanged(scriptInterface.localBlackboard.GetInt(allBlackboardDef.PlayerStateMachine.Vehicle));
      FlightController.GetInstance().SetupMountedToCallback(scriptInterface.localBlackboard);
      (scriptInterface.owner as VehicleObject).ToggleFlightComponent(true);
    };
    //this.EnableOnEnterCondition(true);
  }

  protected final func OnDetach(const stateContext: ref<StateContext>, const scriptInterface: ref<StateGameScriptInterface>) -> Void {
    FlightLog.Info("[VehicleFlightContextDecisions] OnDetach");
    (scriptInterface.owner as VehicleObject).ToggleFlightComponent(false);
    this.m_callbackID = null;
    // FlightController.GetInstance().Disable();
  }

  protected cb func OnVehicleStateChanged(value: Int32) -> Bool {
    FlightLog.Info("[VehicleFlightContextDecisions] OnVehicleStateChanged: " + ToString(IntEnum<gamePSMVehicle>(value)));
    this.EnableOnEnterCondition(value == 8);
  }

  protected const func EnterCondition(const stateContext: ref<StateContext>, const scriptInterface: ref<StateGameScriptInterface>) -> Bool {
    // FlightLog.Info("[VehicleFlightContextDecisions] EnterCondition");
    // if StatusEffectSystem.ObjectHasStatusEffectWithTag(scriptInterface.executionOwner, n"VehicleOnlyForward") {
    //   return false;
    // };
    if StatusEffectSystem.ObjectHasStatusEffectWithTag(scriptInterface.executionOwner, n"NoDriving") {
      return false;
    };
    return true;
  }
}