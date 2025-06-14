@addField(InputContextTransitionDecisions)
protected let m_flightCallbackID: ref<CallbackHandle>;

@addField(InputContextTransitionDecisions)
protected let m_isFlying: Bool;

@addField(InputContextTransitionDecisions)
protected let m_vehicleState: Int32;

@addMethod(InputContextTransitionDecisions)
protected func UpdateEnterConditionForFlight() -> Void {
  let value = !this.m_isFlying;
  this.EnableOnEnterCondition(value);
}

@addMethod(InputContextTransitionDecisions)
protected cb func OnVehicleFlightChanged(value: Bool) -> Bool {
  this.m_isFlying = value;
  this.UpdateEnterConditionForFlight();
}
