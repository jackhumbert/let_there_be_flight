public class VehicleFlightContextDecisions extends InputContextTransitionDecisions {

  private let m_callbackID: ref<CallbackHandle>;

  protected func OnAttach(const stateContext: ref<StateContext>, const scriptInterface: ref<StateGameScriptInterface>) -> Void {
    FlightLog.Info("[VehicleFlightContextDecisions] OnAttach");
    let allBlackboardDef: ref<AllBlackboardDefinitions>;
    let bb = FlightController.GetInstance().GetBlackboard();
    // if IsDefined(scriptInterface.localBlackboard) {
    if IsDefined(bb) {
      allBlackboardDef = GetAllBlackboardDefs();
      // this.m_callbackID = scriptInterface.localBlackboard.RegisterListenerInt(allBlackboardDef.PlayerStateMachine.Vehicle, this, n"OnVehicleStateChanged");
      // this.OnVehicleStateChanged(scriptInterface.localBlackboard.GetInt(allBlackboardDef.PlayerStateMachine.Vehicle));
      
      this.m_callbackID = bb.RegisterListenerBool(allBlackboardDef.VehicleFlight.IsActive, this, n"OnVehicleFlightChanged");
      this.OnVehicleFlightChanged(bb.GetBool(allBlackboardDef.VehicleFlight.IsActive));

      FlightController.GetInstance().SetupMountedToCallback(scriptInterface.localBlackboard);
      (scriptInterface.owner as VehicleObject).ToggleFlightComponent(true);
    };
    //this.EnableOnEnterCondition(true);
  }

  protected func OnDetach(const stateContext: ref<StateContext>, const scriptInterface: ref<StateGameScriptInterface>) -> Void {
    FlightLog.Info("[VehicleFlightContextDecisions] OnDetach");
    (scriptInterface.owner as VehicleObject).ToggleFlightComponent(false);
    this.m_callbackID = null;
    // FlightController.GetInstance().Disable();
  }

  // protected cb func OnVehicleStateChanged(value: Int32) -> Bool {
  //   FlightLog.Info("[VehicleFlightContextDecisions] OnVehicleStateChanged: " + ToString(IntEnum<gamePSMVehicle>(value)) + " " + ToString(value));
  //   this.EnableOnEnterCondition(value == 8);
  // }

  protected cb func OnVehicleFlightChanged(value: Bool) -> Bool {
    FlightLog.Info("[VehicleFlightContextDecisions] OnVehicleFlightChanged: " + ToString(value));
    this.EnableOnEnterCondition(value);
  }

  protected const func DriverCombatTypeEnterCondition(const stateContext: ref<StateContext>) -> Bool {
    let driverCombatType: gamedataDriverCombatType = this.GetDriverCombatType(stateContext);
    return NotEquals(driverCombatType, gamedataDriverCombatType.MountedWeapons);
  }

  protected const func EnterCondition(const stateContext: ref<StateContext>, const scriptInterface: ref<StateGameScriptInterface>) -> Bool {
    // if !this.DriverCombatTypeEnterCondition(stateContext) {
    //   return false;
    // }
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