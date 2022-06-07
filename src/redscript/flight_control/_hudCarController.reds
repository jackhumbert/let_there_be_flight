@wrapMethod(hudCarController)
private final func Reset() -> Void {
  wrappedMethod();
  this.OnFlightActiveChanged(false);
}

@addField(hudCarController)
private let m_flightActiveBBConnectionId: ref<CallbackHandle>;

@addField(hudCarController)
private let m_flightModeBBConnectionId: ref<CallbackHandle>;

@addField(hudCarController)
private let m_flightControllerStatus: wref<inkText>;

@wrapMethod(hudCarController)
private final func RegisterToVehicle(register: Bool) -> Void {
  wrappedMethod(register);
  let flightControllerBlackboard: wref<IBlackboard>;
  let vehicle: ref<VehicleObject> = this.m_activeVehicle;
  if vehicle == null {
    return;
  };
  flightControllerBlackboard = FlightController.GetInstance().GetBlackboard();
  if IsDefined(flightControllerBlackboard) {
    if register {
      // GetRootWidget() returns root widget of base type inkWidget
      // GetRootCompoundWidget() returns root widget casted to inkCompoundWidget
      if !IsDefined(this.m_flightControllerStatus) {
        this.m_flightControllerStatus = FlightController.HUDStatusSetup(this.GetRootCompoundWidget());
      }
      this.m_flightActiveBBConnectionId = flightControllerBlackboard.RegisterListenerBool(GetAllBlackboardDefs().VehicleFlight.IsActive, this, n"OnFlightActiveChanged");
      this.m_flightModeBBConnectionId = flightControllerBlackboard.RegisterListenerInt(GetAllBlackboardDefs().VehicleFlight.Mode, this, n"OnFlightModeChanged");
    } else {
      flightControllerBlackboard.UnregisterListenerBool(GetAllBlackboardDefs().VehicleFlight.IsActive, this.m_flightActiveBBConnectionId);
      flightControllerBlackboard.UnregisterListenerInt(GetAllBlackboardDefs().VehicleFlight.Mode, this.m_flightModeBBConnectionId);
    };
  };
}

@addMethod(hudCarController)
protected cb func OnFlightActiveChanged(active: Bool) -> Bool {
  if !IsDefined(this.m_flightControllerStatus) {
    this.m_flightControllerStatus = FlightController.HUDStatusSetup(this.GetRootCompoundWidget());
  }
  if active {
    this.m_flightControllerStatus.SetText("Flight Active: " + fs().playerComponent.GetFlightMode().GetDescription());
  } else {
    this.m_flightControllerStatus.SetText("Flight Available");
  }
}

@addMethod(hudCarController)
protected cb func OnFlightModeChanged(mode: Int32) -> Bool {
  this.m_flightControllerStatus.SetText("Flight Active: " + fs().playerComponent.GetFlightMode().GetDescription());
}

@wrapMethod(hudCarController)
protected cb func OnSpeedValueChanged(speedValue: Float) -> Bool {
  // speedValue = AbsF(speedValue);
  // let multiplier: Float = GameInstance.GetStatsDataSystem(this.m_activeVehicle.GetGame()).GetValueFromCurve(n"vehicle_ui", speedValue, n"speed_to_multiplier");
  // inkTextRef.SetText(this.m_SpeedValue, IntToString(RoundMath(speedValue)));

  let fc = fs().playerComponent;
  if fc.active {
    let speed = AbsF(fc.stats.d_speed);
    let multiplier: Float = GameInstance.GetStatsDataSystem(this.m_activeVehicle.GetGame()).GetValueFromCurve(n"vehicle_ui", speed, n"speed_to_multiplier");
    inkTextRef.SetText(this.m_SpeedValue, IntToString(RoundMath(speed * multiplier)));
  } else {
    wrappedMethod(speedValue);
  }
}