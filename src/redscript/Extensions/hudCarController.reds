
@wrapMethod(hudCarController)
protected cb func OnSpeedValueChanged(speedValue: Float) -> Bool {

  let fc = fs().playerComponent;
  if fc.active {
    let speed = AbsF(fc.stats.d_speed);
    let multiplier: Float = GameInstance.GetStatsDataSystem(this.m_activeVehicle.GetGame()).GetValueFromCurve(n"vehicle_ui", speed, n"speed_to_multiplier");
    inkTextRef.SetText(this.m_SpeedValue, IntToString(RoundMath(speed * multiplier)));
    this.drawRPMGaugeFull(AbsF(fc.surge) * 5000.0);
  } else {
    wrappedMethod(speedValue);
  }
}
@wrapMethod(hudCarController)
protected cb func OnRpmValueChanged(rpmValue: Float) -> Bool {
  let fc = fs().playerComponent;
  if !fc.active {
    wrappedMethod(rpmValue);
  }
}