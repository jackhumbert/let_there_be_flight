
@wrapMethod(VehiclePassengerContextDecisions)
protected const func EnterCondition(const stateContext: ref<StateContext>, const scriptInterface: ref<StateGameScriptInterface>) -> Bool {
  return wrappedMethod(stateContext, scriptInterface) && !this.IsInFlight();
}

@wrapMethod(VehicleDriverCombatContextDecisions)
protected const func EnterCondition(const stateContext: ref<StateContext>, const scriptInterface: ref<StateGameScriptInterface>) -> Bool {
  return wrappedMethod(stateContext, scriptInterface) && !this.IsInFlight();
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