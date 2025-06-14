@addMethod(VehicleDriverContextDecisions)
protected func UpdateEnterConditionForFlight() -> Void {
  let value = !this.m_isFlying && (this.m_vehicleState == EnumInt(gamePSMVehicle.Driving));
  this.EnableOnEnterCondition(value);
}

@replaceMethod(VehicleDriverContextDecisions)
protected cb func OnVehicleStateChanged(value: Int32) -> Bool {
  this.m_vehicleState = value;
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
