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

@addField(VehicleDriverContextDecisions)
protected let m_flightCallbackID: ref<CallbackHandle>;

@addField(VehicleDriverContextDecisions)
protected let m_isFlying: Bool;

@addField(VehicleDriverContextDecisions)
protected let m_vehicleState: Int32;

@addMethod(VehicleDriverContextDecisions)
protected func UpdateEnterConditionForFlight() -> Void {
  let value = !this.m_isFlying && (this.m_vehicleState == EnumInt(gamePSMVehicle.Driving));
  this.EnableOnEnterCondition(value);
  
  // let inputState = this.m_context.GetStateMachineCurrentState(n"InputContext");
  // FlightLog.Info("[InputContext] Current: " + NameToString(inputState));
}

@replaceMethod(VehicleDriverContextDecisions)
protected cb func OnVehicleStateChanged(value: Int32) -> Bool {
  this.m_vehicleState = value;
  this.UpdateEnterConditionForFlight();
}

@addMethod(VehicleDriverContextDecisions)
protected cb func OnVehicleFlightChanged(value: Bool) -> Bool {
  this.m_isFlying = value;
  this.UpdateEnterConditionForFlight();
}

@wrapMethod(VehicleDriverContextDecisions)
protected final func OnAttach(const stateContext: ref<StateContext>, const scriptInterface: ref<StateGameScriptInterface>) -> Void {
  wrappedMethod(stateContext, scriptInterface);

  let allBlackboardDef: ref<AllBlackboardDefinitions> = GetAllBlackboardDefs();
  let bb = FlightController.GetInstance().GetBlackboard();

  if IsDefined(bb) {
    this.m_flightCallbackID = bb.RegisterListenerBool(allBlackboardDef.VehicleFlight.IsActive, this, n"OnVehicleFlightChanged");
    this.OnVehicleFlightChanged(bb.GetBool(allBlackboardDef.VehicleFlight.IsActive));
  } else {
    FlightLog.Error("[VehicleDriverContextDecisions] Blackboard not defined");
  }
}

@wrapMethod(VehiclePassengerContextDecisions)
protected const func EnterCondition(const stateContext: ref<StateContext>, const scriptInterface: ref<StateGameScriptInterface>) -> Bool {
  return wrappedMethod(stateContext, scriptInterface) && !this.IsInFlight(stateContext);
}

@wrapMethod(VehicleDriverCombatContextDecisions)
protected const func EnterCondition(const stateContext: ref<StateContext>, const scriptInterface: ref<StateGameScriptInterface>) -> Bool {
  return wrappedMethod(stateContext, scriptInterface) && !this.IsInFlight(stateContext);
}

@addMethod(InputContextTransitionEvents)
protected const func GetContextName() -> String {
  return "Unknown";
}

@addMethod(VehiclePassengerContextEvents)
protected const func GetContextName() -> String {
  return "VehiclePassenger";
}

@addMethod(VehicleDriverContextEvents)
protected const func GetContextName() -> String {
  return "VehicleDriver";
}

@addMethod(VehicleDriverMountedWeaponsContextEvents)
protected const func GetContextName() -> String {
  return "VehicleDriverMountedWeapons";
}

@addMethod(VehicleDriverCombatContextEvents)
protected const func GetContextName() -> String {
  return "VehicleDriverCombat";
}

@addMethod(VehicleDriverCombatTPPContextEvents)
protected const func GetContextName() -> String {
  return "VehicleDriverCombatTPP";
}

@addMethod(VehicleDriverCombatAimContextEvents)
protected const func GetContextName() -> String {
  return "VehicleDriverCombatAim";
}

@addMethod(VehicleDriverCombatMountedWeaponsContextEvents)
protected const func GetContextName() -> String {
  return "VehicleDriverCombatMountedWeapons";
}

@addMethod(VehicleCombatContextEvents)
protected const func GetContextName() -> String {
  return "VehicleCombat";
}

@wrapMethod(VehiclePassengerContextEvents)
protected func OnEnter(stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
  FlightLog.Info("[InputContext] " + this.GetContextName() + " from " + NameToString(stateContext.GetStateMachineCurrentState(n"InputContext")));
  wrappedMethod(stateContext, scriptInterface);
}

@wrapMethod(VehicleDriverContextEvents)
protected func OnEnter(stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
  FlightLog.Info("[InputContext] " + this.GetContextName() + " from " + NameToString(stateContext.GetStateMachineCurrentState(n"InputContext")));
  wrappedMethod(stateContext, scriptInterface);
}

@wrapMethod(VehicleDriverCombatContextEvents)
protected func OnEnter(stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
  FlightLog.Info("[InputContext] " + this.GetContextName() + " from " + NameToString(stateContext.GetStateMachineCurrentState(n"InputContext")));
  wrappedMethod(stateContext, scriptInterface);
}

@wrapMethod(VehicleDriverCombatTPPContextEvents)
protected func OnEnter(stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
  FlightLog.Info("[InputContext] " + this.GetContextName() + " from " + NameToString(stateContext.GetStateMachineCurrentState(n"InputContext")));
  wrappedMethod(stateContext, scriptInterface);
}

@wrapMethod(VehicleDriverCombatAimContextEvents)
protected func OnEnter(stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
  FlightLog.Info("[InputContext] " + this.GetContextName() + " from " + NameToString(stateContext.GetStateMachineCurrentState(n"InputContext")));
  wrappedMethod(stateContext, scriptInterface);
}

@wrapMethod(VehicleCombatContextEvents)
protected func OnEnter(stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
  FlightLog.Info("[InputContext] " + this.GetContextName() + " from " + NameToString(stateContext.GetStateMachineCurrentState(n"InputContext")));
  wrappedMethod(stateContext, scriptInterface);
}

@wrapMethod(VehiclePassengerContextEvents)
protected func OnEnter(stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
  FlightLog.Info("[InputContext] " + this.GetContextName() + " from " + NameToString(stateContext.GetStateMachineCurrentState(n"InputContext")));
  wrappedMethod(stateContext, scriptInterface);
}