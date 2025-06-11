@wrapMethod(VehicleDriverContextDecisions)
protected const func EnterCondition(const stateContext: ref<StateContext>, const scriptInterface: ref<StateGameScriptInterface>) -> Bool {
  // FlightLog.Info("[VehicleDriverContextDecisions] EnterCondition");
  let old = wrappedMethod(stateContext, scriptInterface);
  // let currentState: Int32 = scriptInterface.localBlackboard.GetInt(GetAllBlackboardDefs().PlayerStateMachine.Vehicle);
  let inFlight: StateResultBool = stateContext.GetPermanentBoolParameter(n"isInFlight");
  // return old && currentState < 8;
  return old && !inFlight.value;
}

@wrapMethod(VehiclePassengerContextDecisions)
protected const func EnterCondition(const stateContext: ref<StateContext>, const scriptInterface: ref<StateGameScriptInterface>) -> Bool {
  // let currentState: Int32 = scriptInterface.localBlackboard.GetInt(GetAllBlackboardDefs().PlayerStateMachine.Vehicle);
  
  let inFlight: StateResultBool = stateContext.GetPermanentBoolParameter(n"isInFlight");
  // return currentState < 8;
  return !inFlight.value;
}

@wrapMethod(VehicleDriverCombatContextDecisions)
protected const func EnterCondition(const stateContext: ref<StateContext>, const scriptInterface: ref<StateGameScriptInterface>) -> Bool {
  // FlightLog.Info("[VehicleDriverContextDecisions] EnterCondition");
  let old = wrappedMethod(stateContext, scriptInterface);
  let inFlight: StateResultBool = stateContext.GetPermanentBoolParameter(n"isInFlight");
  // let currentState: Int32 = scriptInterface.localBlackboard.GetInt(GetAllBlackboardDefs().PlayerStateMachine.Vehicle);
  // return old && currentState != 9; //!inFlight.value;
  return old && !inFlight.value;
}
