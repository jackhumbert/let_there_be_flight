// @wrapMethod(VehicleDriverContextDecisions)
// protected const func EnterCondition(const stateContext: ref<StateContext>, const scriptInterface: ref<StateGameScriptInterface>) -> Bool {
//   // FlightLog.Info("[VehicleDriverContextDecisions] EnterCondition");
//   let old = wrappedMethod(stateContext, scriptInterface);
//   // let currentState: Int32 = scriptInterface.localBlackboard.GetInt(GetAllBlackboardDefs().PlayerStateMachine.Vehicle);
//   // let inFlight: StateResultBool = stateContext.GetPermanentBoolParameter(n"isInFlight");
//   // return old && currentState < 8;
//   return old && !this.IsInFlight(stateContext);
//   // return old && !(scriptInterface.owner as VehicleObject).GetFlightComponent().active;
// }

@wrapMethod(VehiclePassengerContextDecisions)
protected const func EnterCondition(const stateContext: ref<StateContext>, const scriptInterface: ref<StateGameScriptInterface>) -> Bool {
  return wrappedMethod(stateContext, scriptInterface) && !this.IsInFlight(stateContext);
}

@wrapMethod(VehicleDriverCombatContextDecisions)
protected const func EnterCondition(const stateContext: ref<StateContext>, const scriptInterface: ref<StateGameScriptInterface>) -> Bool {
  return wrappedMethod(stateContext, scriptInterface) && !this.IsInFlight(stateContext);
}

@wrapMethod(VehiclePassengerContextEvents)
protected func OnEnter(stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
  FlightLog.Info("[InputContext] " + NameToString(this.GetStateName()) + " from " + NameToString(stateContext.GetStateMachineCurrentState(n"InputContext")));
  wrappedMethod(stateContext, scriptInterface);
}

@wrapMethod(VehicleDriverContextEvents)
protected func OnEnter(stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
  FlightLog.Info("[InputContext] " + NameToString(this.GetStateName()) + " from " + NameToString(stateContext.GetStateMachineCurrentState(n"InputContext")));
  wrappedMethod(stateContext, scriptInterface);
}

@wrapMethod(VehicleDriverCombatContextEvents)
protected func OnEnter(stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
  FlightLog.Info("[InputContext] " + NameToString(this.GetStateName()) + " from " + NameToString(stateContext.GetStateMachineCurrentState(n"InputContext")));
  wrappedMethod(stateContext, scriptInterface);
}

@wrapMethod(VehicleDriverCombatTPPContextEvents)
protected func OnEnter(stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
  FlightLog.Info("[InputContext] " + NameToString(this.GetStateName()) + " from " + NameToString(stateContext.GetStateMachineCurrentState(n"InputContext")));
  wrappedMethod(stateContext, scriptInterface);
}

@wrapMethod(VehicleDriverCombatAimContextEvents)
protected func OnEnter(stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
  FlightLog.Info("[InputContext] " + NameToString(this.GetStateName()) + " from " + NameToString(stateContext.GetStateMachineCurrentState(n"InputContext")));
  wrappedMethod(stateContext, scriptInterface);
}

@wrapMethod(VehicleCombatContextEvents)
protected func OnEnter(stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
  FlightLog.Info("[InputContext] " + NameToString(this.GetStateName()) + " from " + NameToString(stateContext.GetStateMachineCurrentState(n"InputContext")));
  wrappedMethod(stateContext, scriptInterface);
}

@wrapMethod(VehiclePassengerContextEvents)
protected func OnEnter(stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
  FlightLog.Info("[InputContext] " + NameToString(this.GetStateName()) + " from " + NameToString(stateContext.GetStateMachineCurrentState(n"InputContext")));
  wrappedMethod(stateContext, scriptInterface);
}